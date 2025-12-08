#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>

#include <cjson/cJSON.h>
#include "utils.h"
#include "db.h"
#include "auth.h"

#define PORT 8080
#define BACKLOG 16
#define BUF_SIZE 8192

/* thread routine */
void *client_thread(void *arg) {
    int client = *(int*)arg;
    free(arg);

    char buf[BUF_SIZE];
    int n = read_line_crlf(client, buf, sizeof(buf));
    if (n <= 0) {
        close(client);
        return NULL;
    }

    write_server_log("[client] received: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        send_json_response(client, "131", "Invalid JSON", NULL);
        write_server_log("[client] invalid JSON");
        close(client);
        return NULL;
    }

    cJSON *action = cJSON_GetObjectItem(root, "action");
    cJSON *data = cJSON_GetObjectItem(root, "data");
    if (!action || !cJSON_IsString(action) || !data || !cJSON_IsObject(data)) {
        send_json_response(client, "131", "Missing action or data", NULL);
        cJSON_Delete(root);
        close(client);
        return NULL;
    }

    MYSQL *conn = db_connect();
    if (!conn) {
        send_json_response(client, "501", "Database connection failed", NULL);
        cJSON_Delete(root);
        close(client);
        return NULL;
    }

    const char *act = action->valuestring;
    if (strcmp(act, "register") == 0) {
        handle_register_request(client, data, conn);
    } else if (strcmp(act, "login") == 0) {
        handle_login_request(client, data, conn);
    } else {
        send_json_response(client, "141", "Session conflict", NULL);
    }

    db_close(conn);
    cJSON_Delete(root);
    close(client);
    return NULL;
}

int main() {
    if (db_init_library() != 0) {
        fprintf(stderr, "mysql_library_init failed\n");
        return 1;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(sock, BACKLOG) < 0) { perror("listen"); return 1; }

    write_server_log("Server start on port %d", PORT);
    printf("Server start on port %d\n", PORT);
    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);
        int *client = malloc(sizeof(int));
        *client = accept(sock, (struct sockaddr*)&cli, &clilen);
        if (*client < 0) { free(client); continue; }

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, client) != 0) {
            perror("pthread_create");
            close(*client);
            free(client);
            continue;
        }
        pthread_detach(tid);
    }

    mysql_library_end();
    close(sock);
    return 0;
}
