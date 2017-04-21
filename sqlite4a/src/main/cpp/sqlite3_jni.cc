#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#ifndef LOG_TAG
#define LOG_TAG "sqlite3_jni"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif // LOG_TAG

static JavaVM *gJavaVm = nullptr;

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
} gComparator;

static struct {
    jclass clazz;
    jmethodID method;
} gFunc;

struct SQLiteDb {
    sqlite3 *handle = nullptr;
    jobject trace = nullptr;

    SQLiteDb(sqlite3 *db) {
        handle = db;
    }
};

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (JNI_OK != vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        return JNI_ERR;
    }
    gJavaVm = vm;
    //gString.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/lang/String")));
    gSQLiteException.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteException")));
    gTrace.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteDb$Trace")));
    gTrace.method = env->GetMethodID(gTrace.clazz, "trace", "(Ljava/lang/String;)V");
    gComparator.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("java/util/Comparator")));
    gComparator.method = env->GetMethodID(gComparator.clazz, "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I");
    gFunc.clazz = static_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/JniFunc")));
    gFunc.method = env->GetMethodID(gFunc.clazz, "call", "(J[J)V");
    sqlite3_soft_heap_limit64(8 * 1024 * 1024);
    sqlite3_initialize();
    return JNI_VERSION_1_6;
}

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

static int java_trace(unsigned mask, void *jfunc, void *p, void *x) {
    JNIEnv *env;
    if (jfunc && mask == SQLITE_TRACE_STMT &&
        JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        jobject func = env->NewLocalRef(static_cast<jobject>(jfunc));
        std::string sql(static_cast<const char *>(x));
        std::string trigger("--");
        if (sql.compare(0, trigger.length(), trigger) != 0) {
            sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(p);
            char *sqlChars = sqlite3_expanded_sql(stmt);
            sql = static_cast<const char *>(sqlChars);
            sqlite3_free(sqlChars);
        }
        env->CallVoidMethod(func, gTrace.method, env->NewStringUTF(sql.c_str()));
        env->DeleteLocalRef(func);
    }
    return 0;
}

static int java_compare(void *data, int lhsl, const void *lhsv, int rhsl, const void *rhsv) {
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
    return 0;
}

