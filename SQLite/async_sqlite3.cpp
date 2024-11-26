// Asynchronous API's implemented using DelegateLib
// @see https://github.com/endurodave/Async-SQLite
// @see https://github.com/endurodave/AsyncMulticastDelegateModern 
// David Lafreniere, Nov 2024

#include "async_sqlite3.h"
#include "DelegateLib.h"
#include "WorkerThreadStd.h"

using namespace DelegateLib;

namespace async
{
    // A private worker thread instance to execute all SQLite API functions
    static WorkerThread SQLiteThread("SQLite Thread");

    /// Helper function to simplify asynchronous function calling on SQLiteThread
    /// @param[in] func - a function to invoke
    /// @param[in] timeout - the time to wait for invoke to complete
    /// @param[in] args - the function argument(s) passed to func
    template <typename Func, typename Timeout, typename... Args>
    auto AsyncInvoke(Func func, Timeout timeout, Args&&... args)
    {
        // Deduce return type of func
        using RetType = decltype(func(std::forward<Args>(args)...));

        // Is the calling function executing on the SQLiteThread thread?
        if (SQLiteThread.GetThreadId() != WorkerThread::GetCurrentThreadId())
        {
            // Create a delegate that points to func and is invoked on SQLiteThread
            auto delegate = DelegateLib::MakeDelegate(func, SQLiteThread, timeout);

            // Invoke the delegate target function asynchronously and wait for function call to complete
            auto retVal = delegate.AsyncInvoke(std::forward<Args>(args)...);

            if constexpr (std::is_void<RetType>::value == false)
            {
                // Did async function call succeed?
                if (retVal.has_value())
                {
                    // Return the return value to caller
                    return std::any_cast<RetType>(retVal.value());
                }
                else
                {
                    if constexpr (std::is_same_v<RetType, int>)
                        return SQLITE_ERROR;  // Special case for int
                    else
                        return RetType();
                }
            }
        }
        else
        {
            // Invoke target function synchronously since we're already executing on SQLiteThread
            if constexpr (std::is_void_v<RetType>)
            {
                func(std::forward<Args>(args)...); // Synchronous call
                return RetType(); // No return value for void
            }
            else
            {
                auto retVal = func(std::forward<Args>(args)...);  // Return the result for non-void types
                return std::any_cast<RetType>(retVal);
            }
        }
    }

    void sqlite3_init_async(void)
    {
        // Create the worker thread
        SQLiteThread.CreateThread();
    }

    DelegateLib::DelegateThread* sqlite3_get_thread(void)
    {
        return &SQLiteThread;
    }

