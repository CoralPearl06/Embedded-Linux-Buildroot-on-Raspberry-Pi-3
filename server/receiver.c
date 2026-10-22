#include <mqueue.h>
#include <fcntl.h>
#include <stdio.h>

int main(void) {
    struct mq_attr a = { .mq_flags = 0, .mq_maxmsg = 10,
                         .mq_msgsize = 64, .mq_curmsgs = 0 };
    mqd_t q = mq_open("/ctrl_cmd_q", O_RDONLY | O_CREAT, 0666, &a);
    if (q == (mqd_t)-1) { perror("mq_open"); return 1; }

    printf("Controller waiting for commands...\n");
    char buf[64];
    while (1) {
        ssize_t n = mq_receive(q, buf, sizeof buf, NULL);
        if (n >= 0) printf("CONTROLLER GOT: %s\n", buf);
    }
    return 0;
}
