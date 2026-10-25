
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define NUM_THREADS 4
#define ITERATIONS 500000L
#define RUNS 5

long long unsafe_counter;
long long mutex_counter;
atomic_llong atomic_counter;

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void *unsafe_increment(void *arg)
{
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++)
        unsafe_counter++;
    return NULL;
}

void *mutex_increment(void *arg)
{
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++) {
        pthread_mutex_lock(&counter_mutex);
        mutex_counter++;
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

void *atomic_increment(void *arg)
{
    (void)arg;
    for (long i = 0; i < ITERATIONS; i++)
        atomic_fetch_add(&atomic_counter, 1);
    return NULL;
}

double get_time(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

typedef void *(*thread_function)(void *);

double run_once(thread_function function)
{
    pthread_t threads[NUM_THREADS];
    double start = get_time();

    for (int i = 0; i < NUM_THREADS; i++) {
        int error = pthread_create(&threads[i], NULL, function, NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_create failed: %d\n", error);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        int error = pthread_join(threads[i], NULL);
        if (error != 0) {
            fprintf(stderr, "pthread_join failed: %d\n", error);
            exit(EXIT_FAILURE);
        }
    }

    return get_time() - start;
}

int main(void)
{
    const long long expected =
        (long long)NUM_THREADS * ITERATIONS;

    double total_unsafe = 0;
    double total_mutex = 0;
    double total_atomic = 0;

    printf("Synchronization Benchmark\n");
    printf("Threads: %d | Increments per thread: %ld\n",
           NUM_THREADS, ITERATIONS);
    printf("Runs per method: %d\n", RUNS);
    printf("Expected count per run: %lld\n\n", expected);

    for (int run = 1; run <= RUNS; run++) {
        unsafe_counter = 0;
        mutex_counter = 0;
        atomic_store(&atomic_counter, 0);

        total_unsafe += run_once(unsafe_increment);
        total_mutex += run_once(mutex_increment);
        total_atomic += run_once(atomic_increment);

        printf("Run %d:\n", run);
        printf("  No synchronization: %lld (%s)\n",
               unsafe_counter,
               unsafe_counter == expected ? "expected count" : "COUNT MISMATCH");
        printf("  Mutex:              %lld (%s)\n",
               mutex_counter,
               mutex_counter == expected ? "correct" : "COUNT MISMATCH");
        printf("  Atomic:             %lld (%s)\n\n",
               (long long)atomic_load(&atomic_counter),
               atomic_load(&atomic_counter) == expected
                   ? "correct" : "COUNT MISMATCH");
    }

    printf("Average execution time over %d runs:\n", RUNS);
    printf("No synchronization: %.6f seconds\n", total_unsafe / RUNS);
    printf("Mutex:              %.6f seconds\n", total_mutex / RUNS);
    printf("Atomic:             %.6f seconds\n", total_atomic / RUNS);

    pthread_mutex_destroy(&counter_mutex);
    return 0;
}
