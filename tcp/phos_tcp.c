#include "phos_tcp.h"
#include "client.h"

#ifndef STCP_LISTEN_QUEUE
#define STCP_LISTEN_QUEUE 16
#endif

#ifndef STCP_SERVER_ADDR
#define STCP_SERVER_ADDR "127.0.0.1"
#endif

#ifndef STCP_SERVER_PORT
#define STCP_SERVER_PORT 8080
#endif

#ifndef STCP_TIMEOUT_SEC
#define STCP_TIMEOUT_SEC 60
#endif

#ifndef STCP_TIMEOUT_USEC
#define STCP_TIMEOUT_USEC 60
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

void stcp_init(struct stcp_ctx *ctx)
{
    int ok;

    /* TODO: Error processing */

    ctx->clnt = NULL;
    ctx->clients = malloc(sizeof(struct stcp_client_list));
    stcp_cl_init(ctx->clients);

    ctx->timeout = malloc(sizeof(struct timeval));
    ctx->timeout->tv_sec = STCP_TIMEOUT_SEC;
    ctx->timeout->tv_usec = STCP_TIMEOUT_USEC;

    ctx->sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sfd == -1) {
        exit(1);
    }
    
    ctx->addr = malloc(sizeof(struct sockaddr_in));
    ctx->addr->sin_family = AF_INET;
    ctx->addr->sin_port = htons(STCP_SERVER_PORT);

    ok = inet_aton(STCP_SERVER_ADDR, &ctx->addr->sin_addr);
    if (!ok) {
        exit(1);
    }
}

void stcp_set_addr(struct stcp_ctx *ctx, const char *addr)
{
    int ok;

    ok = inet_aton(addr, &ctx->addr->sin_addr);
    if (!ok) {
        exit(1);
    }
}

void stcp_set_handler(struct stcp_ctx *ctx,
                      void (*handler)(struct stcp_ctx*))
{
    ctx->handler = handler;
}

void stcp_free(struct stcp_ctx *ctx)
{
    ctx->clnt = NULL;
    if (ctx->timeout)
        free(ctx->timeout);
    if (ctx->addr)
        free(ctx->addr);
    if (ctx->clients)
        free(ctx->clients);

    close(ctx->sfd);
}

