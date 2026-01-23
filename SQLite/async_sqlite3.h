#ifndef ASYNC_SQLITE3_H
#define ASYNC_SQLITE3_H

// Asynchronous SQLite wrapper using C++ Delegates
// @see https://github.com/endurodave/Async-SQLite
// @see https://github.com/endurodave/DelegateMQ
// David Lafreniere, Nov 2024

// ------------------------------------------------------------------------------------------------
// ARCHITECTURE OVERVIEW:
// ------------------------------------------------------------------------------------------------
// This library wraps the standard SQLite C API to execute all database operations on a 
// dedicated background thread (Worker Thread). This ensures:
// 1. Thread Safety: SQLite handles are accessed serially by a single thread.
// 2. Non-Blocking UI: Heavy queries run in the background without freezing the main thread.
//
// API CATEGORIES:
// 1. Synchronous (Blocking): Functions like sqlite3_exec() block the calling thread until 
//    the worker completes the task or a timeout occurs.
// 2. Asynchronous (Future): Functions ending in _future() return immediately with a 
//    std::future<int>, allowing the caller to wait or poll for the result later.
//
// SETUP:
// Call sqlite3_init_async() once at application startup before using any other API.
// ------------------------------------------------------------------------------------------------

#include "sqlite3.h"
#include "DelegateMQ.h" 
#include <future>
#include <memory>
#include <string>
#include <tuple>
#include <chrono>

#ifndef SQLITE_API
#define SQLITE_API
#endif

namespace async
{
    // @TODO Change maximum async timeout duration if necessary. See WAIT_INFINITE 
    // for comment on maximum value allowed. 
    constexpr dmq::Duration MAX_WAIT = std::chrono::minutes(2);

    // -------------------------------------------------------------------------
    // Initialization & Management
    // -------------------------------------------------------------------------
    SQLITE_API int sqlite3_init_async(void);
    SQLITE_API int sqlite3_shutdown(dmq::Duration timeout = MAX_WAIT);

    // Accessor for the internal worker thread
    Thread* sqlite3_get_thread(void);

    // -------------------------------------------------------------------------
    // Future / Async API (Non-Blocking)
    // -------------------------------------------------------------------------
    std::future<int> sqlite3_exec_future(sqlite3* db, const char* sql, sqlite3_callback callback, void* pArg, char** errMsg);
    std::future<int> sqlite3_step_future(sqlite3_stmt* pStmt);
    std::future<int> sqlite3_finalize_future(sqlite3_stmt* pStmt);
    std::future<int> sqlite3_exec_commit_future(sqlite3* db);
    std::future<int> sqlite3_exec_rollback_future(sqlite3* db);
    std::future<int> sqlite3_backup_step_future(sqlite3_backup* p, int nPage);

