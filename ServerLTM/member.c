#include "member.h"
#include "utils.h"
#include "db.h"
#include "response.h"
#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>

/* ===== UPDATE MEMBER ===== */
void handle_update_member(int client, cJSON *data, int user_id, MYSQL *conn) {
    cJSON *pid = cJSON_GetObjectItem(data, "project_id");
    cJSON *target_uid = cJSON_GetObjectItem(data, "user_id");
    cJSON *role_obj = cJSON_GetObjectItem(data, "role");

    if (!pid || !cJSON_IsNumber(pid) || !target_uid || !cJSON_IsNumber(target_uid) ||
        !role_obj || !cJSON_IsString(role_obj)) {
        send_json_response(client, ERR_MEMBER_VALIDATE, "Missing or invalid fields", NULL);
        return;
    }

    int project_id = pid->valueint;
    int target_user = target_uid->valueint;
    char *role = role_obj->valuestring;

    if (strcmp(role, "PM") != 0 && strcmp(role, "MEMBER") != 0) {
        send_json_response(client, ERR_MEMBER_VALIDATE, "Invalid role", NULL);
        return;
    }

    // Check PM role for current user
    int my_role = db_get_user_role(conn, user_id, project_id);
    if (my_role != ROLE_PM) {
        send_json_response(client, ERR_MEMBER_PERMISSION, "Permission denied", NULL);
        return;
    }

    // Check existing member and current role to avoid treating no-change as server error
    MYSQL_STMT *chk = mysql_stmt_init(conn);
    const char *sql_chk = "SELECT role FROM project_members WHERE project_id=? AND user_id=?";
    MYSQL_BIND bind_chk[2];
    memset(bind_chk, 0, sizeof(bind_chk));
    bind_chk[0].buffer_type = MYSQL_TYPE_LONG; bind_chk[0].buffer = &project_id;
    bind_chk[1].buffer_type = MYSQL_TYPE_LONG; bind_chk[1].buffer = &target_user;

    if (!chk || mysql_stmt_prepare(chk, sql_chk, strlen(sql_chk)) != 0) {
        send_json_response(client, ERR_MEMBER_SERVER, "Server error", NULL);
        if (chk) mysql_stmt_close(chk);
        return;
    }
    mysql_stmt_bind_param(chk, bind_chk);
    if (mysql_stmt_execute(chk) != 0) {
        send_json_response(client, ERR_MEMBER_SERVER, "Server error", NULL);
        mysql_stmt_close(chk);
        return;
    }
    mysql_stmt_store_result(chk);
    if (mysql_stmt_num_rows(chk) == 0) {
        mysql_stmt_close(chk);
        send_json_response(client, ERR_USER_NOT_FOUND, "Member not in project", NULL);
        return;
    }

    char current_role[16];
    MYSQL_BIND res_chk[1];
    memset(res_chk, 0, sizeof(res_chk));
    res_chk[0].buffer_type = MYSQL_TYPE_STRING; res_chk[0].buffer = current_role; res_chk[0].buffer_length = sizeof(current_role);
    mysql_stmt_bind_result(chk, res_chk);
    mysql_stmt_fetch(chk);
    mysql_stmt_close(chk);

    if (strcmp(current_role, role) == 0) {
        send_json_response(client, RES_UPDATE_MEMBER_OK, "Role unchanged", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "UPDATE project_members SET role=? WHERE project_id=? AND user_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_MEMBER_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING; bind[0].buffer = role; bind[0].buffer_length = strlen(role);
    bind[1].buffer_type = MYSQL_TYPE_LONG; bind[1].buffer = &project_id;
    bind[2].buffer_type = MYSQL_TYPE_LONG; bind[2].buffer = &target_user;
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_MEMBER_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_UPDATE_MEMBER_OK, "Member role updated", NULL);
}
