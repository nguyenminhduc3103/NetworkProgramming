#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_BUF 4096

// -----------------------------
// MOCK DATA
// -----------------------------

int PROJECT_COUNT = 2;
struct Project {
    int id;
    char name[64];
    char desc[256];
} PROJECTS[100] = {
    {1, "Project Alpha", "Demo project A"},
    {2, "Project Beta", "Demo project B"},
};

int TASK_COUNT = 3;
struct Task {
    int id;
    int project_id;
    char name[64];
    char status[32];
    char description[256];
    char assignee[64];
} TASKS[200] = {
    {1, 1, "Fix bug", "todo", "Fix login bug", "alice"},
    {2, 1, "Write docs", "in_progress", "Document APIs", "bob"},
    {3, 2, "Deploy", "done", "Deploy to server", "charlie"},
};


// -----------------------------
// Helper: check substring
// -----------------------------
int contains(const char *s, const char *sub) {
    return strstr(s, sub) != NULL;
}


// -----------------------------
// MAIN: Loop nhận JSON
// -----------------------------
int main() {
    char buffer[MAX_BUF];

    while (1) {
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            continue;
        }

        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;

        // -----------------------------
        // ACTION: LOGIN
        // -----------------------------
        if (contains(buffer, "\"action\":\"login\"")) {
            printf("{\"status\":\"code(ok)\",\"data\":{\"session\":\"S123456789\",\"user\":\"demo\"}}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // LIST PROJECTS
        // -----------------------------
        if (contains(buffer, "\"action\":\"list_projects\"")) {
            printf("{\"status\":\"code(ok)\",\"data\":{\"projects\":[");
            for (int i = 0; i < PROJECT_COUNT; i++) {
                printf("{\"id\":%d,\"name\":\"%s\"}", PROJECTS[i].id, PROJECTS[i].name);
                if (i < PROJECT_COUNT - 1) printf(",");
            }
            printf("]}}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // SEARCH PROJECT
        // -----------------------------
        if (contains(buffer, "\"action\":\"search_project\"")) {
            printf("{\"status\":\"code(ok)\",\"data\":{\"projects\":[");
            printf("{\"id\":1,\"name\":\"Project Alpha\"}");
            printf("]}}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // CREATE PROJECT
        // -----------------------------
        if (contains(buffer, "\"action\":\"create_project\"")) {
            PROJECT_COUNT++;
            PROJECTS[PROJECT_COUNT - 1].id = PROJECT_COUNT;
            strcpy(PROJECTS[PROJECT_COUNT - 1].name, "New Project");
            strcpy(PROJECTS[PROJECT_COUNT - 1].desc, "Mock desc");

            printf("{\"status\":\"code(ok)\",\"data\":{\"id\":%d}}\n",
                   PROJECT_COUNT);
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // LIST TASKS
        // -----------------------------
        if (contains(buffer, "\"action\":\"list_tasks\"")) {
            printf("{\"status\":\"code(ok)\",\"data\":{\"tasks\":[");
            int first = 1;
            for (int i = 0; i < TASK_COUNT; i++) {
                if (!first) printf(",");
                first = 0;

                printf(
                    "{\"id\":%d,\"name\":\"%s\",\"status\":\"%s\","
                    "\"description\":\"%s\",\"assignee\":\"%s\"}",
                    TASKS[i].id, TASKS[i].name, TASKS[i].status,
                    TASKS[i].description, TASKS[i].assignee
                );
            }
            printf("]}}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // UPDATE TASK STATUS
        // -----------------------------
        if (contains(buffer, "\"action\":\"update_task_status\"")) {
            printf("{\"status\":\"code(ok)\",\"message\":\"updated\"}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // CREATE TASK
        // -----------------------------
        if (contains(buffer, "\"action\":\"create_task\"")) {
            TASK_COUNT++;
            int id = TASK_COUNT;

            printf("{\"status\":\"code(ok)\",\"data\":{\"id\":%d}}\n", id);
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // ASSIGN TASK
        // -----------------------------
        if (contains(buffer, "\"action\":\"assign_task\"")) {
            printf("{\"status\":\"code(ok)\",\"message\":\"assigned\"}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // ADD MEMBER
        // -----------------------------
        if (contains(buffer, "\"action\":\"add_member\"")) {
            printf("{\"status\":\"code(ok)\",\"message\":\"member_added\"}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // COMMENT TASK
        // -----------------------------
        if (contains(buffer, "\"action\":\"comment_task\"")) {
            printf("{\"status\":\"code(ok)\",\"message\":\"comment_added\"}\n");
            fflush(stdout);
            continue;
        }

        // -----------------------------
        // UNKNOWN
        // -----------------------------
        printf("{\"status\":\"error\",\"message\":\"unknown_action\"}\n");
        fflush(stdout);
    }

    return 0;
}
