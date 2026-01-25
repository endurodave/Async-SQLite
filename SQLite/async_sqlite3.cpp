// Asynchronous SQLite API's implemented using C++ delegates
// @see https://github.com/endurodave/Async-SQLite
// @see https://github.com/endurodave/DelegateMQ
// David Lafreniere, Nov 2024

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <future>
#include <memory>
#include <tuple>
#include <stdexcept>
#include <thread>

using namespace dmq;

namespace async
{
    // A private worker thread instance to execute all SQLite API functions
    static Thread SQLiteThread("SQLite Thread");

    // --------------------------------------------------------------------------------
    // Internal Helper: AsyncInvokeFuture
    // --------------------------------------------------------------------------------
    template <typename Func, typename... Args>
    auto AsyncInvokeFuture(Func func, Args&&... args)
    {
        using RetType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

        auto promise = std::make_shared<std::promise<RetType>>();
        std::future<RetType> future = promise->get_future();

        if (SQLiteThread.GetThreadId() == std::thread::id()) {
            try {
                throw std::runtime_error("SQLite Async Thread is not running. Call sqlite3_init_async() first.");
            }
            catch (...) {
                promise->set_exception(std::current_exception());
            }
            return future;
        }

        auto taskLambda = [func, promise, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
            try {
                if constexpr (std::is_void_v<RetType>) {
                    std::apply(func, std::move(args));
                    promise->set_value();
                }
                else {
                    promise->set_value(std::apply(func, std::move(args)));
                }
            }
            catch (...) {
                promise->set_exception(std::current_exception());
            }
            };

        auto delegate = dmq::MakeDelegate(
            std::function<void()>(taskLambda),
            SQLiteThread
        );

        delegate.AsyncInvoke();
        return future;
    }

    // --------------------------------------------------------------------------------
    // Internal Helper: AsyncInvoke (Blocking)
    // --------------------------------------------------------------------------------
    template <typename Func, typename Timeout, typename... Args>
    auto AsyncInvoke(Func func, Timeout timeout, Args&&... args)
    {
        using RetType = decltype(func(std::forward<Args>(args)...));

        if (SQLiteThread.GetThreadId() != std::this_thread::get_id())
        {
            auto delegate = MakeDelegate(func, SQLiteThread, timeout);
            auto retVal = delegate.AsyncInvoke(std::forward<Args>(args)...);

            if constexpr (!std::is_void_v<RetType>)
            {
                if (retVal.has_value()) {
                    return std::any_cast<RetType>(retVal.value());
                }
                else {
                    if constexpr (std::is_same_v<RetType, int>) return SQLITE_BUSY;
                    else return RetType();
                }
            }
        }
        else
        {
            if constexpr (std::is_void_v<RetType>) {
                std::invoke(func, std::forward<Args>(args)...);
            }
            else {
                return std::invoke(func, std::forward<Args>(args)...);
            }
        }
    }

    // --------------------------------------------------------------------------------
    // API Implementation
    // --------------------------------------------------------------------------------

    Thread* sqlite3_get_thread(void) {
        return &SQLiteThread;
    }

    int sqlite3_init_async(void) {
        int rc = ::sqlite3_initialize();
        if (rc != SQLITE_OK) return rc;
        SQLiteThread.CreateThread();
        return SQLITE_OK;
    }

