#include "async_sqlite3_ut.h"

#include <cassert>
#include <chrono>
#include <async_sqlite3.h>
#include <iostream>
#include <string>

// Assuming async namespace is available
using namespace async;

// Helper function to open a database
sqlite3* openTestDatabase(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
    remove("test.db");
    sqlite3* db = nullptr;
    int result = async::sqlite3_open("test.db", &db, timeout);
    assert(result == SQLITE_OK);  // Ensure db is opened successfully
    return db;
}

// Helper function to execute an SQL query
void execTestSQL(sqlite3* db, const std::string& sql, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
    char* errMsg = nullptr;
    int result = async::sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg, timeout);
    assert(result == SQLITE_OK);  // Ensure the query executed successfully
    if (errMsg) {
        async::sqlite3_free(errMsg);
    }
}

// Helper function to fetch column text from a query result
const char* getColumnText(sqlite3_stmt* stmt, int col, std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
    const unsigned char* text = async::sqlite3_column_text(stmt, col, timeout);
    assert(text != nullptr);  // Ensure the column text is not null
    return (const char*)text;
}

void testSqliteOpenAsync() {
    sqlite3* db = openTestDatabase();
    assert(db != nullptr);  // Database handle should not be null
    async::sqlite3_close(db);  // Clean up
    std::cout << "Test sqlite3_open async passed!" << std::endl;
}

void testSqliteExecAsync() {
    sqlite3* db = openTestDatabase();

    // Create a table
    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT);";
    execTestSQL(db, createTableSQL);

    // Insert a record
    std::string insertSQL = "INSERT INTO test_table (name) VALUES ('Alice');";
    execTestSQL(db, insertSQL);

    // Check that the record exists
    std::string selectSQL = "SELECT name FROM test_table WHERE id = 1;";
    sqlite3_stmt* stmt;
    int result = async::sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr);
    assert(result == SQLITE_OK);

    if (async::sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = getColumnText(stmt, 0);
        assert(std::string(name) == "Alice");  // Ensure the inserted name is correct
    }
    else {
        assert(false && "Failed to retrieve the record.");
    }

    async::sqlite3_finalize(stmt);
    async::sqlite3_close(db);  // Clean up
    std::cout << "Test sqlite3_exec async passed!" << std::endl;
}

void testSqliteColumnTextAsync() {
    sqlite3* db = openTestDatabase();

    // Create a table and insert a record
    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT);";
    execTestSQL(db, createTableSQL);
    std::string insertSQL = "INSERT INTO test_table (name) VALUES ('Bob');";
    execTestSQL(db, insertSQL);

    // Select the record
    std::string selectSQL = "SELECT name FROM test_table WHERE id = 1;";
    sqlite3_stmt* stmt;
    int result = async::sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr);
    assert(result == SQLITE_OK);

    if (async::sqlite3_step(stmt) == SQLITE_ROW) {
        // Get the text from the first column (name)
        const unsigned char* name = sqlite3_column_text(stmt, 0, std::chrono::milliseconds(5000));

        // Convert unsigned char* to std::string for comparison
        std::string nameStr(reinterpret_cast<const char*>(name));

        // Assert that the name matches the expected value
        assert(nameStr == "Bob");  // Ensure the name is "Bob"
    }
    else {
        assert(false && "Failed to retrieve the column text.");
    }

    async::sqlite3_finalize(stmt);
    async::sqlite3_close(db);  // Clean up
    std::cout << "Test sqlite3_column_text async passed!" << std::endl;
}

#if 0
void testSqlite3BindIntAsync() {
    // Open an in-memory SQLite database
    sqlite3* db = nullptr;
    int result = async::sqlite3_open(":memory:", &db, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);  // Ensure db is opened successfully

    // Create a test table with an integer column
    const char* createTableSQL = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, value INTEGER);";
    result = async::sqlite3_exec(db, createTableSQL, nullptr, nullptr, nullptr, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);  // Ensure table creation is successful

    // Prepare an INSERT statement
    sqlite3_stmt* stmt = nullptr;
    const char* insertSQL = "INSERT INTO test_table (value) VALUES (?);";
    result = async::sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);  // Ensure statement preparation is successful

    // Bind an integer value (e.g., 42) to the placeholder
    result = async::sqlite3_bind_int(stmt, 1, 42, std::chrono::milliseconds(5000));  // Bind integer 42 to the first placeholder
    assert(result == SQLITE_OK);  // Ensure binding is successful

    // Execute the statement
    result = async::sqlite3_step(stmt, std::chrono::milliseconds(5000));
    assert(result == SQLITE_DONE);  // Ensure the statement was executed successfully (row inserted)

    // Finalize the statement
    async::sqlite3_finalize(stmt, std::chrono::milliseconds(5000));

    // Now, select the value from the table to verify it was inserted correctly
    const char* selectSQL = "SELECT value FROM test_table WHERE id = 1;";
    result = async::sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);  // Ensure SELECT statement preparation is successful

    // Execute the select query
    result = async::sqlite3_step(stmt, std::chrono::milliseconds(5000));
    assert(result == SQLITE_ROW);  // Ensure the query returned a row

    // Fetch the value from the result
    int value = async::sqlite3_column_int(stmt, 0, std::chrono::milliseconds(5000));
    assert(value == 42);  // Ensure the value is the same as what we bound (42)

    // Finalize the statement
    async::sqlite3_finalize(stmt, std::chrono::milliseconds(5000));

    // Clean up by closing the database
    async::sqlite3_close(db, std::chrono::milliseconds(5000));

    std::cout << "Test sqlite3_bind_int async passed!" << std::endl;
}
#endif

int RunUnitTests()
{
    testSqliteOpenAsync();
    testSqliteExecAsync();
    testSqliteColumnTextAsync();
    //testSqlite3BindIntAsync();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
