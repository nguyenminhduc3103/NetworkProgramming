#ifndef PROJECT_H
#define PROJECT_H

#include <mysql/mysql.h>
#include <cjson/cJSON.h>

/* Project handlers */
void handle_list_projects(int client, int user_id, MYSQL *conn);
void handle_search_project(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_create_project(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_add_member(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_list_members(int client, cJSON *data, int user_id, MYSQL *conn);

#endif