    /// Registers a trace callback function.
    ///
    /// @warning **Callback Thread Context**: The `xCallback` function is invoked on the 
    ///          **private worker thread**, NOT the calling thread. 
    ///          - Ensure strictly thread-safe access to any shared data inside the callback.
    ///          - Do NOT attempt to update UI elements directly from this callback.
    int sqlite3_trace_v2(sqlite3* db, unsigned mTrace, int(*xCallback)(unsigned, void*, void*, void*), void* pCtx, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_trace_v2, timeout, db, mTrace, xCallback, pCtx);
    }

    /// Registers a progress handler callback.
    ///
    /// @warning **Callback Thread Context**: The `xProgress` function is invoked on the 
    ///          **private worker thread** during long-running queries.
    ///          - Do NOT access thread-local storage of the main thread.
    ///          - Keep operations fast to avoid stalling the database worker.
    void sqlite3_progress_handler(sqlite3* db, int nOps, int(*xProgress)(void*), void* pArg, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_progress_handler, timeout, db, nOps, xProgress, pArg);
    }

    int sqlite3_open(const char* filename, sqlite3** ppDb, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_open, timeout, filename, ppDb);
    }

    int sqlite3_open16(const void* filename, sqlite3** ppDb, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_open16, timeout, filename, ppDb);
    }

    int sqlite3_open_v2(const char* filename, sqlite3** ppDb, int flags, const char* zVfs, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_open_v2, timeout, filename, ppDb, flags, zVfs);
    }

    int sqlite3_close(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_close, timeout, db);
    }

    int sqlite3_close_v2(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_close_v2, timeout, db);
    }

    int sqlite3_initialize(void) {
        return ::sqlite3_initialize();
    }

    int sqlite3_shutdown(dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_shutdown, timeout);
    }

    int sqlite3_extended_result_codes(sqlite3* db, int onoff, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_extended_result_codes, timeout, db, onoff);
    }

    sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_last_insert_rowid, timeout, db);
    }

    void sqlite3_set_last_insert_rowid(sqlite3* db, sqlite3_int64 iRowid, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_set_last_insert_rowid, timeout, db, iRowid);
    }

    int sqlite3_changes(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_changes, timeout, db);
    }

    sqlite3_int64 sqlite3_changes64(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_changes64, timeout, db);
    }

    int sqlite3_total_changes(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_total_changes, timeout, db);
    }

    sqlite3_int64 sqlite3_total_changes64(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_total_changes64, timeout, db);
    }

    void sqlite3_interrupt(sqlite3* db) {
        ::sqlite3_interrupt(db);
    }

    int sqlite3_is_interrupted(sqlite3* db) {
        return ::sqlite3_is_interrupted(db);
    }

    int sqlite3_complete(const char* sql, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_complete, timeout, sql);
    }

    int sqlite3_complete16(const void* sql, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_complete16, timeout, sql);
    }

    int sqlite3_prepare(sqlite3* db, const char* zSql, int nByte, sqlite3_stmt** ppStmt, const char** pzTail, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_prepare, timeout, db, zSql, nByte, ppStmt, pzTail);
    }

    int sqlite3_prepare_v2(sqlite3* db, const char* sql, int nBytes, sqlite3_stmt** ppStmt, const char** pzTail, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_prepare_v2, timeout, db, sql, nBytes, ppStmt, pzTail);
    }

    int sqlite3_prepare_v3(sqlite3* db, const char* zSql, int nByte, unsigned int prepFlags, sqlite3_stmt** ppStmt, const char** pzTail, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_prepare_v3, timeout, db, zSql, nByte, prepFlags, ppStmt, pzTail);
    }

    int sqlite3_prepare16(sqlite3* db, const void* zSql, int nByte, sqlite3_stmt** ppStmt, const void** pzTail, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_prepare16, timeout, db, zSql, nByte, ppStmt, pzTail);
    }

    int sqlite3_prepare16_v2(sqlite3* db, const void* zSql, int nByte, sqlite3_stmt** ppStmt, const void** pzTail, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_prepare16_v2, timeout, db, zSql, nByte, ppStmt, pzTail);
    }

    int sqlite3_prepare16_v3(sqlite3* db, const void* zSql, int nByte, unsigned int prepFlags, sqlite3_stmt** ppStmt, const void** pzTail, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_prepare16_v3, timeout, db, zSql, nByte, prepFlags, ppStmt, pzTail);
    }

    const char* sqlite3_sql(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_sql, timeout, pStmt);
    }

    char* sqlite3_expanded_sql(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_expanded_sql, timeout, pStmt);
    }

    int sqlite3_stmt_readonly(sqlite3_stmt* pStmt) {
        return ::sqlite3_stmt_readonly(pStmt);
    }

    int sqlite3_stmt_isexplain(sqlite3_stmt* pStmt) {
        return ::sqlite3_stmt_isexplain(pStmt);
    }

    int sqlite3_stmt_explain(sqlite3_stmt* pStmt, int eMode, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_stmt_explain, timeout, pStmt, eMode);
    }

    int sqlite3_stmt_busy(sqlite3_stmt* pStmt) {
        return ::sqlite3_stmt_busy(pStmt);
    }

    int sqlite3_finalize(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_finalize, timeout, pStmt);
    }

    int sqlite3_reset(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_reset, timeout, pStmt);
    }

    int sqlite3_step(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_step, timeout, pStmt);
    }

    int sqlite3_data_count(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_data_count, timeout, pStmt);
    }

    int sqlite3_column_count(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_count, timeout, pStmt);
    }

    const char* sqlite3_column_name(sqlite3_stmt* pStmt, int col, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_name, timeout, pStmt, col);
    }

    const void* sqlite3_column_name16(sqlite3_stmt* pStmt, int N, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_name16, timeout, pStmt, N);
    }

    /// Executes SQL on the dedicated worker thread and blocks the calling thread until completion.
    ///
    /// @note **Blocking Behavior**: This function posts the task to the worker thread and 
    ///       waits (sleeps) until the task completes or the timeout expires.
    ///
    /// @warning **Callback Thread Context**: The `callback` function is invoked on the 
    ///          **private worker thread**, NOT the calling thread. 
    ///          - Ensure strictly thread-safe access to any shared data inside the callback.
    ///          - Do NOT attempt to update UI elements directly from the callback.
    ///
    /// @param[in]  db       Database handle.
    /// @param[in]  sql      SQL script to be evaluated.
    /// @param[in]  callback Callback function invoked for each result row (runs on worker thread).
    /// @param[in]  pArg     1st argument to callback function.
    /// @param[out] errMsg   Error msg written here.
    /// @param[in]  timeout  Maximum duration to wait for completion. Returns SQLITE_BUSY if expired.
    /// @return SQLite result code (e.g., SQLITE_OK) or SQLITE_BUSY on timeout.
    int sqlite3_exec(sqlite3* db, const char* sql, sqlite3_callback callback, void* pArg, char** errMsg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_exec, timeout, db, sql, callback, pArg, errMsg);
    }

    int sqlite3_exec_begin(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_exec, timeout, db, "BEGIN", nullptr, nullptr, nullptr);
    }

    int sqlite3_exec_commit(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_exec, timeout, db, "COMMIT", nullptr, nullptr, nullptr);
    }

    int sqlite3_exec_rollback(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_exec, timeout, db, "ROLLBACK", nullptr, nullptr, nullptr);
    }

    int sqlite3_db_status(sqlite3* db, int op, int* pCurrent, int* pHighwater, int resetFlag, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_db_status, timeout, db, op, pCurrent, pHighwater, resetFlag);
    }

    const void* sqlite3_column_blob(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_blob, timeout, pStmt, iCol);
    }

    double sqlite3_column_double(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_double, timeout, pStmt, iCol);
    }

    int sqlite3_column_int(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_int, timeout, pStmt, iCol);
    }

    sqlite3_int64 sqlite3_column_int64(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_int64, timeout, pStmt, iCol);
    }

    const unsigned char* sqlite3_column_text(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_text, timeout, pStmt, iCol);
    }

    const void* sqlite3_column_text16(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_text16, timeout, pStmt, iCol);
    }

    sqlite3_value* sqlite3_column_value(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_value, timeout, pStmt, iCol);
    }

    int sqlite3_column_bytes(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_bytes, timeout, pStmt, iCol);
    }

    int sqlite3_column_bytes16(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_bytes16, timeout, pStmt, iCol);
    }

    int sqlite3_column_type(sqlite3_stmt* pStmt, int iCol, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_type, timeout, pStmt, iCol);
    }

    void* sqlite3_malloc(int size, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_malloc, timeout, size);
    }

    void* sqlite3_malloc64(sqlite3_uint64 size, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_malloc64, timeout, size);
    }

    void* sqlite3_realloc(void* ptr, int size, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_realloc, timeout, ptr, size);
    }

    void* sqlite3_realloc64(void* ptr, sqlite3_uint64 size, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_realloc64, timeout, ptr, size);
    }

    void sqlite3_free(void* ptr, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_free, timeout, ptr);
    }

    sqlite3_uint64 sqlite3_msize(void* ptr, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_msize, timeout, ptr);
    }

    const char* sqlite3_uri_parameter(sqlite3_filename zFilename, const char* zParam, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_uri_parameter, timeout, zFilename, zParam);
    }

    int sqlite3_uri_boolean(sqlite3_filename zFilename, const char* zParam, int bDflt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_uri_boolean, timeout, zFilename, zParam, bDflt);
    }

    sqlite3_int64 sqlite3_uri_int64(sqlite3_filename zFilename, const char* zParam, sqlite3_int64 bDflt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_uri_int64, timeout, zFilename, zParam, bDflt);
    }

    const char* sqlite3_uri_key(sqlite3_filename zFilename, int N, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_uri_key, timeout, zFilename, N);
    }

    const char* sqlite3_filename_database(sqlite3_filename zFilename, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_filename_database, timeout, zFilename);
    }

    const char* sqlite3_filename_journal(sqlite3_filename zFilename, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_filename_journal, timeout, zFilename);
    }

    const char* sqlite3_filename_wal(sqlite3_filename zFilename, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_filename_wal, timeout, zFilename);
    }

    sqlite3_file* sqlite3_database_file_object(const char* zName, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_database_file_object, timeout, zName);
    }

    sqlite3_filename sqlite3_create_filename(const char* zDatabase, const char* zJournal, const char* zWal, int nParam, const char** azParam, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_create_filename, timeout, zDatabase, zJournal, zWal, nParam, azParam);
    }

    void sqlite3_free_filename(sqlite3_filename p, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_free_filename, timeout, p);
    }

    int sqlite3_errcode(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_errcode, timeout, db);
    }

    int sqlite3_extended_errcode(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(&::sqlite3_extended_errcode, timeout, db);
    }

    const char* sqlite3_errmsg(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(&::sqlite3_errmsg, timeout, db);
    }

    const void* sqlite3_errmsg16(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(&::sqlite3_errmsg16, timeout, db);
    }

    const char* sqlite3_errstr(int errorCode, dmq::Duration timeout) {
        return AsyncInvoke(&::sqlite3_errstr, timeout, errorCode);
    }

    int sqlite3_error_offset(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(&::sqlite3_error_offset, timeout, db);
    }

    int sqlite3_limit(sqlite3* db, int limitId, int newLimit, dmq::Duration timeout) {
        return AsyncInvoke(&::sqlite3_limit, timeout, db, limitId, newLimit);
    }

    sqlite3_int64 sqlite3_memory_used(dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_memory_used, timeout);
    }

    sqlite3_int64 sqlite3_memory_highwater(int resetFlag, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_memory_highwater, timeout, resetFlag);
    }

    void sqlite3_randomness(int N, void* P, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_randomness, timeout, N, P);
    }

    /// Registers an authorizer callback.
    ///
    /// @warning **Callback Thread Context**: The `xAuth` function is invoked on the 
    ///          **private worker thread** when a SQL statement is being compiled.
    ///          - Ensure strictly thread-safe access if `pUserData` points to shared state.
    int sqlite3_set_authorizer(sqlite3* db, int (*xAuth)(void*, int, const char*, const char*, const char*, const char*), void* pArg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_set_authorizer, timeout, db, xAuth, pArg);
    }

    int sqlite3_get_table(sqlite3* db, const char* sql, char*** resultpAzResult, int* nrow, int* ncolumn, char** errmsg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_get_table, timeout, db, sql, resultpAzResult, nrow, ncolumn, errmsg);
    }

    void sqlite3_free_table(char** result, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_free_table, timeout, result);
    }

    int sqlite3_busy_timeout(sqlite3* db, int ms, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_busy_timeout, timeout, db, ms);
    }

    int sqlite3_busy_handler(sqlite3* db, int (*xBusy)(void*, int), void* pArg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_busy_handler, timeout, db, xBusy, pArg);
    }

    /// Binds a blob to a prepared statement.
    ///
    /// @warning **Buffer Safety**: If passing `SQLITE_STATIC` as the destructor (`xDel`), 
    ///          you promise that `zData` will not change or be deleted until the statement is finalized.
    ///          In a multi-threaded app, use `SQLITE_TRANSIENT` to force SQLite to make its own copy 
    ///          immediately, preventing race conditions if the caller modifies the buffer later.
    int sqlite3_bind_blob(sqlite3_stmt* pStmt, int i, const void* zData, int nData, void (*xDel)(void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_blob, timeout, pStmt, i, zData, nData, xDel);
    }

    int sqlite3_bind_blob64(sqlite3_stmt* pStmt, int i, const void* zData, sqlite3_uint64 nData, void (*xDel)(void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_blob64, timeout, pStmt, i, zData, nData, xDel);
    }

    int sqlite3_bind_double(sqlite3_stmt* pStmt, int i, double value, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_double, timeout, pStmt, i, value);
    }

    int sqlite3_bind_int(sqlite3_stmt* pStmt, int i, int value, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_int, timeout, pStmt, i, value);
    }

    int sqlite3_bind_int64(sqlite3_stmt* pStmt, int i, sqlite3_int64 value, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_int64, timeout, pStmt, i, value);
    }

    int sqlite3_bind_null(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_null, timeout, pStmt, i);
    }

    /// Binds text to a prepared statement.
    ///
    /// @warning **Buffer Safety**: If passing `SQLITE_STATIC` as the destructor (`xDel`), 
    ///          you promise that `zData` will not change or be deleted until the statement is finalized.
    ///          In a multi-threaded app, use `SQLITE_TRANSIENT` to force SQLite to make its own copy 
    ///          immediately, preventing race conditions if the caller modifies the string later.
    int sqlite3_bind_text(sqlite3_stmt* pStmt, int i, const char* zData, int nData, void (*xDel)(void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_text, timeout, pStmt, i, zData, nData, xDel);
    }

    int sqlite3_bind_text16(sqlite3_stmt* pStmt, int i, const void* zData, int nData, void (*xDel)(void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_text16, timeout, pStmt, i, zData, nData, xDel);
    }

    int sqlite3_bind_text64(sqlite3_stmt* pStmt, int i, const char* zData, sqlite3_uint64 nData, void (*xDel)(void*), unsigned char encoding, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_text64, timeout, pStmt, i, zData, nData, xDel, encoding);
    }

    int sqlite3_bind_value(sqlite3_stmt* pStmt, int i, const sqlite3_value* pValue, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_value, timeout, pStmt, i, pValue);
    }

    int sqlite3_bind_pointer(sqlite3_stmt* pStmt, int i, void* pData, const char* pType, void (*xDel)(void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_pointer, timeout, pStmt, i, pData, pType, xDel);
    }

    int sqlite3_bind_zeroblob(sqlite3_stmt* pStmt, int i, int n, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_zeroblob, timeout, pStmt, i, n);
    }

    int sqlite3_bind_zeroblob64(sqlite3_stmt* pStmt, int i, sqlite3_uint64 n, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_zeroblob64, timeout, pStmt, i, n);
    }

    int sqlite3_bind_parameter_count(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_parameter_count, timeout, pStmt);
    }

    const char* sqlite3_bind_parameter_name(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_parameter_name, timeout, pStmt, i);
    }

    int sqlite3_bind_parameter_index(sqlite3_stmt* pStmt, const char* zName, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_bind_parameter_index, timeout, pStmt, zName);
    }

    int sqlite3_clear_bindings(sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_clear_bindings, timeout, pStmt);
    }

