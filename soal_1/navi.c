#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "protocol.h"

#define PORT 8080

int sock;
int exiting = 0;

/* =========================
   CTRL+C HANDLE
========================= */
void handle_sigint(int sig) {
    char msg[] = "4\n";
    send(sock, msg, strlen(msg), 0);
    exiting = 1;
}

/* =========================
   MAIN
========================= */
int main() {
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];

    signal(SIGINT, handle_sigint);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
    printf("Connection Failed\n");
    close(sock);
    return 1;
}

    if (sock < 0) {
    printf("Socket error\n");
    return 1;
}
  
    printf("Enter your name: ");
    fflush(stdout);

    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    strcat(buffer, "\n");
    send(sock, buffer, strlen(buffer), 0);

    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(0, &readfds);
        FD_SET(sock, &readfds);

        select(sock + 1, &readfds, NULL, NULL, NULL);

        /* =========================
           INPUT USER
        ========================= */
        if (FD_ISSET(0, &readfds)) {
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = 0;
            strcat(buffer, "\n");

            send(sock, buffer, strlen(buffer), 0);

            if (strcmp(buffer, "/exit\n") == 0) {
                exiting = 1;
            }
        }

        /* =========================
           OUTPUT SERVER
        ========================= */
        if (FD_ISSET(sock, &readfds)) {
            int val = read(sock, buffer, sizeof(buffer) - 1);

            if (val <= 0) {
                printf("[System] Disconnected\n");
                break;
            }

            buffer[val] = '\0';
            printf("%s", buffer);
            fflush(stdout);

            if (exiting) {
                break; // 🔥 keluar SETELAH pesan tampil
            }
        }
    }

    close(sock);
    return 0;
}