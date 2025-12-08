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

/* Read until CRLF (\r\n). Return length (excl CRLF) or -1 on error/close */
int read_line_crlf(int client, char *buf, int max) {
    int i = 0;
    char c;
    while (i < max - 1) {
        ssize_t r = read(client, &c, 1);
        if (r == 0) return -1;
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (c == '\r') {
            /* expect \n next */
            r = read(client, &c, 1);
            if (r <= 0) return -1;
            if (c != '\n') return -1;
            buf[i] = '\0';
            return i;
        }
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
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
        char sendbuf[BUFFER_SIZE];
        int n = snprintf(sendbuf, sizeof(sendbuf), "%s\r\n", out);
        write(client, sendbuf, n);
        free(out);
    }
    cJSON_Delete(root);
}
