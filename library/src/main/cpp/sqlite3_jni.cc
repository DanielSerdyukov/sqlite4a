#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#ifndef LOG_TAG
#define LOG_TAG "sqlite3_jni"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif // LOG_TAG

static JavaVM *gJavaVm = 0;

static struct {
    jclass clazz;
} gString;

static struct {
    jclass clazz;
} gSQLiteException;

static struct {
    jclass clazz;
    jmethodID method;
} gTrace;

static struct {
    jclass clazz;
    jmethodID method;
} gExec;

static struct {
    jclass clazz;
    jmethodID method;
} gComparator;

static struct {
    jclass clazz;
    jmethodID method;
} gFunc;

struct SQLiteDb {
    sqlite3 *db = nullptr;
    jobject trace = nullptr;
};

struct SQLiteStmt {
    sqlite3_stmt *stmt = nullptr;
};

static void throw_sqlite_exception(JNIEnv *env, const char *error, const char *sql = nullptr) {
    std::string message(error);
    if (sql) {
        message += ", while executing: ";
        message += sql;
    }
    if (gSQLiteException.clazz) {
        env->ThrowNew(gSQLiteException.clazz, message.c_str());
    } else {
        LOGE("%s", message.c_str());
    }
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    gJavaVm = vm;
    JNIEnv *env;
    if (JNI_OK != vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        return JNI_ERR;
    }
    gString.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/lang/String")));
    gSQLiteException.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteException")));
    gTrace.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteTrace")));
    gTrace.method = env->GetMethodID(gTrace.clazz, "call", "(Ljava/lang/String;)V");
    gExec.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteExec")));
    gExec.method = env->GetMethodID(gExec.clazz, "call", "([Ljava/lang/String;[Ljava/lang/String;)V");
    gComparator.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/util/Comparator")));
    gComparator.method = env->GetMethodID(gComparator.clazz, "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I");
    gFunc.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/JniFunc")));
    gFunc.method = env->GetMethodID(gFunc.clazz, "call", "(J[J)V");
    sqlite3_soft_heap_limit64(8 * 1024 * 1024);
    sqlite3_initialize();
    return JNI_VERSION_1_6;
}

static int binary_compare(void *data, int ll, const void *lv, int rl, const void *rv) {
    int rc, n;
    n = ll < rl ? ll : rl;
    rc = memcmp(lv, rv, static_cast<size_t>(n));
    if (rc == 0) {
        rc = ll - rl;
    }
    return rc;
}

static int jni_trace(unsigned mask, void *jfunc, void *p, void *x) {
    JNIEnv *env;
    if (jfunc && mask == SQLITE_TRACE_STMT &&
        JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        jobject func = env->NewLocalRef(static_cast<jobject>(jfunc));
        std::string sql(static_cast<const char *>(x));
        std::string trigger("--");
        if (sql.compare(0, trigger.length(), trigger) == 0) {
            env->CallVoidMethod(func, gTrace.method, env->NewStringUTF(sql.c_str()));
        } else {
            sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(p);
            char *csql = sqlite3_expanded_sql(stmt);
            jstring jsql = env->NewStringUTF(csql);
            sqlite3_free(csql);
            env->CallVoidMethod(func, gTrace.method, jsql);
            env->DeleteLocalRef(jsql);
        }
        env->DeleteLocalRef(func);
    }
    return 0;
}

static int jni_exec(void *data, int argc, char **values, char **columns) {
    JNIEnv *env;
    if (data && JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        jobject func = static_cast<jobject>(data);
        jobjectArray jcolumns = env->NewObjectArray(argc, gString.clazz, env->NewStringUTF(""));
        jobjectArray jvalues = env->NewObjectArray(argc, gString.clazz, env->NewStringUTF(""));
        for (int i = 0; i < argc; ++i) {
            env->SetObjectArrayElement(jcolumns, i, env->NewStringUTF(columns[i]));
            env->SetObjectArrayElement(jvalues, i, env->NewStringUTF(values[i]));
        }
        env->CallVoidMethod(func, gExec.method, jcolumns, jvalues);
        env->DeleteLocalRef(jcolumns);
        env->DeleteLocalRef(jvalues);
        env->DeleteGlobalRef(func);
    }
    return 0;
}

