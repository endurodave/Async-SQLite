// Unit tests for Async-SQLite Wrapper (Group 5: Transaction & Concurrency)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <thread>
#include <atomic>
#include <chrono>

using namespace async;

static const char* TEST_DB_FILE_TRANS = "test_trans.sqlite";

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupDB() {
    std::remove(TEST_DB_FILE_TRANS);
}

static sqlite3* SetupDB() {
    sqlite3* db = nullptr;
    sqlite3_init_async();
    int rc = async::sqlite3_open(TEST_DB_FILE_TRANS, &db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Create a simple table
    async::sqlite3_exec(db, "CREATE TABLE trans_test (id INT);", nullptr, nullptr, nullptr);
    return db;
}

static void TearDownDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
    // Don't delete file immediately if others might be using it (Busy tests)
}

// -----------------------------------------------------------------------------
// Test 1: Commit Transaction
// -----------------------------------------------------------------------------
static void Test_Trans_Commit()
{
    sqlite3* db = SetupDB();

    // 1. Begin
    int rc = async::sqlite3_exec_begin(db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 2. Insert Data
    rc = async::sqlite3_exec(db, "INSERT INTO trans_test VALUES (1);", nullptr, nullptr, nullptr);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 3. Commit
    rc = async::sqlite3_exec_commit(db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 4. Verify Persistence
    int count = 0;
    auto countCb = [](void* p, int, char**, char**) { (*(int*)p)++; return 0; };
    async::sqlite3_exec(db, "SELECT * FROM trans_test WHERE id=1;", countCb, &count, nullptr);

    ASSERT_TRUE(count == 1);

    TearDownDB(db);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 2: Rollback Transaction
// -----------------------------------------------------------------------------
static void Test_Trans_Rollback()
{
    sqlite3* db = SetupDB();

    // 1. Begin
    async::sqlite3_exec_begin(db);

    // 2. Insert Data
    async::sqlite3_exec(db, "INSERT INTO trans_test VALUES (999);", nullptr, nullptr, nullptr);

    // 3. Rollback
    int rc = async::sqlite3_exec_rollback(db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 4. Verify Data is GONE
    int count = 0;
    auto countCb = [](void* p, int, char**, char**) { (*(int*)p)++; return 0; };
    async::sqlite3_exec(db, "SELECT * FROM trans_test WHERE id=999;", countCb, &count, nullptr);

    ASSERT_TRUE(count == 0);

    TearDownDB(db);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 3: Interrupt (Stop a long running query)
// -----------------------------------------------------------------------------
static void Test_Concurrency_Interrupt()
{
    sqlite3* db = SetupDB();

    // Variable to hold the result of the query running on the background thread
    std::atomic<int> queryResult = SQLITE_OK;
    std::atomic<bool> threadStarted = false;

    // 1. Launch a separate thread to run a "Heavy" query.
    // Since Async Wrapper blocks the caller, we need a separate thread to 
    // act as the "User" who gets blocked.
    std::thread blocker([db, &queryResult, &threadStarted]() {
        threadStarted = true;

        // A Recursive CTE that counts to 10 Million. Takes a few seconds.
        const char* heavySql =
            "WITH RECURSIVE cnt(x) AS ("
            " VALUES(1) UNION ALL SELECT x+1 FROM cnt WHERE x<10000000"
            ") SELECT x FROM cnt;";

        // This call will block this thread until finished or interrupted
        // We expect it to be interrupted.
        int rc = async::sqlite3_exec(db, heavySql, nullptr, nullptr, nullptr);
        queryResult = rc;
        });

    // 2. Wait for thread to likely enter the SQLite kernel
    while (!threadStarted) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 3. Interrupt!
    // sqlite3_interrupt is usually thread-safe and direct (not queued), 
    // so it should stop the currently running query on the worker thread.
    async::sqlite3_interrupt(db);

    // 4. Join thread
    if (blocker.joinable()) {
        blocker.join();
    }

    // 5. Verify Result
    // Should be SQLITE_INTERRUPT (9)
    if (queryResult != SQLITE_INTERRUPT) {
        std::cout << "Warning: Query finished before interrupt! Result: " << queryResult << std::endl;
        // If the machine is super fast, it might finish 10M rows in 100ms. 
        // In a real fail case, result would be SQLITE_OK.
    }
    ASSERT_TRUE(queryResult == SQLITE_INTERRUPT);

    TearDownDB(db);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 4: Busy Timeout
// -----------------------------------------------------------------------------
static void Test_Concurrency_BusyTimeout()
{
    // Need 2 connections to the SAME file
    sqlite3* db1 = SetupDB(); // Creates file
    sqlite3* db2 = nullptr;
    async::sqlite3_open(TEST_DB_FILE_TRANS, &db2); // Opens same file

    // 1. Lock DB1 (Exclusive Transaction)
    async::sqlite3_exec(db1, "BEGIN EXCLUSIVE;", nullptr, nullptr, nullptr);

    // 2. Configure DB2 to wait 500ms
    async::sqlite3_busy_timeout(db2, 500);

    // 3. Attempt write on DB2 (Should block 500ms then fail)
    auto start = std::chrono::steady_clock::now();
    int rc = async::sqlite3_exec(db2, "INSERT INTO trans_test VALUES (50);", nullptr, nullptr, nullptr);
    auto end = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 4. Verify
    ASSERT_TRUE(rc == SQLITE_BUSY);
    // Should have waited at least roughly the timeout (allow some jitter)
    ASSERT_TRUE(elapsed >= 450);

    // Cleanup
    async::sqlite3_exec(db1, "COMMIT;", nullptr, nullptr, nullptr);

    TearDownDB(db2);
    TearDownDB(db1);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Transaction_UT()
{
    std::cout << "Running SQLite Transaction & Concurrency Tests..." << std::endl;

    CleanupDB();

    Test_Trans_Commit();
    Test_Trans_Rollback();

    // Interrupt test involves sleeps/threads, might be sensitive on slow CI
    Test_Concurrency_Interrupt();

    Test_Concurrency_BusyTimeout();

    std::cout << "SQLite Transaction & Concurrency Tests Passed!" << std::endl;
}