    SQLITE_API int sqlite3_trace_v2(
        sqlite3* db,                               /* Trace this connection */
        unsigned mTrace,                           /* Mask of events to be traced */
        int(*xTrace)(unsigned, void*, void*, void*),  /* Callback to invoke */
        void* pArg,                                /* Context */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_trace_v2, timeout, db, mTrace, xTrace, pArg);
    }

    SQLITE_API void sqlite3_progress_handler(
        sqlite3* db,
        int nOps,
        int (*xProgress)(void*),
        void* pArg,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_progress_handler, timeout, db, nOps, xProgress, pArg);
    }

    SQLITE_API int sqlite3_open(
        const char* filename,  /* Database filename (UTF-8) */
        sqlite3** ppDb,        /* OUT: SQLite db handle */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_open, timeout, filename, ppDb);
    }

    SQLITE_API int sqlite3_open16(
        const void* filename,   /* Database filename (UTF-16) */
        sqlite3** ppDb,         /* OUT: SQLite db handle */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_open16, timeout, filename, ppDb);
    }

    SQLITE_API int sqlite3_open_v2(
        const char* filename,   /* Database filename (UTF-8) */
        sqlite3** ppDb,         /* OUT: SQLite db handle */
        int flags,              /* Open flags */
        const char* zVfs,       /* Optional VFS name */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_open_v2, timeout, filename, ppDb, flags, zVfs);
    }

    SQLITE_API int sqlite3_close(
        sqlite3* db,            /* Database handle */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_close, timeout, db);
    }

    SQLITE_API int sqlite3_close_v2(
        sqlite3* db,            /* Database handle */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_close_v2, timeout, db);
    }

    SQLITE_API int sqlite3_initialize(void) {
        // sqlite3_initialize already threaad safe per SQLite
        return ::sqlite3_initialize();
    }

    SQLITE_API int sqlite3_shutdown(
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_shutdown, timeout);
    }

    SQLITE_API int sqlite3_extended_result_codes(
        sqlite3* db,
        int onoff,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_extended_result_codes, timeout, db, onoff);
    }

    SQLITE_API sqlite3_int64 sqlite3_last_insert_rowid(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_last_insert_rowid, timeout, db);
    }

    SQLITE_API void sqlite3_set_last_insert_rowid(
        sqlite3* db,
        sqlite3_int64 iRowid,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_set_last_insert_rowid, timeout, db, iRowid);
    }

    SQLITE_API int sqlite3_changes(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_changes, timeout, db);
    }

    SQLITE_API sqlite3_int64 sqlite3_changes64(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_changes64, timeout, db);
    }

    SQLITE_API int sqlite3_total_changes(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_total_changes, timeout, db);
    }

    SQLITE_API sqlite3_int64 sqlite3_total_changes64(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_total_changes64, timeout, db);
    }

    SQLITE_API void sqlite3_interrupt(sqlite3* db) {
        // Thread safe function call
        ::sqlite3_interrupt(db);
    }

    SQLITE_API int sqlite3_is_interrupted(sqlite3* db) {
        // Thread safe function call
        return ::sqlite3_is_interrupted(db);
    }

    SQLITE_API int sqlite3_complete(
        const char* sql,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_complete, timeout, sql);
    }

    SQLITE_API int sqlite3_complete16(
        const void* sql,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_complete16, timeout, sql);
    }

    SQLITE_API int sqlite3_prepare(
        sqlite3* db,            /* Database handle */
        const char* zSql,       /* SQL statement, UTF-8 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const char** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_prepare, timeout, db, zSql, nByte, ppStmt, pzTail);
    }

    SQLITE_API int sqlite3_prepare_v2(
        sqlite3* db,            /* Database handle */
        const char* sql,        /* SQL query */
        int nBytes,             /* Byte length of SQL query */
        sqlite3_stmt** ppStmt,  /* Prepared statement */
        const char** pzTail,    /* Unused portion of SQL query */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_prepare_v2, timeout, db, sql, nBytes, ppStmt, pzTail);
    }

    SQLITE_API int sqlite3_prepare_v3(
        sqlite3* db,            /* Database handle */
        const char* zSql,       /* SQL statement, UTF-8 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const char** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_prepare_v3, timeout, db, zSql, nByte, prepFlags, ppStmt, pzTail);
    }

    SQLITE_API int sqlite3_prepare16(
        sqlite3* db,            /* Database handle */
        const void* zSql,       /* SQL statement, UTF-16 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const void** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout /* Timeout duration */
    ) {
        return AsyncInvoke(::sqlite3_prepare16, timeout, db, zSql, nByte, ppStmt, pzTail);
    }

    SQLITE_API int sqlite3_prepare16_v2(
        sqlite3* db,            /* Database handle */
        const void* zSql,       /* SQL statement, UTF-16 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const void** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout /* Timeout duration */
    ) {
        return AsyncInvoke(::sqlite3_prepare16_v2, timeout, db, zSql, nByte, ppStmt, pzTail);
    }

    SQLITE_API int sqlite3_prepare16_v3(
        sqlite3* db,            /* Database handle */
        const void* zSql,       /* SQL statement, UTF-16 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        unsigned int prepFlags, /* Zero or more SQLITE_PREPARE_ flags */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const void** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout /* Timeout duration */
    ) {
        return AsyncInvoke(::sqlite3_prepare16_v3, timeout, db, zSql, nByte, prepFlags, ppStmt, pzTail);
    }

    SQLITE_API const char* sqlite3_sql(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_sql, timeout, pStmt);
    }

    SQLITE_API char* sqlite3_expanded_sql(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_expanded_sql, timeout, pStmt);
    }

    SQLITE_API int sqlite3_stmt_readonly(sqlite3_stmt* pStmt) {
        // Thread safe function
        return ::sqlite3_stmt_readonly(pStmt);
    }

    SQLITE_API int sqlite3_stmt_isexplain(sqlite3_stmt* pStmt) {
        // Thread safe function
        return ::sqlite3_stmt_isexplain(pStmt);
    }

    SQLITE_API int sqlite3_stmt_explain(sqlite3_stmt* pStmt, int eMode, std::chrono::milliseconds timeout) {
        return AsyncInvoke(::sqlite3_stmt_explain, timeout, pStmt, eMode);
    }

    SQLITE_API int sqlite3_stmt_busy(sqlite3_stmt* pStmt) {
        // Thread safe function
        return ::sqlite3_stmt_busy(pStmt);
    }

    SQLITE_API int sqlite3_finalize(
        sqlite3_stmt* pStmt,    /* Statement to finalize */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_finalize, timeout, pStmt);
    }

    SQLITE_API int sqlite3_reset(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_reset, timeout, pStmt);
    }

    SQLITE_API int sqlite3_step(
        sqlite3_stmt* pStmt,    /* Statement to step through */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_step, timeout, pStmt);
    }

    SQLITE_API int sqlite3_data_count(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_data_count, timeout, pStmt);
    }

    SQLITE_API int sqlite3_column_count(
        sqlite3_stmt* pStmt,    /* Statement */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_count, timeout, pStmt);
    }

    SQLITE_API const char* sqlite3_column_name(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_name, timeout, pStmt, col);
    }

    SQLITE_API const void* sqlite3_column_name16(
        sqlite3_stmt* pStmt,
        int N,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_name16, timeout, pStmt, N);
    }

    SQLITE_API int sqlite3_exec(
        sqlite3* db,             /* Database handle */
        const char* sql,         /* SQL query */
        sqlite3_callback callback, /* Callback function */
        void* pArg,             /* Callback argument */
        char** errMsg,          /* Error message */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_exec, timeout, db, sql, callback, pArg, errMsg);
    }

    SQLITE_API int sqlite3_db_status(
        sqlite3* db,            /* Database handle */
        int op,                  /* Status operation */
        int* pCurrent,          /* Current value of status */
        int* pHighwater,        /* Highwater mark */
        int resetFlag,          /* Reset status or not */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_db_status, timeout, db, op, pCurrent, pHighwater, resetFlag);
    }

    SQLITE_API int sqlite3_get_table(
        sqlite3* db,            /* Database handle */
        const char* sql,        /* SQL query */
        char*** resultpAzResult,/* Result matrix */
        int* nrow,              /* Number of rows */
        int* ncolumn,           /* Number of columns */
        char** errmsg,          /* Error message */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_get_table, timeout, db, sql, resultpAzResult, nrow, ncolumn, errmsg);
    }

    SQLITE_API const void* sqlite3_column_blob(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_blob, timeout, pStmt, col);
    }

    SQLITE_API double sqlite3_column_double(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_double, timeout, pStmt, col);
    }

    SQLITE_API int sqlite3_column_int(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_int, timeout, pStmt, col);

    }

    SQLITE_API sqlite3_int64 sqlite3_column_int64(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_int64, timeout, pStmt, col);
    }

    SQLITE_API const unsigned char* sqlite3_column_text(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_text, timeout, pStmt, col);
    }

    SQLITE_API const void* sqlite3_column_text16(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_text16, timeout, pStmt, col);
    }

    SQLITE_API sqlite3_value* sqlite3_column_value(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_value, timeout, pStmt, col);
    }

    SQLITE_API int sqlite3_column_bytes(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_bytes, timeout, pStmt, col);
    }

    SQLITE_API int sqlite3_column_bytes16(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_bytes16, timeout, pStmt, col);
    }

    SQLITE_API int sqlite3_column_type(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_type, timeout, pStmt, col);
    }

    SQLITE_API void* sqlite3_malloc(
        int size,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_malloc, timeout, size);
    }

    SQLITE_API void* sqlite3_malloc64(
        sqlite3_uint64 size,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_malloc64, timeout, size);
    }

    SQLITE_API void* sqlite3_realloc(
        void* ptr,
        int size,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_realloc, timeout, ptr, size);
    }

    SQLITE_API void* sqlite3_realloc64(
        void* ptr,
        sqlite3_uint64 size,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_realloc64, timeout, ptr, size);
    }

    SQLITE_API void sqlite3_free(
        void* ptr,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_free, timeout, ptr);
    }

    SQLITE_API sqlite3_uint64 sqlite3_msize(
        void* ptr,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_msize, timeout, ptr);
    }

    SQLITE_API const char* sqlite3_uri_parameter(
        const char* zFilename, 
        const char* zParam,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_uri_parameter, timeout, zFilename, zParam);
    }

    SQLITE_API int sqlite3_uri_boolean(
        const char* zFilename, 
        const char* zParam, 
        int bDflt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_uri_boolean, timeout, zFilename, zParam, bDflt);
    }

    SQLITE_API sqlite3_int64 sqlite3_uri_int64(
        const char* zFilename,    /* Filename as passed to xOpen */
        const char* zParam,       /* URI parameter sought */
        sqlite3_int64 bDflt,      /* return if parameter is missing */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_uri_int64, timeout, zFilename, zParam, bDflt);
    }

    SQLITE_API const char* sqlite3_uri_key(
        const char* zFilename, 
        int N,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_uri_key, timeout, zFilename, N);
    }

    SQLITE_API const char* sqlite3_filename_database(
        const char* zFilename,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_filename_database, timeout, zFilename);
    }

    SQLITE_API const char* sqlite3_filename_journal(
        const char* zFilename,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_filename_journal, timeout, zFilename);
    }

    SQLITE_API const char* sqlite3_filename_wal(
        const char* zFilename,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_filename_wal, timeout, zFilename);
    }

    SQLITE_API sqlite3_file* sqlite3_database_file_object(
        const char* zName,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_database_file_object, timeout, zName);
    }

    SQLITE_API sqlite3_filename sqlite3_create_filename(
        const char* zDatabase,
        const char* zJournal,
        const char* zWal,
        int nParam,
        const char** azParam,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_create_filename, timeout, zDatabase, zJournal, zWal, nParam, azParam);
    }

    SQLITE_API void sqlite3_free_filename(
        const char* p,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_free_filename, timeout, p);
    }

    SQLITE_API int sqlite3_errcode(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_errcode, timeout, db);
    }

    SQLITE_API int sqlite3_extended_errcode(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(&::sqlite3_extended_errcode, timeout, db);
    }

    SQLITE_API const char* sqlite3_errmsg(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(&::sqlite3_errmsg, timeout, db);
    }

    SQLITE_API const void* sqlite3_errmsg16(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(&::sqlite3_errmsg16, timeout, db);
    }

    SQLITE_API const char* sqlite3_errstr(
        int errorCode,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(&::sqlite3_errstr, timeout, errorCode);
    }

    SQLITE_API int sqlite3_error_offset(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(&::sqlite3_error_offset, timeout, db);
    }

    SQLITE_API int sqlite3_limit(
        sqlite3* db, 
        int limitId, 
        int newLimit,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(&::sqlite3_limit, timeout, db, limitId, newLimit);
    }

    SQLITE_API sqlite3_int64 sqlite3_memory_used(
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_memory_used, timeout);
    }

    SQLITE_API sqlite3_int64 sqlite3_memory_highwater(
        int resetFlag,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_memory_highwater, timeout, resetFlag);
    }

    SQLITE_API void sqlite3_randomness(
        int N,
        void* P,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_randomness, timeout, N, P);
    }

    SQLITE_API int sqlite3_set_authorizer(
        sqlite3* db,
        int (*xAuth)(void*, int, const char*, const char*, const char*, const char*),
        void* pArg,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_set_authorizer, timeout, db, xAuth, pArg);
    }

    SQLITE_API void sqlite3_free_table(
        char** result,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_free_table, timeout, result);
    }

    SQLITE_API int sqlite3_busy_timeout(
        sqlite3* db,
        int ms,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_busy_timeout, timeout, db, ms);
    }

    SQLITE_API int sqlite3_busy_handler(
        sqlite3* db,
        int (*xBusy)(void*, int),
        void* pArg,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_busy_handler, timeout, db, xBusy, pArg);
    }

    SQLITE_API int sqlite3_bind_blob(
        sqlite3_stmt* pStmt,
        int i,
        const void* zData,
        int nData,
        void (*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_blob, timeout, pStmt, i, zData, nData, xDel);
    }

    SQLITE_API int sqlite3_bind_blob64(
        sqlite3_stmt* pStmt,
        int i,
        const void* zData,
        sqlite3_uint64 nData,
        void (*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_blob64, timeout, pStmt, i, zData, nData, xDel);
    }

    SQLITE_API int sqlite3_bind_double(
        sqlite3_stmt* pStmt,
        int i,
        double value,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_double, timeout, pStmt, i, value);
    }

    SQLITE_API int sqlite3_bind_int(
        sqlite3_stmt* pStmt,
        int i,
        int value,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_int, timeout, pStmt, i, value);
    }

    SQLITE_API int sqlite3_bind_int64(
        sqlite3_stmt* pStmt,
        int i,
        sqlite3_int64 value,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_int64, timeout, pStmt, i, value);
    }

    SQLITE_API int sqlite3_bind_null(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_null, timeout, pStmt, i);
    }

    SQLITE_API int sqlite3_bind_text(
        sqlite3_stmt* pStmt,
        int i,
        const char* zData,
        int nData,
        void (*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_text, timeout, pStmt, i, zData, nData, xDel);
    }

    SQLITE_API int sqlite3_bind_text16(
        sqlite3_stmt* pStmt,
        int i,
        const void* zData,
        int nData,
        void (*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_text16, timeout, pStmt, i, zData, nData, xDel);
    }

    SQLITE_API int sqlite3_bind_text64(
        sqlite3_stmt* pStmt,
        int i,
        const char* zData,
        sqlite3_uint64 nData,
        void (*xDel)(void*),
        unsigned char encoding,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_text64, timeout, pStmt, i, zData, nData, xDel, encoding);
    }

    SQLITE_API int sqlite3_bind_value(
        sqlite3_stmt* pStmt,
        int i,
        const sqlite3_value* pValue,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_value, timeout, pStmt, i, pValue);
    }

    SQLITE_API int sqlite3_bind_pointer(
        sqlite3_stmt* pStmt,
        int i,
        void* pData,
        const char* pType,
        void (*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_pointer, timeout, pStmt, i, pData, pType, xDel);
    }

    SQLITE_API int sqlite3_bind_zeroblob(
        sqlite3_stmt* pStmt,
        int i,
        int nData,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_zeroblob, timeout, pStmt, i, nData);
    }

    SQLITE_API int sqlite3_bind_zeroblob64(
        sqlite3_stmt* pStmt,
        int i,
        sqlite3_uint64 nData,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_zeroblob64, timeout, pStmt, i, nData);
    }

    SQLITE_API int sqlite3_bind_parameter_count(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_parameter_count, timeout, pStmt);
    }

    SQLITE_API const char* sqlite3_bind_parameter_name(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_parameter_name, timeout, pStmt, i);
    }

    SQLITE_API int sqlite3_bind_parameter_index(
        sqlite3_stmt* pStmt,
        const char* zName,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_bind_parameter_index, timeout, pStmt, zName);
    }

    SQLITE_API int sqlite3_clear_bindings(
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_clear_bindings, timeout, pStmt);
    }

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    SQLITE_API const char* sqlite3_column_database_name(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_database_name, timeout, pStmt, i);
    }

    SQLITE_API const void* sqlite3_column_database_name16(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_database_name16, timeout, pStmt, i);
    }

    SQLITE_API const char* sqlite3_column_table_name(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_table_name, timeout, pStmt, i);
    }

    SQLITE_API const void* sqlite3_column_table_name16(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_table_name16, timeout, pStmt, i);
    }

    SQLITE_API const char* sqlite3_column_origin_name(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_origin_name, timeout, pStmt, i);
    }

    SQLITE_API const void* sqlite3_column_origin_name16(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_origin_name16, timeout, pStmt, i);
    }
