
#include <stdio.h>
#include <pthread.h>

#define THREADS 4
#define ITERATIONS 500000

long shared_counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg)
{
    int id = *(int *)arg;

    for (int i = 0; i < ITERATIONS; i++) {

        // Critical section begins
        pthread_mutex_lock(&mutex);

        shared_counter++;

        pthread_mutex_unlock(&mutex);
        // Critical section ends
    }

    printf("Thread %d completed.\n", id);
    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];
    int ids[THREADS];

    for (int i = 0; i < THREADS; i++) {
        ids[i] = i + 1;

        int rc = pthread_create(&threads[i], NULL, worker, &ids[i]);
        if (rc != 0) {
            fprintf(stderr, "Failed to create thread %d\n", ids[i]);
            return 1;
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Final counter: %ld\n", shared_counter);
    printf("Expected counter: %d\n", THREADS * ITERATIONS);

    pthread_mutex_destroy(&mutex);
    return 0;
}
