#include <jni.h>
#include <string>
#include <android/log.h>
#include "sqlite3.h"

#ifndef LOG_TAG
#define LOG_TAG "sqlite3_jni"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif // LOG_TAG

static const int SOFT_HEAP_LIMIT = 8 * 1024 * 1024;

static JavaVM *gJavaVm = 0;

static struct {
    jclass clazz;
} gSQLiteException;

static struct {
    jclass clazz;
    jmethodID compare;
} gComparator;

static struct {
    jclass clazz;
    jmethodID invoke;
} gSQLiteInvokable;

static struct {
    jclass clazz;
    jmethodID log;
} gSQLiteLog;

struct SQLiteDb {
    sqlite3 *db = nullptr;
    jobject logger = nullptr;
};

static void throw_sqlite_exception(JNIEnv *env, int code, const char *message) {
    if (gSQLiteException.clazz) {
        env->ThrowNew(gSQLiteException.clazz, message);
    } else {
        LOGE("%s [%d]", message, code);
    }
}

static void throw_sqlite_exception(JNIEnv *env, int code, const char *sql, const char *error) {
    std::string message(error);
    message += ", while executing: ";
    message += sql;
    if (gSQLiteException.clazz) {
        env->ThrowNew(gSQLiteException.clazz, message.c_str());
    } else {
        LOGE("%s [%d]", message.c_str(), code);
    }
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

static int java_compare(void *data, int ll, const void *lv, int rl, const void *rv) {
    JNIEnv *env;
    if (gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK && data) {
        std::string lhs(static_cast<const char *>(lv), static_cast<size_t>(ll));
        std::string rhs(static_cast<const char *>(rv), static_cast<size_t>(rl));
        jobject comparator = env->NewLocalRef(reinterpret_cast<jobject>(data));
        jstring lhsStr = env->NewStringUTF(lhs.c_str());
        jstring rhsStr = env->NewStringUTF(rhs.c_str());
        int ret = env->CallIntMethod(comparator, gComparator.compare, lhsStr, rhsStr);
        env->DeleteLocalRef(lhsStr);
        env->DeleteLocalRef(rhsStr);
        env->DeleteLocalRef(comparator);
        if (env->ExceptionCheck()) {
            LOGE("An exception was thrown by custom collation.");
            env->ExceptionClear();
        }
        return ret;
    }
    return binary_compare(data, ll, lv, rl, rv);
}

static void java_invoke(sqlite3_context *context, int argc, sqlite3_value **argv) {
    JNIEnv *env;
    if (gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK) {
        jlong values[argc];
        for (int i = 0; i < argc; ++i) {
            values[i] = reinterpret_cast<jlong>(argv[i]);
        }
        void *data = sqlite3_user_data(context);
        if (data != nullptr) {
            jobject func = env->NewLocalRef(reinterpret_cast<jobject>(data));
            jlongArray valuesArr = env->NewLongArray(argc);
            env->SetLongArrayRegion(valuesArr, 0, argc, values);
            env->CallVoidMethod(func, gSQLiteInvokable.invoke, reinterpret_cast<jlong>(context), valuesArr);
            env->DeleteLocalRef(valuesArr);
            env->DeleteLocalRef(func);
            if (env->ExceptionCheck()) {
                LOGE("An exception was thrown by custom function.");
                env->ExceptionClear();
            }
        }
    }
}

static void java_finalize(void *data) {
    JNIEnv *env;
    if (gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK && data) {
        env->DeleteGlobalRef(reinterpret_cast<jobject>(data));
    }
}

static int java_log(unsigned mask, void *context, void *p, void *x) {
    JNIEnv *env;
    if (gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK && context) {
        jobject logger = env->NewLocalRef(reinterpret_cast<jobject>(context));
        if (mask & SQLITE_TRACE_STMT != 0) {
            std::string trigger = "--";
            std::string sql(reinterpret_cast<const char *>(x));
            if (sql.compare(0, trigger.length(), trigger) != 0) {
                sqlite3_stmt *stmt = static_cast<sqlite3_stmt *>(p);
                char *expandedSql = sqlite3_expanded_sql(stmt);
                jstring sqlStr = env->NewStringUTF(expandedSql);
                sqlite3_free(expandedSql);
                env->CallVoidMethod(logger, gSQLiteLog.log, sqlStr);
                env->DeleteLocalRef(sqlStr);
            }
        }
        env->DeleteLocalRef(logger);
        if (env->ExceptionCheck()) {
            LOGE("An exception was thrown by sqlite_trace_v2 callback.");
            env->ExceptionClear();
        }
    }
    return 0;
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    gJavaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    gSQLiteException.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteException")));
    gComparator.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("java/util/Comparator")));
    gComparator.compare = env->GetMethodID(gComparator.clazz, "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I");
    gSQLiteInvokable.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteInvokable")));
    gSQLiteInvokable.invoke = env->GetMethodID(gSQLiteInvokable.clazz, "invoke", "(J[J)V");
    gSQLiteLog.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteLog")));
    gSQLiteLog.log = env->GetMethodID(gSQLiteLog.clazz, "log", "(Ljava/lang/String;)V");
    sqlite3_soft_heap_limit64(SOFT_HEAP_LIMIT);
    sqlite3_initialize();
    return JNI_VERSION_1_6;
}

