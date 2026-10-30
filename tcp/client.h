#ifndef STCP_CLIENT_H
#define STCP_CLIENT_H

#ifndef STCP_CLIENT_BUFF_SIZE
#define STCP_CLIENT_BUFF_SIZE 2048
#endif

#include <netinet/in.h>

struct stcp_client {
    int fd;
    int is_connected;
    struct sockaddr_in *addr;

    char in_buffer[STCP_CLIENT_BUFF_SIZE];
    int in_len;

    char *out_buffer;
    int out_len;
    int out_sent;
};

struct stcp_client_node {
    struct stcp_client *data;
    struct stcp_client_node *next;
};

struct stcp_client_list {
    int size;
    struct stcp_client_node *head;
};

void stcp_clnt_init(struct stcp_client *clnt);
void stcp_clnt_iwd(struct stcp_client *clnt, int cfd);
void stcp_clnt_free(struct stcp_client *clnt);

void stcp_cl_init(struct stcp_client_list *clients);
void stcp_cl_push(struct stcp_client_list *clients, struct stcp_client *client);
void stcp_cl_remv(struct stcp_client_list *clients, struct stcp_client *client);
void stcp_cl_free(struct stcp_client_list *clients);

#endif
