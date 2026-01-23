// Unit tests for Async-SQLite Wrapper (Group 4: Data Exchange - Binding & Columns)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring> // for memcmp
#include <cmath>   // for fabs

using namespace async;

static const char* TEST_DB_FILE_DATA = "test_data.sqlite";

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupDB() {
    std::remove(TEST_DB_FILE_DATA);
}

static sqlite3* SetupDB() {
    sqlite3* db = nullptr;
    sqlite3_init_async();
    int rc = async::sqlite3_open(TEST_DB_FILE_DATA, &db);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Create a generic table for testing types
    const char* sql = "CREATE TABLE data_test ("
        "id INTEGER PRIMARY KEY, "
        "val_int INTEGER, "
        "val_real REAL, "
        "val_text TEXT, "
        "val_blob BLOB"
        ");";
    async::sqlite3_exec(db, sql, nullptr, nullptr, nullptr);

    return db;
}

static void TearDownDB(sqlite3* db) {
    if (db) {
        async::sqlite3_close(db);
    }
    CleanupDB();
}

// -----------------------------------------------------------------------------
// Test 1: Integer Data Exchange (int and int64)
// -----------------------------------------------------------------------------
static void Test_Data_Integer()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* insertSql = "INSERT INTO data_test (val_int) VALUES (?);";
    async::sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);

    // 1. Bind 32-bit Integer
    int val32 = 123456;
    async::sqlite3_bind_int(stmt, 1, val32);
    async::sqlite3_step(stmt);
    async::sqlite3_reset(stmt);

    // 2. Bind 64-bit Integer (Big value)
    sqlite3_int64 val64 = 9876543210LL;
    async::sqlite3_bind_int64(stmt, 1, val64);
    async::sqlite3_step(stmt);

    async::sqlite3_finalize(stmt);

    // 3. Verify Retrieval
    const char* selectSql = "SELECT val_int FROM data_test ORDER BY id ASC;";
    async::sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);

    // Row 1 (32-bit)
    ASSERT_TRUE(async::sqlite3_step(stmt) == SQLITE_ROW);
    int ret32 = async::sqlite3_column_int(stmt, 0);
    ASSERT_TRUE(ret32 == val32);

    // Row 2 (64-bit)
    ASSERT_TRUE(async::sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_int64 ret64 = async::sqlite3_column_int64(stmt, 0);
    ASSERT_TRUE(ret64 == val64);

    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 2: Double/Float Data Exchange
// -----------------------------------------------------------------------------
static void Test_Data_Double()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* insertSql = "INSERT INTO data_test (val_real) VALUES (?);";
    async::sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);

    double val = 3.1415926535;
    async::sqlite3_bind_double(stmt, 1, val);
    async::sqlite3_step(stmt);

    async::sqlite3_finalize(stmt);

    // Verify
    const char* selectSql = "SELECT val_real FROM data_test;";
    async::sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);

    ASSERT_TRUE(async::sqlite3_step(stmt) == SQLITE_ROW);
    double ret = async::sqlite3_column_double(stmt, 0);

    // Check reasonable precision
    ASSERT_TRUE(std::fabs(ret - val) < 0.000000001);

    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 3: Text Data Exchange
// -----------------------------------------------------------------------------
static void Test_Data_Text()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* insertSql = "INSERT INTO data_test (val_text) VALUES (?);";
    async::sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);

    std::string text = "Hello Async World!";

    // Bind Text
    // Note: SQLITE_TRANSIENT (-1) causes SQLite to make a copy.
    // SQLITE_STATIC (0) assumes the pointer remains valid. 
    // Since the wrapper blocks, SQLITE_STATIC is theoretically safe for stack variables here,
    // but SQLITE_TRANSIENT is safer practice.
    async::sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_TRANSIENT);
    async::sqlite3_step(stmt);

    async::sqlite3_finalize(stmt);

    // Verify
    const char* selectSql = "SELECT val_text FROM data_test;";
    async::sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);

    ASSERT_TRUE(async::sqlite3_step(stmt) == SQLITE_ROW);

    const unsigned char* retPtr = async::sqlite3_column_text(stmt, 0);
    ASSERT_TRUE(retPtr != nullptr);

    std::string retText(reinterpret_cast<const char*>(retPtr));
    ASSERT_TRUE(retText == text);

    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 4: Blob Data Exchange
