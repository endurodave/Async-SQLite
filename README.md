![License MIT](https://img.shields.io/github/license/BehaviorTree/BehaviorTree.CPP?color=blue)
[![conan Ubuntu](https://github.com/endurodave/Async-SQLite/actions/workflows/cmake_ubuntu.yml/badge.svg)](https://github.com/endurodave/Async-SQLite/actions/workflows/cmake_ubuntu.yml)
[![conan Ubuntu](https://github.com/endurodave/Async-SQLite/actions/workflows/cmake_clang.yml/badge.svg)](https://github.com/endurodave/Async-SQLite/actions/workflows/cmake_clang.yml)
[![conan Windows](https://github.com/endurodave/Async-SQLite/actions/workflows/cmake_windows.yml/badge.svg)](https://github.com/endurodave/Async-SQLite/actions/workflows/cmake_windows.yml)

# Asynchronous SQLite API using C++ Delegates

An asynchronous SQLite API wrapper implemented using a C++ delegate libraries. All target platforms are supported including Windows, Linux, and embedded systems.

# Table of Contents

- [Asynchronous SQLite API using C++ Delegates](#asynchronous-sqlite-api-using-c-delegates)
- [Table of Contents](#table-of-contents)
- [Overview](#overview)
- [References](#references)
- [Getting Started](#getting-started)
- [Delegate Quick Start](#delegate-quick-start)
- [Why Asynchronous SQLite](#why-asynchronous-sqlite)
- [Asynchronous SQLite](#asynchronous-sqlite)
- [Examples](#examples)
  - [Simple Example](#simple-example)
  - [Simple Example Block Execute](#simple-example-block-execute)
  - [Multithread Example](#multithread-example)
    - [Synchronous Blocking Execution](#synchronous-blocking-execution)
    - [Asynchronous Non-Blocking Execution](#asynchronous-non-blocking-execution)
    - [Test Results](#test-results)

# Overview

SQLite is a lightweight, serverless, self-contained SQL database engine commonly used for embedded applications and local data storage. The C++ delegate library is a multi-threaded framework capable of anonymously targeting any callable function, either synchronously or asynchronously. This delegate library is used to create an asynchronous API for SQLite. SQLite operates in a private thread of control, with all client API calls being invoked asynchronously on this private thread. The SQLite asynchronous wrapper makes no changes to the SQLite API, except for the addition of a trailing timeout argument for wait duration.

The purpose of the wrapper is twofold: First, to provide a simple asynchronous layer over SQLite, and second, to serve as a working example of an asynchronous delegate library or subsystem interface.

# References

* <a href="https://github.com/endurodave/DelegateMQ">DelegatesMQ</a> - Invoke any C++ callable function synchronously, asynchronously, or on a remote endpoint.
* <a href="https://www.sqlite.org/">SQLite</a> - SQLite is a C-language library that implements a small, fast, self-contained, high-reliability, full-featured, SQL database engine.

# Getting Started
[CMake](https://cmake.org/) is used to create the project build files on any Windows or Linux machine.

1. Clone the repository.
2. From the repository root, run the following CMake command:   
   `cmake -B Build .`
3. Build and run the project within the `Build` directory. 

# Delegate Quick Start

The DelegateMQ contains delegates and delegate containers. The example below creates a delegate with the target function `MyTestFunc()`. The first example is a synchronously delegate function call, and the second example asynchronously. Notice the only difference is adding a thread instance `myThread` argument. See [DelegateMQ](https://github.com/endurodave/DelegateMQ) repository for more details.

```cpp
#include "DelegateMQ.h"

using namespace DelegateMQ;

void MyTestFunc(int val)
{
    printf("%d", val);
}

int main(void)
{
    // Create a synchronous delegate
    auto syncDelegate = MakeDelegate(&MyTestFunc);

    // Invoke MyTestFunc() synchronously
    syncDelegate(123);

    // Create an asynchronous non-blocking delegate
    auto asyncDelegate = MakeDelegate(&MyTestFunc, myThread);

    // Invoke MyTestFunc() asynchronously (non-blocking)
    asyncDelegate(123);

    // Create an asynchronous blocking delegate
    auto asyncDelegateWait = MakeDelegate(&MyTestFunc, myThread, WAIT_INFINITE);

    // Invoke MyTestFunc() asynchronously (blocking)
    asyncDelegateWait(123);
}
```

# Why Asynchronous SQLite

* **Improved Performance:** Offloading SQLite operations to a separate thread allows the main thread to remain responsive, enhancing overall application performance.
* **Non-blocking Operations:** By executing database queries in a separate thread, the main application thread can continue processing other tasks without waiting for SQLite operations to complete. A callback can be used to signal completion.
* **Atomic Operations:** Execute a series of database operations that cannot be interrupted without locks.
* **Isolation:** Running SQLite on a private thread ensures that database-related tasks are isolated from the main application logic, reducing the risk of thread contention or deadlock in the main application.

A side benefit is a real-world example of a multi-threaded application using the delegate library. 

# Asynchronous SQLite

The file `async_sqlite3.h` implement the asynchronous interface. Each async function matches the SQLite library except the addition of a `timeout` argument.

```cpp
namespace async
{
    #undef max  // Prevent compiler error on next line if max is defined
    constexpr auto MAX_WAIT = std::chrono::milliseconds::max();

    // Call one-time at application startup
    void sqlite3_init_async(void);

    // Get a pointer to the internal thread
    Thread* sqlite3_get_thread(void);

    SQLITE_API int sqlite3_open(
        const char* filename,   /* Database filename (UTF-8) */
        sqlite3** ppDb,         /* OUT: SQLite db handle */
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_exec(
        sqlite3* db,                /* The database on which the SQL executes */
        const char* zSql,           /* The SQL to be executed */
        sqlite3_callback xCallback, /* Invoke this callback routine */
        void* pArg,                 /* First argument to xCallback() */
        char** pzErrMsg,            /* Write error messages here */
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_close(
        sqlite3* db,
        std::chrono::milliseconds timeout = MAX_WAIT
    );

    // etc...
}
```

The file `async_sqlite3.cpp` implements each function. 

```cpp
void async::sqlite3_init_async(void)
{
    // Create the worker thread
    SQLiteThread.CreateThread();
}

Thread* async::sqlite3_get_thread(void)
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

// etc...
```

The `AsyncInvoke()` helper function invokes the function asynchronously if the caller is not executing on the `SQLiteThread` thread. Otherwise, if the caller is already on the internal thread the target function is called synchronously.

```cpp
// A private worker thread instance to execute all SQLite API functions
static Thread SQLiteThread("SQLite Thread");

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
    if (SQLiteThread.GetThreadId() != Thread::GetCurrentThreadId())
    {
        // Create a delegate that points to func and is invoked on SQLiteThread
        auto delegate = MakeDelegate(func, SQLiteThread, timeout);

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
```

# Examples

The `main()` function executes example code.

```cpp
int main(void)
{
    std::remove("async_mutithread_example.db");
    std::remove("async_sqlite_simple_example.db");

    // Create all worker threads
    nonBlockingAsyncThread.CreateThread();
    for (int i=0; i<WORKER_THREAD_CNT; i++)
        workerThreads[i].CreateThread();

    // Initialize async sqlite3 interface
    async::sqlite3_init_async();

    // Optionally run unit tests
    RunUnitTests();

    // Run all examples
    example1();
    example2();
    auto blockingDuration = example3();
    auto nonBlockingDuration = example4();

    // Wait for example4() to complete on nonBlockingAsyncThread
    while (!completeFlag)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Exit all worker threads
    nonBlockingAsyncThread.ExitThread();
    for (int i = 0; i < WORKER_THREAD_CNT; i++)
        workerThreads[i].ExitThread();

    // Compare blocking and non-blocking execution times
    std::cout << "Blocking Time: " << blockingDuration.count() << " microseconds." << std::endl;
    std::cout << "Non-Blocking Time: " << nonBlockingDuration.count() << " microseconds." << std::endl;

    return 0;
}
```

## Simple Example

The `async_sqlite_simple_example()` writes to the database first and last name using the `async` API. Each function call matches the underlying SQLite library exactly. A call to `async::sqlite3_open()`, for instance, sends a function pointer and all arguments to the `SQLiteThread`, the target function is invoked on the internal thread, and the return value `rc` is returned to the caller. In short, every `async` API call injects a message into a queue for later execution on  `SQLiteThread`.

```cpp
// Simple example to create and write to the database asynchronously. 
// Use async::sqlite3_<func> series of functions within the async namespace.
int async_sqlite_simple_example()
{
    sqlite3* db;
    char* errMsg = 0;
    int rc;

    // Step 1: Open (or create) the SQLite database file
    rc = async::sqlite3_open("async_sqlite_simple_example.db", &db);

    if (rc) 
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }
    else 
    {
        printf_safe("Opened database successfully\n");
    }

    // Step 2: Create a table if it does not exist
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS people ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "first_name TEXT NOT NULL, "
        "last_name TEXT NOT NULL);";

    rc = async::sqlite3_exec(db, createTableSQL, callback, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return 1;
    }
    else 
    {
        printf_safe("Table created successfully or already exists\n");
    }

    // Step 3: Insert a record
    const char* insertSQL = "INSERT INTO people (first_name, last_name) "
        "VALUES ('John', 'Doe');";

    rc = async::sqlite3_exec(db, insertSQL, callback, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    else 
    {
        printf_safe("Record inserted successfully\n");
    }

    // Step 4: Verify the insertion by querying the table
    const char* selectSQL = "SELECT * FROM people;";

    rc = async::sqlite3_exec(db, selectSQL, callback, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
    else 
    {
        printf_safe("Query executed successfully\n");
    }

    // Step 5: Close the database connection
    async::sqlite3_close(db);
    return 0;
}
```

## Simple Example Block Execute

The previous example generated one queue message per `async` API call. Alternatively, the entire `async_sqlite_simple_example()` function can be executed on the `SQLiteThread`. An asynchronous delegate is created to invoke `async_sqlite_simple_example()` on `sqlThread`. Since all `async::sqlite3_<func>` calls are already executed on `sqlThread`, the helper function `AsyncInvoke()` will not generate a queue message when calling each database API. In this way, all the database interactions within `async_sqlite_simple_example()` are atomic and cannot be interrupted by other operations on the database.

```cpp
// Get the internal SQLite async interface thread
Thread* sqlThread = async::sqlite3_get_thread();

// Create an asynchronous blocking delegate to invoke async_sqlite_simple_example()
auto delegate = dmq::MakeDelegate(&async_sqlite_simple_example, *sqlThread, async::MAX_WAIT);

// Invoke async_sqlite_simple_example() on sqlThread and wait for the retVal
auto retVal = delegate.AsyncInvoke();

// If retVal has a value then the asynchronous function call succeeded
if (retVal.has_value())
{
    // The async_sqlite_simple_example() return value is stored in retVal.value()
    printf_safe("Return Value: %d\n", retVal.value());
}
```

## Multithread Example

The `async_mutithread_example()` uses two separate threads to insert into the database concurrently. The lambda function `WriteDatabaseLambda` is executed by two threads. When both worker threads are complete, the `async_multithread_example()` function returns.

```cpp
// Async SQLite multithread example. The WriteDatabaseLambda() function is called 
// from multiple threads of control. Function returns after all threads are complete.
int async_mutithread_example()
{
    char* errMsg = 0;
    int rc;

    // Step 1: Open (or create) the SQLite database file
    rc = async::sqlite3_open("async_mutithread_example.db", &db_multithread);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db_multithread));
        return(0);
    }
    else
    {
        printf_safe("Opened database successfully\n");
    }

    // Step 2: Create a table if it does not exist
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS threads ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "thread_name TEXT NOT NULL, "
        "cnt TEXT NOT NULL);";

    rc = async::sqlite3_exec(db_multithread, createTableSQL, callback, 0, &errMsg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return 1;
    }
    else
    {
        printf_safe("Table created successfully or already exists\n");
    }

    // Lambda function to write data to SQLite database
    auto WriteDatabaseLambda = +[](std::string thread_name) -> void
    {
        char* errMsg = 0;
        int rc;
        static int cnt = 0;

        for (int i = 0; i < 100; i++)
        {
            // Step 3: Insert a record
            std::string insertSQL = "INSERT INTO threads (thread_name, cnt) "
                "VALUES ('" + thread_name + "', '" + std::to_string(i) + "');";

            rc = async::sqlite3_exec(db_multithread, insertSQL.c_str(), callback, 0, &errMsg);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "SQL error: %s\n", errMsg);
                sqlite3_free(errMsg);
            }
            else
            {
                printf_safe("Record inserted successfully\n");
            }
        }

        // Step 4: Verify the insertion by querying the table
        const char* selectSQL = "SELECT * FROM threads;";

        rc = async::sqlite3_exec(db_multithread, selectSQL, callback, 0, &errMsg);
        if (rc != SQLITE_OK)
        {
            fprintf(stderr, "SQL error: %s\n", errMsg);
            sqlite3_free(errMsg);
        }
        else
        {
            printf_safe("Query executed successfully\n");
        }

        // Last thread complete?
        if (++cnt >= WORKER_THREAD_CNT)
        {
            std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to modify shared state
            ready = true;  // Set the shared condition to true, meaning threads are complete

            cv.notify_all();  // Notify waiting threads time to exit
        }
    };  // End Lambda

    // Invoke WriteDatabaseLambda lambda function on worker threads
    ready = false;
    for (int i = 0; i < WORKER_THREAD_CNT; i++)
    {
        // Create an async delegate to invoke WriteDatabaseLambda()
        auto delegate = MakeDelegate(WriteDatabaseLambda, workerThreads[i]);
        
        // Invoke async target function WriteDatabaseLambda() on workerThread[i] non-blocking
        // i.e. invoke the target function and don't wait for the return.
        delegate(workerThreads[i].GetThreadName());
    }

    // Lock the mutex and wait for the signal from WriteDatabaseLambda
    std::unique_lock<std::mutex> lock(mtx);

    // Wait for all WriteDatabaseLambda worker threads to complete
    while (!ready)
        cv.wait(lock);  // Block the thread until notified

    // Step 5: Close the database connection
    async::sqlite3_close(db_multithread);

    // Invoke delegate callback indicating function is compelete
    completeCallback(0);
    return 0;
}
```

The `async_mutithread_example()` is run synchronously and asynchronously to illustrate the execution time differences from the calling thread's perspective. When using non-blocking, the calling thread is able to proceed without waiting for  lengthy database operations.

```
Blocking Time: 459135 microseconds.
Non-Blocking Time: 17 microseconds.
```

### Synchronous Blocking Execution

The blocking `example3()` calls `async_mutithread_example()` directly. The SQLite database calls within `async_mutithread_example()` are asynchronous, however the caller must wait for all database operations to complete.

```cpp
// Run multithreaded example (blocking) and return the execution time
std::chrono::microseconds example3()
{
    auto blockingStart = std::chrono::high_resolution_clock::now();

    // Call example and wait for completion
    int retVal = async_mutithread_example();

    auto blockingEnd = std::chrono::high_resolution_clock::now();
    auto blockingDuration = std::chrono::duration_cast<std::chrono::microseconds>(blockingEnd - blockingStart);

    return blockingDuration;
}
```

### Asynchronous Non-Blocking Execution 

The non-blocking `example4()` invokes `async_multithread_example()` asynchronously on `nonBlockingAsyncThread` without waiting for the call to complete. A callback is used to signal completion should the caller need notification. The `callbackComplete` delegate invokes the `CompleteCallbackLambda` lambda callback just before `async_multithread_example()`  exits.

```cpp
// Run the multithreaded example (non-blocking) on nonBlockingAsyncThread thread and 
// return the execution time.
std::chrono::microseconds example4()
{
    completeFlag = false;

    // Lambda function to receive the complete callback
    auto CompleteCallbackLambda = +[](int retVal) -> void
    {
        completeFlag = true;
    };

    // Register with delegate to receive callback
    completeCallback += MakeDelegate(CompleteCallbackLambda);

    auto nonBlockingStart = std::chrono::high_resolution_clock::now();

    // Create a delegate to execute on nonBlockingAsyncThread without waiting for completion
    auto noWaitDelegate = dmq::MakeDelegate(&async_mutithread_example, nonBlockingAsyncThread);

    // Call async_mutithread_example() on nonBlockingAsyncThread and don't wait for it to complete
    noWaitDelegate.AsyncInvoke();

    auto nonBlockingEnd = std::chrono::high_resolution_clock::now();
    auto nonBlockingDuration = std::chrono::duration_cast<std::chrono::microseconds>(nonBlockingEnd - nonBlockingStart);

    return nonBlockingDuration;
}
```

### Test Results

Use *DB Browser for SQLite* tool to examine the results. Notice the interleaving of data by both worker threads.

![Database Output](./Figure1.jpg)