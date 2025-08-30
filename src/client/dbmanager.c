#include <sched.h>
#include <sqlite3.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../shared/common.h"


char* get_full_path(char* path) {
    const char *home = getenv("HOME");
    if (!home) {
        home = getpwuid(getuid())->pw_dir;
    }

    char *full_path = malloc(strlen(home) + strlen(path));
    sprintf(full_path, "%s%s", home, path);

    return full_path;
}


int dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        return 1;
    }
    return 0;
}

void init_appdir() {
    char *path = get_full_path("/.local/share/w1re");
    if (dir_exists(path)) {
        free(path);
        return;
    }

    if (mkdir(path, 0755) != 0) {
        printf(COLOR_RED "[ERR] Failed to create app directory %s" COLOR_RESET, path);
        free(path);
        return;
    }

    char *db_path = get_full_path("/.local/share/w1re/identity.db");
    sqlite3 *db;
    char *err_msg = 0;

    if (sqlite3_open(db_path, &db)) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to open database: %s\n" COLOR_RESET, sqlite3_errmsg(db));
        free(db_path);
        free(path);
        return;
    }

    const char *create_table = "CREATE TABLE \"identity\" (\
    	\"id\" INTEGER,\
    	\"private_key\" BLOB NOT NULL,\
    	\"name\" TEXT,\
    	\"username_prefix\" TEXT,\
    	PRIMARY KEY(\"id\" AUTOINCREMENT)\
    );";

    if (sqlite3_exec(db, create_table, 0, 0, &err_msg) != SQLITE_OK) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to create database: %s\n" COLOR_RESET, err_msg);
        sqlite3_close(db);
        free(db_path);
        free(path);
        return;
    }

    sqlite3_close(db);
    printf(COLOR_GREEN "[INFO] Created app directory in %s" COLOR_RESET, path);
    free(db_path);
    free(path);

    char *res_dir = get_full_path("/.local/share/w1re/resources");
    if (!dir_exists(res_dir)) {
        if (mkdir(res_dir, 0755) != 0) {
            printf(COLOR_RED "[ERR] Failed to create resources directory %s\n" COLOR_RESET, res_dir);
            free(res_dir);
            return;
        }
    }

    char *style_path = get_full_path("/.local/share/w1re/resources/style.css");
    FILE* style_src = fopen("resources/style.css", "rb");
    if (style_src == NULL) {
        printf(COLOR_RED "[ERR] Failed to copy file resources/style.css to %s\n" COLOR_RESET, style_path);
    }

    FILE* style_dest = fopen(style_path, "wb");
    if (style_src == NULL) {
        printf(COLOR_RED "[ERR] Failed to copy file resources/style.css to %s\n" COLOR_RESET, style_path);
    }

    uint8_t buffer[2048];
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), style_src)) > 0) {
        fwrite(buffer, 1, bytesRead, style_dest);
    }

    fclose(style_src);
    fclose(style_dest);

    printf(COLOR_GREEN "[INFO] Copied resources/style.css to %s\n" COLOR_RESET, style_path);

    free(res_dir);
    free(style_path);
}


void get_identity(uint8_t privkey[32], uint8_t name[64], uint8_t prefix[17]) {
    char *db_path = get_full_path("/.local/share/w1re/identity.db");
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open(db_path, &db)) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to open database: %s\n" COLOR_RESET, sqlite3_errmsg(db));
        free(db_path);
        return;
    }

    const char *sql = "SELECT private_key, name, username_prefix FROM identity LIMIT 1";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to prepare statment: %s\n" COLOR_RESET, sqlite3_errmsg(db));
        sqlite3_close(db);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const uint8_t *privkey_raw = sqlite3_column_blob(stmt, 0);
        const uint8_t *name_raw = sqlite3_column_text(stmt, 1);
        const uint8_t *prefix_raw = sqlite3_column_text(stmt, 2);

        for (uint8_t i = 0; i < 32; i++) {
            privkey[i] = privkey_raw[i];
        }

        for (size_t i = 0; i < strlen((char*)name_raw); i++) {
            name[i] = name_raw[i];
        }
        name[strlen((char*)name_raw)] = '\0';

        for (size_t i = 0; i < strlen((char*)prefix_raw); i++) {
            prefix[i] = prefix_raw[i];
        }
        prefix[strlen((char*)prefix_raw)] = '\0';
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

void save_identity(uint8_t privkey[32], uint8_t name[64], uint8_t id[32]) {
    char *db_path = get_full_path("/.local/share/w1re/identity.db");
    sqlite3 *db;
    sqlite3_stmt *stmt;

    if (sqlite3_open(db_path, &db)) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to open database: %s\n" COLOR_RESET, sqlite3_errmsg(db));
        free(db_path);
        return;
    }

    const char *sql = "INSERT INTO identity (private_key, name, username_prefix) VALUES (?, ?, ?);";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to prepare statment: %s\n" COLOR_RESET, sqlite3_errmsg(db));
        sqlite3_close(db);
        free(db_path);
        return;
    }

    char prefix[17] = {0};
    uint8_t count = 0;
    for (uint8_t i = 0; i < 17; i++) {
        count++;
        if (id[i] == '#') {break;}
        prefix[i] = id[i];
    }
    prefix[count] = '\0';

    sqlite3_bind_blob(stmt, 1, privkey, 32, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, (char*)name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, prefix, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, COLOR_RED "[ERR] Failed to save identity: %s\n" COLOR_RESET, sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}