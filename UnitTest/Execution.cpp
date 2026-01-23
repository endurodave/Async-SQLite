// Unit tests for Async-SQLite Wrapper (Group 2: Direct Execution & Callbacks)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

using namespace async;

static const char* TEST_DB_FILE_EXEC = "test_exec.sqlite";

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupDB() {
    std::remove(TEST_DB_FILE_EXEC);
}

static sqlite3* SetupDB() {
    sqlite3* db = nullptr;
    // Ensure async system is running
    sqlite3_init_async();
    int rc = async::sqlite3_open(TEST_DB_FILE_EXEC, &db);
    ASSERT_TRUE(rc == SQLITE_OK);
    ASSERT_TRUE(db != nullptr);
    return db;
}

static void TearDownDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Context struct to share data between the Main Thread (Test) 
// and the Worker Thread (Callback)
// -----------------------------------------------------------------------------
struct CallbackCtx {
    int rowCount = 0;
    std::vector<std::string> capturedNames;
};

// Static C-style callback function compatible with sqlite3_exec
// This runs on the WORKER THREAD.
static int TestCallback(void* ptr, int argc, char** argv, char** colNames) {
    CallbackCtx* ctx = static_cast<CallbackCtx*>(ptr);

    ctx->rowCount++;

    // Assumption: We are selecting specific columns, verify argv[0] exists
    if (argc > 0 && argv[0]) {
        ctx->capturedNames.push_back(std::string(argv[0]));
    }
    else {
        ctx->capturedNames.push_back("NULL");
    }

    return 0; // Return 0 to continue
}

// Callback that forces an abort
static int AbortCallback(void* ptr, int argc, char** argv, char** colNames) {
    return 1; // Non-zero return aborts execution
}

// -----------------------------------------------------------------------------
// Test 1: Basic Create Table and Insert (No Callback)
// -----------------------------------------------------------------------------
static void Test_Exec_CreateAndInsert()
{
    sqlite3* db = SetupDB();
    int rc = 0;
    char* errMsg = nullptr;

    // 1. Create Table
    const char* sqlCreate = "CREATE TABLE users (id INT, name TEXT);";
    rc = async::sqlite3_exec(db, sqlCreate, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Create Error: " << (errMsg ? errMsg : "Unknown") << std::endl;
        async::sqlite3_free(errMsg);
    }
    ASSERT_TRUE(rc == SQLITE_OK);

    // 2. Insert Data
    const char* sqlInsert = "INSERT INTO users VALUES (1, 'Alice'); "
        "INSERT INTO users VALUES (2, 'Bob');";
    rc = async::sqlite3_exec(db, sqlInsert, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Insert Error: " << (errMsg ? errMsg : "Unknown") << std::endl;
        async::sqlite3_free(errMsg);
    }
    ASSERT_TRUE(rc == SQLITE_OK);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 2: Select with Callback
// -----------------------------------------------------------------------------
static void Test_Exec_SelectCallback()
{
    sqlite3* db = SetupDB();
    int rc = 0;

    // Seed Data
    const char* seedSql = "CREATE TABLE items (name TEXT); "
        "INSERT INTO items VALUES ('ItemA'); "
        "INSERT INTO items VALUES ('ItemB'); "
        "INSERT INTO items VALUES ('ItemC');";
    async::sqlite3_exec(db, seedSql, nullptr, nullptr, nullptr);

    // Prepare Context on Main Thread Stack
    CallbackCtx ctx;

    // Execute Select
    // NOTE: We pass the address of 'ctx'. The pointer is marshaled to the 
    // worker thread. The worker thread writes to 'ctx' memory. 
    // This is safe because AsyncInvoke waits for completion before returning.
    const char* selectSql = "SELECT name FROM items ORDER BY name ASC;";
    rc = async::sqlite3_exec(db, selectSql, &TestCallback, &ctx, nullptr);

    // Verify Results
    ASSERT_TRUE(rc == SQLITE_OK);
    ASSERT_TRUE(ctx.rowCount == 3);

    ASSERT_TRUE(ctx.capturedNames.size() == 3);
    ASSERT_TRUE(ctx.capturedNames[0] == "ItemA");
    ASSERT_TRUE(ctx.capturedNames[1] == "ItemB");
    ASSERT_TRUE(ctx.capturedNames[2] == "ItemC");

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 3: Aborting Execution via Callback
// -----------------------------------------------------------------------------
static void Test_Exec_Abort()
{
    sqlite3* db = SetupDB();

    // Create a table with multiple rows
    const char* seedSql = "CREATE TABLE nums (val INT); "
        "INSERT INTO nums VALUES (1); "
        "INSERT INTO nums VALUES (2); "
        "INSERT INTO nums VALUES (3);";
    async::sqlite3_exec(db, seedSql, nullptr, nullptr, nullptr);

    // This callback returns 1, which should stop SQLite
    int rc = async::sqlite3_exec(db, "SELECT val FROM nums;", &AbortCallback, nullptr, nullptr);

    // SQLite should return SQLITE_ABORT
    ASSERT_TRUE(rc == SQLITE_ABORT);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 4: Error Handling and Error Message Output
// -----------------------------------------------------------------------------
static void Test_Exec_Error()
{
    sqlite3* db = SetupDB();
    char* zErrMsg = nullptr;

    // Execute invalid SQL (Syntax error)
    const char* badSql = "SELECT * FROM non_existent_table_xyz";
    int rc = async::sqlite3_exec(db, badSql, nullptr, nullptr, &zErrMsg);

    // 1. Verify Error Code
    ASSERT_TRUE(rc != SQLITE_OK);

    // 2. Verify Error Message was populated
    // Since &zErrMsg (char**) was passed to the worker thread, and SQLite allocated 
    // the string, the pointer value in zErrMsg should now point to that string.
    ASSERT_TRUE(zErrMsg != nullptr);

    std::string errStr(zErrMsg);
    // Simple check to ensure it contains relevant info
    ASSERT_TRUE(errStr.find("no such table") != std::string::npos);

    // 3. Free the error string using the Async API
    // (Crucial: Memory allocated by SQLite on worker thread should be freed via SQLite API)
    async::sqlite3_free(zErrMsg);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Execution_UT()
{
    std::cout << "Running SQLite Direct Exec Tests..." << std::endl;

    CleanupDB(); // Pre-flight cleanup

    Test_Exec_CreateAndInsert();
    Test_Exec_SelectCallback();
    Test_Exec_Abort();
    Test_Exec_Error();

    std::cout << "SQLite Direct Exec Tests Passed!" << std::endl;
}