#endif // SQLITE_ENABLE_COLUMN_METADATA

    SQLITE_API const char* sqlite3_column_decltype(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_decltype, timeout, pStmt, i);
    }

    SQLITE_API const void* sqlite3_column_decltype16(
        sqlite3_stmt* pStmt,
        int i,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_column_decltype16, timeout, pStmt, i);
    }

    SQLITE_API void sqlite3_result_blob(
        sqlite3_context* context,
        const void* data,
        int nData,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_blob, timeout, context, data, nData, xDel);
    }

    SQLITE_API void sqlite3_result_blob64(
        sqlite3_context* context,
        const void* data,
        sqlite3_uint64 nData,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_blob64, timeout, context, data, nData, xDel);
    }

    SQLITE_API void sqlite3_result_double(
        sqlite3_context* context,
        double value,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_double, timeout, context, value);
    }

    SQLITE_API void sqlite3_result_error(
        sqlite3_context* context,
        const char* errorMsg,
        int msgLength,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_error, timeout, context, errorMsg, msgLength);
    }

    SQLITE_API void sqlite3_result_error16(
        sqlite3_context* context,
        const void* errorMsg,
        int msgLength,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_error16, timeout, context, errorMsg, msgLength);
    }

    SQLITE_API void sqlite3_result_error_toobig(
        sqlite3_context* context,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_error_toobig, timeout, context);
    }

    SQLITE_API void sqlite3_result_error_nomem(
        sqlite3_context* context,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_error_nomem, timeout, context);
    }

    SQLITE_API void sqlite3_result_error_code(
        sqlite3_context* context,
        int errorCode,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_error_code, timeout, context, errorCode);
    }

    SQLITE_API void sqlite3_result_int(
        sqlite3_context* context,
        int value,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_int, timeout, context, value);
    }

    SQLITE_API void sqlite3_result_int64(
        sqlite3_context* context,
        sqlite3_int64 value,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_int64, timeout, context, value);
    }

    SQLITE_API void sqlite3_result_null(
        sqlite3_context* context,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_null, timeout, context);
    }

    SQLITE_API void sqlite3_result_text(
        sqlite3_context* context,
        const char* text,
        int nText,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_text, timeout, context, text, nText, xDel);
    }

    SQLITE_API void sqlite3_result_text64(
        sqlite3_context* context,
        const char* text,
        sqlite3_uint64 nText,
        void(*xDel)(void*),
        unsigned char encoding,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_text64, timeout, context, text, nText, xDel, encoding);
    }

    SQLITE_API void sqlite3_result_text16(
        sqlite3_context* context,
        const void* text,
        int nText,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_text16, timeout, context, text, nText, xDel);
    }

    SQLITE_API void sqlite3_result_text16le(
        sqlite3_context* context,
        const void* text,
        int nText,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_text16le, timeout, context, text, nText, xDel);
    }

    SQLITE_API void sqlite3_result_text16be(
        sqlite3_context* context,
        const void* text,
        int nText,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_text16be, timeout, context, text, nText, xDel);
    }

    SQLITE_API void sqlite3_result_value(
        sqlite3_context* context,
        sqlite3_value* value,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_value, timeout, context, value);
    }

    SQLITE_API void sqlite3_result_pointer(
        sqlite3_context* context,
        void* pointer,
        const char* type,
        void(*xDel)(void*),
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_pointer, timeout, context, pointer, type, xDel);
    }

    SQLITE_API void sqlite3_result_zeroblob(
        sqlite3_context* context,
        int n,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_zeroblob, timeout, context, n);
    }

    SQLITE_API int sqlite3_result_zeroblob64(
        sqlite3_context* context,
        sqlite3_uint64 n,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_result_zeroblob64, timeout, context, n);
    }

    SQLITE_API void sqlite3_result_subtype(
        sqlite3_context* context,
        unsigned int subtype,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_result_subtype, timeout, context, subtype);
    }

    SQLITE_API int sqlite3_create_collation(
        sqlite3* db,
        const char* zName,
        int eTextRep,
        void* pArg,
        int(*xCompare)(void*, int, const void*, int, const void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_create_collation, timeout, db, zName, eTextRep, pArg, xCompare);
    }

    SQLITE_API int sqlite3_create_collation_v2(
        sqlite3* db,
        const char* zName,
        int eTextRep,
        void* pArg,
        int(*xCompare)(void*, int, const void*, int, const void*),
        void(*xDestroy)(void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_create_collation_v2, timeout, db, zName, eTextRep, pArg, xCompare, xDestroy);
    }

    SQLITE_API int sqlite3_create_collation16(
        sqlite3* db,
        const void* zName,
        int eTextRep,
        void* pArg,
        int(*xCompare)(void*, int, const void*, int, const void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_create_collation16, timeout, db, zName, eTextRep, pArg, xCompare);
    }

    SQLITE_API int sqlite3_collation_needed(
        sqlite3* db,
        void* pArg,
        void(*xCollNeeded)(void*, sqlite3*, int eTextRep, const char*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_collation_needed, timeout, db, pArg, xCollNeeded);
    }

    SQLITE_API int sqlite3_collation_needed16(
        sqlite3* db,
        void* pArg,
        void(*xCollNeeded)(void*, sqlite3*, int eTextRep, const void*),
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_collation_needed16, timeout, db, pArg, xCollNeeded);
    }

    SQLITE_API int sqlite3_sleep(
        int milliseconds,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_sleep, timeout, milliseconds);
    }

    SQLITE_API sqlite3* sqlite3_db_handle(sqlite3_stmt* pStmt) {
        // Thread safe already
        return ::sqlite3_db_handle(pStmt);
    }

    SQLITE_API const char* sqlite3_db_name(sqlite3* db, int N) {
        // Thread safe already
        return ::sqlite3_db_name(db, N);
    }

    SQLITE_API sqlite3_filename sqlite3_db_filename(sqlite3* db, const char* zDbName) {
        return ::sqlite3_db_filename(db, zDbName);
    }

    SQLITE_API int sqlite3_db_readonly(sqlite3* db, const char* zDbName) {
        return ::sqlite3_db_readonly(db, zDbName);
    }

    SQLITE_API sqlite3_stmt* sqlite3_next_stmt(
        sqlite3* pDb,
        sqlite3_stmt* pStmt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_next_stmt, timeout, pDb, pStmt);
    }

    SQLITE_API int sqlite3_release_memory(
        int n,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_release_memory, timeout, n);
    }

    SQLITE_API int sqlite3_db_release_memory(
        sqlite3* pDb,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_db_release_memory, timeout, pDb);
    }

    SQLITE_API int sqlite3_table_column_metadata(
        sqlite3* db,
        const char* zDbName,
        const char* zTableName,
        const char* zColumnName,
        char const** pzDataType,
        char const** pzCollSeq,
        int* pNotNull,
        int* pPrimaryKey,
        int* pAutoinc,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_table_column_metadata, timeout, db, zDbName, zTableName, zColumnName, pzDataType, pzCollSeq, pNotNull, pPrimaryKey, pAutoinc);
    }

    SQLITE_API int sqlite3_load_extension(
        sqlite3* db,
        const char* zFile,
        const char* zProc,
        char** pzErrMsg,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_load_extension, timeout, db, zFile, zProc, pzErrMsg);
    }

    SQLITE_API int sqlite3_enable_load_extension(
        sqlite3* db,
        int onoff,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_enable_load_extension, timeout, db, onoff);
    }

    SQLITE_API sqlite3_str* sqlite3_str_new(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_str_new, timeout, db);
    }

    SQLITE_API char* sqlite3_str_finish(
        sqlite3_str* pStr,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_str_finish, timeout, pStr);
    }

    SQLITE_API void sqlite3_str_append(
        sqlite3_str* pStr,
        const char* zIn,
        int N,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_str_append, timeout, pStr, zIn, N);
    }

    SQLITE_API void sqlite3_str_appendall(
        sqlite3_str* pStr,
        const char* zIn,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_str_appendall, timeout, pStr, zIn);
    }

    SQLITE_API void sqlite3_str_appendchar(
        sqlite3_str* pStr,
        int N,
        char C,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_str_appendchar, timeout, pStr, N, C);
    }

    SQLITE_API void sqlite3_str_reset(
        sqlite3_str* pStr,
        std::chrono::milliseconds timeout
    ) {
        AsyncInvoke(::sqlite3_str_reset, timeout, pStr);
    }

    SQLITE_API int sqlite3_str_errcode(
        sqlite3_str* pStr,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_str_errcode, timeout, pStr);
    }

    SQLITE_API int sqlite3_str_length(
        sqlite3_str* pStr,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_str_length, timeout, pStr);
    }

    SQLITE_API char* sqlite3_str_value(
        sqlite3_str* pStr,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_str_value, timeout, pStr);
    }

    SQLITE_API int sqlite3_status(
        int op,
        int* pCurrent,
        int* pHighwater,
        int resetFlag,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_status, timeout, op, pCurrent, pHighwater, resetFlag);
    }

    SQLITE_API int sqlite3_status64(
        int op,
        sqlite3_int64* pCurrent,
        sqlite3_int64* pHighwater,
        int resetFlag,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_status64, timeout, op, pCurrent, pHighwater, resetFlag);
    }

    SQLITE_API int sqlite3_stmt_status(
        sqlite3_stmt* pStmt,
        int op,
        int resetFlg,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_stmt_status, timeout, pStmt, op, resetFlg);
    }

    SQLITE_API sqlite3_backup* sqlite3_backup_init(
        sqlite3* pDest,
        const char* zDestName,
        sqlite3* pSource,
        const char* zSourceName,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_backup_init, timeout, pDest, zDestName, pSource, zSourceName);
    }

    SQLITE_API int sqlite3_backup_step(
        sqlite3_backup* p,
        int nPage,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_backup_step, timeout, p, nPage);
    }

    SQLITE_API int sqlite3_backup_finish(
        sqlite3_backup* p,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_backup_finish, timeout, p);
    }

    SQLITE_API int sqlite3_backup_remaining(
        sqlite3_backup* p,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_backup_remaining, timeout, p);
    }

    SQLITE_API int sqlite3_backup_pagecount(
        sqlite3_backup* p,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_backup_pagecount, timeout, p);
    }