//region SQLite
extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLite_getVersion(JNIEnv *env, jclass type) {
    return env->NewStringUTF(sqlite3_libversion());
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_getVersionNumber(JNIEnv *env, jclass type) {
    return sqlite3_libversion_number();
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_nativeOpenV2(JNIEnv *env, jclass type, jstring pathStr, jint flags, jint busyTimeout) {
    sqlite3 *db;

    const char *path = env->GetStringUTFChars(pathStr, nullptr);
    int ret = sqlite3_open_v2(path, &db, flags, nullptr);
    env->ReleaseStringUTFChars(pathStr, path);

    if (ret != SQLITE_OK) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(db));
        return 0;
    }

    if ((flags & SQLITE_OPEN_READWRITE) && sqlite3_db_readonly(db, nullptr)) {
        sqlite3_close_v2(db);
        throw_sqlite_exception(env, ret, "Could not open the database in read/write mode.");
        return 0;
    }

    ret = sqlite3_busy_timeout(db, busyTimeout);
    if (ret != SQLITE_OK) {
        throw_sqlite_exception(env, ret, "Could not set busy timeout");
        sqlite3_close(db);
        return 0;
    }

    SQLiteDb *handle = new SQLiteDb;
    handle->db = db;

    return reinterpret_cast<jlong>(handle);
}
//endregion

