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

void testSqlite3ColumnNameAsync() {
    // Open a test database
    sqlite3* db = openTestDatabase();

    // Create a table with known column names
    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT, age INTEGER);";
    execTestSQL(db, createTableSQL);

    // Insert a record into the table
    std::string insertSQL = "INSERT INTO test_table (name, age) VALUES ('John Doe', 30);";
    execTestSQL(db, insertSQL);

    // Select the record from the table
    std::string selectSQL = "SELECT id, name, age FROM test_table WHERE id = 1;";
    sqlite3_stmt* stmt;
    int result = async::sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);

    // Step through the result
    if (async::sqlite3_step(stmt, std::chrono::milliseconds(5000)) == SQLITE_ROW) {
        // Use sqlite3_column_name to get the column names
        const char* columnName1 = async::sqlite3_column_name(stmt, 0, std::chrono::milliseconds(5000));
        const char* columnName2 = async::sqlite3_column_name(stmt, 1, std::chrono::milliseconds(5000));
        const char* columnName3 = async::sqlite3_column_name(stmt, 2, std::chrono::milliseconds(5000));

        // Verify the column names are as expected
        assert(std::string(columnName1) == "id");    // Column 0 should be "id"
        assert(std::string(columnName2) == "name");  // Column 1 should be "name"
        assert(std::string(columnName3) == "age");   // Column 2 should be "age"
    }
    else {
        assert(false && "Failed to retrieve the row.");
    }

    // Finalize the statement and close the database
    async::sqlite3_finalize(stmt, std::chrono::milliseconds(5000));
    async::sqlite3_close(db, std::chrono::milliseconds(5000));

    std::cout << "Test sqlite3_column_name async passed!" << std::endl;
}

void testSqlite3DbStatusAsync() {
    // Open a test database
    sqlite3* db = openTestDatabase();

    // Perform an operation on the database (e.g., create a table)
    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT);";
    execTestSQL(db, createTableSQL);

    // Insert a few rows into the table
    std::string insertSQL1 = "INSERT INTO test_table (name) VALUES ('Alice');";
    execTestSQL(db, insertSQL1);
    std::string insertSQL2 = "INSERT INTO test_table (name) VALUES ('Bob');";
    execTestSQL(db, insertSQL2);

    // Call sqlite3_db_status to get the database statistics
    int currentStatus = 0;
    int currentUsed = 0;
    int currentHighwater = 0;
    int result = async::sqlite3_db_status(db, SQLITE_DBSTATUS_LOOKASIDE_USED, &currentStatus, &currentUsed, currentHighwater, std::chrono::milliseconds(5000));

    assert(result == SQLITE_OK);  // Ensure the db status query was successful

    // Output the values to check manually (for debugging purposes)
    std::cout << "Current status: " << currentStatus << std::endl;
    std::cout << "Current used: " << currentUsed << std::endl;
    std::cout << "Current highwater: " << currentHighwater << std::endl;

    // Verify that the values are reasonable based on the operations we performed
    // For example, there should be some lookaside memory used after inserting rows
    assert(currentStatus >= 0);  // Status should be non-negative
    assert(currentUsed >= 0);    // Used memory should be non-negative
    assert(currentHighwater >= 0);  // Highwater memory should be non-negative

    // Optionally, you can check specific values depending on the expected results, 
    // but these values depend on the SQLite implementation and configuration. 
    // You can compare them with the expected results based on the operations performed.

    // Finalize and close the database
    async::sqlite3_close(db, std::chrono::milliseconds(5000));

    std::cout << "Test sqlite3_db_status async passed!" << std::endl;
}

