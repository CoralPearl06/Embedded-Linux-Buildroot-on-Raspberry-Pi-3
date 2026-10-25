
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>

void *worker(void *arg)
{
    const char *name = (const char *)arg;

    int policy;
    struct sched_param param;

    int rc = pthread_getschedparam(pthread_self(), &policy, &param);

    if (rc != 0) {
        printf("%s: cannot read scheduling settings: %s\n",
               name, strerror(rc));
        return NULL;
    }

    const char *policy_name = "UNKNOWN";

    if (policy == SCHED_OTHER) policy_name = "SCHED_OTHER";
    if (policy == SCHED_RR)    policy_name = "SCHED_RR";
    if (policy == SCHED_FIFO)  policy_name = "SCHED_FIFO";

    printf("%s: policy=%s, priority=%d\n",
           name, policy_name, param.sched_priority);

    // Small CPU workload
    volatile unsigned long long total = 0;
    for (unsigned long long i = 0; i < 50000000ULL; i++) {
        total += i;
    }

    printf("%s: workload finished (total=%llu)\n", name, total);
    return NULL;
}

int main(void)
{
    pthread_t normal_thread, realtime_thread;
    pthread_attr_t attr;
    struct sched_param param;

    // Create a normal scheduling thread
    int rc = pthread_create(&normal_thread, NULL, worker, "Normal thread");
    if (rc != 0) {
        fprintf(stderr, "Normal thread creation failed: %s\n", strerror(rc));
        return 1;
    }

    // Try creating a round-robin real-time thread
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);

    param.sched_priority = sched_get_priority_min(SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);

    rc = pthread_create(&realtime_thread, &attr, worker, "SCHED_RR thread");

    if (rc != 0) {
        printf("SCHED_RR thread creation failed: %s\n", strerror(rc));
        printf("This is often due to missing real-time scheduling permission.\n");
    } else {
        pthread_join(realtime_thread, NULL);
    }

    pthread_attr_destroy(&attr);
    pthread_join(normal_thread, NULL);

    return 0;
}
