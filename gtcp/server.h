#ifndef GTCP_SERVER_H
#define GTCP_SERVER_H

#ifndef GTCP_BUFF_SIZE
#define GTCP_BUFF_SIZE 2048
#endif

#include <netinet/in.h>

struct client {
    int fd;
    int is_connected;
    struct sockaddr_in addr;

    char in_buffer[GTCP_BUFF_SIZE];
    int in_size;

    char *out_buffer;
    int out_size;
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

void cl_init(struct client_list *clients);
void cl_push(struct client_list *clients, struct client *client);
void cl_remv(struct client_list *clients, struct client *client);
void cl_free(struct client_list *clients);

void gtcp_run_echo();

#endif