#ifdef SQLITE_ENABLE_COLUMN_METADATA
    const char* sqlite3_column_database_name(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_database_name, timeout, pStmt, i);
    }

    const void* sqlite3_column_database_name16(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_database_name16, timeout, pStmt, i);
    }

    const char* sqlite3_column_table_name(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_table_name, timeout, pStmt, i);
    }

    const void* sqlite3_column_table_name16(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_table_name16, timeout, pStmt, i);
    }

    const char* sqlite3_column_origin_name(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_origin_name, timeout, pStmt, i);
    }

    const void* sqlite3_column_origin_name16(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_origin_name16, timeout, pStmt, i);
    }
#endif // SQLITE_ENABLE_COLUMN_METADATA

    const char* sqlite3_column_decltype(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_decltype, timeout, pStmt, i);
    }

    const void* sqlite3_column_decltype16(sqlite3_stmt* pStmt, int i, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_column_decltype16, timeout, pStmt, i);
    }

    void sqlite3_result_blob(sqlite3_context* context, const void* data, int nData, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_blob, timeout, context, data, nData, xDel);
    }

    void sqlite3_result_blob64(sqlite3_context* context, const void* data, sqlite3_uint64 nData, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_blob64, timeout, context, data, nData, xDel);
    }

    void sqlite3_result_double(sqlite3_context* context, double value, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_double, timeout, context, value);
    }

    void sqlite3_result_error(sqlite3_context* context, const char* errorMsg, int msgLength, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_error, timeout, context, errorMsg, msgLength);
    }

    void sqlite3_result_error16(sqlite3_context* context, const void* errorMsg, int msgLength, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_error16, timeout, context, errorMsg, msgLength);
    }

    void sqlite3_result_error_toobig(sqlite3_context* context, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_error_toobig, timeout, context);
    }

    void sqlite3_result_error_nomem(sqlite3_context* context, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_error_nomem, timeout, context);
    }

    void sqlite3_result_error_code(sqlite3_context* context, int errorCode, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_error_code, timeout, context, errorCode);
    }

    void sqlite3_result_int(sqlite3_context* context, int value, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_int, timeout, context, value);
    }

    void sqlite3_result_int64(sqlite3_context* context, sqlite3_int64 value, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_int64, timeout, context, value);
    }

    void sqlite3_result_null(sqlite3_context* context, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_null, timeout, context);
    }

    void sqlite3_result_text(sqlite3_context* context, const char* text, int nText, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_text, timeout, context, text, nText, xDel);
    }

    void sqlite3_result_text64(sqlite3_context* context, const char* text, sqlite3_uint64 nText, void(*xDel)(void*), unsigned char encoding, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_text64, timeout, context, text, nText, xDel, encoding);
    }

    void sqlite3_result_text16(sqlite3_context* context, const void* text, int nText, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_text16, timeout, context, text, nText, xDel);
    }

    void sqlite3_result_text16le(sqlite3_context* context, const void* text, int nText, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_text16le, timeout, context, text, nText, xDel);
    }

    void sqlite3_result_text16be(sqlite3_context* context, const void* text, int nText, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_text16be, timeout, context, text, nText, xDel);
    }

    void sqlite3_result_value(sqlite3_context* context, sqlite3_value* value, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_value, timeout, context, value);
    }

    void sqlite3_result_pointer(sqlite3_context* context, void* pointer, const char* type, void(*xDel)(void*), dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_pointer, timeout, context, pointer, type, xDel);
    }

    void sqlite3_result_zeroblob(sqlite3_context* context, int n, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_zeroblob, timeout, context, n);
    }

    int sqlite3_result_zeroblob64(sqlite3_context* context, sqlite3_uint64 n, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_result_zeroblob64, timeout, context, n);
    }

    void sqlite3_result_subtype(sqlite3_context* context, unsigned int subtype, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_result_subtype, timeout, context, subtype);
    }

    int sqlite3_create_collation(sqlite3* db, const char* zName, int eTextRep, void* pArg, int(*xCompare)(void*, int, const void*, int, const void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_create_collation, timeout, db, zName, eTextRep, pArg, xCompare);
    }

    int sqlite3_create_collation_v2(sqlite3* db, const char* zName, int eTextRep, void* pArg, int(*xCompare)(void*, int, const void*, int, const void*), void(*xDestroy)(void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_create_collation_v2, timeout, db, zName, eTextRep, pArg, xCompare, xDestroy);
    }

    int sqlite3_create_collation16(sqlite3* db, const void* zName, int eTextRep, void* pArg, int(*xCompare)(void*, int, const void*, int, const void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_create_collation16, timeout, db, zName, eTextRep, pArg, xCompare);
    }

    int sqlite3_collation_needed(sqlite3* db, void* pArg, void(*xCollNeeded)(void*, sqlite3*, int eTextRep, const char*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_collation_needed, timeout, db, pArg, xCollNeeded);
    }

    int sqlite3_collation_needed16(sqlite3* db, void* pArg, void(*xCollNeeded)(void*, sqlite3*, int eTextRep, const void*), dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_collation_needed16, timeout, db, pArg, xCollNeeded);
    }

    int sqlite3_sleep(int milliseconds, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_sleep, timeout, milliseconds);
    }

    sqlite3* sqlite3_db_handle(sqlite3_stmt* pStmt) {
        return ::sqlite3_db_handle(pStmt);
    }

    const char* sqlite3_db_name(sqlite3* db, int N) {
        return ::sqlite3_db_name(db, N);
    }

    sqlite3_filename sqlite3_db_filename(sqlite3* db, const char* zDbName) {
        return ::sqlite3_db_filename(db, zDbName);
    }

    int sqlite3_db_readonly(sqlite3* db, const char* zDbName) {
        return ::sqlite3_db_readonly(db, zDbName);
    }

    sqlite3_stmt* sqlite3_next_stmt(sqlite3* pDb, sqlite3_stmt* pStmt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_next_stmt, timeout, pDb, pStmt);
    }

    int sqlite3_release_memory(int n, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_release_memory, timeout, n);
    }

    int sqlite3_db_release_memory(sqlite3* pDb, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_db_release_memory, timeout, pDb);
    }

    int sqlite3_table_column_metadata(sqlite3* db, const char* zDbName, const char* zTableName, const char* zColumnName, char const** pzDataType, char const** pzCollSeq, int* pNotNull, int* pPrimaryKey, int* pAutoinc, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_table_column_metadata, timeout, db, zDbName, zTableName, zColumnName, pzDataType, pzCollSeq, pNotNull, pPrimaryKey, pAutoinc);
    }

    int sqlite3_load_extension(sqlite3* db, const char* zFile, const char* zProc, char** pzErrMsg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_load_extension, timeout, db, zFile, zProc, pzErrMsg);
    }

    int sqlite3_enable_load_extension(sqlite3* db, int onoff, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_enable_load_extension, timeout, db, onoff);
    }

    sqlite3_str* sqlite3_str_new(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_str_new, timeout, db);
    }

    char* sqlite3_str_finish(sqlite3_str* pStr, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_str_finish, timeout, pStr);
    }

    void sqlite3_str_append(sqlite3_str* pStr, const char* zIn, int N, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_str_append, timeout, pStr, zIn, N);
    }

    void sqlite3_str_appendall(sqlite3_str* pStr, const char* zIn, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_str_appendall, timeout, pStr, zIn);
    }

    void sqlite3_str_appendchar(sqlite3_str* pStr, int N, char C, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_str_appendchar, timeout, pStr, N, C);
    }

    void sqlite3_str_reset(sqlite3_str* pStr, dmq::Duration timeout) {
        AsyncInvoke(::sqlite3_str_reset, timeout, pStr);
    }

    int sqlite3_str_errcode(sqlite3_str* pStr, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_str_errcode, timeout, pStr);
    }

    int sqlite3_str_length(sqlite3_str* pStr, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_str_length, timeout, pStr);
    }

    char* sqlite3_str_value(sqlite3_str* pStr, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_str_value, timeout, pStr);
    }

    int sqlite3_status(int op, int* pCurrent, int* pHighwater, int resetFlag, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_status, timeout, op, pCurrent, pHighwater, resetFlag);
    }

    int sqlite3_status64(int op, sqlite3_int64* pCurrent, sqlite3_int64* pHighwater, int resetFlag, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_status64, timeout, op, pCurrent, pHighwater, resetFlag);
    }

    int sqlite3_stmt_status(sqlite3_stmt* pStmt, int op, int resetFlg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_stmt_status, timeout, pStmt, op, resetFlg);
    }

    sqlite3_backup* sqlite3_backup_init(sqlite3* pDest, const char* zDestName, sqlite3* pSource, const char* zSourceName, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_backup_init, timeout, pDest, zDestName, pSource, zSourceName);
    }

    int sqlite3_backup_step(sqlite3_backup* p, int nPage, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_backup_step, timeout, p, nPage);
    }

    int sqlite3_backup_finish(sqlite3_backup* p, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_backup_finish, timeout, p);
    }

    int sqlite3_backup_remaining(sqlite3_backup* p, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_backup_remaining, timeout, p);
    }

    int sqlite3_backup_pagecount(sqlite3_backup* p, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_backup_pagecount, timeout, p);
    }

