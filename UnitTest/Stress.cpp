// Unit tests for Async-SQLite Wrapper (Group 9: Thread Safety Stress Test)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <thread>
#include <atomic>
#include <string>

using namespace async;

static const char* STRESS_DB_FILE = "test_stress.sqlite";
static const int THREAD_COUNT = 10;
static const int OPS_PER_THREAD = 100;

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupDB() {
    std::remove(STRESS_DB_FILE);
}

static sqlite3* SetupDB() {
    sqlite3* db = nullptr;
    sqlite3_init_async();
    int rc = async::sqlite3_open(STRESS_DB_FILE, &db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Create table for concurrent inserts
    async::sqlite3_exec(db, "CREATE TABLE stress_test (id INT, thread_id INT);", nullptr, nullptr, nullptr);
    return db;
}

static void TearDownDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 1: Shared Connection Hammer
// Multiple threads sharing ONE sqlite3* handle.
// This proves the wrapper correctly serializes calls to the worker thread.
// -----------------------------------------------------------------------------
static void Test_Stress_SharedConnection()
{
    sqlite3* db = SetupDB();

    std::vector<std::thread> threads;
    std::atomic<int> successCount = 0;
    std::atomic<int> failCount = 0;

    // Launch Threads
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([db, t, &successCount, &failCount]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                // Construct SQL: INSERT INTO stress_test VALUES (i, t);
                std::string sql = "INSERT INTO stress_test VALUES (" +
                    std::to_string(i) + ", " + std::to_string(t) + ");";

                // Call Async API (Blocking wait for this thread, but serialized on worker)
                int rc = async::sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);

                if (rc == SQLITE_OK) {
                    successCount++;
                }
                else {
                    failCount++;
                    // Optional: Print error (might be noisy)
                    // std::cerr << "T" << t << " Fail: " << rc << std::endl;
                }
            }
            });
    }

    // Join Threads
    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    // Validation
    // 1. Check no failures occurred (The wrapper should serialize perfectly)
    if (failCount > 0) {
        std::cerr << "Stress Test Failures: " << failCount << std::endl;
    }
    ASSERT_TRUE(failCount == 0);

    // 2. Check total successful ops
    int expectedTotal = THREAD_COUNT * OPS_PER_THREAD;
    ASSERT_TRUE(successCount == expectedTotal);

    // 3. Verify Database Integrity (Count actual rows)
    int dbRows = 0;
    auto cb = [](void* p, int, char** argv, char**) {
        *(int*)p = std::stoi(argv[0]); return 0;
        };
    async::sqlite3_exec(db, "SELECT count(*) FROM stress_test;", cb, &dbRows, nullptr);

    ASSERT_TRUE(dbRows == expectedTotal);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 2: Separate Connections Hammer (Contention)
// Multiple threads, each with their OWN connection to the SAME file.
// This tests SQLite's internal locking and the wrapper's ability to handle SQLITE_BUSY.
// -----------------------------------------------------------------------------
static void Test_Stress_MultiConnection()
{
    // Initialize file
    {
        sqlite3* setup = SetupDB();
        // Enable WAL mode for better concurrency (optional, but good for stress tests)
        async::sqlite3_exec(setup, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
        TearDownDB(setup);
        // File exists now
    }

    std::vector<std::thread> threads;
    std::atomic<int> busyErrors = 0;

    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([t, &busyErrors]() {
            sqlite3* myDb = nullptr;
            async::sqlite3_open(STRESS_DB_FILE, &myDb);

            // Set a busy timeout so we don't fail immediately on lock contention
            async::sqlite3_busy_timeout(myDb, 1000);

            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                std::string sql = "INSERT INTO stress_test VALUES (" +
                    std::to_string(i) + ", " + std::to_string(t) + ");";

                int rc = async::sqlite3_exec(myDb, sql.c_str(), nullptr, nullptr, nullptr);

                if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
                    busyErrors++;
                }
            }
            async::sqlite3_close(myDb);
            });
    }

    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    // In Multi-Connection tests, SQLITE_BUSY is *expected* and valid.
    // We just want to ensure the application didn't crash or hang.
    std::cout << "Multi-Connection Contention (Busy/Locked) Count: " << busyErrors << std::endl;

    // Cleanup
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Stress_UT()
{
    std::cout << "Running SQLite Thread Safety Stress Tests..." << std::endl;

     // Ensure clean start
    CleanupDB();

    Test_Stress_SharedConnection();
    Test_Stress_MultiConnection();

    std::cout << "SQLite Thread Safety Stress Tests Passed!" << std::endl;
}