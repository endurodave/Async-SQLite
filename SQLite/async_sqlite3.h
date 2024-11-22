#ifndef ASYNC_SQLITE3_H
#define ASYNC_SQLITE3_H

#include "sqlite3.h"
#include <chrono>
#include "DelegateThread.h"

// Asynchronous SQLite API that to invoke from a single thread of control.
// All API functions are thread-safe and can be called from any thread of control.
// Optionally add the timeout argument to any API for a maximum wait time.

namespace async
{
    #undef max  // Prevent compiler error on next line if max is defined
    constexpr auto MAX_WAIT = std::chrono::milliseconds::max();
    constexpr auto NO_WAIT = std::chrono::milliseconds(0);

    // Call one-time at application startup
    void sqlite3_init_async(void);

    SQLITE_API int sqlite3_set_authorizer(
        sqlite3*,
        int (*xAuth)(void*, int, const char*, const char*, const char*, const char*),
        void* pUserData,
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_trace_v2(
        sqlite3*,
        unsigned uMask,
        int(*xCallback)(unsigned, void*, void*, void*),
        void* pCtx,
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*, std::chrono::milliseconds timeout = MAX_WAIT);

    // Get a pointer to the internal thread
    DelegateLib::DelegateThread* sqlite3_get_thread(void);

    SQLITE_API int sqlite3_open(
        const char* filename,   /* Database filename (UTF-8) */
        sqlite3** ppDb,         /* OUT: SQLite db handle */
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    // Opening a database connection with flags
    SQLITE_API int sqlite3_open_v2(
        const char* filename,             /* Database filename (UTF-8) */
        sqlite3** ppDb,                   /* OUT: SQLite db handle */
        int flags,                        /* Open flags */
        const char* zVfs,                 /* Optional VFS name */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Closing a database connection
    SQLITE_API int sqlite3_close(
        sqlite3* db,                     /* Database handle */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_prepare(
        sqlite3* db,            /* Database handle */
        const char* zSql,       /* SQL statement, UTF-8 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const char** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Preparing an SQL statement
    SQLITE_API int sqlite3_prepare_v2(
        sqlite3* db,                     /* Database handle */
        const char* sql,                  /* SQL query */
        int nBytes,                       /* Byte length of SQL query */
        sqlite3_stmt** ppStmt,            /* Prepared statement */
        const char** pzTail,              /* Unused portion of SQL query */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_prepare_v3(
        sqlite3* db,            /* Database handle */
        const char* zSql,       /* SQL statement, UTF-8 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const char** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Finalizing a prepared statement
    SQLITE_API int sqlite3_finalize(
        sqlite3_stmt* pStmt,              /* Statement to finalize */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Step through the prepared statement (execute)
    SQLITE_API int sqlite3_step(
        sqlite3_stmt* pStmt,              /* Statement to step through */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Binding values to a prepared statement
    SQLITE_API int sqlite3_bind_int(
        sqlite3_stmt* pStmt,              /* Statement to bind to */
        int idx,                          /* Parameter index (1-based) */
        int value,                        /* Value to bind */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_bind_text(
        sqlite3_stmt* pStmt,              /* Statement to bind to */
        int idx,                          /* Parameter index (1-based) */
        const char* value,                /* Text value to bind */
        int n,                            /* Length of text */
        sqlite3_destructor_type dtor,     /* Destructor for the string */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Get the number of columns in a query result
    SQLITE_API int sqlite3_column_count(
        sqlite3_stmt* pStmt,              /* Statement */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Getting the column name
    SQLITE_API const char* sqlite3_column_name(
        sqlite3_stmt* pStmt,              /* Statement */
        int col,                          /* Column index */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Executing a simple SQL query (non-prepared statement)
    SQLITE_API int sqlite3_exec(
        sqlite3* db,                     /* Database handle */
        const char* sql,                 /* SQL query */
        sqlite3_callback callback,       /* Callback function */
        void* pArg,                      /* Callback argument */
        char** errMsg,                   /* Error message */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Beginning a transaction
    SQLITE_API int sqlite3_exec_begin(
        sqlite3* db,                     /* Database handle */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Committing a transaction
    SQLITE_API int sqlite3_exec_commit(
        sqlite3* db,                     /* Database handle */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Rolling back a transaction
    SQLITE_API int sqlite3_exec_rollback(
        sqlite3* db,                     /* Database handle */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Getting the row count from a query result
    SQLITE_API int sqlite3_changes(
        sqlite3* db,                     /* Database handle */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    // Getting database status information
    SQLITE_API int sqlite3_db_status(
        sqlite3* db,                     /* Database handle */
        int op,                           /* Status operation */
        int* pCurrent,                   /* Current value of status */
        int* pHighwater,                 /* Highwater mark */
        int resetFlag,                   /* Reset status or not */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API const void* sqlite3_column_blob(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API double sqlite3_column_double(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_int(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const unsigned char* sqlite3_column_text(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_text16(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_value* sqlite3_column_value(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_bytes(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_bytes16(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_type(sqlite3_stmt*, int iCol, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API void* sqlite3_malloc(int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void* sqlite3_malloc64(sqlite3_uint64, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void* sqlite3_realloc(void*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void* sqlite3_realloc64(void*, sqlite3_uint64, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_free(void*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_uint64 sqlite3_msize(void*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_uri_parameter(sqlite3_filename z, const char* zParam, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_uri_boolean(sqlite3_filename z, const char* zParam, int bDefault, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_uri_int64(sqlite3_filename, const char*, sqlite3_int64, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_uri_key(sqlite3_filename z, int N, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_filename_database(sqlite3_filename, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_filename_journal(sqlite3_filename, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_filename_wal(sqlite3_filename, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API sqlite3_file* sqlite3_database_file_object(const char*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API sqlite3_filename sqlite3_create_filename(
        const char* zDatabase,
        const char* zJournal,
        const char* zWal,
        int nParam,
        const char** azParam,
        std::chrono::milliseconds timeout = MAX_WAIT
    );
    SQLITE_API void sqlite3_free_filename(sqlite3_filename, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_errcode(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_extended_errcode(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_errmsg(sqlite3*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_errmsg16(sqlite3*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_errstr(int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_error_offset(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_limit(sqlite3*, int id, int newVal, std::chrono::milliseconds timeout = MAX_WAIT);
}

#endif