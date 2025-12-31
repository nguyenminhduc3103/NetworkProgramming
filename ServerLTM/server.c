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
#include "response.h"
#include "project.h"
#include "task.h"
#include "member.h"

#define PORT 8080
#define BACKLOG 16
#define BUF_SIZE 8192

// Check role base on project_id
static int require_role(
    int client,
    MYSQL *conn,
    int user_id,
    cJSON *data,
    int required_role,
    const char *err_perm,
    const char *err_val
) {
    cJSON *pid = cJSON_GetObjectItem(data, "project_id");
    if (!pid || !cJSON_IsNumber(pid)) {
        send_json_response(client, err_val, "Missing project_id", NULL);
        return -1;
    }

    int role = db_get_user_role(conn, user_id, pid->valueint);
    if (role < required_role) {
        send_json_response(client, err_perm, "Permission denied", NULL);
        return -1;
    }
    return pid->valueint;
}

// Thread routine
void *client_thread(void *arg) {
    int client = *(int*)arg;
    free(arg);

    MYSQL *conn = db_connect();
    if (!conn) {
        close(client);
        return NULL;
    }

    char buf[BUF_SIZE];
    int user_id = -1;

    while (1) {
        int n = read_line_crlf(client, buf, sizeof(buf));
        if (n <= 0) break;

        cJSON *root = cJSON_Parse(buf);
        if (!root) {
            send_json_response(client, S_LOGIN_ERR, "Invalid JSON", NULL);
            continue; 
        }

        cJSON *action = cJSON_GetObjectItem(root, "action");
        cJSON *data   = cJSON_GetObjectItem(root, "data");

        if (!action || !cJSON_IsString(action) || !data || !cJSON_IsObject(data)) {
            send_json_response(client, S_LOGIN_ERR, "Missing action or data", NULL);
            cJSON_Delete(root);
            continue;
        }

        const char *act = action->valuestring;

        // Check session for actions other than login/register
        if (strcmp(act, "login") != 0 && strcmp(act, "register") != 0) {
            cJSON *session = cJSON_GetObjectItem(root, "session");
            if (!session || !cJSON_IsString(session)) {
                send_json_response(client, S_LOGIN_PERM, "Missing session", NULL);
                cJSON_Delete(root);
                continue;
            }

            user_id = db_check_session(conn, session->valuestring);
            if (user_id < 0) {
                send_json_response(client, S_LOGIN_PERM, "Invalid session", NULL);
                cJSON_Delete(root);
                continue;
            }
        }

        /* ===== Dispatch ===== */
        if (strcmp(act, "register") == 0) {
            handle_register_request(client, data, conn);
        }
        else if (strcmp(act, "login") == 0) {
            handle_login_request(client, data, conn);
        }
        else if (strcmp(act, "list_projects") == 0) {
            handle_list_projects(client, user_id, conn);
        }
        else if (strcmp(act, "search_project") == 0) {
            handle_search_project(client, data, user_id, conn);
        }
        else if (strcmp(act, "create_project") == 0) {
            handle_create_project(client, data, user_id, conn);
        }
        else if (strcmp(act, "add_member") == 0) {
            if (require_role(client, conn, user_id, data, ROLE_PM,
                ERR_ADD_MEMBER_PERM, ERR_ADD_MEMBER_VAL) < 0) {
                cJSON_Delete(root);
                continue;
            }
            handle_add_member(client, data, user_id, conn);
        }
        else if (strcmp(act, "list_members") == 0) {
            if (!data || !cJSON_IsObject(data)) {
                send_json_response(client, ERR_MEMBER_VALIDATE, "Missing data", NULL);
                cJSON_Delete(root);
                continue;
            }

            handle_list_members(client, data, user_id, conn);
        }
        else if (strcmp(act, "list_tasks") == 0) {
            cJSON *pid = cJSON_GetObjectItem(data, "project_id");
            if (!pid || !cJSON_IsNumber(pid)) {
                send_json_response(client, ERR_PROJECT_VALIDATE, "Missing project_id", NULL);
                cJSON_Delete(root);
                continue;
            }

            int role = db_get_user_role(conn, user_id, pid->valueint);
            if (role == ROLE_NONE) {
                send_json_response(client, ERR_SEARCH_PERMISSION, "Permission denied", NULL);
                cJSON_Delete(root);
                continue;
            }

            handle_list_tasks(client, data, user_id, conn);
        }
        else if (strcmp(act, "create_task") == 0) {
            if (require_role(client, conn, user_id, data, ROLE_PM,
                ERR_CREATE_TASK_PERM, ERR_CREATE_TASK_VAL) < 0) {
                cJSON_Delete(root);
                continue;
            }
            handle_create_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "assign_task") == 0) {
            handle_assign_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "update_task") == 0) {
            handle_update_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "comment_task") == 0) {
            handle_comment_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "update_member") == 0) {
            if (require_role(client, conn, user_id, data, ROLE_PM,
                ERR_MEMBER_PERMISSION, ERR_MEMBER_VALIDATE) < 0) {
                cJSON_Delete(root);
                continue;
            }
            handle_update_member(client, data, user_id, conn);
        }
        else {
            send_json_response(client, S_LOGIN_ERR, "Unknown action", NULL);
        }

        cJSON_Delete(root);
    }

    db_close(conn);
    close(client);
    return NULL;
}

// Main
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

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    if (listen(sock, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    write_server_log("Server start on port %d", PORT);
    printf("Server start on port %d\n", PORT);

    srand((unsigned)time(NULL) ^ (unsigned)getpid());

    while (1) {
        struct sockaddr_in cli;
        socklen_t clilen = sizeof(cli);

        int *client = malloc(sizeof(int));
        *client = accept(sock, (struct sockaddr*)&cli, &clilen);
        if (*client < 0) {
            free(client);
            continue;
        }

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
