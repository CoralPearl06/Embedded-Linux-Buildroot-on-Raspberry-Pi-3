
#include <stdio.h>
#include <pthread.h>

#define ITERATIONS 1000000

long long counter = 0;
pthread_mutex_t lock;

void *increment_without_mutex(void *arg)
{
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        counter++;
    }

    return NULL;
}

void *increment_with_mutex(void *arg)
{
    (void)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    // Experiment 1: Without mutex
    counter = 0;

    pthread_create(&t1, NULL, increment_without_mutex, NULL);
    pthread_create(&t2, NULL, increment_without_mutex, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Without mutex: %lld (expected %d)\n",
           counter, 2 * ITERATIONS);

    // Experiment 2: With mutex
    counter = 0;
    pthread_mutex_init(&lock, NULL);

    pthread_create(&t1, NULL, increment_with_mutex, NULL);
    pthread_create(&t2, NULL, increment_with_mutex, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("With mutex:    %lld (expected %d)\n",
           counter, 2 * ITERATIONS);

    pthread_mutex_destroy(&lock);

    return 0;
}
