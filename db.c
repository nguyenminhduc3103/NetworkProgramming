#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DB_HOST "172.31.245.233"
#define DB_USER "root"
#define DB_PASS "31032004"
#define DB_NAME "project_management"
#define DB_PORT 3306

int db_init_library(void) {
    return mysql_library_init(0, NULL, NULL);
}

MYSQL *db_connect(void) {
    MYSQL *conn = mysql_init(NULL);
    if (!conn) return NULL;
    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, DB_PORT, NULL, 0)) {
        fprintf(stderr, "[db] connect error: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }
    mysql_set_character_set(conn, "utf8mb4");
    return conn;
}

void db_close(MYSQL *conn) {
    if (conn) mysql_close(conn);
}

/* Insert a log row into logs table. user_id may be -1 to insert NULL */
void db_log(MYSQL *conn, int user_id, const char *action, const char *request, const char *response_json) {
    if (!conn) return;
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "INSERT INTO logs(user_id, action, request, response) VALUES(?, ?, ?, ?)";
    if (!stmt) return;
    if (mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql)) != 0) {
        mysql_stmt_close(stmt);
        return;
    }
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    bool is_null_user = (user_id == -1);
    if (is_null_user) {
        bind[0].buffer_type = MYSQL_TYPE_NULL;
        bind[0].is_null = &is_null_user;
    } else {
        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = (char *)&user_id;
        bind[0].is_null = 0;
    }
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (char *)action;
    bind[1].buffer_length = strlen(action);
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (char *)request;
    bind[2].buffer_length = strlen(request);
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (char *)response_json;
    bind[3].buffer_length = strlen(response_json);

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);
}
