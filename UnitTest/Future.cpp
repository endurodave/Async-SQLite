// Unit tests for Async-SQLite Wrapper (Group 10: Future/Async API)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <string>
#include <vector>
#include <future>
#include <thread>
#include <chrono>

using namespace async;

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------
static sqlite3* OpenDB(const char* filename) {
    sqlite3* db = nullptr;

    // Ensure the worker thread is created and running before we begin
    sqlite3_init_async();

    int rc = async::sqlite3_open(filename, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "[Setup] Failed to open DB: " << rc << std::endl;
    }
    return db;
}

static void CloseDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
}

// -----------------------------------------------------------------------------
// Test 1: Exec Future (Fire and Forget)
// -----------------------------------------------------------------------------
static void Test_Future_Exec()
{
    std::cout << "[Exec] Opening DB..." << std::endl;
    sqlite3* db = OpenDB(":memory:");

    // --- DEBUG CHECK ---
    // If ID is 0, the thread isn't running and promises will be broken.
    Thread* pThread = async::sqlite3_get_thread();
    if (pThread) {
        std::cout << "[Exec] Worker Thread ID: " << pThread->GetThreadId() << std::endl;
    }
    // -------------------

    // 1. Create Table (Async)
    // NOTE: Passing a string literal is safe because static memory lives forever.
    std::cout << "[Exec] Creating table async..." << std::endl;
    auto f1 = async::sqlite3_exec_future(db, "CREATE TABLE test (id INT, val TEXT);", nullptr, nullptr, nullptr);

    try {
        int rc = f1.get(); // Wait for result
        if (rc != SQLITE_OK) std::cerr << "[Exec] Create Table Failed: " << rc << std::endl;
        ASSERT_TRUE(rc == SQLITE_OK);
    }
    catch (const std::future_error& e) {
        std::cerr << "[Exec] FATAL: Future Error: " << e.what() << " Code: " << e.code() << std::endl;
        throw;
    }

    // 2. Insert Data
    std::cout << "[Exec] Inserting data async..." << std::endl;

    // LIFETIME WARNING: The 'sql' variable must remain valid until the async operation completes!
    // Do not let this variable go out of scope before calling f2.get().
    std::string sql = "INSERT INTO test VALUES (1, 'Async Data');";

    auto f2 = async::sqlite3_exec_future(db, sql.c_str(), nullptr, nullptr, nullptr);

    try {
        int rc = f2.get(); // Blocks here until done, ensuring 'sql' is still alive
        ASSERT_TRUE(rc == SQLITE_OK);
    }
    catch (const std::exception& e) {
        std::cerr << "[Exec] FATAL: Insert Failed: " << e.what() << std::endl;
        throw;
    }

    // 3. Verify Data (Synchronous Check)
    int rowCount = 0;
    auto cb = [](void* p, int, char**, char**) { (*(int*)p)++; return 0; };
    async::sqlite3_exec(db, "SELECT * FROM test;", cb, &rowCount, nullptr);
    ASSERT_TRUE(rowCount == 1);

    CloseDB(db);
}

