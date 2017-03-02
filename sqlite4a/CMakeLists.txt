cmake_minimum_required(VERSION 3.4.1)

set(PROJECT_NAME sqlite3_jni)

set(SOURCES
    src/main/cpp/sqlite3.h
    src/main/cpp/sqlite3.c
    src/main/cpp/sqlite3_jni.cc)

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
        -DSQLITE_DEFAULT_FOREIGN_KEYS=1
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_library(LIB_m m)
find_library(LIB_log log)

target_link_libraries(${PROJECT_NAME} ${LIB_m} ${LIB_log})