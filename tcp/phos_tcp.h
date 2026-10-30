#ifndef PHOS_TCP_H
#define PHOS_TCP_H

#include <sys/select.h>

struct timeval;
struct sockaddr_in;
struct stcp_client;
struct stcp_client_list;

struct stcp_ctx {
    int sfd;
    fd_set readfds;
    fd_set writefds;
    int max_fd;

    struct timeval *timeout;
    struct sockaddr_in *addr;

    struct stcp_client_list *clients;
    struct stcp_client *clnt;

    void (*handler)(struct stcp_ctx*);
};

void stcp_init(struct stcp_ctx *ctx);
void stcp_listen(struct stcp_ctx *ctx);
void stcp_event_loop(struct stcp_ctx *ctx);
void stcp_free(struct stcp_ctx *ctx);

void stcp_set_handler(struct stcp_ctx *ctx,
                      void (*handler)(struct stcp_ctx*));
void stcp_set_addr(struct stcp_ctx *ctx, const char *addr);

void stcp_write(struct stcp_ctx *ctx, char *msg, int len);
char *stcp_get_msg(struct stcp_ctx *ctx);
int stcp_get_msg_len(struct stcp_ctx *ctx);

#endif
