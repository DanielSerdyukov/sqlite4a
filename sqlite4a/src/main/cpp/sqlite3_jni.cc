#include <jni.h>
#include "sqlite3.h"

extern "C" JNIEXPORT jlong JNICALL
Java_sqlite4a_SQLite_getLibVersion(JNIEnv *env, jclass type) {
    return sqlite3_libversion_number();
}