static int jni_compare(void *data, int lhsl, const void *lhsv, int rhsl, const void *rhsv) {
    JNIEnv *env;
    if (data && JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        jobject comparator = env->NewLocalRef(static_cast<jobject>(data));
        std::string lhs(static_cast<const char *>(lhsv), static_cast<size_t>(lhsl));
        std::string rhs(static_cast<const char *>(rhsv), static_cast<size_t>(rhsl));
        int ret = env->CallIntMethod(comparator, gComparator.method,
                env->NewStringUTF(lhs.c_str()),
                env->NewStringUTF(rhs.c_str()));
        env->DeleteLocalRef(comparator);
        return ret;
    }
    return binary_compare(data, lhsl, lhsv, rhsl, rhsv);
}

static void jni_invoke(sqlite3_context *context, int argc, sqlite3_value **argv) {
    JNIEnv *env;
    if (JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        void *data = sqlite3_user_data(context);
        if (data) {
            jobject func = env->NewLocalRef(static_cast<jobject>(data));
            jlong values[argc];
            for (int i = 0; i < argc; ++i) {
                values[i] = reinterpret_cast<jlong>(argv[i]);
            }
            jlongArray jvalues = env->NewLongArray(argc);
            env->SetLongArrayRegion(jvalues, 0, argc, values);
            env->CallVoidMethod(func, gFunc.method, reinterpret_cast<jlong>(context), jvalues);
            env->DeleteLocalRef(jvalues);
            env->DeleteLocalRef(func);
        }
    }
}

static void jni_destroy(void *data) {
    JNIEnv *env;
    if (data && JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        env->DeleteGlobalRef(static_cast<jobject>(data));
    }
}


//region SQLite
extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_getLibVersionNumber(JNIEnv *env, jclass type) {
    return sqlite3_libversion_number();
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_nativeOpen(JNIEnv *env, jclass type, jstring pathStr, jint flags) {
    sqlite3 *db;

    const char *path = env->GetStringUTFChars(pathStr, nullptr);
    int ret = sqlite3_open_v2(path, &db, flags, nullptr);
    env->ReleaseStringUTFChars(pathStr, path);

    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(db));
        return 0;
    }

    if ((flags & SQLITE_OPEN_READWRITE) && sqlite3_db_readonly(db, nullptr)) {
        sqlite3_close_v2(db);
        throw_sqlite_exception(env, "Could not open the database in read/write mode.");
        return 0;
    }

    ret = sqlite3_busy_timeout(db, 2500);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, "Could not set busy timeout");
        sqlite3_close(db);
        return 0;
    }

    SQLiteDb *conn = new SQLiteDb;
    conn->db = db;

    return reinterpret_cast<jlong>(conn);
}
//endregion

