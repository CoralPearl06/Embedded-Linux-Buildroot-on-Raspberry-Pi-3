
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define NUM_THREADS 4

void *worker(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < 5; i++) {
        int cpu = sched_getcpu();

        printf("Thread %d | PID %d | CPU core %d\n",
               id, getpid(), cpu);

        // Give other threads a chance to run
        sched_yield();
    }

    return NULL;
}

int main(void)
{
    long available = sysconf(_SC_NPROCESSORS_ONLN);

    printf("Online CPU cores reported: %ld\n", available);

    if (available < 1) {
        fprintf(stderr, "No online CPU cores detected.\n");
        return 1;
    }

    int usable = available < NUM_THREADS ? (int)available : NUM_THREADS;

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    cpu_set_t allowed;
    CPU_ZERO(&allowed);

    // Restrict worker threads to the first few allowed CPU IDs.
    // CPU IDs may not start at zero in containers or restricted environments.
    cpu_set_t process_mask;
    CPU_ZERO(&process_mask);

    if (sched_getaffinity(0, sizeof(process_mask), &process_mask) != 0) {
        perror("sched_getaffinity");
        return 1;
    }

    int cpus[NUM_THREADS];
    int found = 0;

    for (int cpu = 0; cpu < CPU_SETSIZE && found < usable; cpu++) {
        if (CPU_ISSET(cpu, &process_mask)) {
            cpus[found++] = cpu;
        }
    }

    if (found == 0) {
        fprintf(stderr, "No allowed CPU cores found.\n");
        return 1;
    }

    printf("Available CPU IDs for this process:");
    for (int i = 0; i < found; i++) {
        printf(" %d", cpus[i]);
    }
    printf("\n");

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i + 1;

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        CPU_ZERO(&allowed);
        CPU_SET(cpus[i % found], &allowed);

        int rc = pthread_attr_setaffinity_np(
            &attr, sizeof(allowed), &allowed);

        if (rc != 0) {
            fprintf(stderr, "Failed to set affinity for thread %d: %d\n",
                    ids[i], rc);
            pthread_attr_destroy(&attr);
            return 1;
        }

        rc = pthread_create(&threads[i], &attr, worker, &ids[i]);
        pthread_attr_destroy(&attr);

        if (rc != 0) {
            fprintf(stderr, "Failed to create thread %d: %d\n",
                    ids[i], rc);
            return 1;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("All worker threads finished.\n");
    return 0;
}
