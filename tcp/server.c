#include "phos_tcp.h"

#include <stdio.h>
#include <unistd.h>

static void echo_handler(struct stcp_ctx *ctx)
{
    puts(stcp_get_msg(ctx));
    stcp_write(ctx, stcp_get_msg(ctx), stcp_get_msg_len(ctx));
}

int main(void)
{
    struct stcp_ctx ctx;

    stcp_init(&ctx);
    stcp_set_handler(&ctx, echo_handler);
    stcp_listen(&ctx);
    stcp_event_loop(&ctx);
    stcp_free(&ctx);

    return 0;
}
