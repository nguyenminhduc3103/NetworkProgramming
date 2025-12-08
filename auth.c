#include "auth.h"
#include "utils.h"
#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* Status codes (strings) */
#define S_LOGIN_OK    "101"
#define S_LOGIN_ERR   "131"
#define S_LOGIN_PERM  "141"
#define S_LOGIN_NF    "151"
#define S_LOGIN_CFL   "161"
#define S_LOGIN_SRV   "501"

#define S_REG_OK      "102"
#define S_REG_ERR     "132"
#define S_REG_CFL     "162"
#define S_REG_SRV     "502"

/* generate 32-char hex token */
static void gen_token(char *buf, size_t n) {
    const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 32 && i < (int)(n-1); ++i) buf[i] = hex[rand() % 16];
    buf[32] = '\0';
}

/* safe get string from cJSON object, return NULL if not present or not string */
static const char *cj_str(cJSON *obj, const char *key) {
    cJSON *v = cJSON_GetObjectItem(obj, key);
    if (!v || !cJSON_IsString(v)) return NULL;
    return v->valuestring;
}

/* REGISTER: use prepared statement, check conflict, insert user, respond and log */
void handle_register_request(int client, cJSON *data, MYSQL *conn) {
    const char *username = cj_str(data, "username");
    const char *password = cj_str(data, "password");

    if (!username || !password || strlen(username) == 0 || strlen(password) == 0) {
        send_json_response(client, S_REG_ERR, "Invalid input", NULL);
        db_log(conn, -1, "register", "{\"error\":\"invalid_input\"}", "{\"status\":\"132\"}");
        write_server_log("[register] invalid input");
        return;
    }

    /* check exists */
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sel_sql = "SELECT user_id FROM users WHERE username = ?";
    if (!stmt || mysql_stmt_prepare(stmt, sel_sql, strlen(sel_sql)) != 0) {
        send_json_response(client, S_REG_SRV, "Server error", NULL);
        db_log(conn, -1, "register", username, "{\"status\":\"502\"}");
        write_server_log("[register] prepare/select failed");
        if (stmt) mysql_stmt_close(stmt);
        return;
    }
    MYSQL_BIND bind_sel[1];
    memset(bind_sel, 0, sizeof(bind_sel));
    bind_sel[0].buffer_type = MYSQL_TYPE_STRING;
    bind_sel[0].buffer = (char*)username;
    bind_sel[0].buffer_length = strlen(username);
    mysql_stmt_bind_param(stmt, bind_sel);
    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, S_REG_SRV, "Server error", NULL);
        db_log(conn, -1, "register", username, "{\"status\":\"502\"}");
        mysql_stmt_close(stmt);
        return;
    }
    mysql_stmt_store_result(stmt);
    my_ulonglong rows = mysql_stmt_num_rows(stmt);
    mysql_stmt_close(stmt);

    if (rows > 0) {
        send_json_response(client, S_REG_CFL, "Username exists", NULL);
        db_log(conn, -1, "register", username, "{\"status\":\"162\"}");
        write_server_log("[register] username exists: %s", username);
        return;
    }

    /* insert user for new register */
    MYSQL_STMT *ins = mysql_stmt_init(conn);
    const char *ins_sql = "INSERT INTO users(username, password) VALUES(?, ?)";
    if (!ins || mysql_stmt_prepare(ins, ins_sql, strlen(ins_sql)) != 0) {
        send_json_response(client, S_REG_SRV, "Server error", NULL);
        db_log(conn, -1, "register", username, "{\"status\":\"502\"}");
        if (ins) mysql_stmt_close(ins);
        return;
    }
    MYSQL_BIND bind_ins[2];
    memset(bind_ins, 0, sizeof(bind_ins));
    bind_ins[0].buffer_type = MYSQL_TYPE_STRING;
    bind_ins[0].buffer = (char*)username;
    bind_ins[0].buffer_length = strlen(username);
    bind_ins[1].buffer_type = MYSQL_TYPE_STRING;
    bind_ins[1].buffer = (char*)password;
    bind_ins[1].buffer_length = strlen(password);
    mysql_stmt_bind_param(ins, bind_ins);

    if (mysql_stmt_execute(ins) != 0) {
        send_json_response(client, S_REG_SRV, "Server error", NULL);
        db_log(conn, -1, "register", username, "{\"status\":\"502\"}");
        write_server_log("[register] insert failed: %s", mysql_error(conn));
        mysql_stmt_close(ins);
        return;
    }
    mysql_stmt_close(ins);

    send_json_response(client, S_REG_OK, "Register successful", NULL);
    db_log(conn, -1, "register", username, "{\"status\":\"102\"}");
    write_server_log("[register] created user: %s", username);
}