#ifdef SQLITE_ENABLE_UNLOCK_NOTIFY
    int sqlite3_unlock_notify(sqlite3* pBlocked, void (*xNotify)(void** apArg, int nArg), void* pNotifyArg, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_unlock_notify, timeout, pBlocked, xNotify, pNotifyArg);
    }
#endif

    int sqlite3_stricmp(const char* z1, const char* z2, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_stricmp, timeout, z1, z2);
    }

    int sqlite3_strnicmp(const char* z1, const char* z2, int N, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_strnicmp, timeout, z1, z2, N);
    }

    int sqlite3_strglob(const char* zGlob, const char* zStr, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_strglob, timeout, zGlob, zStr);
    }

    int sqlite3_strlike(const char* zGlob, const char* zStr, unsigned int cEsc, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_strlike, timeout, zGlob, zStr, cEsc);
    }

    int sqlite3_wal_autocheckpoint(sqlite3* db, int N, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_wal_autocheckpoint, timeout, db, N);
    }

    int sqlite3_wal_checkpoint(sqlite3* db, const char* zDb, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_wal_checkpoint, timeout, db, zDb);
    }

    int sqlite3_wal_checkpoint_v2(sqlite3* db, const char* zDb, int eMode, int* pnLog, int* pnCkpt, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_wal_checkpoint_v2, timeout, db, zDb, eMode, pnLog, pnCkpt);
    }

    int sqlite3_db_cacheflush(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_db_cacheflush, timeout, db);
    }

    int sqlite3_system_errno(sqlite3* db, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_system_errno, timeout, db);
    }

    unsigned char* sqlite3_serialize(sqlite3* db, const char* zSchema, sqlite3_int64* piSize, unsigned int mFlags, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_serialize, timeout, db, zSchema, piSize, mFlags);
    }

    int sqlite3_deserialize(sqlite3* db, const char* zSchema, unsigned char* pData, sqlite3_int64 szDb, sqlite3_int64 szBuf, unsigned mFlags, dmq::Duration timeout) {
        return AsyncInvoke(::sqlite3_deserialize, timeout, db, zSchema, pData, szDb, szBuf, mFlags);
    }

    /// Asynchronously executes SQL on the dedicated worker thread (Non-Blocking).
    ///
    /// @note **Fire-and-Forget**: This function returns immediately. The main thread continues 
    ///       execution while the SQL query runs in the background.
    ///
    /// @warning **LIFETIME SAFETY WARNING**: 
    ///          Because execution is deferred, you must ensure that **ALL pointers passed as arguments**
    ///          (`sql`, `pArg`, `errMsg`) point to memory that remains valid until the operation completes.
    ///          - **BAD:** Passing a pointer to a local stack variable that goes out of scope.
    ///          - **GOOD:** Passing string literals, global variables, or heap-allocated objects managed by shared_ptr.
    ///
    /// @warning **Callback Thread Context**: The `callback` function is invoked on the 
    ///          **private worker thread**, NOT the calling thread. 
    ///
    /// @param[in]  db       Database handle.
    /// @param[in]  sql      SQL script to be evaluated. Must remain valid until execution completes.
    /// @param[in]  callback Callback function invoked for each result row (runs on worker thread).
    /// @param[in]  pArg     1st argument to callback function. Must remain valid until execution completes.
    /// @param[out] errMsg   Pointer to a char* where error message is written. The location must remain valid.
    /// @return std::future<int> A future that resolves to the SQLite result code (e.g., SQLITE_OK).
    std::future<int> sqlite3_exec_future(sqlite3* db, const char* sql, sqlite3_callback callback, void* pArg, char** errMsg) {
        return AsyncInvokeFuture(::sqlite3_exec, db, sql, callback, pArg, errMsg);
    }

    std::future<int> sqlite3_step_future(sqlite3_stmt* pStmt) {
        return AsyncInvokeFuture(::sqlite3_step, pStmt);
    }

    std::future<int> sqlite3_finalize_future(sqlite3_stmt* pStmt) {
        return AsyncInvokeFuture(::sqlite3_finalize, pStmt);
    }

    std::future<int> sqlite3_exec_commit_future(sqlite3* db) {
        return AsyncInvokeFuture(::sqlite3_exec, db, "COMMIT", nullptr, nullptr, nullptr);
    }

    std::future<int> sqlite3_exec_rollback_future(sqlite3* db) {
        return AsyncInvokeFuture(::sqlite3_exec, db, "ROLLBACK", nullptr, nullptr, nullptr);
    }

    std::future<int> sqlite3_backup_step_future(sqlite3_backup* p, int nPage) {
        return AsyncInvokeFuture(::sqlite3_backup_step, p, nPage);
    }

    /// Asynchronously serializes the database content.
    ///
    /// @warning **LIFETIME SAFETY WARNING**: 
    ///          This function returns immediately, but the worker thread accesses the arguments later.
    ///          - `zSchema` MUST point to a string that remains valid until the future completes.
    ///          - `piSize` MUST point to a valid integer location until the future completes.
    ///          - **Do NOT pass pointers to local stack variables.**
    std::future<unsigned char*> sqlite3_serialize_future(sqlite3* db, const char* zSchema, sqlite3_int64* piSize, unsigned int mFlags) {
        return AsyncInvokeFuture(::sqlite3_serialize, db, zSchema, piSize, mFlags);
    }

    /// Asynchronously deserializes a buffer into an in-memory database.
    ///
    /// @warning **LIFETIME SAFETY WARNING**: 
    ///          This function returns immediately, but the worker thread accesses the arguments later.
    ///          - `zSchema` and `pData` MUST remain valid pointers until the future completes.
    ///          - If you allocated `pData`, do not `free()` it until you have checked the future result.
    std::future<int> sqlite3_deserialize_future(sqlite3* db, const char* zSchema, unsigned char* pData, sqlite3_int64 szDb, sqlite3_int64 szBuf, unsigned mFlags) {
        return AsyncInvokeFuture(::sqlite3_deserialize, db, zSchema, pData, szDb, szBuf, mFlags);
    }

} // namespace async