void testSqlite3ColumnTextAsync() {
    // Open a test database
    sqlite3* db = openTestDatabase();

    // Create a table and insert some data
    std::string createTableSQL = "CREATE TABLE IF NOT EXISTS test_table (id INTEGER PRIMARY KEY, name TEXT);";
    execTestSQL(db, createTableSQL);

    std::string insertSQL1 = "INSERT INTO test_table (name) VALUES ('Alice');";
    execTestSQL(db, insertSQL1);

    std::string insertSQL2 = "INSERT INTO test_table (name) VALUES ('Bob');";
    execTestSQL(db, insertSQL2);

    // Select the data from the table
    std::string selectSQL = "SELECT name FROM test_table WHERE id = 1;";
    sqlite3_stmt* stmt;
    int result = async::sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);  // Ensure the SELECT query preparation was successful

    // Step through the result set
    if (async::sqlite3_step(stmt, std::chrono::milliseconds(5000)) == SQLITE_ROW) {
        // Retrieve the text from the first column (name) using sqlite3_column_text
        const unsigned char* name = async::sqlite3_column_text(stmt, 0, std::chrono::milliseconds(5000));
        assert(name != nullptr);  // Ensure the text is not null

        // Convert the unsigned char* to std::string for easier comparison
        std::string nameStr(reinterpret_cast<const char*>(name));

        // Verify the retrieved name matches the inserted value
        assert(nameStr == "Alice");  // Should be "Alice" as per the inserted value
    }
    else {
        assert(false && "Failed to retrieve the record.");
    }

    // Finalize the statement
    async::sqlite3_finalize(stmt, std::chrono::milliseconds(5000));

    // Now, select the second row to verify further
    selectSQL = "SELECT name FROM test_table WHERE id = 2;";
    result = async::sqlite3_prepare_v2(db, selectSQL.c_str(), -1, &stmt, nullptr, std::chrono::milliseconds(5000));
    assert(result == SQLITE_OK);  // Ensure the SELECT query preparation was successful

    // Step through the result set
    if (async::sqlite3_step(stmt, std::chrono::milliseconds(5000)) == SQLITE_ROW) {
        // Retrieve the text from the first column (name) using sqlite3_column_text
        const unsigned char* name = async::sqlite3_column_text(stmt, 0, std::chrono::milliseconds(5000));
        assert(name != nullptr);  // Ensure the text is not null

        // Convert the unsigned char* to std::string for easier comparison
        std::string nameStr(reinterpret_cast<const char*>(name));

        // Verify the retrieved name matches the inserted value
        assert(nameStr == "Bob");  // Should be "Bob" as per the inserted value
    }
    else {
        assert(false && "Failed to retrieve the record.");
    }

    // Finalize the statement
    async::sqlite3_finalize(stmt, std::chrono::milliseconds(5000));

    // Close the database
    async::sqlite3_close(db, std::chrono::milliseconds(5000));

    std::cout << "Test sqlite3_column_text async passed!" << std::endl;
}

void testSqliteMemoryFunctionsAsync() {
    // 1. Test sqlite3_malloc
    void* ptr1 = async::sqlite3_malloc(100, std::chrono::milliseconds(5000));
    assert(ptr1 != nullptr);  // Ensure memory was allocated successfully

    // Check the allocated size using sqlite3_msize
    sqlite3_uint64 size1 = async::sqlite3_msize(ptr1, std::chrono::milliseconds(5000));
    assert(size1 >= 100);  // Ensure the allocated size is 100 bytes

    // 2. Test sqlite3_malloc64 (for larger allocation)
    void* ptr2 = async::sqlite3_malloc64(1000, std::chrono::milliseconds(5000));
    assert(ptr2 != nullptr);  // Ensure memory was allocated successfully

    // Check the allocated size using sqlite3_msize
    sqlite3_uint64 size2 = async::sqlite3_msize(ptr2, std::chrono::milliseconds(5000));
    assert(size2 == 1000);  // Ensure the allocated size is 1000 bytes

    // 3. Test sqlite3_realloc (resize allocation)
    void* ptr3 = async::sqlite3_realloc(ptr1, 200, std::chrono::milliseconds(5000));
    assert(ptr3 != nullptr);  // Ensure memory was reallocated successfully

    // Check the new allocated size using sqlite3_msize
    sqlite3_uint64 size3 = async::sqlite3_msize(ptr3, std::chrono::milliseconds(5000));
    assert(size3 == 200);  // Ensure the new size is 200 bytes

    // 4. Test sqlite3_realloc64 (resize allocation for larger memory)
    void* ptr4 = async::sqlite3_realloc64(ptr2, 2000, std::chrono::milliseconds(5000));
    assert(ptr4 != nullptr);  // Ensure memory was reallocated successfully

    // Check the new allocated size using sqlite3_msize
    sqlite3_uint64 size4 = async::sqlite3_msize(ptr4, std::chrono::milliseconds(5000));
    assert(size4 == 2000);  // Ensure the new size is 2000 bytes

    // 5. Test sqlite3_free (free memory)
    async::sqlite3_free(ptr3, std::chrono::milliseconds(5000));  // Free the reallocated memory
    async::sqlite3_free(ptr4, std::chrono::milliseconds(5000));  // Free the reallocated memory

    // 6. Ensure that freeing memory does not cause errors (no further operations on freed pointers)
    // We don't need additional asserts after free, as we're relying on the absence of crashes and issues

    std::cout << "Test sqlite3 memory functions async passed!" << std::endl;
}

int RunUnitTests()
{
    testSqliteOpenAsync();
    testSqliteExecAsync();
    testSqliteColumnTextAsync();
    testSqlite3BindIntAsync();
    testSqlite3ColumnNameAsync();
    testSqlite3DbStatusAsync();
    testSqlite3ColumnTextAsync();
    testSqliteMemoryFunctionsAsync();

    std::cout << "All tests passed!" << std::endl;
    return 0;
}
