/*
 * File: db.h
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: SQLite persistence interface.
 */

#ifndef DB_H
#define DB_H

int db_initialize(const char *path);
int db_save_payload(const char *payload);
void db_shutdown(void);

#endif

/*==== End of file db.h ====*/
