#include "client.h"

#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>

void stcp_clnt_init(struct stcp_client *clnt)
{
    if (!clnt)
        return;

    clnt->fd = 0;
    clnt->is_connected = 0;
    clnt->addr = malloc(sizeof(struct sockaddr_in));
    clnt->in_len = 0;
    clnt->out_buffer = NULL;
    clnt->in_buffer[STCP_CLIENT_BUFF_SIZE - 1] = '\0';
    clnt->out_len = 0;
    clnt->out_sent = 0;
}

void stcp_clnt_iwd(struct stcp_client *clnt, int cfd)
{
    if (!clnt)
        return;

    clnt->fd = cfd;
    clnt->is_connected = 1;
}

void stcp_clnt_free(struct stcp_client *clnt)
{
    if (!clnt)
        return;

    clnt->is_connected = 0;
    if (clnt->addr)
        free(clnt->addr);
    close(clnt->fd);
}

void stcp_cl_init(struct stcp_client_list *clients)
{
    if (!clients)
        return;

    clients->size = 0;
    clients->head = NULL;
}

void stcp_cl_push(struct stcp_client_list *clients, struct stcp_client *clnt)
{
    struct stcp_client_node *new_node, *curr;

    if (!clients || !clnt)
        return;

    new_node = malloc(sizeof(struct stcp_client_node));
    new_node->data = clnt;
    new_node->next = NULL;
    
    if (!clients->head) {
        clients->head = new_node;
        clients->size++;
        return;
    }

    curr = clients->head;
    while (curr->next && curr->data->is_connected)
        curr = curr->next;

    if (!curr->data->is_connected) {
        free(curr->data);
        free(new_node);
        curr->data = clnt;
        clients->size++;
        return;
    }

    curr->next = new_node;
    clients->size++;
}

void stcp_cl_remv(struct stcp_client_list *clients, struct stcp_client *clnt)
{
    struct stcp_client_node *curr;

    if (!clients || !clnt)
        return;
    
    curr = clients->head;
    while (curr->next || curr->data->fd != clnt->fd)
        curr = curr->next;
    
    stcp_clnt_free(curr->data);
}

static void stcp_cl_free_rec(struct stcp_client_node *node)
{
    if (!node)
        return;
    if (node->next)
        stcp_cl_free_rec(node->next);
    free(node);
    node = NULL;
}

void stcp_cl_free(struct stcp_client_list *clients)
{
    if (!clients)
        return;
    stcp_cl_free_rec(clients->head);
}

