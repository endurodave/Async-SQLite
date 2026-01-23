// Unit tests for Async-SQLite Wrapper (Group 1: Init & Connection)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio> // for remove()

using namespace async;

static const char* TEST_DB_FILE = "test_db.sqlite";

// Helper to clean up any leftover DB file
static void CleanupDB() {
    std::remove(TEST_DB_FILE);
}

// -----------------------------------------------------------------------------
// Test 1: Initialization and Thread Verification
// -----------------------------------------------------------------------------
static void Test_InitAndThread()
{
    // Initialize the async system
    sqlite3_init_async();

    // Verify thread is created
    Thread* thread = sqlite3_get_thread();
    ASSERT_TRUE(thread != nullptr);

    // Verify thread has a name (optional, but good sanity check)
    ASSERT_TRUE(thread->GetThreadName() == "SQLite Thread");
}

// -----------------------------------------------------------------------------
// Test 2: Open and Close (File Database)
// -----------------------------------------------------------------------------
static void Test_OpenClose_File()
{
    sqlite3* db = nullptr;
    int rc = SQLITE_ERROR;

    // 1. Open a new database file
    rc = async::sqlite3_open(TEST_DB_FILE, &db);
    ASSERT_TRUE(rc == SQLITE_OK);
    ASSERT_TRUE(db != nullptr);

    // 2. Verify connection works (sanity check via synchronous API or simple async check)
    // We can use a simple async exec or just check the handle is valid.
    // For this test group, just opening successfully is enough.

    // 3. Close the database
    rc = async::sqlite3_close(db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Clean up file
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 3: Open and Close (In-Memory Database)
// -----------------------------------------------------------------------------
static void Test_OpenClose_Memory()
{
    sqlite3* db = nullptr;
    int rc = SQLITE_ERROR;

    // 1. Open in-memory DB
    rc = async::sqlite3_open(":memory:", &db);
    ASSERT_TRUE(rc == SQLITE_OK);
    ASSERT_TRUE(db != nullptr);

    // 2. Close
    rc = async::sqlite3_close(db);
    ASSERT_TRUE(rc == SQLITE_OK);
}

// -----------------------------------------------------------------------------
// Test 4: Open v2 (Flags)
// -----------------------------------------------------------------------------
static void Test_OpenV2()
{
    sqlite3* db = nullptr;
    int rc = SQLITE_ERROR;

    // 1. Open with Read/Write + Create flags
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    rc = async::sqlite3_open_v2(TEST_DB_FILE, &db, flags, nullptr);
    ASSERT_TRUE(rc == SQLITE_OK);
    ASSERT_TRUE(db != nullptr);

    // 2. Close v2
    rc = async::sqlite3_close_v2(db);
    ASSERT_TRUE(rc == SQLITE_OK);

    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 5: Open Fail Case (Read-Only Non-Existent File)
// -----------------------------------------------------------------------------
static void Test_OpenFail()
{
    sqlite3* db = nullptr;
    int rc = SQLITE_ERROR;

    // Ensure file doesn't exist
    CleanupDB();

    // 1. Try to open READONLY a file that doesn't exist
    int flags = SQLITE_OPEN_READONLY; // No CREATE flag
    rc = async::sqlite3_open_v2(TEST_DB_FILE, &db, flags, nullptr);

    // Should fail (CantOpen)
    ASSERT_TRUE(rc != SQLITE_OK);
    // db handle might be allocated even on failure (SQLite quirk), but usually null if totally failed
    if (db) {
        async::sqlite3_close(db);
    }
}

// -----------------------------------------------------------------------------
// Test 6: Shutdown
// -----------------------------------------------------------------------------
static void Test_Shutdown()
{
    // Note: sqlite3_shutdown is a process-wide operation. 
    // It should be the last thing tested or handled carefully.
    // For this unit test, we verify it returns OK.
    int rc = async::sqlite3_shutdown();
    ASSERT_TRUE(rc == SQLITE_OK);
}

// -----------------------------------------------------------------------------
// Main Entry Point for SQLite Tests
// -----------------------------------------------------------------------------
void Initialization_UT()
{
    std::cout << "Running SQLite Init/Connection Tests..." << std::endl;

    CleanupDB();

    Test_InitAndThread();
    Test_OpenClose_Memory();
    Test_OpenClose_File();
    Test_OpenV2();
    Test_OpenFail();
    Test_Shutdown();

    std::cout << "SQLite Init/Connection Tests Passed!" << std::endl;
}