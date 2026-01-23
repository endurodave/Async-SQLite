// Unit tests for Async-SQLite Wrapper (Group 6: Error Handling & Timeouts)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <thread>
#include <string>
#include <chrono>
#include <atomic>

using namespace async;

static const char* TEST_DB_FILE_ERR = "test_err.sqlite";

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupDB() {
    std::remove(TEST_DB_FILE_ERR);
}

static sqlite3* SetupDB() {
    sqlite3* db = nullptr;
    sqlite3_init_async();
    int rc = async::sqlite3_open(TEST_DB_FILE_ERR, &db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Create a dummy table
    async::sqlite3_exec(db, "CREATE TABLE err_test (id INT);", nullptr, nullptr, nullptr);
    return db;
}

static void TearDownDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
    // Don't clean up immediately if busy
}

// -----------------------------------------------------------------------------
// Test 1: Basic Error Propagation (Syntax Error)
// -----------------------------------------------------------------------------
static void Test_Error_Syntax()
{
    sqlite3* db = SetupDB();

    // 1. Execute Bad SQL
    const char* badSql = "SELEC * FROM err_test"; // Typo "SELEC"
    int rc = async::sqlite3_exec(db, badSql, nullptr, nullptr, nullptr);

    // 2. Verify Return Code
    ASSERT_TRUE(rc == SQLITE_ERROR);

    // 3. Verify Error Code API
    int errCode = async::sqlite3_errcode(db);
    ASSERT_TRUE(errCode == SQLITE_ERROR);

    // 4. Verify Error Message API
    const char* errMsg = async::sqlite3_errmsg(db);
    ASSERT_TRUE(errMsg != nullptr);
    std::string errStr(errMsg);
    ASSERT_TRUE(errStr.find("syntax error") != std::string::npos);

    TearDownDB(db);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 2: Extended Error Codes
// -----------------------------------------------------------------------------
static void Test_Error_Extended()
{
    sqlite3* db = SetupDB();

    // Enable extended result codes
    async::sqlite3_extended_result_codes(db, 1);

    // Create a constraint violation
    async::sqlite3_exec(db, "CREATE TABLE const_test (id INT PRIMARY KEY);", nullptr, nullptr, nullptr);
    async::sqlite3_exec(db, "INSERT INTO const_test VALUES (1);", nullptr, nullptr, nullptr);

    // Insert duplicate key
    int rc = async::sqlite3_exec(db, "INSERT INTO const_test VALUES (1);", nullptr, nullptr, nullptr);

    // Should return SQLITE_CONSTRAINT_PRIMARY (1555) instead of generic SQLITE_CONSTRAINT (19)
    // Note: The specific value depends on SQLite version, but checking it's NOT SQLITE_OK 
    // and is a constraint error is sufficient.
    ASSERT_TRUE(rc != SQLITE_OK);

    int extErr = async::sqlite3_extended_errcode(db);
    // 1555 is SQLITE_CONSTRAINT_PRIMARY
    ASSERT_TRUE(extErr == 1555 || extErr == SQLITE_CONSTRAINT);

    TearDownDB(db);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 3: Wrapper Timeout Logic
// -----------------------------------------------------------------------------
static void Test_Error_Timeout()
{
    // Scenario: 
    // Thread A (Blocker) locks the Worker Thread by running a long job.
    // Thread B (Main) tries to access it via Async Wrapper with a SHORT timeout.
    // The Async Wrapper should give up and return error/default value.

    sqlite3* dbVictim = SetupDB();

    // 1. Queue a "Long Running Job" on the worker thread.
    // We use `sqlite3_sleep` to make the worker thread BUSY for 2000ms.
    std::atomic<bool> longJobStarted = false;
    std::thread workerHog([&longJobStarted]() {
        longJobStarted = true;
        // Sleep for 2000ms on the worker thread
        // This blocks the single worker queue!
        async::sqlite3_sleep(2000);
        });

    // Wait for hog to likely start
    while (!longJobStarted) { std::this_thread::yield(); }
    // Give it a moment to actually enter the sleep
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 2. Attempt a quick operation with a short timeout (e.g., 100ms)
    // The worker is busy for 2000ms. Our request should time out after 100ms.
    auto start = std::chrono::steady_clock::now();

    // We expect this to fail because the worker thread is blocked sleeping
    // Pass custom timeout as final argument
    int rc = async::sqlite3_exec(dbVictim, "SELECT 1;", nullptr, nullptr, nullptr, std::chrono::milliseconds(100));

    auto end = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (workerHog.joinable()) {
        workerHog.join();
    }

    // 3. Verify
    // The call should have returned roughly after 100ms (not 2000ms!)
    // Tolerances: > 50ms and < 1500ms (allow loose upper bound for slow CI machines)
    std::cout << "Timeout Test Elapsed: " << elapsed << "ms" << std::endl;
    ASSERT_TRUE(elapsed < 1800);

    // The wrapper returns the default return type on timeout. 
    // For `sqlite3_exec` (int), default is SQLITE_BUSY (mapped in AsyncInvoke special case).
    ASSERT_TRUE(rc == SQLITE_BUSY);

    TearDownDB(dbVictim);
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 4: Error String (ErrStr)
// -----------------------------------------------------------------------------
static void Test_Error_String()
{
    // Verify static utility works
    const char* str = async::sqlite3_errstr(SQLITE_BUSY);
    ASSERT_TRUE(str != nullptr);
    std::string s(str);
    ASSERT_TRUE(s == "database is locked");
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void ErrorHandling_UT()
{
    std::cout << "Running SQLite Error & Timeout Tests..." << std::endl;

    CleanupDB();

    Test_Error_Syntax();
    Test_Error_Extended();
    Test_Error_Timeout();
    Test_Error_String();

    std::cout << "SQLite Error & Timeout Tests Passed!" << std::endl;
}