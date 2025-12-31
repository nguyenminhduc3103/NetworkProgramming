#include "project.h"
#include "utils.h"
#include "db.h"
#include "response.h"
#include <string.h>
#include <stdio.h>

/* ===== LIST PROJECTS ===== */
void handle_list_projects(int client, int user_id, MYSQL *conn) {
    if (user_id <= 0) {
        send_json_response(client, ERR_PROJECT_VALIDATE, "Invalid session/token", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "SELECT p.project_id, p.project_name, p.description "
                      "FROM projects p "
                      "JOIN project_members pm ON p.project_id = pm.project_id "
                      "WHERE pm.user_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        send_json_response(client, ERR_PROJECT_SERVER, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[1];
    memset(bind,0,sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &user_id;
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_PROJECT_SERVER, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_store_result(stmt);
    if (mysql_stmt_num_rows(stmt) == 0) {
        mysql_stmt_close(stmt);
        send_json_response(client, ERR_PROJECT_NOT_FOUND, "No projects found", NULL);
        return;
    }

    MYSQL_BIND res[3];
    int pid;
    char name[256], desc[1024];
    memset(res,0,sizeof(res));
    res[0].buffer_type = MYSQL_TYPE_LONG; res[0].buffer = &pid;
    res[1].buffer_type = MYSQL_TYPE_STRING; res[1].buffer = name; res[1].buffer_length=sizeof(name);
    res[2].buffer_type = MYSQL_TYPE_STRING; res[2].buffer = desc; res[2].buffer_length=sizeof(desc);
    mysql_stmt_bind_result(stmt,res);

    cJSON *arr = cJSON_CreateArray();
    while(mysql_stmt_fetch(stmt)==0){
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p,"project_id",pid);
        cJSON_AddStringToObject(p,"project_name",name);
        cJSON_AddStringToObject(p,"description",desc);
        cJSON_AddItemToArray(arr,p);
    }
    mysql_stmt_close(stmt);

    send_json_response(client, RES_LIST_PROJECT_OK, "Projects retrieved", arr);
}

/* ===== SEARCH PROJECT ===== */
void handle_search_project(int client, cJSON *data, int user_id, MYSQL *conn) {
    if (user_id <= 0) {
        send_json_response(client, ERR_SEARCH_PERMISSION, "Invalid session/token", NULL);
        return;
    }

    cJSON *kw = cJSON_GetObjectItem(data, "keyword");
    if (!kw || !cJSON_IsString(kw)) {
        send_json_response(client, ERR_PROJECT_VALIDATE, "Missing keyword", NULL);
        return;
    }

    char query[512];
    snprintf(query,sizeof(query),
             "SELECT p.project_id,p.project_name,p.description "
             "FROM projects p "
             "JOIN project_members pm ON p.project_id=pm.project_id "
             "WHERE pm.user_id=%d AND p.project_name LIKE '%%%s%%'", user_id, kw->valuestring);

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, query, strlen(query)) != 0) {
        send_json_response(client, ERR_SEARCH_PROJECT_SRV, "Server error", NULL);
        if (stmt) mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt) != 0) {
        send_json_response(client, ERR_SEARCH_PROJECT_SRV, "Server error", NULL);
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_store_result(stmt);
    if (mysql_stmt_num_rows(stmt) == 0) {
        mysql_stmt_close(stmt);
        send_json_response(client, ERR_SEARCH_NOT_FOUND, "No projects found", NULL);
        return;
    }

    MYSQL_BIND res[3];
    int pid;
    char name[256], desc[1024];
    memset(res,0,sizeof(res));
    res[0].buffer_type = MYSQL_TYPE_LONG; res[0].buffer = &pid;
    res[1].buffer_type = MYSQL_TYPE_STRING; res[1].buffer = name; res[1].buffer_length=sizeof(name);
    res[2].buffer_type = MYSQL_TYPE_STRING; res[2].buffer = desc; res[2].buffer_length=sizeof(desc);
    mysql_stmt_bind_result(stmt,res);

    cJSON *arr = cJSON_CreateArray();
    while(mysql_stmt_fetch(stmt)==0){
        cJSON *p = cJSON_CreateObject();
        cJSON_AddNumberToObject(p,"project_id",pid);
        cJSON_AddStringToObject(p,"project_name",name);
        cJSON_AddStringToObject(p,"description",desc);
        cJSON_AddItemToArray(arr,p);
    }
    mysql_stmt_close(stmt);

    send_json_response(client, RES_SEARCH_PROJECT_OK, "Search results", arr);
}