/* LOGIN: verify password, create session token, store in sessions table, return token */
void handle_login_request(int client, cJSON *data, MYSQL *conn) {
    const char *username = cj_str(data, "username");
    const char *password = cj_str(data, "password");

    if (!username || !password) {
        send_json_response(client, S_LOGIN_ERR, "Missing input", NULL);
        db_log(conn, -1, "login", "{\"error\":\"missing_input\"}", "{\"status\":\"131\"}");
        write_server_log("[login] missing input");
        return;
    }

    /* select user_id if exists */
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sel_sql = "SELECT user_id FROM users WHERE username = ? AND password = ?";
    if (!stmt || mysql_stmt_prepare(stmt, sel_sql, strlen(sel_sql)) != 0) {
        send_json_response(client, S_LOGIN_SRV, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        db_log(conn, -1, "login", username, "{\"status\":\"501\"}");
        return;
    }
    MYSQL_BIND bindp[2];
    memset(bindp, 0, sizeof(bindp));
    bindp[0].buffer_type = MYSQL_TYPE_STRING;
    bindp[0].buffer = (char*)username;
    bindp[0].buffer_length = strlen(username);
    bindp[1].buffer_type = MYSQL_TYPE_STRING;
    bindp[1].buffer = (char*)password;
    bindp[1].buffer_length = strlen(password);
    mysql_stmt_bind_param(stmt, bindp);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, S_LOGIN_SRV, "Server error", NULL);
        mysql_stmt_close(stmt);
        db_log(conn, -1, "login", username, "{\"status\":\"501\"}");
        return;
    }

    mysql_stmt_store_result(stmt);
    if (mysql_stmt_num_rows(stmt) == 0) {
        send_json_response(client, S_LOGIN_NF, "Invalid login", NULL);
        db_log(conn, -1, "login", username, "{\"status\":\"151\"}");
        mysql_stmt_close(stmt);
        write_server_log("[login] invalid credentials: %s", username);
        return;
    }

    /* bind result */
    MYSQL_BIND res_bind[1];
    memset(res_bind, 0, sizeof(res_bind));
    int user_id = 0;
    res_bind[0].buffer_type = MYSQL_TYPE_LONG;
    res_bind[0].buffer = (char*)&user_id;
    res_bind[0].is_null = 0;
    mysql_stmt_bind_result(stmt, res_bind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);

    /* generate token */
    char token[33];
    gen_token(token, sizeof(token));

    /* delete old sessions for user and insert new one */
    MYSQL_STMT *del = mysql_stmt_init(conn);
    const char *del_sql = "DELETE FROM sessions WHERE user_id = ?";
    if (del && mysql_stmt_prepare(del, del_sql, strlen(del_sql)) == 0) {
        MYSQL_BIND bdel[1];
        memset(bdel,0,sizeof(bdel));
        bdel[0].buffer_type = MYSQL_TYPE_LONG;
        bdel[0].buffer = (char*)&user_id;
        mysql_stmt_bind_param(del, bdel);
        mysql_stmt_execute(del);
        mysql_stmt_close(del);
    } else if (del) mysql_stmt_close(del);

    MYSQL_STMT *ins = mysql_stmt_init(conn);
    const char *ins_sql = "INSERT INTO sessions(user_id, token) VALUES(?, ?)";
    if (!ins || mysql_stmt_prepare(ins, ins_sql, strlen(ins_sql)) != 0) {
        send_json_response(client, S_LOGIN_SRV, "Server error", NULL);
        if (ins) mysql_stmt_close(ins);
        db_log(conn, user_id, "login", username, "{\"status\":\"501\"}");
        return;
    }
    MYSQL_BIND bins[2];
    memset(bins,0,sizeof(bins));
    bins[0].buffer_type = MYSQL_TYPE_LONG;
    bins[0].buffer = (char*)&user_id;
    bins[1].buffer_type = MYSQL_TYPE_STRING;
    bins[1].buffer = token;
    bins[1].buffer_length = strlen(token);
    mysql_stmt_bind_param(ins, bins);

    if (mysql_stmt_execute(ins) != 0) {
        send_json_response(client, S_LOGIN_SRV, "Server error", NULL);
        mysql_stmt_close(ins);
        db_log(conn, user_id, "login", username, "{\"status\":\"501\"}");
        return;
    }
    mysql_stmt_close(ins);

    cJSON *d = cJSON_CreateObject();
    cJSON_AddStringToObject(d, "session", token);
    send_json_response(client, S_LOGIN_OK, "Login successful", d);
    db_log(conn, user_id, "login", username, "{\"status\":\"101\"}");
    write_server_log("[login] user %d logged in (%s)", user_id, username);
}