// -----------------------------------------------------------------------------
static void Test_Data_Blob()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* insertSql = "INSERT INTO data_test (val_blob) VALUES (?);";
    async::sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);

    // Binary data (0x00, 0xFF, etc)
    std::vector<unsigned char> blob = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xFF };

    // Bind Blob
    async::sqlite3_bind_blob(stmt, 1, blob.data(), (int)blob.size(), SQLITE_TRANSIENT);
    async::sqlite3_step(stmt);

    async::sqlite3_finalize(stmt);

    // Verify
    const char* selectSql = "SELECT val_blob FROM data_test;";
    async::sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);

    ASSERT_TRUE(async::sqlite3_step(stmt) == SQLITE_ROW);

    // Verify Size
    int bytes = async::sqlite3_column_bytes(stmt, 0);
    ASSERT_TRUE(bytes == (int)blob.size());

    // Verify Content
    const void* retPtr = async::sqlite3_column_blob(stmt, 0);
    ASSERT_TRUE(retPtr != nullptr);
    ASSERT_TRUE(std::memcmp(retPtr, blob.data(), bytes) == 0);

    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 5: Null and Type Checking
// -----------------------------------------------------------------------------
static void Test_Data_NullAndTypes()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    // Insert NULL explicitly
    const char* insertSql = "INSERT INTO data_test (val_int) VALUES (?);";
    async::sqlite3_prepare_v2(db, insertSql, -1, &stmt, nullptr);

    async::sqlite3_bind_null(stmt, 1);
    async::sqlite3_step(stmt);
    async::sqlite3_finalize(stmt);

    // Verify
    const char* selectSql = "SELECT val_int FROM data_test;";
    async::sqlite3_prepare_v2(db, selectSql, -1, &stmt, nullptr);

    ASSERT_TRUE(async::sqlite3_step(stmt) == SQLITE_ROW);

    // Check Type
    int type = async::sqlite3_column_type(stmt, 0);
    ASSERT_TRUE(type == SQLITE_NULL);

    // Retrieving value should be 0 for int
    int ret = async::sqlite3_column_int(stmt, 0);
    ASSERT_TRUE(ret == 0);

    async::sqlite3_finalize(stmt);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 6: Named Parameters (Bind Index)
// -----------------------------------------------------------------------------
static void Test_Data_NamedParams()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "INSERT INTO data_test (val_int) VALUES (:myVal);";
    async::sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    // Get Index of ":myVal"
    int idx = async::sqlite3_bind_parameter_index(stmt, ":myVal");
    ASSERT_TRUE(idx > 0);

    // Bind using that index
    async::sqlite3_bind_int(stmt, idx, 777);
    async::sqlite3_step(stmt);

    async::sqlite3_finalize(stmt);

    // Verify
    int rowCount = 0;
    async::sqlite3_exec(db, "SELECT * FROM data_test WHERE val_int=777;",
        [](void* p, int, char**, char**) { (*(int*)p)++; return 0; }, &rowCount, nullptr);

    ASSERT_TRUE(rowCount == 1);

    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Test 7: Clear Bindings
// -----------------------------------------------------------------------------
static void Test_Data_ClearBindings()
{
    sqlite3* db = SetupDB();
    sqlite3_stmt* stmt = nullptr;

    const char* sql = "INSERT INTO data_test (val_int) VALUES (?);";
    async::sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    // 1. Bind and Insert 888
    async::sqlite3_bind_int(stmt, 1, 888);
    async::sqlite3_step(stmt);
    async::sqlite3_reset(stmt);

    // 2. Clear Bindings (Should revert ? to NULL)
    async::sqlite3_clear_bindings(stmt);
    async::sqlite3_step(stmt);

    async::sqlite3_finalize(stmt);

    // Verify: Should have 888 and NULL
    sqlite3_stmt* check = nullptr;
    async::sqlite3_prepare_v2(db, "SELECT val_int FROM data_test ORDER BY id ASC;", -1, &check, nullptr);

    // Row 1: 888
    async::sqlite3_step(check);
    ASSERT_TRUE(async::sqlite3_column_int(check, 0) == 888);

    // Row 2: NULL (0 when fetched as int)
    async::sqlite3_step(check);
    ASSERT_TRUE(async::sqlite3_column_type(check, 0) == SQLITE_NULL);

    async::sqlite3_finalize(check);
    TearDownDB(db);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void DataExchange_UT()
{
    std::cout << "Running SQLite Data Exchange Tests..." << std::endl;

    CleanupDB();

    Test_Data_Integer();
    Test_Data_Double();
    Test_Data_Text();
    Test_Data_Blob();
    Test_Data_NullAndTypes();
    Test_Data_NamedParams();
    Test_Data_ClearBindings();

    std::cout << "SQLite Data Exchange Tests Passed!" << std::endl;
}