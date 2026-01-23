// Unit tests for Async-SQLite Wrapper (Group 8: Advanced Features - Backup & Serialize)

#include "async_sqlite3.h"
#include "DelegateMQ.h"
#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>

using namespace async;

static const char* BACKUP_SRC_FILE = "test_backup_src.sqlite";
static const char* BACKUP_DST_FILE = "test_backup_dst.sqlite";

// -----------------------------------------------------------------------------
// Helper Infrastructure
// -----------------------------------------------------------------------------

static void CleanupFiles() {
    std::remove(BACKUP_SRC_FILE);
    std::remove(BACKUP_DST_FILE);
}

static sqlite3* OpenDB(const char* filename) {
    sqlite3* db = nullptr;
    sqlite3_init_async();
    int rc = async::sqlite3_open(filename, &db);
    ASSERT_TRUE(rc == SQLITE_OK);
    return db;
}

static void CloseDB(sqlite3* db) {
    if (db) {
        // Verify close succeeds (if it returns BUSY, we have a leak)
        int rc = async::sqlite3_close(db);
        if (rc != SQLITE_OK) {
            std::cerr << "Warning: sqlite3_close failed with rc=" << rc << std::endl;
        }
    }
}

// -----------------------------------------------------------------------------
// Test 1: Serialize and Deserialize (In-Memory Round Trip)
// -----------------------------------------------------------------------------
static void Test_Adv_Serialize()
{
#ifdef SQLITE_ENABLE_DESERIALIZE
    // 1. Setup Source DB (In-Memory)
    sqlite3* dbSrc = OpenDB(":memory:");

    // Seed Data
    async::sqlite3_exec(dbSrc, "CREATE TABLE snap (id INT); INSERT INTO snap VALUES (42);", nullptr, nullptr, nullptr);

    // 2. Serialize
    sqlite3_int64 size = 0;
    // Serialize "main" database to a buffer (malloc'd by SQLite)
    unsigned char* buffer = async::sqlite3_serialize(dbSrc, "main", &size, 0);

    ASSERT_TRUE(buffer != nullptr);
    ASSERT_TRUE(size > 0);

    // 3. Setup Dest DB (In-Memory)
    sqlite3* dbDst = OpenDB(":memory:");

    // 4. Deserialize
    // flags: SQLITE_DESERIALIZE_FREEONCLOSE | SQLITE_DESERIALIZE_RESIZEABLE
    unsigned int flags = 1 | 2;
    int rc = async::sqlite3_deserialize(dbDst, "main", buffer, size, size, flags);

    ASSERT_TRUE(rc == SQLITE_OK);

    // 5. Verify Data in Dest
    int val = 0;
    auto cb = [](void* p, int, char** argv, char**) {
        *(int*)p = std::stoi(argv[0]);
        return 0;
        };
    async::sqlite3_exec(dbDst, "SELECT id FROM snap;", cb, &val, nullptr);

    ASSERT_TRUE(val == 42);

    // Note: buffer is freed by sqlite3_deserialize due to FREEONCLOSE flag, 
    // or owned by dbDst. We don't free it manually here if success.

    CloseDB(dbDst);
    CloseDB(dbSrc);
#else
    std::cout << "[Skipped] Serialization not enabled in this build." << std::endl;
#endif
}

// -----------------------------------------------------------------------------
// Test 2: Online Backup API (Step-by-Step)
// -----------------------------------------------------------------------------
static void Test_Adv_Backup()
{
    CleanupFiles();

    // 1. Setup Source (Disk File)
    sqlite3* pSrc = OpenDB(BACKUP_SRC_FILE);
    async::sqlite3_exec(pSrc, "CREATE TABLE backup_test (data TEXT);", nullptr, nullptr, nullptr);
    // Insert enough data to potentially create multiple pages
    async::sqlite3_exec(pSrc, "INSERT INTO backup_test VALUES ('Row 1');", nullptr, nullptr, nullptr);
    async::sqlite3_exec(pSrc, "INSERT INTO backup_test VALUES ('Row 2');", nullptr, nullptr, nullptr);

    // 2. Setup Dest (Disk File)
    sqlite3* pDest = OpenDB(BACKUP_DST_FILE);

    // 3. Initialize Backup
    // Copies from pSrc "main" to pDest "main"
    sqlite3_backup* pBackup = async::sqlite3_backup_init(pDest, "main", pSrc, "main");
    ASSERT_TRUE(pBackup != nullptr);

    // 4. Step (Copy pages)
    // -1 copies all pages at once
    int rc = async::sqlite3_backup_step(pBackup, -1);
    ASSERT_TRUE(rc == SQLITE_DONE);

    // 5. Finish
    rc = async::sqlite3_backup_finish(pBackup);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 6. Verify Dest Data
    int rowCount = 0;
    auto cb = [](void* p, int, char**, char**) { (*(int*)p)++; return 0; };
    async::sqlite3_exec(pDest, "SELECT * FROM backup_test;", cb, &rowCount, nullptr);

    ASSERT_TRUE(rowCount == 2);

    CloseDB(pDest);
    CloseDB(pSrc);
    CleanupFiles();
}

