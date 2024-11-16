#include "async_sqlite3.h"
#include "DelegateLib.h"
#include "WorkerThreadStd.h"

// Asynchronous API's implemented using DelegateLib
// @see https://github.com/endurodave/Async-SQLite
// @see https://github.com/endurodave/AsyncMulticastDelegateModern 

using namespace DelegateLib;

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

        // Did the async function call succeed?
        if (retVal.has_value())
        {
            // Return the target functions return value
            return retVal.value();  
        }
        // Else async function call failed
        else
        {
            if constexpr (std::is_pointer_v<RetType>) 
            {
                // If return type is a pointer, return nullptr for errors
                return nullptr;
            }
            if constexpr (std::is_same_v<RetType, int>) 
            {
                // Special case for int, return SQLITE_ERROR
                return SQLITE_ERROR;
            }
            else 
            {
                // Return default-constructed value of the return type if the async call fails
                return RetType{};
            }
        }
    }
    else
    {
        // Invoke the target function synchronously since already executing on SQLiteThread
        return func(std::forward<Args>(args)...);
    }
}

void async::sqlite3_init_async(void)
{
    // Create the worker thread
    SQLiteThread.CreateThread();
}

DelegateLib::DelegateThread* async::sqlite3_get_thread(void)
{
    return &SQLiteThread;
}

SQLITE_API int async::sqlite3_open(
    const char* filename,   /* Database filename (UTF-8) */
    sqlite3** ppDb,         /* OUT: SQLite db handle */
    std::chrono::milliseconds timeout
)
{
    // Asynchronously invoke ::sqlite3_open on the SQLiteThread thread
    auto retVal = AsyncInvoke(::sqlite3_open, timeout, filename, ppDb);
    return retVal;
}

SQLITE_API int async::sqlite3_exec(
    sqlite3* db,                /* The database on which the SQL executes */
    const char* zSql,           /* The SQL to be executed */
    sqlite3_callback xCallback, /* Invoke this callback routine */
    void* pArg,                 /* First argument to xCallback() */
    char** pzErrMsg,            /* Write error messages here */
    std::chrono::milliseconds timeout
)
{
    auto retVal = AsyncInvoke(::sqlite3_exec, timeout, db, zSql, xCallback, pArg, pzErrMsg);
    return retVal;
}

SQLITE_API int async::sqlite3_close(
    sqlite3* db,
    std::chrono::milliseconds timeout
)
{
    auto retVal = AsyncInvoke(::sqlite3_close, timeout, db);
    return retVal;
}

// TODO: Add more sqlite async API's as necessary