/* ===== CREATE PROJECT ===== */
void handle_create_project(int client, cJSON *data, int user_id, MYSQL *conn) {
    if (user_id <= 0) {
        send_json_response(client, ERR_CREATE_PROJECT_VAL, "Invalid session/token", NULL);
        return;
    }

    cJSON *name = cJSON_GetObjectItem(data,"project_name");
    cJSON *desc = cJSON_GetObjectItem(data,"description");

    if (!name || !cJSON_IsString(name) || strlen(name->valuestring)==0) {
        send_json_response(client, ERR_CREATE_PROJECT_VAL, "Missing project_name", NULL);
        return;
    }

    // check conflict
    MYSQL_STMT *chk = mysql_stmt_init(conn);
    const char *sql_chk = "SELECT project_id FROM projects WHERE project_name=?";
    if (!chk || mysql_stmt_prepare(chk,sql_chk,strlen(sql_chk))!=0){
        if (chk) mysql_stmt_close(chk);
        send_json_response(client, ERR_CREATE_PROJECT_SRV, "Server error", NULL);
        return;
    }
    MYSQL_BIND bind_chk[1];
    memset(bind_chk,0,sizeof(bind_chk));
    bind_chk[0].buffer_type = MYSQL_TYPE_STRING;
    bind_chk[0].buffer = name->valuestring;
    bind_chk[0].buffer_length = strlen(name->valuestring);
    mysql_stmt_bind_param(chk,bind_chk);
    if (mysql_stmt_execute(chk)!=0){
        mysql_stmt_close(chk);
        send_json_response(client, ERR_CREATE_PROJECT_SRV,"Server error",NULL);
        return;
    }
    mysql_stmt_store_result(chk);
    if(mysql_stmt_num_rows(chk)>0){
        mysql_stmt_close(chk);
        send_json_response(client, ERR_PROJECT_CONFLICT,"Project name exists",NULL);
        return;
    }
    mysql_stmt_close(chk);

    // insert project
    MYSQL_STMT *ins = mysql_stmt_init(conn);
    const char *sql_ins="INSERT INTO projects(project_name,description) VALUES(?,?)";
    if (!ins || mysql_stmt_prepare(ins,sql_ins,strlen(sql_ins))!=0){
        if(ins) mysql_stmt_close(ins);
        send_json_response(client, ERR_CREATE_PROJECT_SRV,"Server error",NULL);
        return;
    }
    MYSQL_BIND bind_ins[2];
    memset(bind_ins,0,sizeof(bind_ins));
    bind_ins[0].buffer_type = MYSQL_TYPE_STRING;
    bind_ins[0].buffer = name->valuestring;
    bind_ins[0].buffer_length = strlen(name->valuestring);
    bind_ins[1].buffer_type = MYSQL_TYPE_STRING;
    if(desc && cJSON_IsString(desc)){
        bind_ins[1].buffer = desc->valuestring;
        bind_ins[1].buffer_length = strlen(desc->valuestring);
    } else {
        bind_ins[1].buffer_type = MYSQL_TYPE_NULL;
    }
    mysql_stmt_bind_param(ins,bind_ins);
    if(mysql_stmt_execute(ins)!=0){
        mysql_stmt_close(ins);
        send_json_response(client, ERR_CREATE_PROJECT_SRV,"Server error",NULL);
        return;
    }
    int proj_id = mysql_stmt_insert_id(ins);
    mysql_stmt_close(ins);

    // add creator as PM
    MYSQL_STMT *pm = mysql_stmt_init(conn);
    const char *sql_pm="INSERT INTO project_members(project_id,user_id,role) VALUES(?,?,?)";
    if (!pm || mysql_stmt_prepare(pm,sql_pm,strlen(sql_pm))!=0){
        if(pm) mysql_stmt_close(pm);
        send_json_response(client, ERR_CREATE_PROJECT_SRV,"Server error",NULL);
        return;
    }
    int role_val = ROLE_PM;  // Insert numeric role, not string
    MYSQL_BIND bind_pm[3];
    memset(bind_pm,0,sizeof(bind_pm));
    bind_pm[0].buffer_type = MYSQL_TYPE_LONG; bind_pm[0].buffer = &proj_id;
    bind_pm[1].buffer_type = MYSQL_TYPE_LONG; bind_pm[1].buffer = &user_id;
    bind_pm[2].buffer_type = MYSQL_TYPE_LONG; bind_pm[2].buffer = &role_val;  // Changed to LONG
    mysql_stmt_bind_param(pm,bind_pm);
    if(mysql_stmt_execute(pm)!=0){
        mysql_stmt_close(pm);
        send_json_response(client, ERR_CREATE_PROJECT_SRV,"Server error",NULL);
        return;
    }
    mysql_stmt_close(pm);

    send_json_response(client, RES_CREATE_PROJECT_OK,"Project created successfully",NULL);
}

