#ifndef UTILS_H
#define UTILS_H

#include <cjson/cJSON.h>

/* Read line until CRLF, returns malloc'd buffer or NULL */
char* read_line_crlf_dynamic(int client);

void send_json_response(int client, const char *status, const char *message, cJSON *dataobj);
void write_server_log(const char *fmt, ...);

#endif