//region SQLiteDb
extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteDb_nativeIsReadOnly(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);
    return sqlite3_db_readonly(conn->db, nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeTrace(JNIEnv *env, jclass type, jlong ptr, jobject func) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);
    if (conn->trace) {
        env->DeleteGlobalRef(conn->trace);
        conn->trace = nullptr;
    }
    if (func) {
        conn->trace = env->NewGlobalRef(func);
        int ret = sqlite3_trace_v2(conn->db, SQLITE_TRACE_STMT, jni_trace, conn->trace);
        if (SQLITE_OK != ret) {
            throw_sqlite_exception(env, sqlite3_errmsg(conn->db));
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeExec(JNIEnv *env, jclass type, jlong ptr, jstring jsql, jobject jfunc) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);
    const char *csql = env->GetStringUTFChars(jsql, nullptr);
    std::string sql(csql);
    env->ReleaseStringUTFChars(jsql, csql);

    int ret = sqlite3_exec(conn->db, sql.c_str(), jni_exec, env->NewGlobalRef(jfunc), nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(conn->db), sql.c_str());
    }
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteDb_nativeExecForDouble(JNIEnv *env, jclass type, jlong ptr, jstring jsql) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);

    const char *csql = env->GetStringUTFChars(jsql, nullptr);
    std::string sql(csql);
    env->ReleaseStringUTFChars(jsql, csql);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(conn->db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(conn->db), sql.c_str());
    }

    double value = 0;
    if (SQLITE_ROW == sqlite3_step(stmt)) {
        value = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteDb_nativeGetAutocommit(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(ptr);
    return sqlite3_get_autocommit(handle->db);
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteDb_nativePrepare(JNIEnv *env, jclass type, jlong ptr, jstring jsql) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);

    const char *csql = env->GetStringUTFChars(jsql, nullptr);
    std::string sql(csql);
    env->ReleaseStringUTFChars(jsql, csql);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(conn->db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(conn->db), sql.c_str());
    }

    SQLiteStmt *wrapper = new SQLiteStmt;
    wrapper->stmt = stmt;

    return reinterpret_cast<jlong>(wrapper);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCreateCollation(JNIEnv *env, jclass type, jlong ptr, jstring jname, jobject jcomparator) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);
    jobject comparator = env->NewGlobalRef(jcomparator);
    const char *cname = env->GetStringUTFChars(jname, nullptr);
    std::string name(cname);
    env->ReleaseStringUTFChars(jname, cname);
    int ret = sqlite3_create_collation_v2(conn->db, name.c_str(), SQLITE_UTF8, comparator, jni_compare, jni_destroy);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(conn->db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCreateFunction(JNIEnv *env, jclass type, jlong ptr, jstring jname,
        jint numArgs, jobject jfunc) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);
    const char *cname = env->GetStringUTFChars(jname, nullptr);
    std::string name(cname);
    env->ReleaseStringUTFChars(jname, cname);
    jobject func = env->NewGlobalRef(jfunc);
    int ret = sqlite3_create_function_v2(conn->db, name.c_str(), numArgs, SQLITE_UTF8, func, jni_invoke, nullptr,
            nullptr, jni_destroy);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(conn->db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeClose(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteDb *conn = reinterpret_cast<SQLiteDb *>(ptr);
    sqlite3_close_v2(conn->db);
    if (conn->trace) {
        env->DeleteGlobalRef(conn->trace);
        conn->trace = nullptr;
    }
    delete conn;
}
//endregion

//region SQLiteStmt
extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteStmt_nativeGetSql(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    return env->NewStringUTF(sqlite3_sql(sw->stmt));
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteStmt_nativeGetExpandedSql(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    char *expandedSql = sqlite3_expanded_sql(sw->stmt);
    jstring sql = env->NewStringUTF(expandedSql);
    sqlite3_free(expandedSql);
    return sql;
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindNull(JNIEnv *env, jclass type, jlong ptr, jint index) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    int ret = sqlite3_bind_null(sw->stmt, index);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(sw->stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindLong(JNIEnv *env, jclass type, jlong ptr, jint index, jlong value) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    int ret = sqlite3_bind_int64(sw->stmt, index, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(sw->stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindDouble(JNIEnv *env, jclass type, jlong ptr, jint index, jdouble value) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    int ret = sqlite3_bind_double(sw->stmt, index, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(sw->stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindString(JNIEnv *env, jclass type, jlong ptr, jint index, jstring jvalue) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    const char *value = env->GetStringUTFChars(jvalue, nullptr);
    int ret = sqlite3_bind_text(sw->stmt, index, value, static_cast<int>(strlen(value)), SQLITE_TRANSIENT);
    env->ReleaseStringUTFChars(jvalue, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(sw->stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindBlob(JNIEnv *env, jclass type, jlong ptr, jint index,
        jbyteArray jvalue) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    jbyte *value = env->GetByteArrayElements(jvalue, nullptr);
    int ret = sqlite3_bind_blob(sw->stmt, index, value, env->GetArrayLength(jvalue), SQLITE_TRANSIENT);
    env->ReleaseByteArrayElements(jvalue, value, 0);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(sw->stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeClearBindings(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    int ret = sqlite3_reset(sw->stmt);
    if (SQLITE_OK == ret) {
        ret = sqlite3_clear_bindings(sw->stmt);
    }
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(sw->stmt)));
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteStmt_nativeExecuteInsert(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    sqlite3 *db = sqlite3_db_handle(sw->stmt);
    int ret = sqlite3_step(sw->stmt);
    if (SQLITE_DONE != ret) {
        sqlite3_finalize(sw->stmt);
        throw_sqlite_exception(env, sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteStmt_nativeExecuteUpdateDelete(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    sqlite3 *db = sqlite3_db_handle(sw->stmt);
    int ret = sqlite3_step(sw->stmt);
    if (SQLITE_DONE != ret) {
        sqlite3_finalize(sw->stmt);
        throw_sqlite_exception(env, sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_changes(db);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeFinalize(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    sqlite3_finalize(sw->stmt);
    delete sw;
}
//endregion

//region SQLiteCursor
extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteCursor_nativeStep(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    return sqlite3_step(sw->stmt);
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnLong(JNIEnv *env, jclass type, jlong ptr, jint index) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    return sqlite3_column_int64(sw->stmt, index);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnDouble(JNIEnv *env, jclass type, jlong ptr, jint index) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    return sqlite3_column_double(sw->stmt, index);
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnString(JNIEnv *env, jclass type, jlong ptr, jint index) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    const unsigned char *text = sqlite3_column_text(sw->stmt, index);
    return env->NewStringUTF(reinterpret_cast<const char *>(text));
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnBlob(JNIEnv *env, jclass type, jlong ptr, jint index) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    int size = sqlite3_column_bytes(sw->stmt, index);
    const void *value = sqlite3_column_blob(sw->stmt, index);
    jbyteArray jvalue = env->NewByteArray(size);
    env->SetByteArrayRegion(jvalue, 0, size, reinterpret_cast<const jbyte *>(value));
    return jvalue;
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnCount(JNIEnv *env, jclass type, jlong ptr) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    return sqlite3_column_count(sw->stmt);
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnName(JNIEnv *env, jclass type, jlong ptr, jint index) {
    SQLiteStmt *sw = reinterpret_cast<SQLiteStmt *>(ptr);
    return env->NewStringUTF(sqlite3_column_name(sw->stmt, index));
}
//endregion

//region SQLiteValue
extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteValue_nativeLongValue(JNIEnv *env, jclass type, jlong ptr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(ptr);
    return sqlite3_value_int64(value);
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteValue_nativeStringValue(JNIEnv *env, jclass type, jlong ptr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(ptr);
    const unsigned char *text = sqlite3_value_text(value);
    return env->NewStringUTF(reinterpret_cast<const char *>(text));
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteValue_nativeDoubleValue(JNIEnv *env, jclass type, jlong ptr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(ptr);
    return sqlite3_value_double(value);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_sqlite4a_SQLiteValue_nativeBlobValue(JNIEnv *env, jclass type, jlong ptr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(ptr);
    int size = sqlite3_value_bytes(value);
    const void *blob = sqlite3_value_blob(value);
    jbyteArray jvalue = env->NewByteArray(size);
    env->SetByteArrayRegion(jvalue, 0, size, reinterpret_cast<const jbyte *>(blob));
    return jvalue;
}
//endregion

//region SQLiteContext
extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultNull(JNIEnv *env, jclass type, jlong ptr) {
    sqlite3_result_null(reinterpret_cast<sqlite3_context *>(ptr));
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultLong(JNIEnv *env, jclass type, jlong ptr, jlong result) {
    sqlite3_result_int64(reinterpret_cast<sqlite3_context *>(ptr), result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultText(JNIEnv *env, jclass type, jlong ptr, jstring resultStr) {
    sqlite3_context *context = reinterpret_cast<sqlite3_context *>(ptr);
    const char *result = env->GetStringUTFChars(resultStr, nullptr);
    sqlite3_result_text(context, result, static_cast<int>(strlen(result)), SQLITE_TRANSIENT);
    env->ReleaseStringUTFChars(resultStr, result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultDouble(JNIEnv *env, jclass type, jlong ptr, jdouble result) {
    sqlite3_result_double(reinterpret_cast<sqlite3_context *>(ptr), result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultBlob(JNIEnv *env, jclass type, jlong ptr, jbyteArray resultArr) {
    sqlite3_context *context = reinterpret_cast<sqlite3_context *>(ptr);
    jbyte *result = env->GetByteArrayElements(resultArr, nullptr);
    sqlite3_result_blob(context, result, env->GetArrayLength(resultArr), SQLITE_TRANSIENT);
    env->ReleaseByteArrayElements(resultArr, result, 0);
}
//endregion