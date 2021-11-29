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
#define MSG_LENGTH 300

bool server_not_ready = true;
pthread_t server;

int read_from_client(int fd, char *address) {
    char buffer[MSG_LENGTH];
    int nbytes;

    nbytes = read(fd, buffer, MSG_LENGTH);
    if (nbytes < 0) {
        perror("\033[1;31mread(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }
    else if (nbytes == 0)
        return -1;
    else {
      printf("\t %s says: %s\n", address, buffer);
      return 0;
    }
}

void* server_func(void* arg) {
    int sockfd;
    struct sockaddr_in address;
    fd_set active_set, read_set;
    int opt = 1, i, j;
    char addresses[FD_SETSIZE][15];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\033[1;31msocket(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("\033[1;31msetsockopt(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("\033[1;31mbind(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 10) < 0) {
        perror("\033[1;31mlisten(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }
    
    FD_ZERO(&active_set);
    FD_SET(sockfd, &active_set);

    socklen_t addrlen = (socklen_t)sizeof(address);
    server_not_ready = false;

    while (1) {
        read_set = active_set;
        if (select(FD_SETSIZE, &read_set, NULL, NULL, NULL) < 0) {
            perror("\033[1;31mselect(...) failed\033[0m");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET(i, &read_set)) {
                if (i == sockfd) {
                    int client;
                    if ((client = accept(sockfd, (struct sockaddr*)&address, &addrlen)) < 0) {
                        perror("\033[1;31maccept(...) failed\033[0m");
                        exit(EXIT_FAILURE);
                    }

                    strcpy(addresses[j++], inet_ntoa(address.sin_addr));
                    FD_SET(client, &active_set);

                } else {
                    if (read_from_client(i, addresses[i-5]) < 0) {
                        close(i);
                        FD_CLR(i, &active_set);
                    }
                }
            }
        }
    }
}

void client_func(const char* ip_str) {
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("\033[1;31msocket(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip_str, &serv_addr.sin_addr) <= 0) {
        perror("\033[1;31minet_pton(...) failed\033[0m");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec  = 5;
    timeout.tv_usec = 0;

    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    printf("Connecting to server...\n");
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\033[1;31mCouldn't connect!\033[0m\n");
        pthread_cancel(server);
        pthread_join(server, NULL);
        exit(EXIT_FAILURE);
    }
    printf("\033[1;32mConnected!\033[0m\n");

    char buffer[MSG_LENGTH];
    do {
        memset(buffer, 0, MSG_LENGTH);
        fgets(buffer, MSG_LENGTH, stdin);
        write(sockfd, buffer, MSG_LENGTH);
    } while (strcmp(buffer, "exit\n") != 0);

    pthread_cancel(server);
    pthread_join(server, NULL);
    close(sockfd);
}

int main() {
    pthread_create(&server, NULL, server_func, NULL);

    while (server_not_ready);

    //client_func("68.7.100.87"); // Enable port forwarding on router for this to work
    client_func("127.0.0.1");

    return 0;
}
