#ifndef GTCP_CLIENT_H
#define GTCP_CLIENT_H

#ifndef GTCP_CLIENT_BUFF_SIZE
#define GTCP_CLIENT_BUFF_SIZE 2048
#endif

#include <netinet/in.h>

struct client {
    int fd;
    int is_connected;
    struct sockaddr_in *addr;

    char in_buffer[GTCP_CLIENT_BUFF_SIZE];
    int in_len;

    char *out_buffer;
    int out_len;
    int out_sent;
};

struct client_node {
    struct client *data;
    struct client_node *next;
};

struct client_list {
    int size;
    struct client_node *head;
};

void clnt_init(struct client *clnt);
void clnt_iwd(struct client *clnt, int cfd);
void clnt_free(struct client *clnt);

void cl_init(struct client_list *clients);
void cl_push(struct client_list *clients, struct client *client);
void cl_remv(struct client_list *clients, struct client *client);
void cl_free(struct client_list *clients);

#endif
