#ifndef AUTH_H
#define AUTH_H

#include <mysql/mysql.h>
#include <cjson/cJSON.h>

void handle_register_request(int client, cJSON *data, MYSQL *conn);
void handle_login_request(int client, cJSON *data, MYSQL *conn);

#endif