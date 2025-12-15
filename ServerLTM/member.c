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

    // Check PM role for current user
    int my_role = db_get_user_role(conn, user_id, project_id);
    if (my_role != ROLE_PM) {
        send_json_response(client, ERR_MEMBER_PERMISSION, "Permission denied", NULL);
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

    if (mysql_stmt_execute(stmt) != 0 || mysql_stmt_affected_rows(stmt) == 0) {
        send_json_response(client, ERR_MEMBER_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_UPDATE_MEMBER_OK, "Member role updated", NULL);
}
