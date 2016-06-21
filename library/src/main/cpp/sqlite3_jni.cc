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

static void throw_sqlite_exception(int code, const char *message) {
    JNIEnv *env;
    if (gJavaVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_OK) {
        throw_sqlite_exception(env, code, message);
    } else {
        LOGE("[%d] %s", code, message);
    }
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    gJavaVm = vm;
    JNIEnv *env;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    gSQLiteException.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteException")));
/*    gComparator.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("java/util/Comparator")));
    gComparator.compare = env->GetMethodID(gComparator.clazz, "compare", "(Ljava/lang/Object;Ljava/lang/Object;)I");
    gSQLiteInvokable.clazz = reinterpret_cast<jclass>(env->NewGlobalRef(env->FindClass("sqlite4a/SQLiteInvokable")));
    gSQLiteInvokable.invoke = env->GetMethodID(gSQLiteInvokable.clazz, "invoke", "(J[J)V");*/
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

    return reinterpret_cast<jlong>(db);
}
//endregion

//region SQLiteDb
extern "C" JNIEXPORT jboolean JNICALL
Java_sqlite4a_SQLiteDb_nativeIsReadOnly(JNIEnv *env, jclass type, jlong dbPtr) {
    sqlite3 *db = reinterpret_cast<sqlite3 *>(dbPtr);
    if (sqlite3_db_readonly(db, nullptr)) {
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeTrace(JNIEnv *env, jclass type, jlong dbPtr) {
    sqlite3 *db = reinterpret_cast<sqlite3 *>(dbPtr);
    sqlite3_trace(db, [](void *unused, const char *sql) { LOGI("%s", sql); }, nullptr);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeExec(JNIEnv *env, jclass type, jlong dbPtr, jstring sqlStr) {
    sqlite3 *db = reinterpret_cast<sqlite3 *>(dbPtr);

    const char *sqlChars = env->GetStringUTFChars(sqlStr, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(sqlStr, sqlChars);

    int ret = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sqlite3_errmsg(db));
    }
}

extern "C" JNIEXPORT jdouble JNICALL
Java_sqlite4a_SQLiteDb_nativeExecForDouble(JNIEnv *env, jclass type, jlong dbPtr, jstring sqlStr) {
    sqlite3 *db = reinterpret_cast<sqlite3 *>(dbPtr);

    const char *sqlChars = env->GetStringUTFChars(sqlStr, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(sqlStr, sqlChars);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sql.c_str(), sqlite3_errmsg(db));
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
    return sqlite3_get_autocommit(reinterpret_cast<sqlite3 *>(dbPtr));
}

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLiteDb_nativePrepareV2(JNIEnv *env, jclass type, jlong dbPtr, jstring sqlStr) {
    sqlite3 *db = reinterpret_cast<sqlite3 *>(dbPtr);

    const char *sqlChars = env->GetStringUTFChars(sqlStr, nullptr);
    std::string sql(sqlChars);
    env->ReleaseStringUTFChars(sqlStr, sqlChars);

    sqlite3_stmt *stmt;
    int ret = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.length()), &stmt, nullptr);
    if (SQLITE_OK != ret) {
        throw_sqlite_exception(env, ret, sql.c_str(), sqlite3_errmsg(db));
    }

    return reinterpret_cast<jlong>(stmt);
}

extern "C" JNIEXPORT void JNICALL
Java_sqlite4a_SQLiteDb_nativeCloseV2(JNIEnv *env, jclass type, jlong dbPtr) {
    sqlite3 *db = reinterpret_cast<sqlite3 *>(dbPtr);
    sqlite3_close_v2(db);
}
//endregion

//region SQLiteStmt
extern "C" JNIEXPORT jstring JNICALL
Java_sqlite4a_SQLiteStmt_nativeGetSql(JNIEnv *env, jclass type, jlong stmtPtr) {
    return env->NewStringUTF(sqlite3_sql(reinterpret_cast<sqlite3_stmt *>(stmtPtr)));
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