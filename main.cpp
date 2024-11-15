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

using namespace std;
using namespace DelegateLib;

// Worker thread instances
WorkerThread workerThreads[] = {
    { "WorkerThread1" },
    { "WorkerThread2" } 
};

static const int WORKER_THREAD_CNT = sizeof(workerThreads) / sizeof(workerThreads[0]);

static std::mutex mtx;                  // Mutex to synchronize access to the condition variable
static std::condition_variable cv;      // Condition variable to block and notify threads
static bool ready = false;              // Shared state to check if the thread should proceed
static std::mutex printMutex;
static sqlite3* db_multithread = nullptr;     

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
int async_simple_example()
{
    sqlite3* db;
    char* errMsg = 0;
    int rc;

    // Step 1: Open (or create) the SQLite database file
    rc = async::sqlite3_open("async_simple_example.db", &db);

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

#if 0
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
#endif
        }

        // Last thread complete?
        if (++cnt >= WORKER_THREAD_CNT)
        {
            std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex to modify shared state
            ready = true;  // Set the shared condition to true, meaning threads are complete

            cv.notify_all();  // Notify waiting threads time to exit
        }
    };

    // Invoke WriteDatabaseLambda lambda function on worker thread
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
    return 0;
}

//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------
int main(void)
{
    // Create all worker threads
    for (int i=0; i<WORKER_THREAD_CNT; i++)
        workerThreads[i].CreateThread();

    // Initialize async sqlite3 interface
    async::sqlite3_init_async();

    // Run simple example. Each database call is invoked on the SQLite internal thread.
    async_simple_example();

    // Run simple example entirely on the internal async SQLite thread. This shows how 
    // to execute multiple SQL commands uninterrupted.
    DelegateLib::DelegateThread* sqlThread = async::sqlite3_get_thread();
    auto delegate = DelegateLib::MakeDelegate(&async_simple_example, *sqlThread, std::chrono::milliseconds::max());
    delegate.AsyncInvoke();

    // Run multithreaded example
    async_mutithread_example();

    // Exit all worker threads
    for (int i = 0; i < WORKER_THREAD_CNT; i++)
        workerThreads[i].ExitThread();

	return 0;
}

