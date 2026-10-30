#ifndef GTCP_SERVER_H
#define GTCP_SERVER_H

#ifndef GTCP_LISTEN_QUEUE
#define GTCP_LISTEN_QUEUE 16
#endif

#ifndef GTCP_SERVER_ADDR
#define GTCP_SERVER_ADDR "127.0.0.1"
#endif

#ifndef GTCP_SERVER_PORT
#define GTCP_SERVER_PORT 8080
#endif

#ifndef GTCP_TIMEOUT_SEC
#define GTCP_TIMEOUT_SEC 60
#endif

#ifndef GTCP_TIMEOUT_USEC
#define GTCP_TIMEOUT_USEC 60
#endif

#include "client.h"

#include <netinet/in.h>

struct gtcp_ctx {
    int sfd;
    fd_set readfds;
    fd_set writefds;
    int max_fd;

    struct timeval *timeout;
    struct sockaddr_in *addr;

    struct client_list *clients;
    struct client *clnt;

    void (*handler)(struct gtcp_ctx*);
};

void gtcp_init(struct gtcp_ctx *ctx);
void gtcp_listen(struct gtcp_ctx *ctx);
void gtcp_free(struct gtcp_ctx *ctx);

void gtcp_set_addr(struct gtcp_ctx *server, const char *addr);
void gtcp_set_handler(struct gtcp_ctx *ctx,
                      void (*handler)(struct gtcp_ctx*));

void gtcp_run_echo();

#endif


