#ifndef UTILS_H
#define UTILS_H

#include <cjson/cJSON.h>

int read_line_crlf(int client, char *buf, int max);
void send_json_response(int client, const char *status, const char *message, cJSON *dataobj);
void write_server_log(const char *fmt, ...);

#endif