//region SQLiteDb
extern "C" JNIEXPORT jboolean JNICALL
Java_sqlite4a_SQLiteDb_nativeIsReadOnly(JNIEnv *env, jclass type, jlong dbPtr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);
    if (sqlite3_db_readonly(handle->db, nullptr)) {
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeTraceV2(JNIEnv *env, jclass type, jlong dbPtr, jobject logger, jint mask) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);
    jobject newLogger = env->NewGlobalRef(logger);
    int ret = sqlite3_trace_v2(handle->db, static_cast<unsigned int>(mask), &java_log, newLogger);
    if (handle->logger) {
        env->DeleteGlobalRef(handle->logger);
    }
    handle->logger = newLogger;
    if (SQLITE_OK != ret) {
        env->DeleteGlobalRef(handle->logger);
        handle->logger = nullptr;
        throw_sqlite_exception(env, ret, sqlite3_errmsg(handle->db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeExec(JNIEnv *env, jclass type, jlong dbPtr, jstring sqlStr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);

    const char *sqlChars = env->GetStringUTFChars(sqlStr, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(sqlStr, sqlChars);

    int ret = sqlite3_exec(handle->db, sql.c_str(), nullptr, nullptr, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(handle->db));
    }
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteDb_nativeExecForDouble(JNIEnv *env, jclass type, jlong dbPtr, jstring sqlStr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);

    const char *sqlChars = env->GetStringUTFChars(sqlStr, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(sqlStr, sqlChars);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(handle->db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sql.c_str(), sqlite3_errmsg(handle->db));
    }

    double value = 0;
    if (SQLITE_ROW == sqlite3_step(stmt)) {
        value = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return value;
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteDb_nativeGetAutocommit(JNIEnv *env, jclass type, jlong dbPtr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);
    return sqlite3_get_autocommit(handle->db);
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteDb_nativePrepareV2(JNIEnv *env, jclass type, jlong dbPtr, jstring sqlStr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);

    const char *sqlChars = env->GetStringUTFChars(sqlStr, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(sqlStr, sqlChars);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(handle->db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sql.c_str(), sqlite3_errmsg(handle->db));
    }

    return reinterpret_cast<jlong>(stmt);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCreateCollationV2(JNIEnv *env, jclass caller, jlong dbPtr, jstring nameStr,
                                               jobject comparator) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);
    jobject comparatorRef = env->NewGlobalRef(comparator);
    const char *name = env->GetStringUTFChars(nameStr, nullptr);
    int ret = sqlite3_create_collation_v2(handle->db, name, SQLITE_UTF8, reinterpret_cast<void *>(comparatorRef),
                                          &java_compare, &java_finalize);
    env->ReleaseStringUTFChars(nameStr, name);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(handle->db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCreateFunctionV2(JNIEnv *env, jclass caller, jlong dbPtr, jstring nameStr,
                                              jint numArgs, jobject func) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);
    jobject funcRef = env->NewGlobalRef(func);
    const char *name = env->GetStringUTFChars(nameStr, nullptr);
    int ret = sqlite3_create_function_v2(handle->db, name, numArgs, SQLITE_UTF8, reinterpret_cast<void *>(funcRef),
                                         &java_invoke, nullptr, nullptr, &java_finalize);
    env->ReleaseStringUTFChars(nameStr, name);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(handle->db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCloseV2(JNIEnv *env, jclass type, jlong dbPtr) {
    SQLiteDb *handle = reinterpret_cast<SQLiteDb *>(dbPtr);
    sqlite3_close_v2(handle->db);
    if (handle->logger) {
        env->DeleteGlobalRef(handle->logger);
        handle->logger = nullptr;
    }
    delete handle;
}
//endregion

//region SQLiteStmt
extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteStmt_nativeGetSql(JNIEnv *env, jclass type, jlong stmtPtr) {
    return env->NewStringUTF(sqlite3_sql(reinterpret_cast<sqlite3_stmt *>(stmtPtr)));
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteStmt_nativeGetExpandedSql(JNIEnv *env, jclass type, jlong stmtPtr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    char *expandedSql = sqlite3_expanded_sql(stmt);
    jstring sqlStr = env->NewStringUTF(expandedSql);
    sqlite3_free(expandedSql);
    return sqlStr;
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindNull(JNIEnv *env, jclass caller, jlong stmtPtr, jint index) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    int ret = sqlite3_bind_null(stmt, index);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindLong(JNIEnv *env, jclass caller, jlong stmtPtr, jint index, jlong value) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    int ret = sqlite3_bind_int64(stmt, index, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindDouble(JNIEnv *env, jclass caller, jlong stmtPtr, jint index, jdouble value) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    int ret = sqlite3_bind_double(stmt, index, value);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindString(JNIEnv *env, jclass caller, jlong stmtPtr, jint index,
                                          jstring valueStr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    const char *value = env->GetStringUTFChars(valueStr, nullptr);
    int ret = sqlite3_bind_text(stmt, index, value, static_cast<int>(strlen(value)), SQLITE_TRANSIENT);
    env->ReleaseStringUTFChars(valueStr, value);
    if (SQLITE_OK != ret) {
        sqlite3 *db = sqlite3_db_handle(stmt);
        throw_sqlite_exception(env, ret, sqlite3_errmsg(db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeBindBlob(JNIEnv *env, jclass caller, jlong stmtPtr, jint index,
                                        jbyteArray valueArr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    jbyte *value = env->GetByteArrayElements(valueArr, nullptr);
    int ret = sqlite3_bind_blob(stmt, index, value, env->GetArrayLength(valueArr), SQLITE_TRANSIENT);
    env->ReleaseByteArrayElements(valueArr, value, 0);
    if (SQLITE_OK != ret) {
        sqlite3 *db = sqlite3_db_handle(stmt);
        throw_sqlite_exception(env, ret, sqlite3_errmsg(db));
    }
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeClearBindings(JNIEnv *env, jclass caller, jlong stmtPtr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    int ret = sqlite3_reset(stmt);
    if (ret == SQLITE_OK) {
        ret = sqlite3_clear_bindings(stmt);
    }
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(sqlite3_db_handle(stmt)));
    }
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteStmt_nativeExecuteInsert(JNIEnv *env, jclass caller, jlong stmtPtr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    sqlite3 *db = sqlite3_db_handle(stmt);
    int ret = sqlite3_step(stmt);
    if (SQLITE_DONE != ret) {
        sqlite3_finalize(stmt);
        throw_sqlite_exception(env, ret, sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteStmt_nativeExecuteUpdateDelete(JNIEnv *env, jclass caller, jlong stmtPtr) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    sqlite3 *db = sqlite3_db_handle(stmt);
    int ret = sqlite3_step(stmt);
    if (SQLITE_DONE != ret) {
        sqlite3_finalize(stmt);
        throw_sqlite_exception(env, ret, sqlite3_errmsg(db));
        return -1;
    }
    return sqlite3_changes(db);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteStmt_nativeFinalize(JNIEnv *env, jclass caller, jlong stmtPtr) {
    sqlite3_finalize(reinterpret_cast<sqlite3_stmt *>(stmtPtr));
}
//endregion

//region SQLiteCursor
extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteCursor_nativeStep(JNIEnv *env, jclass caller, jlong stmtPtr) {
    return sqlite3_step(reinterpret_cast<sqlite3_stmt *>(stmtPtr));
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnLong(JNIEnv *env, jclass caller, jlong stmtPtr, jint index) {
    return sqlite3_column_int64(reinterpret_cast<sqlite3_stmt *>(stmtPtr), index);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnDouble(JNIEnv *env, jclass caller, jlong stmtPtr, jint index) {
    return sqlite3_column_double(reinterpret_cast<sqlite3_stmt *>(stmtPtr), index);
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnString(JNIEnv *env, jclass caller, jlong stmtPtr, jint index) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    const unsigned char *text = sqlite3_column_text(stmt, index);
    return env->NewStringUTF(reinterpret_cast<const char *>(text));
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnBlob(JNIEnv *env, jclass caller, jlong stmtPtr, jint index) {
    sqlite3_stmt *stmt = reinterpret_cast<sqlite3_stmt *>(stmtPtr);
    const void *value = sqlite3_column_blob(stmt, index);
    int size = sqlite3_column_bytes(stmt, index);
    jbyteArray valueArr = env->NewByteArray(size);
    env->SetByteArrayRegion(valueArr, 0, size, reinterpret_cast<const jbyte *>(value));
    return valueArr;
}

extern "C" JNIEXPORT jint JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnCount(JNIEnv *env, jclass caller, jlong stmtPtr) {
    return sqlite3_column_count(reinterpret_cast<sqlite3_stmt *>(stmtPtr));
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteCursor_nativeGetColumnName(JNIEnv *env, jclass caller, jlong stmtPtr, jint index) {
    return env->NewStringUTF(sqlite3_column_name(reinterpret_cast<sqlite3_stmt *>(stmtPtr), index));
}
//endregion

//region SQLiteValue
extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteValue_nativeLongValue(JNIEnv *env, jclass type, jlong valuePtr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(valuePtr);
    return sqlite3_value_int64(value);
}

extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteValue_nativeStringValue(JNIEnv *env, jclass type, jlong valuePtr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(valuePtr);
    const unsigned char *text = sqlite3_value_text(value);
    return env->NewStringUTF(reinterpret_cast<const char *>(text));
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteValue_nativeDoubleValue(JNIEnv *env, jclass type, jlong valuePtr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(valuePtr);
    return sqlite3_value_double(value);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_sqlite4a_SQLiteValue_nativeBlobValue(JNIEnv *env, jclass type, jlong valuePtr) {
    sqlite3_value *value = reinterpret_cast<sqlite3_value *>(valuePtr);
    const void *blob = sqlite3_value_blob(value);
    int size = sqlite3_value_bytes(value);
    jbyteArray blobArr = env->NewByteArray(size);
    env->SetByteArrayRegion(blobArr, 0, size, reinterpret_cast<const jbyte *>(blob));
    return blobArr;
}
//endregion

//region SQLiteContext
extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultNull(JNIEnv *env, jclass type, jlong contextPtr) {
    sqlite3_result_null(reinterpret_cast<sqlite3_context *>(contextPtr));
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultLong(JNIEnv *env, jclass type, jlong contextPtr, jlong result) {
    sqlite3_result_int64(reinterpret_cast<sqlite3_context *>(contextPtr), result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultText(JNIEnv *env, jclass type, jlong contextPtr, jstring resultStr) {
    sqlite3_context *context = reinterpret_cast<sqlite3_context *>(contextPtr);
    const char *result = env->GetStringUTFChars(resultStr, nullptr);
    sqlite3_result_text(context, result, static_cast<int>(strlen(result)), SQLITE_TRANSIENT);
    env->ReleaseStringUTFChars(resultStr, result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultDouble(JNIEnv *env, jclass type, jlong contextPtr, jdouble result) {
    sqlite3_result_double(reinterpret_cast<sqlite3_context *>(contextPtr), result);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteContext_nativeResultBlob(JNIEnv *env, jclass type, jlong contextPtr, jbyteArray resultArr) {
    sqlite3_context *context = reinterpret_cast<sqlite3_context *>(contextPtr);
    jbyte *result = env->GetByteArrayElements(resultArr, nullptr);
    sqlite3_result_blob(context, result, env->GetArrayLength(resultArr), SQLITE_TRANSIENT);
    env->ReleaseByteArrayElements(resultArr, result, 0);
}
//endregion