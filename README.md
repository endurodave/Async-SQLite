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
  - [Future Example](#future-example)
  - [Test Results](#test-results)

# Overview

SQLite is a lightweight, serverless, self-contained SQL database engine commonly used for embedded applications and local data storage. The C++ DelegateMQ library is a multi-threaded framework capable of anonymously targeting any callable function, either synchronously or asynchronously.

This project leverages the delegate library to create a robust, thread-safe asynchronous wrapper for SQLite. All database operations are marshaled to a dedicated private worker thread, ensuring strict serialization of database access while freeing the client thread from blocking operations.

**Key Features**

1. **Thread Safety:** A single background thread manages all SQLite interactions, preventing race conditions without complex client-side locking.

2. **Synchronous API (Blocking):** Wrappers that mimic the standard SQLite API but execute on the worker thread. The calling thread blocks until completion or a specified timeout occurs.

    * *Usage:* Ideal for linear logic where the result is needed immediately.
    * *Signature:* `sqlite3_exec(..., dmq::Duration timeout)`

3. **Future API (Non-Blocking):** "Fire-and-Forget" functions that return a `std::future<int>` immediately. The main thread can continue processing UI updates or other tasks while the heavy SQL operation runs in the background.

   * *Usage:* Ideal for high-performance applications, UI responsiveness, and concurrent task processing.
   * *Signature:* `std::future<int> sqlite3_exec_future(...)`

## References

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

* **Thread Safety by Design:** SQLite handles are accessed exclusively by a single private worker thread. This serialization eliminates race conditions and removes the need for complex mutex locking code in your main application.

* **Responsive UI (Non-Blocking):** Using the Future API, expensive database operations (like bulk inserts or complex joins) are offloaded to the background. The main thread receives a `std::future` immediately, allowing it to keep the UI smooth and responsive while waiting for the result.

* **Flexible Control Flow:** The wrapper provides two distinct ways to interact with the database:

  * **Synchronous:** Block and wait for a result (simple, linear logic for standard tasks).

  * **Asynchronous:** "Fire-and-forget" using futures (high-concurrency for heavy tasks).

* **Sequential Atomicity:** Requests sent to the worker thread are executed in the exact order they are received. This allows you to safely dispatch a chain of dependent commands (e.g., `BEGIN`, multiple `INSERT`s, `COMMIT`) knowing they will run uninterrupted.

# Asynchronous SQLite

The file `async_sqlite3.h` defines the asynchronous interface. Each async function matches the SQLite library except the addition of a `timeout` argument.

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
        dmq::Duration timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_exec(
        sqlite3* db,                /* The database on which the SQL executes */
        const char* zSql,           /* The SQL to be executed */
        sqlite3_callback xCallback, /* Invoke this callback routine */
        void* pArg,                 /* First argument to xCallback() */
        char** pzErrMsg,            /* Write error messages here */
        dmq::Duration timeout = MAX_WAIT
    );

    SQLITE_API int sqlite3_close(
        sqlite3* db,
        dmq::Duration timeout = MAX_WAIT
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
    dmq::Duration timeout
)
{
    // Asynchronously invoke ::sqlite3_open on the SQLiteThread thread
    return AsyncInvoke(::sqlite3_open, timeout, filename, ppDb);
}

SQLITE_API int async::sqlite3_exec(
    sqlite3* db,                /* The database on which the SQL executes */
    const char* zSql,           /* The SQL to be executed */
    sqlite3_callback xCallback, /* Invoke this callback routine */
    void* pArg,                 /* First argument to xCallback() */
    char** pzErrMsg,            /* Write error messages here */
    dmq::Duration timeout
)
{
    return AsyncInvoke(::sqlite3_exec, timeout, db, zSql, xCallback, pArg, pzErrMsg);
}

SQLITE_API int async::sqlite3_close(
    sqlite3* db,
    dmq::Duration timeout
)
{
    return AsyncInvoke(::sqlite3_close, timeout, db);
}

// etc...
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
    example_future();

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

## Future Example

This example illustrates how to utilize `std::future` for concurrent database operations. By leveraging the asynchronous API, you can implement a "Fire-and-Forget" pattern where the main thread remains fully responsive—performing UI updates or other processing—while heavy SQL operations execute in the background.

```cpp
void example_future()
{
    printf_safe("\n--- Starting Example 5 (Future/Async) ---\n");

    sqlite3* db = nullptr;
    // Open DB synchronously to ensure valid handle
    async::sqlite3_open("async_future_example.db", &db);

    // Create a table synchronously (we need it before inserting)
    async::sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS heavy_data (id INT, val TEXT);", nullptr, nullptr, nullptr);

    // 1. Launch a heavy insert operation asynchronously
    // This returns immediately, giving us a std::future
    printf_safe("[Main] Launching heavy async insert...\n");

    // Note: The SQL string must remain valid until the future completes. 
    // Ideally use string literals or manage lifetime carefully.
    std::string sql = "INSERT INTO heavy_data VALUES (123, 'Concurrent Data');";

    // Pass 5 arguments matching the raw API signature
    auto future = async::sqlite3_exec_future(db, sql.c_str(), nullptr, nullptr, nullptr);

    // 2. Perform other work on the main thread while DB is busy
    // In a real app, this would be UI updates or input processing.
    printf_safe("[Main] DB is busy. Performing other tasks on main thread...\n");
    for (int i = 0; i < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        printf_safe("[Main] Working... %d%%\n", (i + 1) * 33);
    }

    // 3. Wait for the database operation to complete and check the result
    printf_safe("[Main] Waiting for DB to finish...\n");

    // .get() blocks here ONLY if the DB task is still running.
    int rc = future.get();

    if (rc == SQLITE_OK) {
        printf_safe("[Main] Async insert completed successfully!\n");
    }
    else {
        printf_safe("[Main] Async insert failed with code: %d\n", rc);
    }

    async::sqlite3_close(db);
    std::remove("async_future_example.db");
}
```

## Test Results

Use *DB Browser for SQLite* tool to examine the results. Notice the interleaving of data by both worker threads.

![Database Output](./Figure1.jpg)