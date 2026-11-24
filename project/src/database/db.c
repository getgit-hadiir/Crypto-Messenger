/*
 * File: db.c
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: Stores encrypted message payloads in SQLite.
 */

#include <sqlite3.h>
#include <stdio.h>
#include "db.h"

static sqlite3 *g_db = NULL;

int db_initialize(const char *path)
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "payload TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");";

    if (sqlite3_open(path, &g_db) != SQLITE_OK) {
        if (g_db) sqlite3_close(g_db);
        g_db = NULL;
        return -1;
    }

    sqlite3_busy_timeout(g_db, 3000);
    char *error = NULL;
    if (sqlite3_exec(g_db, sql, NULL, NULL, &error) != SQLITE_OK) {
        sqlite3_free(error);
        return -1;
    }
    return 0;
}

int db_save_payload(const char *payload)
{
    if (!g_db || !payload) return -1;

    sqlite3_stmt *stmt = NULL;
    const char *sql = "INSERT INTO messages(payload) VALUES(?1);";
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, payload, -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE ? 0 : -1;
}

void db_shutdown(void)
{
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

/*==== End of file db.c ====*/

