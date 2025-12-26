#ifndef MEMBER_H
#define MEMBER_H

#include <cjson/cJSON.h>
#include <mysql/mysql.h>

void handle_update_member(int client, cJSON *data, int user_id, MYSQL *conn);

#endif
