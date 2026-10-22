#include <libwebsockets.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>

#define QUEUE_NAME "/led_queue"

static int interrupted = 0;
static void on_sigint(int sig) { interrupted = 1; }

static mqd_t q;

static int ipc_init(void) {
    struct mq_attr a = { .mq_flags = 0, .mq_maxmsg = 10,
                         .mq_msgsize = 64, .mq_curmsgs = 0 };
    q = mq_open(QUEUE_NAME, O_WRONLY | O_CREAT | O_NONBLOCK, 0666, &a);
    return q == (mqd_t)-1 ? -1 : 0;
}

static int validate(const char *m, const char **err) {
    if (!strcmp(m, "LED_ON") || !strcmp(m, "LED_OFF") ||
        !strcmp(m, "START")  || !strcmp(m, "STOP"))
        return 1;

    if (!strncmp(m, "SET_INTERVAL:", 13)) {
        const char *num = m + 13;
        char *end;
        long v = strtol(num, &end, 10);
        if (*num == '\0' || *end != '\0') { *err = "interval must be a number"; return 0; }
        if (v < 50 || v > 5000)           { *err = "interval must be 50-5000"; return 0; }
        return 1;
    }
    *err = "unknown command";
    return 0;
}

static void reply(struct lws *wsi, const char *json) {
    unsigned char buf[LWS_PRE + 160];
    size_t n = strlen(json);
    memcpy(buf + LWS_PRE, json, n);
    lws_write(wsi, buf + LWS_PRE, n, LWS_WRITE_TEXT);
}

static int callback_cmd(struct lws *wsi, enum lws_callback_reasons reason,
                        void *user, void *in, size_t len) {
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        printf("Browser connected\n");
        break;
    case LWS_CALLBACK_RECEIVE: {
        char msg[64];
        char out[160];
        const char *err = "";
        if (len >= sizeof(msg)) {
            reply(wsi, "{\"status\":\"error\",\"msg\":\"command too long\"}");
            break;
        }
        memcpy(msg, in, len);
        msg[len] = '\0';

        if (!validate(msg, &err)) {
            printf("REJECTED: %s (%s)\n", msg, err);
            snprintf(out, sizeof out, "{\"status\":\"error\",\"msg\":\"%s\"}", err);
        } else if (mq_send(q, msg, strlen(msg) + 1, 0) == 0) {
            printf("SENT TO QUEUE: %s\n", msg);
            snprintf(out, sizeof out, "{\"status\":\"ok\",\"cmd\":\"%s\"}", msg);
        } else if (errno == EAGAIN) {
            printf("QUEUE FULL: %s\n", msg);
            snprintf(out, sizeof out, "{\"status\":\"error\",\"msg\":\"controller busy\"}");
        } else {
            perror("mq_send");
            snprintf(out, sizeof out, "{\"status\":\"error\",\"msg\":\"ipc failure\"}");
        }
        reply(wsi, out);
        break;
    }
    case LWS_CALLBACK_CLOSED:
        printf("Browser disconnected\n");
        break;
    default:
        break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "cmd", callback_cmd, 0, 128 },
    { NULL, NULL, 0, 0 }
};

static const struct lws_http_mount mount = {
    .mountpoint      = "/",
    .origin          = "./www",
    .def             = "index.html",
    .origin_protocol = LWSMPRO_FILE,
    .mountpoint_len  = 1,
};

int main(void) {
    signal(SIGINT, on_sigint);

    if (ipc_init() != 0) { perror("mq_open"); return 1; }

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof info);
    info.port      = 8080;
    info.mounts    = &mount;
    info.protocols = protocols;

    struct lws_context *ctx = lws_create_context(&info);
    if (!ctx) {
        printf("Failed to start server\n");
        return 1;
    }

    printf("Server running on http://localhost:8080 (Ctrl+C to stop)\n");
    while (!interrupted)
        lws_service(ctx, 0);

    lws_context_destroy(ctx);
    mq_close(q);
    return 0;
}
