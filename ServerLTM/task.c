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

    int project_id = pid->valueint;

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql =
        "SELECT task_id, task_name, description, assigned_to, status "
        "FROM tasks WHERE project_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_LIST_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[1] = {0};
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

    res[0].buffer_type = MYSQL_TYPE_LONG;   res[0].buffer = &task_id;
    res[1].buffer_type = MYSQL_TYPE_STRING; res[1].buffer = task_name;  res[1].buffer_length = sizeof(task_name);
    res[2].buffer_type = MYSQL_TYPE_STRING; res[2].buffer = description; res[2].buffer_length = sizeof(description);
    res[3].buffer_type = MYSQL_TYPE_LONG;   res[3].buffer = &assigned_to;
    res[4].buffer_type = MYSQL_TYPE_STRING; res[4].buffer = status;     res[4].buffer_length = sizeof(status);

    mysql_stmt_bind_result(stmt, res);

    cJSON *tasks = cJSON_CreateArray();
    while (mysql_stmt_fetch(stmt) == 0) {
        cJSON *task = cJSON_CreateObject();
        cJSON_AddNumberToObject(task, "task_id", task_id);
        cJSON_AddStringToObject(task, "task_name", task_name);
        cJSON_AddStringToObject(task, "description", description);
        cJSON_AddNumberToObject(task, "assigned_to", assigned_to);
        cJSON_AddStringToObject(task, "status", status);
        cJSON_AddItemToArray(tasks, task);
    }

    mysql_stmt_close(stmt);

    cJSON *res_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(res_data, "project_id", project_id);
    cJSON_AddItemToObject(res_data, "tasks", tasks);

    send_json_response(client, RES_LIST_TASK_OK, "List tasks success", res_data);
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

    int project_id = pid->valueint;
    char *description = (desc && cJSON_IsString(desc)) ? desc->valuestring : "";
    char status[] = "in_progress";

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql =
        "INSERT INTO tasks(project_id, task_name, description, status) "
        "VALUES(?,?,?,?)";

    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_CREATE_TASK_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[4] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;   bind[0].buffer = &project_id;
    bind[1].buffer_type = MYSQL_TYPE_STRING; bind[1].buffer = tname->valuestring;
    bind[1].buffer_length = strlen(tname->valuestring);
    bind[2].buffer_type = MYSQL_TYPE_STRING; bind[2].buffer = description;
    bind[2].buffer_length = strlen(description);
    bind[3].buffer_type = MYSQL_TYPE_STRING; bind[3].buffer = status;
    bind[3].buffer_length = strlen(status);

    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_CREATE_TASK_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    int task_id = mysql_insert_id(conn);
    mysql_stmt_close(stmt);

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "project_id", project_id);
    cJSON_AddNumberToObject(res, "task_id", task_id);

    send_json_response(client, RES_CREATE_TASK_OK, "Task created", res);
}

