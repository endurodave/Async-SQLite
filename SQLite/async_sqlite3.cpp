#include "async_sqlite3.h"
#include "DelegateLib.h"
#include "WorkerThreadStd.h"

// Asynchronous API's implemented using DelegateLib
// @see https://github.com/endurodave/Async-SQLite
// @see https://github.com/endurodave/AsyncMulticastDelegateModern 

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
        auto retVal = AsyncInvoke(::sqlite3_open, timeout, filename, ppDb);
        return retVal;
    }

    SQLITE_API int sqlite3_open_v2(
        const char* filename,   /* Database filename (UTF-8) */
        sqlite3** ppDb,         /* OUT: SQLite db handle */
        int flags,              /* Open flags */
        const char* zVfs,       /* Optional VFS name */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_open_v2, timeout, filename, ppDb, flags, zVfs);
        return retVal;
    }

    SQLITE_API int sqlite3_close(
        sqlite3* db,            /* Database handle */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_close, timeout, db);
        return retVal;
    }

    SQLITE_API int sqlite3_prepare(
        sqlite3* db,            /* Database handle */
        const char* zSql,       /* SQL statement, UTF-8 encoded */
        int nByte,              /* Maximum length of zSql in bytes. */
        sqlite3_stmt** ppStmt,  /* OUT: Statement handle */
        const char** pzTail,    /* OUT: Pointer to unused portion of zSql */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_prepare, timeout, db, zSql, nByte, ppStmt, pzTail);
        return retVal;
    }

    SQLITE_API int sqlite3_prepare_v2(
        sqlite3* db,            /* Database handle */
        const char* sql,        /* SQL query */
        int nBytes,             /* Byte length of SQL query */
        sqlite3_stmt** ppStmt,  /* Prepared statement */
        const char** pzTail,    /* Unused portion of SQL query */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_prepare_v2, timeout, db, sql, nBytes, ppStmt, pzTail);
        return retVal;
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
        auto retVal = AsyncInvoke(::sqlite3_prepare_v3, timeout, db, zSql, nByte, prepFlags, ppStmt, pzTail);
        return retVal;
    }

    SQLITE_API int sqlite3_finalize(
        sqlite3_stmt* pStmt,    /* Statement to finalize */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_finalize, timeout, pStmt);
        return retVal;
    }

    SQLITE_API int sqlite3_step(
        sqlite3_stmt* pStmt,    /* Statement to step through */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_step, timeout, pStmt);
        return retVal;
    }

    SQLITE_API int sqlite3_bind_int(
        sqlite3_stmt* pStmt,    /* Statement to bind to */
        int idx,                /* Parameter index (1-based) */
        int value,              /* Value to bind */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_bind_int, timeout, pStmt, idx, value);
        return retVal;
    }

    SQLITE_API int sqlite3_bind_text(
        sqlite3_stmt* pStmt,    /* Statement to bind to */
        int idx,                /* Parameter index (1-based) */
        const char* value,      /* Text value to bind */
        int n,                  /* Length of text */
        sqlite3_destructor_type dtor, /* Destructor for the string */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_bind_text, timeout, pStmt, idx, value, n, dtor);
        return retVal;
    }

    SQLITE_API int sqlite3_column_count(
        sqlite3_stmt* pStmt,    /* Statement */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_count, timeout, pStmt);
        return retVal;
    }

    SQLITE_API const char* sqlite3_column_name(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_name, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API int sqlite3_exec(
        sqlite3* db,             /* Database handle */
        const char* sql,         /* SQL query */
        sqlite3_callback callback, /* Callback function */
        void* pArg,             /* Callback argument */
        char** errMsg,          /* Error message */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_exec, timeout, db, sql, callback, pArg, errMsg);
        return retVal;
    }

    SQLITE_API int sqlite3_changes(
        sqlite3* db,            /* Database handle */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_changes, timeout, db);
        return retVal;
    }

    SQLITE_API int sqlite3_db_status(
        sqlite3* db,            /* Database handle */
        int op,                  /* Status operation */
        int* pCurrent,          /* Current value of status */
        int* pHighwater,        /* Highwater mark */
        int resetFlag,          /* Reset status or not */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_db_status, timeout, db, op, pCurrent, pHighwater, resetFlag);
        return retVal;
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
        auto retVal = AsyncInvoke(::sqlite3_get_table, timeout, db, sql, resultpAzResult, nrow, ncolumn, errmsg);
        return retVal;
    }

    SQLITE_API const void* sqlite3_column_blob(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_blob, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API double sqlite3_column_double(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_double, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API int sqlite3_column_int(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_int, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API sqlite3_int64 sqlite3_column_int64(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_int64, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API const unsigned char* sqlite3_column_text(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_text, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API const void* sqlite3_column_text16(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_text16, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API sqlite3_value* sqlite3_column_value(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_value, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API int sqlite3_column_bytes(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_bytes, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API int sqlite3_column_bytes16(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_bytes16, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API int sqlite3_column_type(
        sqlite3_stmt* pStmt,    /* Statement */
        int col,                /* Column index */
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_column_type, timeout, pStmt, col);
        return retVal;
    }

    SQLITE_API void* sqlite3_malloc(
        int size,
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_malloc, timeout, size);
        return retVal;
    }

    SQLITE_API void* sqlite3_malloc64(
        sqlite3_uint64 size,
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_malloc64, timeout, size);
        return retVal;
    }

    SQLITE_API void* sqlite3_realloc(
        void* ptr,
        int size,
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_realloc, timeout, ptr, size);
        return retVal;
    }

    SQLITE_API void* sqlite3_realloc64(
        void* ptr,
        sqlite3_uint64 size,
        std::chrono::milliseconds timeout
    ) {
        auto retVal = AsyncInvoke(::sqlite3_realloc64, timeout, ptr, size);
        return retVal;
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

#if 0  // char*** not supported by delegate library
    SQLITE_API int sqlite3_get_table(
        sqlite3* db,          /* An open database */
        const char* zSql,     /* SQL to be evaluated */
        char*** pazResult,    /* Results of the query */
        int* pnRow,           /* Number of result rows written here */
        int* pnColumn,        /* Number of result columns written here */
        char** pzErrmsg,      /* Error msg written here */
        std::chrono::milliseconds timeout
    ) {
        return AsyncInvoke(::sqlite3_get_table, timeout, db, zSql, pazResult, pnRow, pnColumn, pzErrmsg);
    }
#endif

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

}  // namespace async