// -----------------------------------------------------------------------------
// Test 2: Step Future (Async Querying)
// -----------------------------------------------------------------------------
static void Test_Future_Step()
{
    std::cout << "[Step] Starting Step Test..." << std::endl;
    sqlite3* db = OpenDB(":memory:");

    // Setup data synchronously
    async::sqlite3_exec(db, "CREATE TABLE step_test (id INT);", nullptr, nullptr, nullptr);
    async::sqlite3_exec(db, "INSERT INTO step_test VALUES (100);", nullptr, nullptr, nullptr);
    async::sqlite3_exec(db, "INSERT INTO step_test VALUES (200);", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt = nullptr;
    async::sqlite3_prepare_v2(db, "SELECT id FROM step_test ORDER BY id;", -1, &stmt, nullptr);
    ASSERT_TRUE(stmt != nullptr);

    // 1. Step Row 1 Async
    std::future<int> fStep1 = async::sqlite3_step_future(stmt);

    // Simulate doing other work on the main thread while DB fetches
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    int rc = fStep1.get();
    ASSERT_TRUE(rc == SQLITE_ROW);

    // Verify Value (Synchronous column access is fine after step completes)
    int val1 = async::sqlite3_column_int(stmt, 0);
    ASSERT_TRUE(val1 == 100);

    // 2. Step Row 2 Async
    std::future<int> fStep2 = async::sqlite3_step_future(stmt);
    rc = fStep2.get();
    ASSERT_TRUE(rc == SQLITE_ROW);

    int val2 = async::sqlite3_column_int(stmt, 0);
    ASSERT_TRUE(val2 == 200);

    // 3. Step Done Async
    std::future<int> fStep3 = async::sqlite3_step_future(stmt);
    rc = fStep3.get();
    ASSERT_TRUE(rc == SQLITE_DONE);

    // 4. Finalize Async
    auto fFin = async::sqlite3_finalize_future(stmt);
    rc = fFin.get();
    ASSERT_TRUE(rc == SQLITE_OK);

    CloseDB(db);
}

// -----------------------------------------------------------------------------
// Test 3: Wait_For and Timeout Logic
// -----------------------------------------------------------------------------
static void Test_Future_Timeout_Control()
{
    std::cout << "[Timeout] Starting Timeout Test..." << std::endl;
    sqlite3* db = OpenDB(":memory:");

    // Create a slow query using recursive common table expression (100k rows)
    std::string slowSql =
        "WITH RECURSIVE cnt(x) AS (VALUES(1) UNION ALL SELECT x+1 FROM cnt WHERE x<100000) "
        "SELECT sum(x) FROM cnt;";

    // Start async
    auto future = async::sqlite3_exec_future(db, slowSql.c_str(), nullptr, nullptr, nullptr);

    // Wait for 1 microsecond (guaranteed timeout on most systems)
    auto status = future.wait_for(std::chrono::microseconds(1));

    if (status == std::future_status::ready) {
        std::cout << "[Timeout] Info: Database was too fast to test timeout logic!" << std::endl;
    }
    else if (status == std::future_status::timeout) {
        // This is the expected path for a robust test
    }

    // Must wait for completion before 'slowSql' is destroyed!
    int rc = future.get();
    ASSERT_TRUE(rc == SQLITE_OK);

    CloseDB(db);
}

// -----------------------------------------------------------------------------
// Test 4: Transaction Commit Future
// -----------------------------------------------------------------------------
static void Test_Future_Transaction()
{
    std::cout << "[Trans] Starting Transaction Test..." << std::endl;
    sqlite3* db = OpenDB(":memory:");
    async::sqlite3_exec(db, "CREATE TABLE trans (id INT);", nullptr, nullptr, nullptr);

    // Start Transaction (Blocking/Sync is typically fine for BEGIN)
    async::sqlite3_exec(db, "BEGIN;", nullptr, nullptr, nullptr);
    async::sqlite3_exec(db, "INSERT INTO trans VALUES (999);", nullptr, nullptr, nullptr);

    // Commit Async (Disk I/O happens here)
    auto fCommit = async::sqlite3_exec_commit_future(db);

    // Wait
    int rc = fCommit.get();
    ASSERT_TRUE(rc == SQLITE_OK);

    // Verify
    int count = 0;
    async::sqlite3_exec(db, "SELECT count(*) FROM trans;",
        [](void* p, int, char** argv, char**) { *(int*)p = std::stoi(argv[0]); return 0; },
        &count, nullptr);
    ASSERT_TRUE(count == 1);

    CloseDB(db);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Future_UT()
{
    std::cout << "Running SQLite Future/Async API Tests..." << std::endl;

    Test_Future_Exec();
    Test_Future_Step();
    Test_Future_Timeout_Control();
    Test_Future_Transaction();

    std::cout << "SQLite Future/Async API Tests Passed!" << std::endl;
}