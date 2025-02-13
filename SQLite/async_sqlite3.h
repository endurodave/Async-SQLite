// Asynchronous SQLite wrapper using C++ Delegates
// @see https://github.com/endurodave/Async-SQLite
// @see https://github.com/endurodave/AsyncMulticastDelegateModern
// David Lafreniere, Nov 2024

// An asynchronous SQLite API that invokes all calls on an internal thread of control.
// All API functions are thread-safe and can be called from any thread. Optionally, 
// you can add the timeout argument to any API function to specify a maximum wait time.
// This will block the calling thread for up to the specified timeout while waiting 
// for the database worker thread to complete the requested function call. If the wait 
// time expires, SQLITE_ERROR is returned, and the function is not called.
//
// Call sqlite3_init_async() once at startup before any other API.

#ifndef ASYNC_SQLITE3_H
#define ASYNC_SQLITE3_H

#include "sqlite3.h"
#include <chrono>
#include "DelegateMQ.h"

namespace async
{
    #undef max  // Prevent compiler error on next line if max is defined
    constexpr auto MAX_WAIT = std::chrono::milliseconds::max();

    // Call one-time at application startup
    void sqlite3_init_async(void);

    // Get a pointer to the internal thread
    Thread* sqlite3_get_thread(void);

    SQLITE_API int sqlite3_trace_v2(
        sqlite3*,
        unsigned uMask,
        int(*xCallback)(unsigned, void*, void*, void*),
        void* pCtx,
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_open(
        const char* filename,   /* Database filename (UTF-8) */
        sqlite3** ppDb,         /* OUT: SQLite db handle */
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_open16(
        const void* filename,   /* Database filename (UTF-16) */
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

    SQLITE_API int sqlite3_close_v2(sqlite3* db,
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_initialize(void);

    SQLITE_API int sqlite3_shutdown(std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_extended_result_codes(sqlite3* db, int onoff, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_set_last_insert_rowid(sqlite3* db, sqlite3_int64 iRowid, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_changes(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_changes64(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_total_changes(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_total_changes64(sqlite3* db, std::chrono::milliseconds timeout = MAX_WAIT);
    
    SQLITE_API void sqlite3_interrupt(sqlite3* db);
    SQLITE_API int sqlite3_is_interrupted(sqlite3* db);

    SQLITE_API int sqlite3_complete(const char* sql, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_complete16(const void* sql, std::chrono::milliseconds timeout = MAX_WAIT);

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

    SQLITE_API int sqlite3_prepare16(
        sqlite3* db,            /* Database handle */
        const void* zSql,       /* SQL statement, UTF-16 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const void** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_prepare16_v2(
        sqlite3* db,            /* Database handle */
        const void* zSql,       /* SQL statement, UTF-16 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const void** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_prepare16_v3(
        sqlite3* db,            /* Database handle */
        const void* zSql,       /* SQL statement, UTF-16 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const void** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API const char* sqlite3_sql(sqlite3_stmt* pStmt, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API char* sqlite3_expanded_sql(sqlite3_stmt* pStmt, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_stmt_readonly(sqlite3_stmt* pStmt);
    SQLITE_API int sqlite3_stmt_isexplain(sqlite3_stmt* pStmt);
    SQLITE_API int sqlite3_stmt_explain(sqlite3_stmt* pStmt, int eMode, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_stmt_busy(sqlite3_stmt*);

    // Finalizing a prepared statement
    SQLITE_API int sqlite3_finalize(
        sqlite3_stmt* pStmt,              /* Statement to finalize */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_reset(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    // Step through the prepared statement (execute)
    SQLITE_API int sqlite3_step(
        sqlite3_stmt* pStmt,              /* Statement to step through */
        std::chrono::milliseconds timeout = MAX_WAIT /* Timeout duration */
    );

    SQLITE_API int sqlite3_data_count(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout = MAX_WAIT
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

    SQLITE_API const void* sqlite3_column_name16(sqlite3_stmt*, int N, std::chrono::milliseconds timeout = MAX_WAIT);

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

    SQLITE_API sqlite3_int64 sqlite3_memory_used(std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_memory_highwater(int resetFlag, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API void sqlite3_randomness(int N, void* P, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_set_authorizer(
        sqlite3*,
        int (*xAuth)(void*, int, const char*, const char*, const char*, const char*),
        void* pUserData,
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_get_table(
        sqlite3* db,          /* An open database */
        const char* zSql,     /* SQL to be evaluated */
        char*** pazResult,    /* Results of the query */
        int* pnRow,           /* Number of result rows written here */
        int* pnColumn,        /* Number of result columns written here */
        char** pzErrmsg,      /* Error msg written here */
        std::chrono::milliseconds timeout = MAX_WAIT
    );
    SQLITE_API void sqlite3_free_table(char** result, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_busy_timeout(sqlite3*, int ms, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_busy_handler(sqlite3*, int(*)(void*, int), void*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_blob64(sqlite3_stmt*, int, const void*, sqlite3_uint64,
        void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_double(sqlite3_stmt*, int, double, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_int(sqlite3_stmt*, int, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_null(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_text64(sqlite3_stmt*, int, const char*, sqlite3_uint64,
        void(*)(void*), unsigned char encoding, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_pointer(sqlite3_stmt*, int, void*, const char*, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_zeroblob64(sqlite3_stmt*, int, sqlite3_uint64, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_bind_parameter_count(sqlite3_stmt*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_bind_parameter_name(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_bind_parameter_index(sqlite3_stmt*, const char* zName, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_clear_bindings(sqlite3_stmt*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_column_database_name(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_database_name16(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_column_table_name(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_table_name16(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_column_origin_name(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_origin_name16(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_column_decltype(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_decltype16(sqlite3_stmt*, int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API void sqlite3_result_blob(sqlite3_context*, const void*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_blob64(sqlite3_context*, const void*,
        sqlite3_uint64, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_double(sqlite3_context*, double, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_error(sqlite3_context*, const char*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_error16(sqlite3_context*, const void*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_error_toobig(sqlite3_context*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_error_nomem(sqlite3_context*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_error_code(sqlite3_context*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_int(sqlite3_context*, int, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_int64(sqlite3_context*, sqlite3_int64, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_null(sqlite3_context*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_text(sqlite3_context*, const char*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_text64(sqlite3_context*, const char*, sqlite3_uint64,
        void(*)(void*), unsigned char encoding, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_text16(sqlite3_context*, const void*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_text16le(sqlite3_context*, const void*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_text16be(sqlite3_context*, const void*, int, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_value(sqlite3_context*, sqlite3_value*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_pointer(sqlite3_context*, void*, const char*, void(*)(void*), std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_result_zeroblob(sqlite3_context*, int n, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_result_zeroblob64(sqlite3_context*, sqlite3_uint64 n, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API void sqlite3_result_subtype(sqlite3_context*, unsigned int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_create_collation(
        sqlite3*,
        const char* zName,
        int eTextRep,
        void* pArg,
        int(*xCompare)(void*, int, const void*, int, const void*),
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_create_collation_v2(
        sqlite3*,
        const char* zName,
        int eTextRep,
        void* pArg,
        int(*xCompare)(void*, int, const void*, int, const void*),
        void(*xDestroy)(void*),
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_create_collation16(
        sqlite3*,
        const void* zName,
        int eTextRep,
        void* pArg,
        int(*xCompare)(void*, int, const void*, int, const void*),
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_collation_needed(
        sqlite3*,
        void*,
        void(*)(void*, sqlite3*, int eTextRep, const char*),
        std::chrono::milliseconds timeout = MAX_WAIT
    );
    SQLITE_API int sqlite3_collation_needed16(
        sqlite3*,
        void*,
        void(*)(void*, sqlite3*, int eTextRep, const void*),
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_sleep(int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API sqlite3* sqlite3_db_handle(sqlite3_stmt*);

    SQLITE_API const char* sqlite3_db_name(sqlite3* db, int N);

    SQLITE_API sqlite3_filename sqlite3_db_filename(sqlite3* db, const char* zDbName);

    SQLITE_API int sqlite3_db_readonly(sqlite3* db, const char* zDbName);

    SQLITE_API sqlite3_stmt* sqlite3_next_stmt(sqlite3* pDb, sqlite3_stmt* pStmt, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_release_memory(int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_db_release_memory(sqlite3*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_table_column_metadata(
        sqlite3* db,                /* Connection handle */
        const char* zDbName,        /* Database name or NULL */
        const char* zTableName,     /* Table name */
        const char* zColumnName,    /* Column name */
        char const** pzDataType,    /* OUTPUT: Declared data type */
        char const** pzCollSeq,     /* OUTPUT: Collation sequence name */
        int* pNotNull,              /* OUTPUT: True if NOT NULL constraint exists */
        int* pPrimaryKey,           /* OUTPUT: True if column part of PK */
        int* pAutoinc,              /* OUTPUT: True if column is auto-increment */
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_load_extension(
        sqlite3* db,          /* Load the extension into this database connection */
        const char* zFile,    /* Name of the shared library containing extension */
        const char* zProc,    /* Entry point.  Derived from zFile if 0 */
        char** pzErrMsg,      /* Put error message here if not 0 */
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_enable_load_extension(sqlite3* db, int onoff, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API sqlite3_str* sqlite3_str_new(sqlite3*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API char* sqlite3_str_finish(sqlite3_str*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API void sqlite3_str_append(sqlite3_str*, const char* zIn, int N, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_appendall(sqlite3_str*, const char* zIn, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_appendchar(sqlite3_str*, int N, char C, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_reset(sqlite3_str*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_str_errcode(sqlite3_str*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_str_length(sqlite3_str*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API char* sqlite3_str_value(sqlite3_str*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_status(int op, int* pCurrent, int* pHighwater, int resetFlag, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_status64(
        int op,
        sqlite3_int64* pCurrent,
        sqlite3_int64* pHighwater,
        int resetFlag, 
        std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_stmt_status(sqlite3_stmt*, int op, int resetFlg, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API sqlite3_backup* sqlite3_backup_init(
        sqlite3* pDest,                        /* Destination database handle */
        const char* zDestName,                 /* Destination database name */
        sqlite3* pSource,                      /* Source database handle */
        const char* zSourceName,               /* Source database name */
        std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_step(sqlite3_backup* p, int nPage, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_finish(sqlite3_backup* p, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_remaining(sqlite3_backup* p, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_pagecount(sqlite3_backup* p, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_unlock_notify(
        sqlite3* pBlocked,                          /* Waiting connection */
        void (*xNotify)(void** apArg, int nArg),    /* Callback function to invoke */
        void* pNotifyArg,                           /* Argument to pass to xNotify */
        std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_stricmp(const char*, const char*, std::chrono::milliseconds timeout = MAX_WAIT);
    SQLITE_API int sqlite3_strnicmp(const char*, const char*, int, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_strglob(const char* zGlob, const char* zStr, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_strlike(const char* zGlob, const char* zStr, unsigned int cEsc, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_wal_autocheckpoint(sqlite3* db, int N, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_wal_checkpoint(sqlite3* db, const char* zDb, std::chrono::milliseconds timeout = MAX_WAIT);
    
    SQLITE_API int sqlite3_wal_checkpoint_v2(
        sqlite3* db,                    /* Database handle */
        const char* zDb,                /* Name of attached database (or NULL) */
        int eMode,                      /* SQLITE_CHECKPOINT_* value */
        int* pnLog,                     /* OUT: Size of WAL log in frames */
        int* pnCkpt,                    /* OUT: Total number of frames checkpointed */
        std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_db_cacheflush(sqlite3*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_system_errno(sqlite3*, std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API unsigned char* sqlite3_serialize(
        sqlite3* db,           /* The database connection */
        const char* zSchema,   /* Which DB to serialize. ex: "main", "temp", ... */
        sqlite3_int64* piSize, /* Write size of the DB here, if not NULL */
        unsigned int mFlags,   /* Zero or more SQLITE_SERIALIZE_* flags */
        std::chrono::milliseconds timeout = MAX_WAIT);

    SQLITE_API int sqlite3_deserialize(
        sqlite3* db,            /* The database connection */
        const char* zSchema,    /* Which DB to reopen with the deserialization */
        unsigned char* pData,   /* The serialized database content */
        sqlite3_int64 szDb,     /* Number bytes in the deserialization */
        sqlite3_int64 szBuf,    /* Total size of buffer pData[] */
        unsigned mFlags,        /* Zero or more SQLITE_DESERIALIZE_* flags */
        std::chrono::milliseconds timeout = MAX_WAIT);
} // namespace async

#endif // ASYNC_SQLITE3_H