cmake_minimum_required(VERSION 3.4.1)

set(SOURCES
    src/main/cpp/sqlite3.c
    src/main/cpp/sqlite3_jni.cc)

set(HEADERS src/main/cpp/sqlite3.h)

add_definitions(
        -D__ANDROID__
        -DHAVE_USLEEP=1
        -DSQLITE_HAVE_ISNAN
        -DSQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576
        -DSQLITE_THREADSAFE=2
        -DSQLITE_TEMP_STORE=3
        -DSQLITE_POWERSAFE_OVERWRITE=1
        -DSQLITE_DEFAULT_AUTOVACUUM=1
        -DSQLITE_ENABLE_MEMORY_MANAGEMENT=1
        -DSQLITE_ENABLE_FTS5
        -DSQLITE_ENABLE_JSON1
        -DSQLITE_OMIT_BUILTIN_TEST
        -DSQLITE_OMIT_COMPILEOPTION_DIAGS
        -DSQLITE_DEFAULT_FILE_PERMISSIONS=0600
)

add_library(sqlite3_jni SHARED ${HEADERS} ${SOURCES})

target_link_libraries(sqlite3_jni m log)