void stcp_listen(struct stcp_ctx *ctx)
{
    int opt;
    int res;

    opt = 1;
    res = setsockopt(ctx->sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (res == -1) {
        exit(1);
    }

    res = bind(ctx->sfd, (struct sockaddr*)ctx->addr,
               sizeof(*ctx->addr));
    if (res == -1) {
        exit(1);
    }

    res = listen(ctx->sfd, STCP_LISTEN_QUEUE);
    if (res == -1) {
        exit(1);
    }
    printf("Listening on %s:%d...\n", STCP_SERVER_ADDR, STCP_SERVER_PORT);
}

static void stcp_el_set_fds(struct stcp_ctx *ctx)
{
    int cfd;

    ctx->max_fd = ctx->sfd;
    
    struct stcp_client_node *curr;
    struct stcp_client *clnt;

    FD_ZERO(&ctx->readfds);
    FD_ZERO(&ctx->writefds);
    FD_SET(ctx->sfd, &ctx->readfds);

    curr = ctx->clients->head;
    while (curr) {
        clnt = curr->data;
        cfd = clnt->fd;

        if (!clnt->is_connected) {
            curr = curr->next;
            continue;
        }

        FD_SET(cfd, &ctx->readfds);
        if (clnt->out_sent < clnt->out_len)
            FD_SET(cfd, &ctx->writefds);
        if (cfd > ctx->max_fd)
            ctx->max_fd = cfd;

        curr = curr->next;
    }
}

static int stcp_el_select(struct stcp_ctx *ctx)
{
    int res;

    ctx->timeout->tv_sec = STCP_TIMEOUT_SEC;
    ctx->timeout->tv_usec = STCP_TIMEOUT_USEC;

    res = select(ctx->max_fd + 1, &ctx->readfds, &ctx->writefds, NULL,
                 ctx->timeout);
    if (res == -1) {
        if (errno == EINTR) {}
        else {}
        return 1;
    }

    if (res == 0) {
        /* Timeout event */
        return 1;
    }

    return 0;
}

static void stcp_el_accept(struct stcp_ctx *ctx)
{
    int cfd;
    int flags;
    unsigned int client_addr_len;

    if (FD_ISSET(ctx->sfd, &ctx->readfds)) {
        ctx->clnt = malloc(sizeof(struct stcp_client));
        stcp_clnt_init(ctx->clnt);

        client_addr_len = sizeof(*ctx->clnt->addr);
        cfd = accept(ctx->sfd, (struct sockaddr*)ctx->clnt->addr,
                           &client_addr_len);
        if (cfd == -1)
            exit(1);
        printf("Accepted connection with %s:%d\n",
               inet_ntoa(ctx->clnt->addr->sin_addr),
               ntohs(ctx->clnt->addr->sin_port));

        flags = fcntl(cfd, F_GETFL);
        fcntl(cfd, F_SETFL, flags | O_NONBLOCK);

        stcp_clnt_iwd(ctx->clnt, cfd);
        stcp_cl_push(ctx->clients, ctx->clnt);
    }
}

/* (pho)sTCP event loop bust clients file descriptors */
static void stcp_el_bust_cfds(struct stcp_ctx *ctx)
{
    int res;
    int cfd;

    struct stcp_client_node *curr;

    curr = ctx->clients->head;
    while (curr) {
        ctx->clnt = curr->data;
        cfd = ctx->clnt->fd;

        if (!ctx->clnt->is_connected) {
            curr = curr->next;
            continue;
        }

        if (FD_ISSET(cfd, &ctx->readfds)) {
            res = read(cfd, ctx->clnt->in_buffer, STCP_CLIENT_BUFF_SIZE - 1);

            if (res == -1) {
                curr = curr->next;
                continue;
            }
            if (res == 0) {
                stcp_clnt_free(ctx->clnt);
                curr = curr->next;
                continue;
            } else {
                printf("Receiving data...\n");
                ctx->clnt->in_buffer[res] = '\0';
                curr->data->in_len = res;
                ctx->handler(ctx);
            }
        }

        if (FD_ISSET(cfd, &ctx->writefds)) {
            res = write(cfd, ctx->clnt->out_buffer + ctx->clnt->out_sent,
                        ctx->clnt->out_len);
            if (res == -1) {
                curr = curr->next;
                continue;
            }
            if (res == 0) {
                curr = curr->next;
                continue;
            } else {
                ctx->clnt->out_sent += res;
                if (ctx->clnt->out_sent == ctx->clnt->out_len)
                    free(ctx->clnt->out_buffer);
            }
        }

        curr = curr->next;
    }

}

void stcp_event_loop(struct stcp_ctx *ctx)
{
    int res;

    for (;;) {
        stcp_el_set_fds(ctx);

        /* 
         * No need to go through list of clients if return 1
         * otherwise need to check them
         */
        res = stcp_el_select(ctx);
        if (res)
            continue;

        /* 
         * If there's someone we can accept, this function will add new
         * client connection to the ctx.clients
         */
        stcp_el_accept(ctx);
        
        stcp_el_bust_cfds(ctx);
    }
}


void stcp_write(struct stcp_ctx *ctx, char *msg, int len)
{
    ctx->clnt->out_buffer = malloc(len + 1);
    ctx->clnt->out_buffer[len] = '\0';
    memcpy(ctx->clnt->out_buffer, msg, len);
    ctx->clnt->out_len = len;
    ctx->clnt->out_sent = 0;
}

char *stcp_get_msg(struct stcp_ctx *ctx)
{
    return ctx->clnt->in_buffer;
}

int stcp_get_msg_len(struct stcp_ctx *ctx)
{
    return ctx->clnt->in_len;
}
