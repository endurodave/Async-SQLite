#include "async_sqlite3.h"
#include "DelegateLib.h"
#include "WorkerThreadStd.h"

// Asynchronous API's implemented using DelegateLib
// @see https://github.com/endurodave/AsyncMulticastDelegateModern 

using namespace DelegateLib;

// A private worker thread to execute all SQLite API functions
static WorkerThread SQLiteThread("SQLite Thread");

constexpr auto ASYNC_API_WAIT = std::chrono::milliseconds::max();
//constexpr auto ASYNC_API_WAIT = std::chrono::milliseconds(100);

void async::sqlite3_init_async(void)
{
    // Create the worker thread
    SQLiteThread.CreateThread();
}

SQLITE_API int async::sqlite3_open(
    const char* filename,   /* Database filename (UTF-8) */
    sqlite3** ppDb          /* OUT: SQLite db handle */
)
{
    // Asynchronous blocking function call ::sqlite3_open on SQLiteThread using the DelegateLib
    auto retVal = MakeDelegate(&::sqlite3_open, SQLiteThread, ASYNC_API_WAIT).AsyncInvoke(filename, ppDb);

    // Did the async invoke succeed?
    if (retVal.has_value())
        return retVal.value();  // Return sqlite3_open return value
    else
        return SQLITE_ERROR;    // Async function call failed
}

SQLITE_API int async::sqlite3_exec(
    sqlite3* db,                /* The database on which the SQL executes */
    const char* zSql,           /* The SQL to be executed */
    sqlite3_callback xCallback, /* Invoke this callback routine */
    void* pArg,                 /* First argument to xCallback() */
    char** pzErrMsg             /* Write error messages here */
)
{
    auto retVal = MakeDelegate(&::sqlite3_exec, SQLiteThread, ASYNC_API_WAIT).AsyncInvoke(db, zSql, xCallback, pArg, pzErrMsg);
    if (retVal.has_value())
        return retVal.value();  // Return sqlite3_exec return value
    else
        return SQLITE_ERROR;    // Async function call failed
}

SQLITE_API int async::sqlite3_close(sqlite3* db)
{
    auto retVal = MakeDelegate(&::sqlite3_close, SQLiteThread, ASYNC_API_WAIT).AsyncInvoke(db);
    if (retVal.has_value())
        return retVal.value();  // Return sqlite3_exec return value
    else
        return SQLITE_ERROR;    // Async function call failed
}

// TODO: Add more sqlite async API's as necessary