// -----------------------------------------------------------------------------
// Test 3: Backup Remaining/PageCount
// -----------------------------------------------------------------------------
static void Test_Adv_BackupProgress()
{
    // Use unique filenames to avoid conflict
    const char* SRC_FILE = "test_progress_src.sqlite";
    const char* DST_FILE = "test_progress_dst.sqlite";

    // Ensure clean state
    std::remove(SRC_FILE);
    std::remove(DST_FILE);

    sqlite3* pSrc = OpenDB(SRC_FILE);
    sqlite3* pDest = OpenDB(DST_FILE);
    int rc = 0;

    // 1. Create Table and Populate
    rc = async::sqlite3_exec(pSrc, "CREATE TABLE progress (x TEXT);", nullptr, nullptr, nullptr);
    ASSERT_TRUE(rc == SQLITE_OK);

    rc = async::sqlite3_exec(pSrc, "BEGIN;", nullptr, nullptr, nullptr);
    ASSERT_TRUE(rc == SQLITE_OK);

    // Insert 1000 rows (enough to generate ~15-20 pages)
    for (int i = 0; i < 1000; ++i) {
        rc = async::sqlite3_exec(pSrc, "INSERT INTO progress VALUES ('A long string to fill up database pages for backup testing...');", nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) break;
    }
    ASSERT_TRUE(rc == SQLITE_OK);

    rc = async::sqlite3_exec(pSrc, "COMMIT;", nullptr, nullptr, nullptr);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 2. Initialize Backup
    sqlite3_backup* pBackup = async::sqlite3_backup_init(pDest, "main", pSrc, "main");
    ASSERT_TRUE(pBackup != nullptr);

    // -------------------------------------------------------------------------
    // CRITICAL FIX: Step 0
    // -------------------------------------------------------------------------
    // In some SQLite versions, pagecount is not calculated until the first step.
    // Calling step with 0 pages locks the source and calculates the size 
    // without copying data.
    rc = async::sqlite3_backup_step(pBackup, 0);
    ASSERT_TRUE(rc == SQLITE_OK);

    // 3. Check Total Pages
    int total = async::sqlite3_backup_pagecount(pBackup);

    if (total == 0) {
        std::cerr << "Backup Page Count is still 0 after step(0)!" << std::endl;
        std::cerr << "Source Rows: 1000. DB Error: " << async::sqlite3_errmsg(pDest) << std::endl;
    }
    ASSERT_TRUE(total > 0);

    // 4. Check Remaining (Should equal total at start)
    int remaining = async::sqlite3_backup_remaining(pBackup);
    ASSERT_TRUE(remaining == total);

    // 5. Step 1 Page (Actually copy data now)
    rc = async::sqlite3_backup_step(pBackup, 1);

    // Note: If the DB is very small, 1 page might finish it. 
    // So we accept OK (more to do) or DONE (finished).
    ASSERT_TRUE(rc == SQLITE_OK || rc == SQLITE_DONE);

    // 6. Check Remaining Decreased
    int remainingAfter = async::sqlite3_backup_remaining(pBackup);

    if (remainingAfter >= remaining) {
        std::cerr << "Progress stalled. Start: " << remaining << " End: " << remainingAfter << std::endl;
    }
    ASSERT_TRUE(remainingAfter < remaining);

    // 7. Finish
    async::sqlite3_backup_finish(pBackup);

    CloseDB(pDest);
    CloseDB(pSrc);

    // Cleanup
    std::remove(SRC_FILE);
    std::remove(DST_FILE);
}

// -----------------------------------------------------------------------------
// Main Entry Point
// -----------------------------------------------------------------------------
void Advanced_UT()
{
    std::cout << "Running SQLite Advanced (Backup/Serialize) Tests..." << std::endl;

    Test_Adv_Serialize();
    Test_Adv_Backup();
    Test_Adv_BackupProgress();

    std::cout << "SQLite Advanced Tests Passed!" << std::endl;
}