/* ===== ASSIGN TASK ===== */
void handle_assign_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *tid = cJSON_GetObjectItem(data, "task_id");
    cJSON *assign_to = cJSON_GetObjectItem(data, "assigned_to");

    if (!tid || !cJSON_IsNumber(tid) ||
        !assign_to || !cJSON_IsString(assign_to)) {
        send_json_response(client, ERR_ASSIGN_TASK_VAL, "Missing fields", NULL);
        return;
    }

    int task_id = tid->valueint;
    const char *assigned_username = assign_to->valuestring;
    int assigned_user_id = -1;
    int project_id = -1;

    /* ===== 1. Get project_id from task ===== */
    MYSQL_STMT *q = mysql_stmt_init(conn);
    const char *sql_q = "SELECT project_id FROM tasks WHERE task_id=?";
    MYSQL_BIND qb[1] = {0};

    qb[0].buffer_type = MYSQL_TYPE_LONG;
    qb[0].buffer = &task_id;

    mysql_stmt_prepare(q, sql_q, strlen(sql_q));
    mysql_stmt_bind_param(q, qb);
    mysql_stmt_execute(q);

    MYSQL_BIND qr[1] = {0};
    qr[0].buffer_type = MYSQL_TYPE_LONG;
    qr[0].buffer = &project_id;

    mysql_stmt_bind_result(q, qr);
    mysql_stmt_store_result(q);

    if (mysql_stmt_num_rows(q) == 0) {
        mysql_stmt_close(q);
        send_json_response(client, ERR_ASSIGN_TASK_VAL, "Task not found", NULL);
        return;
    }

    mysql_stmt_fetch(q);
    mysql_stmt_close(q);

    /* ===== 2. Check PM permission ===== */
    int my_role = db_get_user_role(conn, user_id, project_id);
    if (my_role != ROLE_PM) {
        send_json_response(client, ERR_ASSIGN_TASK_PERM, "Permission denied", NULL);
        return;
    }

    /* ===== 3. Find user_id from username ===== */
    MYSQL_STMT *su = mysql_stmt_init(conn);
    const char *sql_user =
        "SELECT user_id FROM users WHERE username=?";

    MYSQL_BIND ub[1] = {0};
    ub[0].buffer_type = MYSQL_TYPE_STRING;
    ub[0].buffer = (char*)assigned_username;
    ub[0].buffer_length = strlen(assigned_username);

    mysql_stmt_prepare(su, sql_user, strlen(sql_user));
    mysql_stmt_bind_param(su, ub);
    mysql_stmt_execute(su);

    MYSQL_BIND ur[1] = {0};
    ur[0].buffer_type = MYSQL_TYPE_LONG;
    ur[0].buffer = &assigned_user_id;

    mysql_stmt_bind_result(su, ur);
    mysql_stmt_store_result(su);

    if (mysql_stmt_num_rows(su) == 0) {
        mysql_stmt_close(su);
        send_json_response(client, ERR_ASSIGN_TASK_VAL, "Assigned user not found", NULL);
        return;
    }

    mysql_stmt_fetch(su);
    mysql_stmt_close(su);

    /* ===== 4. Assign task ===== */
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql =
        "UPDATE tasks SET assigned_to=? WHERE task_id=?";

    MYSQL_BIND bind[2] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &assigned_user_id;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &task_id;

    mysql_stmt_prepare(stmt, sql, strlen(sql));
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0 ||
        mysql_stmt_affected_rows(stmt) == 0) {
        mysql_stmt_close(stmt);
        send_json_response(client, ERR_ASSIGN_TASK_SERVER, "Assign failed", NULL);
        return;
    }

    mysql_stmt_close(stmt);

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "task_id", task_id);
    cJSON_AddStringToObject(res, "assigned_to", assigned_username);
    cJSON_AddNumberToObject(res, "assigned_user_id", assigned_user_id);

    send_json_response(client, RES_ASSIGN_TASK_OK, "Task assigned", res);
}

/* ===== UPDATE TASK ===== */
void handle_update_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *tid = cJSON_GetObjectItem(data, "task_id");
    cJSON *status = cJSON_GetObjectItem(data, "status");

    if (!tid || !cJSON_IsNumber(tid) ||
        !status || !cJSON_IsString(status)) {
        send_json_response(client, ERR_UPDATE_TASK_VAL, "Missing fields", NULL);
        return;
    }

    int task_id = tid->valueint;
    char *stat = status->valuestring;
    int project_id = -1;
    int assigned_to = -1;

    /* ===== 1. Get project_id and assignee from task ===== */
    MYSQL_STMT *q = mysql_stmt_init(conn);
    const char *sql_q = "SELECT project_id, assigned_to FROM tasks WHERE task_id=?";
    MYSQL_BIND qb[1] = {0};

    qb[0].buffer_type = MYSQL_TYPE_LONG;
    qb[0].buffer = &task_id;

    mysql_stmt_prepare(q, sql_q, strlen(sql_q));
    mysql_stmt_bind_param(q, qb);
    mysql_stmt_execute(q);

    MYSQL_BIND qr[2] = {0};
    qr[0].buffer_type = MYSQL_TYPE_LONG;
    qr[0].buffer = &project_id;
    qr[1].buffer_type = MYSQL_TYPE_LONG;
    qr[1].buffer = &assigned_to;

    mysql_stmt_bind_result(q, qr);
    mysql_stmt_store_result(q);

    if (mysql_stmt_num_rows(q) == 0) {
        mysql_stmt_close(q);
        send_json_response(client, ERR_UPDATE_TASK_VAL, "Task not found", NULL);
        return;
    }

    mysql_stmt_fetch(q);
    mysql_stmt_close(q);

    /* ===== 2. Permission: PM or assigned user inside project ===== */
    int role = db_get_user_role(conn, user_id, project_id);
    if (role == ROLE_NONE) {
        send_json_response(client, ERR_UPDATE_TASK_PERM, "Permission denied", NULL);
        return;
    }

    if (role != ROLE_PM && assigned_to != user_id) {
        send_json_response(client, ERR_UPDATE_TASK_PERM, "Permission denied", NULL);
        return;
    }

    /* ===== 3. Update task ===== */
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql =
        "UPDATE tasks SET status=? WHERE task_id=?";

    MYSQL_BIND bind[2] = {0};
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = stat;
    bind[0].buffer_length = strlen(stat);
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &task_id;

    mysql_stmt_prepare(stmt, sql, strlen(sql));
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_UPDATE_TASK_SERVER, "Update failed", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "task_id", task_id);
    cJSON_AddStringToObject(res, "status", stat);

    send_json_response(client, RES_UPDATE_TASK_OK, "Task updated", res);
}

