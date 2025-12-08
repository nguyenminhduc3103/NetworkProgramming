#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cjson/cJSON.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define BUFFER_SIZE 4096

int send_to_server(const char *json_str, char *response) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sock);
        return -2;
    }

    send(sock, json_str, strlen(json_str), 0);

    int len = recv(sock, response, BUFFER_SIZE - 1, 0);
    if (len < 0) {
        close(sock);
        return -3;
    }

    response[len] = '\0';
    close(sock);
    return 0;
}

// --------------------------------------------
// MAIN LOOP: stdin → call server → in stdout
// --------------------------------------------
int main() {
    char line[256];
    char response[BUFFER_SIZE];

    char session[128] = "";

    printf("Client C ready. Waiting for Python input...\n");
    fflush(stdout);

    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = 0;

        // CreateJson
        cJSON *req = cJSON_CreateObject();
        cJSON_AddStringToObject(req, "action", line);
        cJSON_AddStringToObject(req, "session", session);

        cJSON *data = cJSON_CreateObject();
        cJSON_AddItemToObject(req, "data", data);

        char *raw_json = cJSON_PrintUnformatted(req);

        int status = send_to_server(raw_json, response);
        free(raw_json);
        cJSON_Delete(req);

        if (status != 0) {
            printf("{\"status\":\"error\",\"message\":\"TCP error\"}\n");
            fflush(stdout);
            continue;
        }

        // login → save session
        cJSON *resp_json = cJSON_Parse(response);
        if (resp_json) {
            cJSON *status_item = cJSON_GetObjectItem(resp_json, "status");
            if (status_item && strcmp(line, "login") == 0 &&
                strcmp(status_item->valuestring, "code(ok)") == 0) {
                
                cJSON *data_obj = cJSON_GetObjectItem(resp_json, "data");
                if (data_obj) {
                    cJSON *session_item = cJSON_GetObjectItem(data_obj, "session");
                    if (session_item) {
                        strcpy(session, session_item->valuestring);
                    }
                }
            }
            cJSON_Delete(resp_json);
        }

        printf("%s\n", response);
        fflush(stdout);
    }

    return 0;
}
