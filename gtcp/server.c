#include "server.h"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

void gtcp_init(struct gtcp_ctx *ctx)
{
    int ok;

    /* TODO: Error processing */

    ctx->clnt = NULL;
    ctx->clients = malloc(sizeof(struct client_list));
    cl_init(ctx->clients);

    ctx->timeout = malloc(sizeof(struct timeval));
    ctx->timeout->tv_sec = GTCP_TIMEOUT_SEC;
    ctx->timeout->tv_usec = GTCP_TIMEOUT_USEC;

    ctx->sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sfd == -1) {
        exit(1);
    }
    
    ctx->addr = malloc(sizeof(struct sockaddr_in));
    ctx->addr->sin_family = AF_INET;
    ctx->addr->sin_port = htons(GTCP_SERVER_PORT);

    ok = inet_aton(GTCP_SERVER_ADDR, &ctx->addr->sin_addr);
    if (!ok) {
        exit(1);
    }
}

void gtcp_set_addr(struct gtcp_ctx *ctx, const char *addr)
{
    int ok;

    ok = inet_aton(addr, &ctx->addr->sin_addr);
    if (!ok) {
        exit(1);
    }
}

void gtcp_set_handler(struct gtcp_ctx *ctx,
                      void (*handler)(struct gtcp_ctx*))
{
    ctx->handler = handler;
}

void gtcp_free(struct gtcp_ctx *ctx)
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

void gtcp_listen(struct gtcp_ctx *ctx)
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

    res = listen(ctx->sfd, GTCP_LISTEN_QUEUE);
    if (res == -1) {
        exit(1);
    }
    printf("Listening on %s:%d...\n", GTCP_SERVER_ADDR, GTCP_SERVER_PORT);
}

static void gtcp_el_set_fds(struct gtcp_ctx *ctx)
{
    int cfd;

    ctx->max_fd = ctx->sfd;
    
    struct client_node *curr;
    struct client *clnt;

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

static int gtcp_el_select(struct gtcp_ctx *ctx)
{
    int res;

    ctx->timeout->tv_sec = GTCP_TIMEOUT_SEC;
    ctx->timeout->tv_usec = GTCP_TIMEOUT_USEC;

    res = select(ctx->max_fd + 1, &ctx->readfds, &ctx->writefds, NULL, ctx->timeout);
    if (res == -1) {
        if (errno == EINTR) {}
        else {}
        return 1;
    }

    if (res == 0) {
        /* Событие тайм аут */
        return 1;
    }

    return 0;
}

static void gtcp_el_accept(struct gtcp_ctx *ctx)
{
    int cfd;
    int flags;
    unsigned int client_addr_len;

    if (FD_ISSET(ctx->sfd, &ctx->readfds)) {
        ctx->clnt = malloc(sizeof(struct client));
        clnt_init(ctx->clnt);

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

        clnt_iwd(ctx->clnt, cfd);
        cl_push(ctx->clients, ctx->clnt);
    }
}

/* gTCP event loop bust clients file descriptors */
static void gtcp_el_bust_cfds(struct gtcp_ctx *ctx)
{
    int res;
    int cfd;

    struct client_node *curr;

    curr = ctx->clients->head;
    while (curr) {
        ctx->clnt = curr->data;
        cfd = ctx->clnt->fd;

        if (!ctx->clnt->is_connected) {
            curr = curr->next;
            continue;
        }

        if (FD_ISSET(cfd, &ctx->readfds)) {
            res = read(cfd, ctx->clnt->in_buffer, GTCP_CLIENT_BUFF_SIZE - 1);

            if (res == -1) {
                curr = curr->next;
                continue;
            }
            if (res == 0) {
                clnt_free(ctx->clnt);
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

        }

        curr = curr->next;
    }

}

static void gtcp_event_loop(struct gtcp_ctx *ctx)
{
    int res;

    for (;;) {
        gtcp_el_set_fds(ctx);

        /* 
         * No need to go through list of clients if return 1
         * otherwise need to check them
         */
        res = gtcp_el_select(ctx);
        if (res)
            continue;

        /* 
         * If there's someone we can accept, this function will add new
         * client connection to the ctx.clients
         */
        gtcp_el_accept(ctx);
        
        gtcp_el_bust_cfds(ctx);
    }
}

void echo_handler(struct gtcp_ctx *ctx)
{
    puts(ctx->clnt->in_buffer);

    ctx->clnt->out_buffer = malloc(ctx->clnt->in_len + 1);
    ctx->clnt->out_buffer[ctx->clnt->in_len] = '\0';
    strcpy(ctx->clnt->out_buffer, ctx->clnt->in_buffer);
    ctx->clnt->out_len = ctx->clnt->in_len;
    write(ctx->clnt->fd, ctx->clnt->out_buffer, ctx->clnt->out_len);
    ctx->clnt->out_sent = ctx->clnt->out_len;
    free(ctx->clnt->out_buffer);
    ctx->clnt->out_buffer = NULL;
}

void gtcp_run_echo()
{
    struct gtcp_ctx ctx;

    gtcp_init(&ctx);
    gtcp_set_handler(&ctx, echo_handler);
    gtcp_listen(&ctx);
    gtcp_event_loop(&ctx);
    gtcp_free(&ctx);
}

