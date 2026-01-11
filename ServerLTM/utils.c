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
    time_t t = time(NULL) + 7 * 3600;
    struct tm *tm = gmtime(&t);
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
    int cap = 1024, len = 0;
    char *buf = malloc(cap);
    if (!buf) return NULL;

    while (1) {
        ssize_t r = read(client, buf + len, cap - len - 1);
        if (r <= 0) break;

        len += r;
        buf[len] = '\0';

        char *eol = strstr(buf, "\r\n");
        if (eol) {
            *eol = '\0';
            return buf;
        }

        if (len > cap / 2) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
    }

    free(buf);
    return NULL;
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
            /* Log the response status and message for auditing */
            write_server_log("[RESPONSE] status=%s message=%s", status, message);
            free(sendbuf);
        }
        free(out);
    }
    cJSON_Delete(root);
}
