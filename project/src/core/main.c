/*
 * File: main.c
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: Application entry point and subsystem initialization.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include "app.h"
#include "db.h"
#include "http_server.h"

int main(void)
{
    if (mkdir("data", 0700) != 0) {
        struct stat st;
        if (stat("data", &st) != 0 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Unable to create data directory.\n");
            return 1;
        }
    }

    if (db_initialize(DB_PATH) != 0) {
        fprintf(stderr, "Unable to initialize SQLite database.\n");
        return 1;
    }

    signal(SIGPIPE, SIG_IGN);
    printf("%s %s starting on http://127.0.0.1:%d\n",
           APP_NAME, APP_VERSION, APP_PORT);

    int result = http_server_run();
    db_shutdown();
    return result == 0 ? 0 : 1;
}

/*==== End of file main.c ====*/
