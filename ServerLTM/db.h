#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>

int db_init_library(void);
MYSQL *db_connect(void);
void db_close(MYSQL *conn);
void db_log(MYSQL *conn, int user_id, const char *action, const char *request, const char *response_json);

#endif
