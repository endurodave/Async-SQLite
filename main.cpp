// @see https://github.com/endurodave/Async-SQLite
// David Lafreniere, Nov 2024.
//
// Asynchronous SQLite wrapper using a C++ delegate library. 

// TODO:
// Update this file header
// Update modern delegate article, async wait still invokes even if timeout expires

#include "DelegateLib.h"
#include "WorkerThreadStd.h"
#include <stdio.h>
#include <sqlite3.h>
#include <string>
#include <iostream>
#include "async_sqlite3.h"
#include "async_sqlite3_ut.h"

using namespace std;
using namespace DelegateLib;

// Worker thread instances
WorkerThread workerThreads[] = {
    { "WorkerThread1" },
    { "WorkerThread2" } 
};

WorkerThread nonBlockingAsyncThread("NonBlockingAsyncThread");

static const int WORKER_THREAD_CNT = sizeof(workerThreads) / sizeof(workerThreads[0]);

static std::mutex mtx;                  // Mutex to synchronize access to the condition variable
static std::condition_variable cv;      // Condition variable to block and notify threads
static bool ready = false;              // Shared state to check if the thread should proceed
static std::mutex printMutex;
static sqlite3* db_multithread = nullptr;
static MulticastDelegateSafe<void(int)> completeCallback;
static std::atomic<bool> completeFlag = false;

// Thread safe printf function that locks the mutex
void printf_safe(const char* format, ...) 
{
    std::lock_guard<std::mutex> lock(printMutex);  // Lock the mutex

    // Start variadic arguments processing
    va_list args;
    va_start(args, format);

    vprintf(format, args);  // Call vprintf to handle the format string and arguments

    va_end(args);  // Clean up the variadic arguments
}

// Callback function to display query results (optional).
// When using async SQLite interface, callback is called on the async SQLite 
// internal thread of control.
static int callback(void* NotUsed, int argc, char** argv, char** azColName) 
{
    for (int i = 0; i < argc; i++) 
    {
        printf_safe("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    return 0;
}

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

    // Invoke WriteDatabaseLambda lambda function on worker thread
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

// Run simple async example
void example1()
{
    async_sqlite_simple_example();
}
// Run simple async example entirely on the internal async SQLite thread. This shows how 
// to execute multiple SQL commands uninterrupted.
void example2()
{
    // Get the internal SQLite async interface thread
    DelegateLib::DelegateThread* sqlThread = async::sqlite3_get_thread();

    // Create an asynchronous blocking delegate to invoke async_sqlite_simple_example()
    auto delegate = DelegateLib::MakeDelegate(&async_sqlite_simple_example, *sqlThread, async::MAX_WAIT);

    // Invoke async_sqlite_simple_example() on sqlThread and wait for the retVal
    auto retVal = delegate.AsyncInvoke();

    // If retVal has a value then the asynchronous function call succeeded
    if (retVal.has_value())
    {
        // The async_sqlite_simple_example() return value is stored in retVal.value()
        printf_safe("Return Value: %d\n", retVal.value());
    }
}

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

    // Create a delegate to execute on nonBlockingAsyncThread without waiting for completion (NO_WAIT)
    auto noWaitDelegate = DelegateLib::MakeDelegate(&async_mutithread_example, nonBlockingAsyncThread, async::NO_WAIT);

    // Call async_mutithread_example() on nonBlockingAsyncThread and don't wait for it to complete
    auto retVal = noWaitDelegate.AsyncInvoke();
    if (retVal.has_value())   // have_value() will be false; not waiting for return value
        printf_safe("Return Value: %d\n", retVal.value());

    auto nonBlockingEnd = std::chrono::high_resolution_clock::now();
    auto nonBlockingDuration = std::chrono::duration_cast<std::chrono::microseconds>(nonBlockingEnd - nonBlockingStart);

    return nonBlockingDuration;
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
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