    std::future<unsigned char*> sqlite3_serialize_future(sqlite3* db, const char* zSchema, sqlite3_int64* piSize, unsigned int mFlags);
    std::future<int> sqlite3_deserialize_future(sqlite3* db, const char* zSchema, unsigned char* pData, sqlite3_int64 szDb, sqlite3_int64 szBuf, unsigned mFlags);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Connection & Core)
    // -------------------------------------------------------------------------
    SQLITE_API int sqlite3_open(const char* filename, sqlite3** ppDb, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_open16(const void* filename, sqlite3** ppDb, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_open_v2(const char* filename, sqlite3** ppDb, int flags, const char* zVfs, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_close(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_close_v2(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_exec(sqlite3* db, const char* sql, sqlite3_callback callback, void* pArg, char** errMsg, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_exec_begin(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_exec_commit(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_exec_rollback(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_initialize(void);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Prepared Statements)
    // -------------------------------------------------------------------------
    SQLITE_API int sqlite3_prepare(sqlite3* db, const char* zSql, int nByte, sqlite3_stmt** ppStmt, const char** pzTail, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_prepare_v2(sqlite3* db, const char* sql, int nBytes, sqlite3_stmt** ppStmt, const char** pzTail, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_prepare_v3(sqlite3* db, const char* zSql, int nByte, unsigned int prepFlags, sqlite3_stmt** ppStmt, const char** pzTail, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_prepare16(sqlite3* db, const void* zSql, int nByte, sqlite3_stmt** ppStmt, const void** pzTail, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_prepare16_v2(sqlite3* db, const void* zSql, int nByte, sqlite3_stmt** ppStmt, const void** pzTail, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_prepare16_v3(sqlite3* db, const void* zSql, int nByte, unsigned int prepFlags, sqlite3_stmt** ppStmt, const void** pzTail, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_step(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_finalize(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_reset(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_sql(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API char* sqlite3_expanded_sql(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_stmt_readonly(sqlite3_stmt* pStmt);
    SQLITE_API int sqlite3_stmt_isexplain(sqlite3_stmt* pStmt);
    SQLITE_API int sqlite3_stmt_explain(sqlite3_stmt* pStmt, int eMode, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_stmt_busy(sqlite3_stmt*);
    SQLITE_API int sqlite3_data_count(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_count(sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_column_name(sqlite3_stmt* pStmt, int col, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_name16(sqlite3_stmt*, int N, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_clear_bindings(sqlite3_stmt*, dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Binding)
    // -------------------------------------------------------------------------
    SQLITE_API int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int n, void(*)(void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_blob64(sqlite3_stmt*, int, const void*, sqlite3_uint64, void(*)(void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_double(sqlite3_stmt*, int, double, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_int(sqlite3_stmt*, int, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_int64(sqlite3_stmt*, int, sqlite3_int64, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_null(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int, void(*)(void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_text16(sqlite3_stmt*, int, const void*, int, void(*)(void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_text64(sqlite3_stmt*, int, const char*, sqlite3_uint64, void(*)(void*), unsigned char encoding, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_value(sqlite3_stmt*, int, const sqlite3_value*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_pointer(sqlite3_stmt*, int, void*, const char*, void(*)(void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_zeroblob(sqlite3_stmt*, int, int n, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_zeroblob64(sqlite3_stmt*, int, sqlite3_uint64, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_parameter_count(sqlite3_stmt*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_bind_parameter_name(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_bind_parameter_index(sqlite3_stmt*, const char* zName, dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Column Access)
    // -------------------------------------------------------------------------
    SQLITE_API const void* sqlite3_column_blob(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API double sqlite3_column_double(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_int(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_column_int64(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const unsigned char* sqlite3_column_text(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_text16(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_value* sqlite3_column_value(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_bytes(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_bytes16(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_column_type(sqlite3_stmt*, int iCol, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_column_database_name(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_database_name16(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_column_table_name(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_table_name16(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_column_origin_name(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_origin_name16(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_column_decltype(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_column_decltype16(sqlite3_stmt*, int, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_table_column_metadata(sqlite3* db, const char* zDbName, const char* zTableName, const char* zColumnName, char const** pzDataType, char const** pzCollSeq, int* pNotNull, int* pPrimaryKey, int* pAutoinc, dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Utilities & Metadata)
    // -------------------------------------------------------------------------
    SQLITE_API int sqlite3_changes(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_changes64(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_last_insert_rowid(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_set_last_insert_rowid(sqlite3* db, sqlite3_int64 iRowid, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_total_changes(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_total_changes64(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_interrupt(sqlite3* db);
    SQLITE_API int sqlite3_is_interrupted(sqlite3* db);
    SQLITE_API int sqlite3_complete(const char* sql, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_complete16(const void* sql, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API const char* sqlite3_errmsg(sqlite3*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const void* sqlite3_errmsg16(sqlite3*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_errcode(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_extended_errcode(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_extended_result_codes(sqlite3* db, int onoff, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_errstr(int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_error_offset(sqlite3* db, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_limit(sqlite3* db, int id, int newVal, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_memory_used(dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_memory_highwater(int resetFlag, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_randomness(int N, void* P, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_status(int op, int* pCurrent, int* pHighwater, int resetFlag, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_status64(int op, sqlite3_int64* pCurrent, sqlite3_int64* pHighwater, int resetFlag, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_stmt_status(sqlite3_stmt*, int op, int resetFlg, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_db_status(sqlite3* db, int op, int* pCurrent, int* pHighwater, int resetFlag, dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Advanced & Admin)
    // -------------------------------------------------------------------------
    SQLITE_API sqlite3_backup* sqlite3_backup_init(sqlite3* pDest, const char* zDestName, sqlite3* pSource, const char* zSourceName, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_step(sqlite3_backup* p, int nPage, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_finish(sqlite3_backup* p, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_remaining(sqlite3_backup* p, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_backup_pagecount(sqlite3_backup* p, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_wal_autocheckpoint(sqlite3* db, int N, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_wal_checkpoint(sqlite3* db, const char* zDb, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_wal_checkpoint_v2(sqlite3* db, const char* zDb, int eMode, int* pnLog, int* pnCkpt, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API unsigned char* sqlite3_serialize(sqlite3* db, const char* zSchema, sqlite3_int64* piSize, unsigned int mFlags, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_deserialize(sqlite3* db, const char* zSchema, unsigned char* pData, sqlite3_int64 szDb, sqlite3_int64 szBuf, unsigned mFlags, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_db_cacheflush(sqlite3*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_system_errno(sqlite3*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_unlock_notify(sqlite3* pBlocked, void (*xNotify)(void** apArg, int nArg), void* pNotifyArg, dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // Synchronous / Blocking API (Helpers & Utils)
    // -------------------------------------------------------------------------
    SQLITE_API int sqlite3_trace_v2(sqlite3*, unsigned uMask, int(*xCallback)(unsigned, void*, void*, void*), void* pCtx, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_progress_handler(sqlite3*, int, int(*)(void*), void*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_set_authorizer(sqlite3*, int (*xAuth)(void*, int, const char*, const char*, const char*, const char*), void* pUserData, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_get_table(sqlite3* db, const char* zSql, char*** pazResult, int* pnRow, int* pnColumn, char** pzErrmsg, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_free_table(char** result, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API void* sqlite3_malloc(int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void* sqlite3_malloc64(sqlite3_uint64, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void* sqlite3_realloc(void*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void* sqlite3_realloc64(void*, sqlite3_uint64, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_free(void*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_uint64 sqlite3_msize(void*, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_sleep(int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3* sqlite3_db_handle(sqlite3_stmt*);
    SQLITE_API const char* sqlite3_db_name(sqlite3* db, int N);
    SQLITE_API sqlite3_filename sqlite3_db_filename(sqlite3* db, const char* zDbName);
    SQLITE_API int sqlite3_db_readonly(sqlite3* db, const char* zDbName);
    SQLITE_API sqlite3_stmt* sqlite3_next_stmt(sqlite3* pDb, sqlite3_stmt* pStmt, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_release_memory(int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_db_release_memory(sqlite3*, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_busy_timeout(sqlite3* db, int ms, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_busy_handler(sqlite3* db, int(*)(void*, int), void*, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_stricmp(const char*, const char*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_strnicmp(const char*, const char*, int, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_strglob(const char* zGlob, const char* zStr, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_strlike(const char* zGlob, const char* zStr, unsigned int cEsc, dmq::Duration timeout = MAX_WAIT);

    SQLITE_API int sqlite3_create_collation(sqlite3*, const char* zName, int eTextRep, void* pArg, int(*xCompare)(void*, int, const void*, int, const void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_create_collation_v2(sqlite3*, const char* zName, int eTextRep, void* pArg, int(*xCompare)(void*, int, const void*, int, const void*), void(*xDestroy)(void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_create_collation16(sqlite3*, const void* zName, int eTextRep, void* pArg, int(*xCompare)(void*, int, const void*, int, const void*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_collation_needed(sqlite3*, void*, void(*)(void*, sqlite3*, int eTextRep, const char*), dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_collation_needed16(sqlite3*, void*, void(*)(void*, sqlite3*, int eTextRep, const void*), dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // URI & Filename helpers
    // -------------------------------------------------------------------------
    SQLITE_API const char* sqlite3_uri_parameter(sqlite3_filename z, const char* zParam, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_uri_boolean(sqlite3_filename z, const char* zParam, int bDefault, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_int64 sqlite3_uri_int64(sqlite3_filename, const char*, sqlite3_int64, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_uri_key(sqlite3_filename z, int N, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_filename_database(sqlite3_filename, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_filename_journal(sqlite3_filename, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API const char* sqlite3_filename_wal(sqlite3_filename, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_file* sqlite3_database_file_object(const char*, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API sqlite3_filename sqlite3_create_filename(const char* zDatabase, const char* zJournal, const char* zWal, int nParam, const char** azParam, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_free_filename(sqlite3_filename, dmq::Duration timeout = MAX_WAIT);

    // -------------------------------------------------------------------------
    // String Builder
    // -------------------------------------------------------------------------
    SQLITE_API sqlite3_str* sqlite3_str_new(sqlite3* db, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API char* sqlite3_str_finish(sqlite3_str* pStr, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_append(sqlite3_str* pStr, const char* zIn, int N, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_appendall(sqlite3_str* pStr, const char* zIn, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_appendchar(sqlite3_str* pStr, int N, char C, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API void sqlite3_str_reset(sqlite3_str* pStr, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_str_errcode(sqlite3_str* pStr, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API int sqlite3_str_length(sqlite3_str* pStr, dmq::Duration timeout = MAX_WAIT);
    SQLITE_API char* sqlite3_str_value(sqlite3_str* pStr, dmq::Duration timeout = MAX_WAIT);

} // namespace async

#endif // ASYNC_SQLITE3_H