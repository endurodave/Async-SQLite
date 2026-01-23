// Unit tests for Async-SQLite Wrapper (Group 3: Prepared Statement Lifecycle)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>

using namespace async;

static const char* TEST_DB_FILE_STMT = "test_stmt.sqlite";

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupDB() {
    std::remove(TEST_DB_FILE_STMT);
}

static sqlite3* SetupDB() {
    sqlite3* db = nullptr;
    sqlite3_init_async();
    int rc = async::sqlite3_open(TEST_DB_FILE_STMT, &db);
    ASSERT_TRUE(rc == SQLITE_OK);
    return db;
}

static void TearDownDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 1: Basic Prepare, Step, Finalize (DDL)
// -----------------------------------------------------------------------------
static void Test_Stmt_Lifecycle_Basic()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;
    int rc = 0;

    // 1. Prepare: Create Table
    const char* sql = "CREATE TABLE test_lifecycle (id INT, data TEXT);";
    rc = async::sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    ASSERT_TRUE(rc == SQLITE_OK);
    ASSERT_TRUE(stmt != nullptr);

    // 2. Step: Execute the statement
    // DDL statements return SQLITE_DONE on the first step
    rc = async::sqlite3_step(stmt);
    ASSERT_TRUE(rc == SQLITE_DONE);

    // 3. Finalize: Clean up memory on the worker thread
    rc = async::sqlite3_finalize(stmt);
    ASSERT_TRUE(rc == SQLITE_OK);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 2: Reading Rows (Select Loop with Columns)
// -----------------------------------------------------------------------------
static void Test_Stmt_SelectLoop()
{
    sqlite3* db = SetupDB();

    // Seed data using simple exec
    const char* seed = "CREATE TABLE users (id INT, name TEXT);"
        "INSERT INTO users VALUES (10, 'Alice');"
        "INSERT INTO users VALUES (20, 'Bob');";
    async::sqlite3_exec(db, seed, nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt = nullptr;
    const char* query = "SELECT id, name FROM users ORDER BY id ASC;";

    // 1. Prepare
    int rc = async::sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 2. Step Row 1
    rc = async::sqlite3_step(stmt);
    ASSERT_TRUE(rc == SQLITE_ROW);

    // 3. Verify Columns (Row 1)
    int id = async::sqlite3_column_int(stmt, 0);
    ASSERT_TRUE(id == 10);

    const unsigned char* text = async::sqlite3_column_text(stmt, 1);
    ASSERT_TRUE(text != nullptr);
    std::string name1(reinterpret_cast<const char*>(text));
    ASSERT_TRUE(name1 == "Alice");

    // 4. Step Row 2
    rc = async::sqlite3_step(stmt);
    ASSERT_TRUE(rc == SQLITE_ROW);

    // 5. Verify Columns (Row 2)
    id = async::sqlite3_column_int(stmt, 0);
    ASSERT_TRUE(id == 20);

    text = async::sqlite3_column_text(stmt, 1);
    std::string name2(reinterpret_cast<const char*>(text));
    ASSERT_TRUE(name2 == "Bob");

    // 6. Step End
    rc = async::sqlite3_step(stmt);
    ASSERT_TRUE(rc == SQLITE_DONE);

    // 7. Finalize
    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 3: Reset and Re-execution
// -----------------------------------------------------------------------------
static void Test_Stmt_Reset()
{
    sqlite3* db = SetupDB();
    async::sqlite3_exec(db, "CREATE TABLE counter (val INT);", nullptr, nullptr, nullptr);

    sqlite3_stmt* stmt = nullptr;
    // Note: In real usage we would bind inputs, but here we just reuse the SQL
    const char* insertSql = "INSERT INTO counter VALUES (100);";

    // 1. Prepare ONCE
    async::sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);

    // 2. Run First Time
    int rc = async::sqlite3_step(stmt);
    ASSERT_TRUE(rc == SQLITE_DONE);

    // 3. Reset (Reset puts the statement back to the beginning)
    // IMPORTANT: It does not clear bindings (tested in Group 4)
    rc = async::sqlite3_reset(stmt);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 4. Run Second Time
    rc = async::sqlite3_step(stmt);
    ASSERT_TRUE(rc == SQLITE_DONE);

    async::sqlite3_finalize(stmt);

    // Verify we have 2 rows now
    int rowCount = 0;
    auto countCb = [](void* ptr, int, char**, char**) -> int {
        (*static_cast<int*>(ptr))++;
        return 0;
        };
    async::sqlite3_exec(db, "SELECT * FROM counter;", countCb, &rowCount, nullptr);

    ASSERT_TRUE(rowCount == 2);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 4: Prepare Failure (Invalid SQL)
// -----------------------------------------------------------------------------
static void Test_Stmt_PrepareFail()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    // Syntax error (missing FROM table)
    const char* badSql = "SELECT * missing_table";

    int rc = async::sqlite3_prepare_v2(db, badSql, -1, &stmt, nullptr);

    // Should fail
    ASSERT_TRUE(rc != SQLITE_OK);
    // stmt should be NULL on failure (usually, depends on specific SQLite version behavior, 
    // but wrapper should propagate it)
    ASSERT_TRUE(stmt == nullptr);

    // Verify we can get the error message from the DB handle
    const char* errMsg = async::sqlite3_errmsg(db);
    ASSERT_TRUE(errMsg != nullptr);
    // Just ensure the string isn't empty
    ASSERT_TRUE(std::string(errMsg).length() > 0);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 5: Column Metadata
// -----------------------------------------------------------------------------
static void Test_Stmt_ColumnMetadata()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "SELECT 1 AS my_num, 'text' AS my_str";
    async::sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    // Must step at least once (or just prepare, depending on SQLite version)
    // to get column count, but Step is usually not required for Col Count.
    // However, Step IS required for Column Data.

    int colCount = async::sqlite3_column_count(stmt);
    ASSERT_TRUE(colCount == 2);

    const char* name0 = async::sqlite3_column_name(stmt, 0);
    ASSERT_TRUE(std::string(name0) == "my_num");

    const char* name1 = async::sqlite3_column_name(stmt, 1);
    ASSERT_TRUE(std::string(name1) == "my_str");

    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Prepared_UT()
{
    std::cout << "Running SQLite Prepared Statement Tests..." << std::endl;

    CleanupDB();

    Test_Stmt_Lifecycle_Basic();
    Test_Stmt_SelectLoop();
    Test_Stmt_Reset();
    Test_Stmt_PrepareFail();
    Test_Stmt_ColumnMetadata();

    std::cout << "SQLite Prepared Statement Tests Passed!" << std::endl;
}