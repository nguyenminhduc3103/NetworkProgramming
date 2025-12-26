#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>

#define ROLE_NONE   0
#define ROLE_PM     1
#define ROLE_MEMBER 2

int db_init_library(void);
MYSQL *db_connect(void);
void db_close(MYSQL *conn);
void db_log(MYSQL *conn, int user_id, const char *action, const char *request, const char *response_json);

//Session and Role Management
int db_check_session(MYSQL *conn, const char *token);
int db_get_user_role(MYSQL *conn, int user_id, int project_id);

#endif
