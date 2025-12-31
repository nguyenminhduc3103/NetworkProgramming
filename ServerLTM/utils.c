#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#define SERVER_LOG_FILE "server.log"
#define BUFFER_SIZE 8192

void write_server_log(const char *fmt, ...) {
    FILE *f = fopen(SERVER_LOG_FILE, "a");
    if (!f) return;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char ts[64];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(f, "[%s] ", ts);

    // make arg pointer to handle variable arguments
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);

    fprintf(f, "\n");
    fclose(f);
}

/* Read until CRLF (\r\n). Return malloc'd buffer or NULL on error/close */
char* read_line_crlf_dynamic(int client) {
    int capacity = 256;
    int len = 0;
    char *buf = (char*)malloc(capacity);
    if (!buf) return NULL;
    
    char c;
    while (1) {
        ssize_t r = read(client, &c, 1);
        if (r == 0 || r < 0) {
            free(buf);
            return NULL;
        }
        
        if (c == '\r') {
            /* expect \n next */
            r = read(client, &c, 1);
            if (r <= 0) {
                free(buf);
                return NULL;
            }
            if (c != '\n') {
                free(buf);
                return NULL;
            }
            buf[len] = '\0';
            return buf;
        }
        
        buf[len++] = c;
        
        /* Expand buffer if needed */
        if (len >= capacity - 1) {
            capacity *= 2;
            char *new_buf = (char*)realloc(buf, capacity);
            if (!new_buf) {
                free(buf);
                return NULL;
            }
            buf = new_buf;
        }
    }
}

/* Send JSON object with CRLF. dataobj may be NULL (empty {}). */
void send_json_response(int client, const char *status, const char *message, cJSON *dataobj) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", status);
    cJSON_AddStringToObject(root, "message", message);
    if (dataobj) cJSON_AddItemToObject(root, "data", dataobj);
    else cJSON_AddItemToObject(root, "data", cJSON_CreateObject());

    char *out = cJSON_PrintUnformatted(root);
    if (out) {
        int json_len = strlen(out);
        /* Send JSON + CRLF */
        char *sendbuf = (char*)malloc(json_len + 3);
        if (sendbuf) {
            memcpy(sendbuf, out, json_len);
            sendbuf[json_len] = '\r';
            sendbuf[json_len + 1] = '\n';
            sendbuf[json_len + 2] = '\0';
            write(client, sendbuf, json_len + 2);
            free(sendbuf);
        }
        free(out);
    }
    cJSON_Delete(root);
}
