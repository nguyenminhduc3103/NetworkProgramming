#include "task.h"
#include "utils.h"
#include "db.h"
#include "response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>

/* ===== LIST TASKS ===== */
void handle_list_tasks(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *pid = cJSON_GetObjectItem(data, "project_id");
    if (!pid || !cJSON_IsNumber(pid)) {
        send_json_response(client, ERR_PROJECT_VALIDATE, "Missing project_id", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "SELECT task_id, task_name, description, assigned_to, status FROM tasks WHERE project_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_LIST_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    int project_id = pid->valueint;
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &project_id;
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_LIST_TASK_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_store_result(stmt);
    MYSQL_BIND res[5];
    int task_id, assigned_to;
    char task_name[256], description[1024], status[32];
    memset(res, 0, sizeof(res));
    res[0].buffer_type = MYSQL_TYPE_LONG; res[0].buffer = &task_id;
    res[1].buffer_type = MYSQL_TYPE_STRING; res[1].buffer = task_name; res[1].buffer_length = sizeof(task_name);
    res[2].buffer_type = MYSQL_TYPE_STRING; res[2].buffer = description; res[2].buffer_length = sizeof(description);
    res[3].buffer_type = MYSQL_TYPE_LONG; res[3].buffer = &assigned_to;
    res[4].buffer_type = MYSQL_TYPE_STRING; res[4].buffer = status; res[4].buffer_length = sizeof(status);

    mysql_stmt_bind_result(stmt, res);

    cJSON *arr = cJSON_CreateArray();
    while (mysql_stmt_fetch(stmt) == 0) {
        cJSON *task = cJSON_CreateObject();
        cJSON_AddNumberToObject(task, "task_id", task_id);
        cJSON_AddStringToObject(task, "task_name", task_name);
        cJSON_AddStringToObject(task, "description", description);
        cJSON_AddNumberToObject(task, "assigned_to", assigned_to);
        cJSON_AddStringToObject(task, "status", status);
        cJSON_AddItemToArray(arr, task);
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_LIST_TASK_OK, "List tasks success", arr);
}

/* ===== CREATE TASK ===== */
void handle_create_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *pid = cJSON_GetObjectItem(data, "project_id");
    cJSON *tname = cJSON_GetObjectItem(data, "task_name");
    cJSON *desc = cJSON_GetObjectItem(data, "description");

    if (!pid || !cJSON_IsNumber(pid) || !tname || !cJSON_IsString(tname)) {
        send_json_response(client, ERR_CREATE_TASK_VAL, "Missing fields", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "INSERT INTO tasks(project_id, task_name, description, status) VALUES(?,?,?,?)";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_CREATE_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    char *description = desc && cJSON_IsString(desc) ? desc->valuestring : "";
    char status[] = "in_progress";

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    int project_id = pid->valueint;
    bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &project_id;
    bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = tname->valuestring; bind[1].buffer_length = strlen(tname->valuestring);
    bind[2].buffer_type = MYSQL_TYPE_STRING; bind[2].buffer = description; bind[2].buffer_length = strlen(description);
    bind[3].buffer_type = MYSQL_TYPE_STRING; bind[3].buffer = status; bind[3].buffer_length = strlen(status);

    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_CREATE_TASK_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_CREATE_TASK_OK, "Task created", NULL);
}

/* ===== ASSIGN TASK ===== */
void handle_assign_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *tid = cJSON_GetObjectItem(data, "task_id");
    cJSON *assign_to = cJSON_GetObjectItem(data, "assigned_to");

    if (!tid || !cJSON_IsNumber(tid) || !assign_to || !cJSON_IsNumber(assign_to)) {
        send_json_response(client, ERR_ASSIGN_TASK_VAL, "Missing fields", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "UPDATE tasks SET assigned_to=? WHERE task_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_ASSIGN_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    int task_id = tid->valueint;
    int assigned_user = assign_to->valueint;
    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &assigned_user;
    bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &task_id;
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0 || mysql_stmt_affected_rows(stmt) == 0) {
        send_json_response(client, ERR_ASSIGN_TASK_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_ASSIGN_TASK_OK, "Task assigned", NULL);
}

/* ===== UPDATE TASK ===== */
void handle_update_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *tid = cJSON_GetObjectItem(data, "task_id");
    cJSON *status = cJSON_GetObjectItem(data, "status");

    if (!tid || !cJSON_IsNumber(tid) || !status || !cJSON_IsString(status)) {
        send_json_response(client, ERR_UPDATE_TASK_VAL, "Missing fields", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "UPDATE tasks SET status=? WHERE task_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_UPDATE_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    int task_id = tid->valueint;
    char *stat = status->valuestring;

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = stat; bind[0].buffer_length = strlen(stat);
    bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &task_id;
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0 || mysql_stmt_affected_rows(stmt) == 0) {
        send_json_response(client, ERR_UPDATE_TASK_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_UPDATE_TASK_OK, "Task updated", NULL);
}

/* ===== COMMENT TASK ===== */
void handle_comment_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *tid = cJSON_GetObjectItem(data, "task_id");
    cJSON *comment = cJSON_GetObjectItem(data, "comment");

    if (!tid || !cJSON_IsNumber(tid) || !comment || !cJSON_IsString(comment)) {
        send_json_response(client, ERR_COMMENT_TASK_SERVER, "Missing fields", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "INSERT INTO task_comments(task_id, user_id, comment) VALUES(?,?,?)";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_COMMENT_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    int task_id = tid->valueint;
    char *cmt = comment->valuestring;

    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG; bind[0].buffer = &task_id;
    bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &user_id;
    bind[2].buffer_type = MYSQL_TYPE_STRING; bind[2].buffer = cmt; bind[2].buffer_length = strlen(cmt);
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_COMMENT_TASK_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_COMMENT_TASK_OK, "Comment added", NULL);
}