/* ===== ADD MEMBER ===== */
void handle_add_member(int client, cJSON *data, int user_id, MYSQL *conn) {
    if (user_id <= 0) {
        send_json_response(client, ERR_MEMBER_VALIDATE, "Invalid session/token", NULL);
        return;
    }

    cJSON *pid   = cJSON_GetObjectItem(data, "project_id");
    cJSON *uname = cJSON_GetObjectItem(data, "username");
    cJSON *role  = cJSON_GetObjectItem(data, "role");

    if (!pid || !cJSON_IsNumber(pid) ||
        !uname || !cJSON_IsString(uname) ||
        !role || !cJSON_IsString(role)) {
        send_json_response(client, ERR_ADD_MEMBER_VAL, "Missing required fields", NULL);
        return;
    }

    int project_id = pid->valueint;

    /* ===== MAP ROLE STRING -> INT ===== */
    int role_val;
    if (strcmp(role->valuestring, "PM") == 0) {
        role_val = ROLE_PM;
    } else if (strcmp(role->valuestring, "MEMBER") == 0) {
        role_val = ROLE_MEMBER;
    } else {
        send_json_response(client, ERR_MEMBER_VALIDATE, "Invalid role", NULL);
        return;
    }

    /* ===== CHECK PM PERMISSION ===== */
    int my_role = db_get_user_role(conn, user_id, project_id);
    if (my_role != ROLE_PM) {
        send_json_response(client, ERR_MEMBER_PERMISSION, "Permission denied", NULL);
        return;
    }

    /* ===== FIND USER_ID BY USERNAME ===== */
    MYSQL_STMT *chk = mysql_stmt_init(conn);
    const char *sql_chk = "SELECT user_id FROM users WHERE username=?";
    MYSQL_BIND bind_chk[1];
    int new_user_id;

    memset(bind_chk, 0, sizeof(bind_chk));
    bind_chk[0].buffer_type   = MYSQL_TYPE_STRING;
    bind_chk[0].buffer        = uname->valuestring;
    bind_chk[0].buffer_length = strlen(uname->valuestring);

    mysql_stmt_prepare(chk, sql_chk, strlen(sql_chk));
    mysql_stmt_bind_param(chk, bind_chk);
    mysql_stmt_execute(chk);

    MYSQL_BIND res[1];
    memset(res, 0, sizeof(res));
    res[0].buffer_type = MYSQL_TYPE_LONG;
    res[0].buffer      = &new_user_id;

    mysql_stmt_bind_result(chk, res);
    mysql_stmt_store_result(chk);

    if (mysql_stmt_num_rows(chk) == 0) {
        mysql_stmt_close(chk);
        send_json_response(client, ERR_USER_NOT_FOUND, "User not found", NULL);
        return;
    }

    mysql_stmt_fetch(chk);
    mysql_stmt_close(chk);

    /* ===== CHECK MEMBER CONFLICT ===== */
    MYSQL_STMT *conf = mysql_stmt_init(conn);
    const char *sql_conf =
        "SELECT 1 FROM project_members WHERE project_id=? AND user_id=?";
    MYSQL_BIND bind_conf[2];

    memset(bind_conf, 0, sizeof(bind_conf));
    bind_conf[0].buffer_type = MYSQL_TYPE_LONG;
    bind_conf[0].buffer      = &project_id;
    bind_conf[1].buffer_type = MYSQL_TYPE_LONG;
    bind_conf[1].buffer      = &new_user_id;

    mysql_stmt_prepare(conf, sql_conf, strlen(sql_conf));
    mysql_stmt_bind_param(conf, bind_conf);
    mysql_stmt_execute(conf);
    mysql_stmt_store_result(conf);

    if (mysql_stmt_num_rows(conf) > 0) {
        mysql_stmt_close(conf);
        send_json_response(client, ERR_ADD_MEMBER_CONFLICT,
                           "User already member", NULL);
        return;
    }
    mysql_stmt_close(conf);

    /* ===== INSERT MEMBER ===== */
    MYSQL_STMT *ins = mysql_stmt_init(conn);
    const char *sql_ins =
        "INSERT INTO project_members (project_id, user_id, role) VALUES (?,?,?)";
    MYSQL_BIND bind_ins[3];

    memset(bind_ins, 0, sizeof(bind_ins));
    bind_ins[0].buffer_type = MYSQL_TYPE_LONG;
    bind_ins[0].buffer      = &project_id;
    bind_ins[1].buffer_type = MYSQL_TYPE_LONG;
    bind_ins[1].buffer      = &new_user_id;
    bind_ins[2].buffer_type = MYSQL_TYPE_LONG;
    bind_ins[2].buffer      = &role_val;

    mysql_stmt_prepare(ins, sql_ins, strlen(sql_ins));
    mysql_stmt_bind_param(ins, bind_ins);

    if (mysql_stmt_execute(ins) != 0) {
        send_json_response(client, ERR_ADD_MEMBER_SERVER,
                           mysql_error(conn), NULL);
        mysql_stmt_close(ins);
        return;
    }
    mysql_stmt_close(ins);

    send_json_response(client, RES_ADD_MEMBER_OK,
                       "Member added successfully", NULL);
}

