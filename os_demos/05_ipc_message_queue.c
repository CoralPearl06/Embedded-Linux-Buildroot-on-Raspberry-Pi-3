
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <errno.h>

#define QUEUE_NAME "/os_demo_queue"
#define MESSAGE_SIZE 64

int main(void)
{
    // Remove any leftover queue from a previous run
    mq_unlink(QUEUE_NAME);

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MESSAGE_SIZE;

    mqd_t queue = mq_open(
        QUEUE_NAME,
        O_CREAT | O_RDWR,
        0600,
        &attr
    );

    if (queue == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        mq_close(queue);
        mq_unlink(QUEUE_NAME);
        return 1;
    }

    if (pid == 0) {
        // Child process: receiver/controller
        mqd_t receiver = mq_open(QUEUE_NAME, O_RDONLY);

        if (receiver == (mqd_t)-1) {
            perror("receiver mq_open");
            _exit(1);
        }

        char message[MESSAGE_SIZE];

        ssize_t bytes = mq_receive(
            receiver, message, sizeof(message), NULL
        );

        if (bytes >= 0) {
            message[bytes] = '\0';
            printf("Controller received command: %s\n", message);
            fflush(stdout);
        } else {
            perror("mq_receive");
        }

        mq_close(receiver);
        _exit(bytes >= 0 ? 0 : 1);
    }

    // Parent process: sender/server
    const char *command = "LED_ON";

    if (mq_send(queue, command, strlen(command) + 1, 0) == -1) {
        perror("mq_send");
    } else {
        printf("Server sent command: %s\n", command);
    }

    wait(NULL);

    mq_close(queue);
    mq_unlink(QUEUE_NAME);

    return 0;
}
