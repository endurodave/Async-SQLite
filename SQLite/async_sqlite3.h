#ifndef ASYNC_SQLITE3_H
#define ASYNC_SQLITE3_H

#include "sqlite3.h"

// Asynchronous SQLite API that to invoke from a single thread of control.
// All API functions are thread-safe and can be called from any thread of control.

namespace async
{
    // Call one-time at application startup
    void sqlite3_init_async(void);

    SQLITE_API int sqlite3_open(
        const char* filename,   /* Database filename (UTF-8) */
        sqlite3** ppDb          /* OUT: SQLite db handle */
    );

    SQLITE_API int sqlite3_exec(
        sqlite3* db,                /* The database on which the SQL executes */
        const char* zSql,           /* The SQL to be executed */
        sqlite3_callback xCallback, /* Invoke this callback routine */
        void* pArg,                 /* First argument to xCallback() */
        char** pzErrMsg             /* Write error messages here */
    );

    SQLITE_API int sqlite3_close(sqlite3* db);

    // TODO: Add more sqlite async API's as necessary
}

#endif