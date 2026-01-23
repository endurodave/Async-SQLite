// Unit tests for Async-SQLite Wrapper (Group 7: Memory & String Utilities)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>

using namespace async;

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void InitSys() {
    sqlite3_init_async();
    // Initialize standard SQLite to ensure allocator is ready
    async::sqlite3_initialize();
}

// -----------------------------------------------------------------------------
// Test 1: Memory Allocation Lifecycle (Malloc, Realloc, Msize, Free)
// -----------------------------------------------------------------------------
static void Test_Mem_Lifecycle()
{
    InitSys();

    // 1. Allocate Memory (128 bytes)
    // The call happens on worker thread, pointer returned to main thread.
    void* ptr = async::sqlite3_malloc(128);
    ASSERT_TRUE(ptr != nullptr);

    // 2. Check Size
    // Verify the allocator knows about this pointer
    sqlite3_uint64 size = async::sqlite3_msize(ptr);
    ASSERT_TRUE(size >= 128);

    // 3. Reallocate (Expand to 256)
    // Note: realloc might move the pointer, so we capture the new one
    void* newPtr = async::sqlite3_realloc(ptr, 256);
    ASSERT_TRUE(newPtr != nullptr);

    size = async::sqlite3_msize(newPtr);
    ASSERT_TRUE(size >= 256);

    // 4. Free
    // Passes the pointer address back to worker thread for deallocation
    async::sqlite3_free(newPtr);
}

// -----------------------------------------------------------------------------
// Test 2: 64-bit Memory Allocation
// -----------------------------------------------------------------------------
static void Test_Mem_Alloc64()
{
    InitSys();

    // Allocate a small amount using the 64-bit API
    // (Allocating actual >4GB would kill the test machine, so we test the API mapping)
    void* ptr = async::sqlite3_malloc64(1024);
    ASSERT_TRUE(ptr != nullptr);

    sqlite3_uint64 size = async::sqlite3_msize(ptr);
    ASSERT_TRUE(size >= 1024);

    async::sqlite3_free(ptr);
}

// -----------------------------------------------------------------------------
// Test 3: String Builder (Append and Finish)
// -----------------------------------------------------------------------------
static void Test_Str_Builder()
{
    InitSys();

    // 1. Create a new string builder on the worker thread
    // Passing NULL db is allowed for generic string building
    sqlite3_str* s = async::sqlite3_str_new(nullptr);
    ASSERT_TRUE(s != nullptr);

    // 2. Append pieces
    async::sqlite3_str_append(s, "Hello", 5);
    async::sqlite3_str_appendchar(s, 1, ' ');
    async::sqlite3_str_appendall(s, "World");

    // 3. Check Length
    int len = async::sqlite3_str_length(s);
    ASSERT_TRUE(len == 11);

    // 4. Finish (Destroys builder, returns char*)
    char* result = async::sqlite3_str_finish(s);
    ASSERT_TRUE(result != nullptr);

    // 5. Verify Content
    std::string resStr(result);
    ASSERT_TRUE(resStr == "Hello World");

    // 6. Cleanup result string
    async::sqlite3_free(result);
}

// -----------------------------------------------------------------------------
// Test 4: String Builder Reset
// -----------------------------------------------------------------------------
static void Test_Str_Reset()
{
    InitSys();

    sqlite3_str* s = async::sqlite3_str_new(nullptr);

    // Append garbage
    async::sqlite3_str_append(s, "Garbage", 7);

    // Reset (Should clear content but keep builder alive)
    async::sqlite3_str_reset(s);

    // Append fresh
    async::sqlite3_str_append(s, "Clean", 5);

    // Finish
    char* result = async::sqlite3_str_finish(s);
    ASSERT_TRUE(std::string(result) == "Clean");

    async::sqlite3_free(result);
}

// -----------------------------------------------------------------------------
// Test 5: String Error Status
// -----------------------------------------------------------------------------
static void Test_Str_Status()
{
    InitSys();
    sqlite3_str* s = async::sqlite3_str_new(nullptr);

    // Check initial status
    int rc = async::sqlite3_str_errcode(s);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Finish empty (returns NULL or empty string depending on version, 
    // but finish calls finalize on the builder)
    char* res = async::sqlite3_str_finish(s);
    async::sqlite3_free(res);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Memory_UT()
{
    std::cout << "Running SQLite Memory & String Utility Tests..." << std::endl;

    Test_Mem_Lifecycle();
    Test_Mem_Alloc64();
    Test_Str_Builder();
    Test_Str_Reset();
    Test_Str_Status();

    std::cout << "SQLite Memory & String Utility Tests Passed!" << std::endl;
}