/* ===== COMMENT TASK ===== */
void handle_comment_task(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *tid = cJSON_GetObjectItem(data, "task_id");
    cJSON *comment = cJSON_GetObjectItem(data, "comment");

    if (!tid || !cJSON_IsNumber(tid) ||
        !comment || !cJSON_IsString(comment)) {
        send_json_response(client, ERR_COMMENT_TASK_PERM, "Missing fields", NULL);
        return;
    }

    int task_id = tid->valueint;
    char *cmt = comment->valuestring;
    int project_id = -1;

    /* ===== 1. Get project_id from task ===== */
    MYSQL_STMT *q = mysql_stmt_init(conn);
    const char *sql_q = "SELECT project_id FROM tasks WHERE task_id=?";
    MYSQL_BIND qb[1] = {0};

    qb[0].buffer_type = MYSQL_TYPE_LONG;
    qb[0].buffer = &task_id;

    mysql_stmt_prepare(q, sql_q, strlen(sql_q));
    mysql_stmt_bind_param(q, qb);
    mysql_stmt_execute(q);

    MYSQL_BIND qr[1] = {0};
    qr[0].buffer_type = MYSQL_TYPE_LONG;
    qr[0].buffer = &project_id;

    mysql_stmt_bind_result(q, qr);
    mysql_stmt_store_result(q);

    if (mysql_stmt_num_rows(q) == 0) {
        mysql_stmt_close(q);
        send_json_response(client, ERR_COMMENT_TASK_PERM, "Task not found", NULL);
        return;
    }

    mysql_stmt_fetch(q);
    mysql_stmt_close(q);

    /* ===== 2. Check MEMBER permission ===== */
    int role = db_get_user_role(conn, user_id, project_id);
    if (role <= ROLE_NONE) {
        send_json_response(client, ERR_COMMENT_TASK_PERM, "Permission denied", NULL);
        return;
    }

    /* ===== 3. Insert comment ===== */
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql =
        "INSERT INTO task_comments(task_id, user_id, comment) "
        "VALUES(?,?,?)";

    MYSQL_BIND bind[3] = {0};
    bind[0].buffer_type = MYSQL_TYPE_LONG;   bind[0].buffer = &task_id;
    bind[1].buffer_type = MYSQL_TYPE_LONG;   bind[1].buffer = &user_id;
    bind[2].buffer_type = MYSQL_TYPE_STRING; bind[2].buffer = cmt;
    bind[2].buffer_length = strlen(cmt);

    mysql_stmt_prepare(stmt, sql, strlen(sql));
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        send_json_response(client, ERR_COMMENT_TASK_SERVER, "Comment failed", NULL);
        return;
    }

    mysql_stmt_close(stmt);

    cJSON *res = cJSON_CreateObject();
    cJSON_AddNumberToObject(res, "task_id", task_id);

    send_json_response(client, RES_COMMENT_TASK_OK, "Comment added", res);
}

