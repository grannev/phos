#include "client.h"

#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>

void clnt_init(struct client *clnt)
{
    if (!clnt)
        return;

    clnt->fd = 0;
    clnt->is_connected = 0;
    clnt->addr = malloc(sizeof(struct sockaddr_in));
    clnt->in_len = 0;
    clnt->out_buffer = NULL;
    clnt->in_buffer[GTCP_CLIENT_BUFF_SIZE - 1] = '\0';
    clnt->out_len = 0;
    clnt->out_sent = 0;
}

void clnt_iwd(struct client *clnt, int cfd)
{
    if (!clnt)
        return;

    clnt->fd = cfd;
    clnt->is_connected = 1;
}

void clnt_free(struct client *clnt)
{
    if (!clnt)
        return;

    clnt->is_connected = 0;
    if (clnt->out_buffer)
        free(clnt->out_buffer);
    if (clnt->addr)
        free(clnt->addr);
    close(clnt->fd);
}

void cl_init(struct client_list *clients)
{
    if (!clients)
        return;

    clients->size = 0;
    clients->head = NULL;
}

void cl_push(struct client_list *clients, struct client *clnt)
{
    struct client_node *new_node, *curr;

    if (!clients || !clnt)
        return;

    new_node = malloc(sizeof(struct client_node));
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

void cl_remv(struct client_list *clients, struct client *clnt)
{
    struct client_node *curr;

    if (!clients || !clnt)
        return;
    
    curr = clients->head;
    while (curr->next || curr->data->fd != clnt->fd)
        curr = curr->next;
    
    clnt_free(curr->data);
}

static void cl_free_rec(struct client_node *node)
{
    if (!node)
        return;
    if (node->next)
        cl_free_rec(node->next);
    free(node);
    node = NULL;
}

void cl_free(struct client_list *clients)
{
    if (!clients)
        return;
    cl_free_rec(clients->head);
}

