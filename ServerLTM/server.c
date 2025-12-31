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

// Client info struct
typedef struct {
    int socket;
    char ip_addr[64];
    int port;
} ClientInfo;

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
    ClientInfo *client_info = (ClientInfo *)arg;
    int client = client_info->socket;
    char *client_ip = client_info->ip_addr;
    int client_port = client_info->port;
    free(client_info);

    MYSQL *conn = db_connect();
    if (!conn) {
        close(client);
        return NULL;
    }

    char buf[BUF_SIZE];
    int user_id = -1;
    char username[128] = "anonymous";

    while (1) {
        char *line = read_line_crlf_dynamic(client);
        if (!line) break;

        cJSON *root = cJSON_Parse(line);
        free(line);
        
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
            
            // Get username from user_id for logging
            char query[256];
            snprintf(query, sizeof(query), "SELECT username FROM users WHERE user_id = %d", user_id);
            if (mysql_query(conn, query) == 0) {
                MYSQL_RES *res = mysql_store_result(conn);
                if (res) {
                    MYSQL_ROW row = mysql_fetch_row(res);
                    if (row) {
                        strncpy(username, row[0], sizeof(username) - 1);
                        username[sizeof(username) - 1] = '\0';
                    }
                    mysql_free_result(res);
                }
            }
        }

        /* ===== Dispatch ===== */
        if (strcmp(act, "register") == 0) {
            cJSON *un = cJSON_GetObjectItem(data, "username");
            const char *uname = (un && cJSON_IsString(un)) ? un->valuestring : "unknown";
            write_server_log("[DISPATCH] action=register username=%s", uname);
            handle_register_request(client, data, conn);
        }
        else if (strcmp(act, "login") == 0) {
            cJSON *un = cJSON_GetObjectItem(data, "username");
            const char *uname = (un && cJSON_IsString(un)) ? un->valuestring : "unknown";
            write_server_log("[DISPATCH] action=login username=%s", uname);
            handle_login_request(client, data, conn);
        }
        else if (strcmp(act, "list_projects") == 0) {
            write_server_log("[DISPATCH] action=list_projects user_id=%d username=%s", user_id, username);
            handle_list_projects(client, user_id, conn);
        }
        else if (strcmp(act, "search_project") == 0) {
            cJSON *pname = cJSON_GetObjectItem(data, "project_name");
            const char *pn = (pname && cJSON_IsString(pname)) ? pname->valuestring : "";
            write_server_log("[DISPATCH] action=search_project user_id=%d search_term=%s", user_id, pn);
            handle_search_project(client, data, user_id, conn);
        }
        else if (strcmp(act, "create_project") == 0) {
            cJSON *pname = cJSON_GetObjectItem(data, "name");
            const char *pn = (pname && cJSON_IsString(pname)) ? pname->valuestring : "unknown";
            write_server_log("[DISPATCH] action=create_project user_id=%d project_name=%s", user_id, pn);
            handle_create_project(client, data, user_id, conn);
        }
        else if (strcmp(act, "add_member") == 0) {
            cJSON *pmem = cJSON_GetObjectItem(data, "username");
            const char *pmn = (pmem && cJSON_IsString(pmem)) ? pmem->valuestring : "";
            cJSON *prole = cJSON_GetObjectItem(data, "role");
            const char *pr = (prole && cJSON_IsString(prole)) ? prole->valuestring : "";
            write_server_log("[DISPATCH] action=add_member user_id=%d new_member=%s role=%s", user_id, pmn, pr);
            if (require_role(client, conn, user_id, data, ROLE_PM,
                ERR_ADD_MEMBER_PERM, ERR_ADD_MEMBER_VAL) < 0) {
                write_server_log("[DISPATCH] add_member DENIED - permission denied");
                cJSON_Delete(root);
                continue;
            }
            handle_add_member(client, data, user_id, conn);
        }
        else if (strcmp(act, "list_members") == 0) {
            cJSON *pid = cJSON_GetObjectItem(data, "project_id");
            int proj_id = (pid && cJSON_IsNumber(pid)) ? pid->valueint : -1;
            write_server_log("[DISPATCH] action=list_members user_id=%d project_id=%d", user_id, proj_id);
            if (!data || !cJSON_IsObject(data)) {
                send_json_response(client, ERR_MEMBER_VALIDATE, "Missing data", NULL);
                cJSON_Delete(root);
                continue;
            }

            handle_list_members(client, data, user_id, conn);
        }
        else if (strcmp(act, "list_tasks") == 0) {
            cJSON *pid = cJSON_GetObjectItem(data, "project_id");
            int proj_id = (pid && cJSON_IsNumber(pid)) ? pid->valueint : -1;
            write_server_log("[DISPATCH] action=list_tasks user_id=%d project_id=%d", user_id, proj_id);
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
            cJSON *tname = cJSON_GetObjectItem(data, "task_name");
            const char *tn = (tname && cJSON_IsString(tname)) ? tname->valuestring : "unknown";
            cJSON *pid = cJSON_GetObjectItem(data, "project_id");
            int proj_id = (pid && cJSON_IsNumber(pid)) ? pid->valueint : -1;
            write_server_log("[DISPATCH] action=create_task user_id=%d project_id=%d task_name=%s", user_id, proj_id, tn);
            if (require_role(client, conn, user_id, data, ROLE_PM,
                ERR_CREATE_TASK_PERM, ERR_CREATE_TASK_VAL) < 0) {
                write_server_log("[DISPATCH] create_task DENIED - permission denied");
                cJSON_Delete(root);
                continue;
            }
            handle_create_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "assign_task") == 0) {
            cJSON *tid = cJSON_GetObjectItem(data, "task_id");
            cJSON *ato = cJSON_GetObjectItem(data, "assigned_to");
            int task_id = (tid && cJSON_IsNumber(tid)) ? tid->valueint : -1;
            int assigned = (ato && cJSON_IsNumber(ato)) ? ato->valueint : -1;
            write_server_log("[DISPATCH] action=assign_task user_id=%d task_id=%d assigned_to_user=%d", user_id, task_id, assigned);
            handle_assign_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "update_task") == 0) {
            cJSON *tid = cJSON_GetObjectItem(data, "task_id");
            cJSON *status = cJSON_GetObjectItem(data, "status");
            int task_id = (tid && cJSON_IsNumber(tid)) ? tid->valueint : -1;
            const char *st = (status && cJSON_IsString(status)) ? status->valuestring : "";
            write_server_log("[DISPATCH] action=update_task user_id=%d task_id=%d new_status=%s", user_id, task_id, st);
            handle_update_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "comment_task") == 0) {
            cJSON *tid = cJSON_GetObjectItem(data, "task_id");
            int task_id = (tid && cJSON_IsNumber(tid)) ? tid->valueint : -1;
            write_server_log("[DISPATCH] action=comment_task user_id=%d task_id=%d", user_id, task_id);
            handle_comment_task(client, data, user_id, conn);
        }
        else if (strcmp(act, "get_task_detail") == 0) {
            cJSON *tid = cJSON_GetObjectItem(data, "task_id");
            int task_id = (tid && cJSON_IsNumber(tid)) ? tid->valueint : -1;
            write_server_log("[DISPATCH] action=get_task_detail user_id=%d task_id=%d", user_id, task_id);
            handle_get_task_detail(client, data, user_id, conn);
        }
        else if (strcmp(act, "update_member") == 0) {
            cJSON *pmem = cJSON_GetObjectItem(data, "username");
            cJSON *prole = cJSON_GetObjectItem(data, "role");
            const char *pmn = (pmem && cJSON_IsString(pmem)) ? pmem->valuestring : "";
            const char *pr = (prole && cJSON_IsString(prole)) ? prole->valuestring : "";
            cJSON *pid = cJSON_GetObjectItem(data, "project_id");
            int proj_id = (pid && cJSON_IsNumber(pid)) ? pid->valueint : -1;
            write_server_log("[DISPATCH] action=update_member user_id=%d project_id=%d member=%s new_role=%s", user_id, proj_id, pmn, pr);
            if (require_role(client, conn, user_id, data, ROLE_PM,
                ERR_MEMBER_PERMISSION, ERR_MEMBER_VALIDATE) < 0) {
                write_server_log("[DISPATCH] update_member DENIED - permission denied");
                cJSON_Delete(root);
                continue;
            }
            handle_update_member(client, data, user_id, conn);
        }
        else {
            write_server_log("[DISPATCH] unknown action: %s", act);
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

        ClientInfo *client_info = malloc(sizeof(ClientInfo));
        client_info->socket = accept(sock, (struct sockaddr*)&cli, &clilen);
        if (client_info->socket < 0) {
            free(client_info);
            continue;
        }

        // Get client IP and port
        inet_ntop(AF_INET, &cli.sin_addr, client_info->ip_addr, sizeof(client_info->ip_addr));
        client_info->port = ntohs(cli.sin_port);
        
        write_server_log("Client connected");

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, client_info) != 0) {
            perror("pthread_create");
            close(client_info->socket);
            free(client_info);
            continue;
        }
        pthread_detach(tid);
    }

    mysql_library_end();
    close(sock);
    return 0;
}
