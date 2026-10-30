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
    int server_fd;

    struct sockaddr_in server_addr;

    char buffer[BUFF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    ok = inet_aton(SERVER_ADDR, &server_addr.sin_addr);
    if (!ok) {
        exit(1);
    }

    res = connect(
        server_fd,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    );
    if (res == -1) {
        exit(1);
    }

    res = write(server_fd, "ping", 4);
    if (res == -1) {
        exit(1);
    } 
    printf("Ping -> %s:%d\n", SERVER_ADDR, SERVER_PORT);

    res = read(server_fd, buffer, BUFF_SIZE);
    if (res == -1) {
        exit(1);
    }
    printf("Recv \"%s\" <- %s:%d\n", buffer, SERVER_ADDR, SERVER_PORT);

    clear_buffer(buffer, BUFF_SIZE);
    scanf("%d", &res);

    res = shutdown(server_fd, SHUT_RDWR);
    if (res == -1) {
        exit(1);
    }
    close(server_fd);
}