/* ===== LIST MEMBERS ===== */
void handle_list_members(int client, cJSON *data, int user_id, MYSQL *conn) {
    if (user_id <= 0) {
        send_json_response(client, ERR_MEMBER_VALIDATE, "Invalid session/token", NULL);
        return;
    }

    cJSON *pid_json = cJSON_GetObjectItem(data, "project_id");
    if (!pid_json || !cJSON_IsNumber(pid_json)) {
        send_json_response(client, ERR_MEMBER_VALIDATE, "Missing project_id", NULL);
        return;
    }

    int project_id = pid_json->valueint;

    // Kiểm tra quyền của user: chỉ PM hoặc MEMBER mới được xem
    int my_role = db_get_user_role(conn, user_id, project_id);
    if (my_role != ROLE_PM && my_role != ROLE_MEMBER) {
        send_json_response(client, ERR_MEMBER_PERMISSION, "Permission denied", NULL);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    const char *sql = "SELECT u.user_id, u.username, pm.role "
                      "FROM project_members pm "
                      "JOIN users u ON pm.user_id = u.user_id "
                      "WHERE pm.project_id=?";
    if (!stmt || mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
        if (stmt) mysql_stmt_close(stmt);
        send_json_response(client, ERR_PROJECT_SERVER, "Server error", NULL);
        return;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer = &project_id;
    mysql_stmt_bind_param(stmt, bind);

    if (mysql_stmt_execute(stmt) != 0) {
        mysql_stmt_close(stmt);
        send_json_response(client, ERR_PROJECT_SERVER, "Server error", NULL);
        return;
    }

    mysql_stmt_store_result(stmt);
    if (mysql_stmt_num_rows(stmt) == 0) {
        mysql_stmt_close(stmt);
        send_json_response(client, ERR_MEMBER_NOT_FOUND, "No members found", NULL);
        return;
    }

    MYSQL_BIND res[3];
    int uid, role_val;
    char username[256];
    memset(res, 0, sizeof(res));
    res[0].buffer_type = MYSQL_TYPE_LONG; res[0].buffer = &uid;
    res[1].buffer_type = MYSQL_TYPE_STRING; res[1].buffer = username; res[1].buffer_length = sizeof(username);
    res[2].buffer_type = MYSQL_TYPE_LONG; res[2].buffer = &role_val;
    mysql_stmt_bind_result(stmt, res);

    cJSON *arr = cJSON_CreateArray();
    while (mysql_stmt_fetch(stmt) == 0) {
        cJSON *m = cJSON_CreateObject();
        cJSON_AddNumberToObject(m, "user_id", uid);
        cJSON_AddStringToObject(m, "username", username);
        const char *role_str = (role_val == ROLE_PM) ? "PM" : "MEMBER";
        cJSON_AddStringToObject(m, "role", role_str);
        cJSON_AddItemToArray(arr, m);
    }

    mysql_stmt_close(stmt);
    send_json_response(client, RES_LIST_MEMBER_OK, "Members retrieved", arr);
}