#ifdef SQLITE_ENABLE_UNLOCK_NOTIFY
    SQLITE_API int sqlite3_unlock_notify(
        sqlite3* pBlocked,
        void (*xNotify)(void** apArg, int nArg),
        void* pNotifyArg,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_unlock_notify, timeout, pBlocked, xNotify, pNotifyArg);
    }
#endif

    SQLITE_API int sqlite3_stricmp(
        const char* z1,
        const char* z2,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_stricmp, timeout, z1, z2);
    }

    SQLITE_API int sqlite3_strnicmp(
        const char* z1,
        const char* z2,
        int N,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_strnicmp, timeout, z1, z2, N);
    }

    SQLITE_API int sqlite3_strglob(
        const char* zGlob,
        const char* zStr,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_strglob, timeout, zGlob, zStr);
    }

    SQLITE_API int sqlite3_strlike(
        const char* zGlob,
        const char* zStr,
        unsigned int cEsc,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_strlike, timeout, zGlob, zStr, cEsc);
    }

    SQLITE_API int sqlite3_wal_autocheckpoint(
        sqlite3* db,
        int N,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_wal_autocheckpoint, timeout, db, N);
    }

    SQLITE_API int sqlite3_wal_checkpoint(
        sqlite3* db,
        const char* zDb,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_wal_checkpoint, timeout, db, zDb);
    }

    SQLITE_API int sqlite3_wal_checkpoint_v2(
        sqlite3* db,
        const char* zDb,
        int eMode,
        int* pnLog,
        int* pnCkpt,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_wal_checkpoint_v2, timeout, db, zDb, eMode, pnLog, pnCkpt);
    }

    SQLITE_API int sqlite3_db_cacheflush(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_db_cacheflush, timeout, db);
    }

    SQLITE_API int sqlite3_system_errno(
        sqlite3* db,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_system_errno, timeout, db);
    }

    SQLITE_API unsigned char* sqlite3_serialize(
        sqlite3* db,
        const char* zSchema,
        sqlite3_int64* piSize,
        unsigned int mFlags,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_serialize, timeout, db, zSchema, piSize, mFlags);
    }

    SQLITE_API int sqlite3_deserialize(
        sqlite3* db,
        const char* zSchema,
        unsigned char* pData,
        sqlite3_int64 szDb,
        sqlite3_int64 szBuf,
        unsigned mFlags,
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_deserialize, timeout, db, zSchema, pData, szDb, szBuf, mFlags);
    }

}  // namespace async


