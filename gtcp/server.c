#include "server.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#ifndef GTCP_LISTEN_QUEUE
#define GTCP_LISTEN_QUEUE 16
#endif

#ifndef GTCP_SERVER_ADDR
#define GTCP_SERVER_ADDR "127.0.0.1"
#endif

#ifndef GTCP_SERVER_PORT
#define GTCP_SERVER_PORT 8080
#endif

static void clear_buffer(char *buffer, int size)
{
    int i;
    for (i = 0; i < size || buffer[i] != '\0'; i++)
        buffer[i] = '\0';
}


void cl_init(struct client_list *clients)
{
    clients->size = 0;
    clients->head = NULL;
}

void cl_push(struct client_list *clients, struct client *clnt)
{
    struct client_node *node, *curr;

    node = malloc(sizeof(struct client_node));
    node->data = clnt;
    node->next = NULL;
    
    if (clients->head == NULL) {
        clients->head = node;
        clients->size++;
        return;
    }

    curr = clients->head;
    while (curr->next != NULL && curr->data->is_connected)
        curr = curr->next;

    if (curr->data->is_connected) {
        free(curr->data);
        free(node);
        curr->data = clnt;
        clients->size++;
        return;
    }

    curr->next = node;
    clients->size++;
}

void cl_remv(struct client_list *clients, struct client *clnt)
{
    struct client_node *curr;
    
    curr = clients->head;
    while (curr->next != NULL || curr->data->fd != clnt->fd)
        curr = curr->next;
    
    curr->data->is_connected = 0;
}

static void cl_free_rec(struct client_node *node)
{
    if (node == NULL)
        return;
    if (node->next == NULL)
        free(node);
    else
        cl_free_rec(node->next);
}

void cl_free(struct client_list *clients)
{
    cl_free_rec(clients->head);
}

void gtcp_run_echo()
{
    int ok;
    int res;
    int opt;
    int flags;
    int max_fd;
    int server_fd;
    int client_fd;
    fd_set readfds;
    fd_set writefds;
    unsigned int client_addr_len;

    struct client_list clients;
    struct sockaddr_in server_addr;
    struct client_node *curr;
    struct timeval timeout;
    struct client *clnt;
    

    char buffer[GTCP_BUFF_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
    /* TODO: Error Processing */
        exit(1);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(GTCP_SERVER_PORT);
    ok = inet_aton(GTCP_SERVER_ADDR, &server_addr.sin_addr);
    if (!ok) {
    /* TODO: Error Processing */
        exit(1);
    }

    opt = 1;
    res = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (res == -1) {
    /* TODO: Error Processing */
        exit(1);
    }

    res = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (res == -1) {
    /* TODO: Error Processing */
        exit(1);
    }

    res = listen(server_fd, GTCP_LISTEN_QUEUE);
    if (res == -1) {
    /* TODO: Error Processing */
        exit(1);
    }
    printf("Listening on %s:%d...\n", GTCP_SERVER_ADDR, GTCP_SERVER_PORT);

    
    /* Echo Server Begin */
    for (;;) {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);
        FD_SET(server_fd, &readfds);

        curr = clients.head;
        while (curr) {
            client_fd = curr->data->fd;

            FD_SET(client_fd, &readfds);
            if (curr->data->out_sent < curr->data->out_size)
                FD_SET(client_fd, &writefds);
            if (client_fd > max_fd)
                max_fd = client_fd;

            curr = curr->next;
        }
        timeout.tv_sec = 60;
        timeout.tv_usec = 0;

        res = select(max_fd + 1, &readfds, &writefds, NULL, &timeout);
        if (res == -1) {
            if (errno == EINTR) {}
            else {}
            continue;
        }

        if (res == 0) {
            /* Событие тайм аут */
            continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            clnt = malloc(sizeof(struct client));

            client_fd = accept(server_fd, (struct sockaddr*)&clnt->addr,
                               &client_addr_len);
            if (client_fd == -1)
                exit(1);
            printf("Accepted connection with %s:%d\n",
                   inet_ntoa(clnt->addr.sin_addr),
                   ntohs(clnt->addr.sin_port));

            flags = fcntl(client_fd, F_GETFL);
            fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

            cl_push(&clients, clnt);
        }

        curr = clients.head;
        while (curr) {
            client_fd = curr->data->fd;

            if (FD_ISSET(client_fd, &readfds)) {
                res = read(client_fd, curr->data->in_buffer, GTCP_BUFF_SIZE);
                if (res == -1)
                    continue;
            }
            if (FD_ISSET(client_fd, &writefds)) {
                res = write(client_fd, curr->data->out_buffer, );
            }

            curr = curr->next;
        }

        /* res = read(client_fd, buffer, GTCP_BUFF_SIZE); */
        /* if (res == -1) { */
        /*     break; */
        /* } */
        /**/
        /* printf( */
        /*     "Recv '%s' <- %s:%d\n", */
        /*     buffer, */
        /*     inet_ntoa(client_addr.sin_addr), */
        /*     ntohs(client_addr.sin_port) */
        /* ); */
        /**/
        /* res = write(client_fd, buffer, GTCP_BUFF_SIZE); */
        /* if (res == -1) { */
        /*     exit(1); */
        /* } */
        /*  */
        /* printf( */
        /*     "Send '%s' -> %s:%d\n", */
        /*     buffer, */
        /*     inet_ntoa(client_addr.sin_addr), */
        /*     ntohs(client_addr.sin_port) */
        /* ); */
        /* clear_buffer(buffer, GTCP_BUFF_SIZE); */
    }
    /* Echo Server End */

}

