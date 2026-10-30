#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#ifndef SERVER_ADDR
#define SERVER_ADDR "127.0.0.1" 
#endif

#ifndef SERVER_PORT
#define SERVER_PORT 8080
#endif

#define BUFF_SIZE 2048

void clear_buffer(char *buffer, int size)
{
    int i;
    for (i = 0; i < size || buffer[i] != '\0'; i++)
        buffer[i] = '\0';
}

int main()
{
    int ok;
    int res;
    int client_fd;

    struct sockaddr_in server_addr;

    char buffer[BUFF_SIZE];

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
    /* TODO: Error Processing */
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    ok = inet_aton(SERVER_ADDR, &server_addr.sin_addr);
    if (!ok) {
    /* TODO: Error Processing */
        exit(1);
    }

    res = connect(
        client_fd,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    );
    if (res == -1) {
    /* TODO: Error Processing */
        exit(1);
    }

    /* Echo Client Begin */
    for (;;) {
        printf("> ");
        fgets(buffer, BUFF_SIZE, stdin);

        res = write(client_fd, buffer, 20);
        if (res == -1) {
        /* TODO: Error Processing */
            exit(1);
        }

        printf("Send '%s' -> %s\n", buffer, SERVER_ADDR);    
        clear_buffer(buffer, BUFF_SIZE);

        res = read(client_fd, buffer, BUFF_SIZE);
        if (res == -1) {
        /* TODO: Error Processing */
            exit(1);
        }

        printf("Get '%s' <- %s\n", buffer, SERVER_ADDR);
        clear_buffer(buffer, BUFF_SIZE);
        /* Echo Client End */
    }

    res = shutdown(client_fd, SHUT_RDWR);
    if (res == -1) {
    /* TODO: Error Processing */
        exit(1);
    }
    close(client_fd);
}

