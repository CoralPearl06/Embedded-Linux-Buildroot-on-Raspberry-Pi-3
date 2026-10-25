
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

int shared_value = 10;

// Function executed by each thread
void *thread_function(void *arg)
{
    int thread_number = *(int *)arg;
    int local_value = thread_number * 100;

    printf("Thread %d: PID = %d\n",
           thread_number, getpid());

    printf("Thread %d: shared_value address = %p, value = %d\n",
           thread_number, (void *)&shared_value, shared_value);

    printf("Thread %d: local_value address = %p, value = %d\n",
           thread_number, (void *)&local_value, local_value);

    return NULL;
}

int main(void)
{
    int value = 10;

    printf("=== PROCESS DEMONSTRATION ===\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Child process
        value = 20;

        printf("Child process: PID = %d, value = %d\n",
               getpid(), value);
        exit(0);
    } else {
        // Parent process
        wait(NULL);

        printf("Parent process: PID = %d, value = %d\n",
               getpid(), value);
    }

    printf("\n=== THREAD DEMONSTRATION ===\n");

    pthread_t threads[2];
    int ids[2] = {1, 2};

    for (int i = 0; i < 2; i++) {
        if (pthread_create(&threads[i], NULL,
                           thread_function, &ids[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Main thread: shared_value = %d\n", shared_value);

    return 0;
}