static void java_func(sqlite3_context *context, int argc, sqlite3_value **argv) {
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

static void java_destroy(void *data) {
    JNIEnv *env;
    if (data && JNI_OK == gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6)) {
        env->DeleteGlobalRef(static_cast<jobject>(data));
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_getLibVersion(JNIEnv *env, jclass type) {
    return sqlite3_libversion_number();
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_nativeOpen(JNIEnv *env, jclass type, jstring jpath, jint jflags) {
    sqlite3 *handle;
    const char *path = env->GetStringUTFChars(jpath, nullptr);
    int ret = sqlite3_open_v2(path, &handle, jflags, nullptr);
    env->ReleaseStringUTFChars(jpath, path);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(handle));
        return 0;
    }
    if ((jflags & SQLITE_OPEN_READWRITE) && sqlite3_db_readonly(handle, nullptr)) {
        sqlite3_close_v2(handle);
        throw_sqlite_exception(env, "Could not open the database in read/write mode.");
        return 0;
    }
    ret = sqlite3_busy_timeout(handle, 2500);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, "Could not set busy timeout");
        sqlite3_close(handle);
        return 0;
    }
    return reinterpret_cast<jlong>(new SQLiteDb(handle));
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeClose(JNIEnv *env, jclass type, jlong jptr) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    if (db->trace) {
        env->DeleteGlobalRef(db->trace);
    }
    sqlite3_close_v2(db->handle);
    delete db;
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteDb_nativeIsReadOnly(JNIEnv *env, jclass type, jlong jptr) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    return sqlite3_db_readonly(db->handle, nullptr);
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteDb_nativeGetAutocommit(JNIEnv *env, jclass type, jlong jptr) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    return sqlite3_get_autocommit(db->handle);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeExec(JNIEnv *env, jclass type, jlong jptr, jstring jsql) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    const char *sqlChars = env->GetStringUTFChars(jsql, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(jsql, sqlChars);
    int ret = sqlite3_exec(db->handle, sql.c_str(), nullptr, nullptr, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(db->handle), sql.c_str());
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeTrace(JNIEnv *env, jclass type, jlong jptr, jobject jfunc) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    if (db->trace) {
        env->DeleteGlobalRef(db->trace);
        db->trace = nullptr;
    }
    if (jfunc) {
        db->trace = env->NewGlobalRef(jfunc);
        int ret = sqlite3_trace_v2(db->handle, SQLITE_TRACE_STMT, java_trace, db->trace);
        if (SQLITE_OK != ret) {
            throw_sqlite_exception(env, sqlite3_errmsg(db->handle));
        }
    }
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteDb_nativeExecForDouble(JNIEnv *env, jclass type, jlong jptr, jstring jsql) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    const char *csql = env->GetStringUTFChars(jsql, nullptr);
    std::string sql(csql);
    env->ReleaseStringUTFChars(jsql, csql);
    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(db->handle, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(db->handle), sql.c_str());
    }
    double value = 0;
    if (SQLITE_ROW == sqlite3_step(stmt)) {
        value = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteDb_nativePrepare(JNIEnv *env, jclass type, jlong jptr, jstring jsql) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    const char *sqlChars = env->GetStringUTFChars(jsql, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(jsql, sqlChars);
    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(db->handle, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(db->handle), sql.c_str());
    }
    return reinterpret_cast<jlong>(stmt);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCreateCollation(JNIEnv *env, jclass type, jlong jptr, jstring jname,
                                             jobject jcomparator) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    jobject comparator = env->NewGlobalRef(jcomparator);
    const char *nameChars = env->GetStringUTFChars(jname, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(jname, nameChars);
    int ret = sqlite3_create_collation_v2(db->handle, name.c_str(), SQLITE_UTF8, comparator, java_compare,
            java_destroy);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(db->handle));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCreateFunction(JNIEnv *env, jclass type, jlong jptr, jstring jname, jint numArgs,
                                            jobject jfunc) {
    SQLiteDb *db = reinterpret_cast<SQLiteDb *>(jptr);
    const char *nameChars = env->GetStringUTFChars(jname, nullptr);
    std::string name(nameChars);
    env->ReleaseStringUTFChars(jname, nameChars);
    jobject func = env->NewGlobalRef(jfunc);
    int ret = sqlite3_create_function_v2(db->handle, name.c_str(), numArgs, SQLITE_UTF8, func, java_func, nullptr,
            nullptr, java_destroy);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(db->handle));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindNull(JNIEnv *env, jclass type, jlong jptr, jint index) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    int ret = sqlite3_bind_null(stmt, index);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindLong(JNIEnv *env, jclass type, jlong jptr, jint index, jlong value) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    int ret = sqlite3_bind_int64(stmt, index, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindDouble(JNIEnv *env, jclass type, jlong jptr, jint index, jdouble value) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    int ret = sqlite3_bind_double(stmt, index, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindString(JNIEnv *env, jclass type, jlong jptr, jint index, jstring jvalue) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    const char *value = env->GetStringUTFChars(jvalue, nullptr);
    int ret = sqlite3_bind_text(stmt, index, value, static_cast<int>(strlen(value)), SQLITE_TRANSIENT);
    env->ReleaseStringUTFChars(jvalue, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindBlob(JNIEnv *env, jclass type, jlong jptr, jint index, jbyteArray jvalue) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    jbyte *value = env->GetByteArrayElements(jvalue, nullptr);
    int ret = sqlite3_bind_blob(stmt, index, value, env->GetArrayLength(jvalue), SQLITE_TRANSIENT);
    env->ReleaseByteArrayElements(jvalue, value, 0);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeClearBindings(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    int ret = sqlite3_reset(stmt);
    if (SQLITE_OK == ret) {
        ret = sqlite3_clear_bindings(stmt);
    }
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteStmt_nativeInsert(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    sqlite3 *db = sqlite3_db_handle(stmt);
    int ret = sqlite3_step(stmt);
    if (SQLITE_DONE != ret) {
        sqlite3_finalize(stmt);
        throw_sqlite_exception(env, sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteStmt_nativeExecute(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    sqlite3 *db = sqlite3_db_handle(stmt);
    int ret = sqlite3_step(stmt);
    if (SQLITE_DONE != ret) {
        sqlite3_finalize(stmt);
        throw_sqlite_exception(env, sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_changes(db);
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteStmt_nativeBusy(JNIEnv *env, jclass type, jlong jptr) {
    return sqlite3_stmt_busy(reinterpret_cast<sqlite3_stmt *>(jptr));
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeReset(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    int ret = sqlite3_reset(stmt);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeFinalize(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_finalize(reinterpret_cast<sqlite3_stmt *>(jptr));
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_RowIterator_nativeStep(JNIEnv *env, jclass type, jlong jptr) {
    return sqlite3_step(reinterpret_cast<sqlite3_stmt *>(jptr));
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_RowIterator_nativeGetColumnCount(JNIEnv *env, jclass type, jlong jptr) {
    return sqlite3_column_count(reinterpret_cast<sqlite3_stmt *>(jptr));
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_RowIterator_nativeGetColumnName(JNIEnv *env, jclass type, jlong jptr, jint index) {
    return env->NewStringUTF(sqlite3_column_name(reinterpret_cast<sqlite3_stmt *>(jptr), index));
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_RowIterator_nativeGetColumnLong(JNIEnv *env, jclass type, jlong jptr, jint index) {
    return sqlite3_column_int64(reinterpret_cast<sqlite3_stmt *>(jptr), index);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_RowIterator_nativeGetColumnDouble(JNIEnv *env, jclass type, jlong jptr, jint index) {
    return sqlite3_column_double(reinterpret_cast<sqlite3_stmt *>(jptr), index);
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_RowIterator_nativeGetColumnString(JNIEnv *env, jclass type, jlong jptr, jint index) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    const unsigned char *text = sqlite3_column_text(stmt, index);
    return env->NewStringUTF(reinterpret_cast<const char *>(text));
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_sqlite4a_RowIterator_nativeGetColumnBlob(JNIEnv *env, jclass type, jlong jptr, jint index) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(jptr);
    int size = sqlite3_column_bytes(stmt, index);
    const void *value = sqlite3_column_blob(stmt, index);
    jbyteArray jvalue = env->NewByteArray(size);
    env->SetByteArrayRegion(jvalue, 0, size, reinterpret_cast<const jbyte *>(value));
    return jvalue;
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteValue_nativeLongValue(JNIEnv *env, jclass type, jlong jptr) {
    return sqlite3_value_int64(reinterpret_cast<sqlite3_value *>(jptr));
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteValue_nativeDoubleValue(JNIEnv *env, jclass type, jlong jptr) {
    return sqlite3_value_double(reinterpret_cast<sqlite3_value *>(jptr));
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteValue_nativeStringValue(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(jptr);
    const unsigned char *text = sqlite3_value_text(value);
    return env->NewStringUTF(reinterpret_cast<const char *>(text));
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_sqlite4a_SQLiteValue_nativeBlobValue(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(jptr);
    int size = sqlite3_value_bytes(value);
    const void *blob = sqlite3_value_blob(value);
    jbyteArray jvalue = env->NewByteArray(size);
    env->SetByteArrayRegion(jvalue, 0, size, reinterpret_cast<const jbyte *>(blob));
    return jvalue;
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultNull(JNIEnv *env, jclass type, jlong jptr) {
    sqlite3_result_null(reinterpret_cast<sqlite3_context *>(jptr));
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultLong(JNIEnv *env, jclass type, jlong jptr, jlong result) {
    sqlite3_result_int64(reinterpret_cast<sqlite3_context *>(jptr), result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultText(JNIEnv *env, jclass type, jlong jptr, jstring resultStr) {
    sqlite3_context *context = reinterpret_cast<sqlite3_context *>(jptr);
    const char *result = env->GetStringUTFChars(resultStr, nullptr);
    sqlite3_result_text(context, result, static_cast<int>(strlen(result)), SQLITE_TRANSIENT);
    env->ReleaseStringUTFChars(resultStr, result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultDouble(JNIEnv *env, jclass type, jlong jptr, jdouble result) {
    sqlite3_result_double(reinterpret_cast<sqlite3_context *>(jptr), result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultBlob(JNIEnv *env, jclass type, jlong jptr, jbyteArray resultArr) {
    sqlite3_context *context = reinterpret_cast<sqlite3_context *>(jptr);
    jbyte *result = env->GetByteArrayElements(resultArr, nullptr);
    sqlite3_result_blob(context, result, env->GetArrayLength(resultArr), SQLITE_TRANSIENT);
    env->ReleaseByteArrayElements(resultArr, result, 0);
}
