#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 2000
#define MSG_LENGTH 280

bool server_not_ready = true;
pthread_t server;

void* server_func(void* arg) {
    int sockfd;
    struct sockaddr_in address;
    int opt = 1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket(...) failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(...) failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind(...) failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen(...) failed");
        exit(EXIT_FAILURE);
    }

    socklen_t addrlen = (socklen_t)sizeof(address);
    server_not_ready = false;

    int client;
    if ((client = accept(sockfd, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("accept(...) failed");
        exit(EXIT_FAILURE);
    }

    char buffer[MSG_LENGTH];
    do {
        read(client, buffer, MSG_LENGTH);
        printf("\t %s says: %s\n", inet_ntoa(address.sin_addr), buffer);
    } while(strcmp(buffer, "exit\n") != 0);

    close(client);
    close(sockfd);
    return NULL;
}

void client_func(const char* ip_str) {
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket(...) failed");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip_str, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton(...) failed");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec  = 5;
    timeout.tv_usec = 0;

    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect(...) failed");
        pthread_cancel(server);
        pthread_join(server, NULL);
        exit(EXIT_FAILURE);
    }

    char buffer[MSG_LENGTH];
    do {
        fgets(buffer, MSG_LENGTH, stdin);
        write(sockfd, buffer, MSG_LENGTH);
    } while(strcmp(buffer, "exit\n") != 0);

    close(sockfd);
}

int main() {
    pthread_create(&server, NULL, server_func, NULL);

    while (server_not_ready);

    //client_func("68.7.100.87"); // Enable port forwarding on router for this to work
    client_func("127.0.0.1");

    return 0;
}
