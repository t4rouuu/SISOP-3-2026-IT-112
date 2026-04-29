#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "protocol.h"

typedef struct {
    int fd;
    char name[50];
    int logged_in;
    int is_admin;
    int waiting_password;
} Client;

Client clients[MAX_CLIENTS];
time_t start_time;

/* ========================= */
void reset_client(Client *c) {
    c->fd = 0;
    c->logged_in = 0;
    c->is_admin = 0;
    c->waiting_password = 0;
    memset(c->name, 0, sizeof(c->name));
}

/* ========================= */
void trim(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

/* ========================= */
void log_event(char *type, char *msg) {
    FILE *fp = fopen("history.log", "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    fprintf(fp,
        "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s]\n",
        t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
        t->tm_hour, t->tm_min, t->tm_sec,
        type, msg);

    fclose(fp);
}

/* ========================= */
int is_name_taken(char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd &&
            clients[i].logged_in &&
            strcmp(clients[i].name, name) == 0)
            return 1;
    }
    return 0;
}

/* ========================= */
void broadcast(char *msg, int sender) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd && clients[i].fd != sender)
            send(clients[i].fd, msg, strlen(msg), 0);
    }
}

/* ========================= */
void send_menu(int fd) {
    char menu[] =
        "\n=== THE KNIGHTS CONSOLE ===\n"
        "1. Check Active Entities (Users)\n"
        "2. Check Server Uptime\n"
        "3. Execute Emergency Shutdown\n"
        "4. Disconnect\n"
        "Command >> ";
    send(fd, menu, strlen(menu), 0);
}

/* ========================= */
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    fd_set master_set, read_fds;

    memset(clients, 0, sizeof(clients));
    start_time = time(NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 10);

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);

    int fdmax = server_fd;

    log_event("System", "SERVER ONLINE");

    while (1) {
        read_fds = master_set;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for (int i = 0; i <= fdmax; i++) {
            if (!FD_ISSET(i, &read_fds)) continue;

            /* =========================
               NEW CONNECTION
            ========================= */
            if (i == server_fd) {
                new_socket = accept(server_fd, NULL, NULL);
                FD_SET(new_socket, &master_set);
                if (new_socket > fdmax) fdmax = new_socket;

                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (!clients[j].fd) {
                        reset_client(&clients[j]);
                        clients[j].fd = new_socket;
                        break;
                    }
                }
            }

            /* =========================
               CLIENT MESSAGE
            ========================= */
            else {
                char buffer[BUFFER_SIZE];
                int val = read(i, buffer, sizeof(buffer) - 1);

                if (val <= 0) {
                    close(i);
                    FD_CLR(i, &master_set);
                    continue;
                }

                int idx = -1;
                for (int j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j].fd == i) {
                        idx = j;
                        break;
                    }
                }
                if (idx == -1) continue;

                buffer[val] = '\0';
                trim(buffer);

                /* =========================
                   PASSWORD MODE
                ========================= */
                if (clients[idx].waiting_password) {
                    if (strcmp(buffer, "protocol7") == 0) {
                        clients[idx].is_admin = 1;
                        clients[idx].logged_in = 1;
                        clients[idx].waiting_password = 0;

                        send(i,
                            "\n[System] Authentication Successful. Granted Admin privileges.\n",
                            64, 0);

                        log_event("Admin", "LOGIN SUCCESS");

                        send_menu(i);
                    } else {
                        send(i, "[System] Wrong password\n", 26, 0);
                    }
                    continue;
                }

                /* =========================
                   LOGIN
                ========================= */
                if (!clients[idx].logged_in) {

                    if (strlen(buffer) == 0) {
                        send(i, "[System] Name cannot be empty\n", 31, 0);
                        continue;
                    }

                    if (strcasecmp(buffer, "the knights") == 0) {
                        clients[idx].waiting_password = 1;
                        send(i, "Enter Password: ", 16, 0);
                        continue;
                    }

                    if (is_name_taken(buffer)) {
                        send(i, "[System] Name already used\n", 29, 0);
                        continue;
                    }

                    strncpy(clients[idx].name, buffer, 49);
                    clients[idx].logged_in = 1;

                    char msg[128];
                    snprintf(msg, sizeof(msg),
                             "--- Welcome to The Wired, %.50s ---\n",
                             buffer);

                    send(i, msg, strlen(msg), 0);

                    char logmsg[128];
                    snprintf(logmsg, sizeof(logmsg),
                             "User '%.50s' connected", buffer);
                    log_event("System", logmsg);

                    continue;
                }

                /* =========================
                   EXIT
                ========================= */
                if (strcmp(buffer, "/exit") == 0) {
                    send(i, "[System] Disconnecting from The Wired...\n", 43, 0);

                    usleep(100000); // 🔥 FIX

                    char logmsg[128];
                    snprintf(logmsg, sizeof(logmsg),
                             "User '%.50s' disconnected",
                             clients[idx].name);
                    log_event("System", logmsg);

                    close(i);
                    FD_CLR(i, &master_set);
                    reset_client(&clients[idx]);
                    continue;
                }

                /* =========================
                   ADMIN MODE
                ========================= */
                if (clients[idx].is_admin) {

                    if (strcmp(buffer, "1") == 0) {
                        int count = 0;
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].fd &&
                                clients[j].logged_in &&
                                !clients[j].is_admin)
                                count++;
                        }

                        char msg[64];
                        snprintf(msg, sizeof(msg),
                                 "Active Users: %d\n", count);
                        send(i, msg, strlen(msg), 0);
                        log_event("Admin", "RPC_GET_USERS");
                    }

                    else if (strcmp(buffer, "2") == 0) {
                        long uptime = time(NULL) - start_time;

                        char msg[64];
                        snprintf(msg, sizeof(msg),
                                 "Uptime: %ld seconds\n", uptime);
                        send(i, msg, strlen(msg), 0);
                        log_event("Admin", "RPC_GET_UPTIME");
                    }

                    else if (strcmp(buffer, "3") == 0) {
                        log_event("Admin", "RPC_SHUTDOWN");
                        log_event("System", "EMERGENCY SHUTDOWN INITIATED");
                        exit(0);
                    }

                    else if (strcmp(buffer, "4") == 0) {
                        send(i, "[System] Disconnecting from The Wired...\n", 43, 0);

                        usleep(100000); // 🔥 FIX

                        close(i);
                        FD_CLR(i, &master_set);
                        reset_client(&clients[idx]);
                        continue;
                    }

                    send_menu(i);
                    continue;
                }

                /* =========================
                   CHAT
                ========================= */
                char msg[BUFFER_SIZE];
                char logmsg[BUFFER_SIZE];

                snprintf(msg, sizeof(msg),
                         "[%.50s]: %.900s\n",
                         clients[idx].name, buffer);

                snprintf(logmsg, sizeof(logmsg),
                         "[%.50s]: %.900s",
                         clients[idx].name, buffer);

                broadcast(msg, i);
                log_event("User", logmsg);
            }
        }
    }
}