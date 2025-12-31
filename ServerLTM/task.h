#ifndef TASK_H
#define TASK_H

#include <cjson/cJSON.h>
#include <mysql/mysql.h>

void handle_list_tasks(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_create_task(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_assign_task(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_update_task(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_comment_task(int client, cJSON *data, int user_id, MYSQL *conn);
void handle_get_task_detail(int client, cJSON *data, int user_id, MYSQL *conn);

#endif
