#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>
#include "gpio.h"
#include <signal.h>

#define QUEUE_NAME "/led_queue"
#define MSG_SIZE 64

#define MIN_INTERVAL_MS 50
#define MAX_INTERVAL_MS 5000

#define POLL_DELAY_NS 1000000L   /* 1 ms */

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int signum)
{
    (void)signum;
    stop_requested = 1;
}

/* Convert a timespec to nanoseconds. */
static int64_t to_ns(const struct timespec *t)
{
    return (int64_t)t->tv_sec * 1000000000LL + t->tv_nsec;
}

/* Convert nanoseconds to milliseconds. */
static double ns_to_ms(int64_t ns)
{
    return (double)ns / 1000000.0;
}

/* Add milliseconds to a timestamp. */
static void add_ms(struct timespec *t, long ms)
{
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000L;

    if (t->tv_nsec >= 1000000000L)
    {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

/* Return nonzero when a >= b. */
static int time_reached(const struct timespec *a,
                        const struct timespec *b)
{
    return a->tv_sec > b->tv_sec ||
           (a->tv_sec == b->tv_sec &&
            a->tv_nsec >= b->tv_nsec);
}

/* Sleep for a duration in nanoseconds. */
static void sleep_ns(long ns)
{
    struct timespec req;

    req.tv_sec = ns / 1000000000L;
    req.tv_nsec = ns % 1000000000L;

    while (nanosleep(&req, &req) == -1 && errno == EINTR)
    {
        /* Continue the remaining sleep. */
    }
}

/* Timing statistics. */
struct timing_stats
{
    double min_ms;
    double max_ms;
    double total_ms;
    double total_abs_error_ms;
    long samples;
};

static void reset_stats(struct timing_stats *s)
{
    s->min_ms = 0.0;
    s->max_ms = 0.0;
    s->total_ms = 0.0;
    s->total_abs_error_ms = 0.0;
    s->samples = 0;
}

static void record_interval(struct timing_stats *s,
                           double actual_ms,
                           long target_ms)
{
    double error_ms = actual_ms - (double)target_ms;

    if (s->samples == 0)
    {
        s->min_ms = actual_ms;
        s->max_ms = actual_ms;
    }
    else
    {
        if (actual_ms < s->min_ms)
            s->min_ms = actual_ms;

        if (actual_ms > s->max_ms)
            s->max_ms = actual_ms;
    }

    s->total_ms += actual_ms;
    s->total_abs_error_ms +=
        error_ms < 0.0 ? -error_ms : error_ms;

    s->samples++;
}

static void print_stats(const struct timing_stats *s)
{
    if (s->samples == 0)
    {
        printf("No timing samples recorded yet.\n");
        return;
    }

    printf("\n---------- TIMING STATISTICS ----------\n");
    printf("Timing samples: %ld\n", s->samples);
    printf("Minimum interval: %.3f ms\n", s->min_ms);
    printf("Maximum interval: %.3f ms\n", s->max_ms);

    printf("Average interval: %.3f ms\n",
           s->total_ms / s->samples);

    printf("Average absolute error: %.3f ms\n",
           s->total_abs_error_ms / s->samples);

    printf("Observed interval range: %.3f ms\n",
           s->max_ms - s->min_ms);

    printf("---------------------------------------\n");
}

int main(void)
{
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_SIZE;

    /*
     * Create the command queue.
     * O_NONBLOCK allows the controller to keep checking its timer.
     */
    mqd_t mq = mq_open(
        QUEUE_NAME,
        O_CREAT | O_RDONLY | O_NONBLOCK,
        0660,
        &attr
    );

    if (mq == (mqd_t)-1)
    {
        perror("mq_open");
        return EXIT_FAILURE;
    }

     if (signal(SIGTERM, handle_signal) == SIG_ERR || signal(SIGINT, handle_signal) == SIG_ERR)
    {
        perror("signal");
        mq_close(mq);
        mq_unlink(QUEUE_NAME);
        return EXIT_FAILURE;
    }

    if (gpio_init() != 0)
    {
        fprintf(stderr, "Failed to initialize GPIO\n");
        mq_close(mq);
        mq_unlink(QUEUE_NAME);
        return EXIT_FAILURE;
    }

    int led_state = 0;
    int blinking = 0;
    long interval_ms = 1000;

    char command[MSG_SIZE + 1];

    struct timespec next_toggle = {0};
    struct timespec last_toggle = {0};

    int have_last_toggle = 0;

    struct timing_stats stats;
    reset_stats(&stats);

    printf("====================================\n");
    printf(" Embedded Linux Hardware Controller\n");
    printf(" Precise Timing Version\n");
    printf("====================================\n");

    printf("Message queue: %s\n", QUEUE_NAME);
    printf("Default interval: %ld ms\n", interval_ms);
    printf("Timing clock: CLOCK_MONOTONIC\n");
    printf("Waiting for commands...\n\n");

    fflush(stdout);

    while (!stop_requested)
    {
        /*
         * Process one available command per iteration.
         */
        ssize_t bytes = mq_receive(
            mq,
            command,
            MSG_SIZE,
            NULL
        );

        if (bytes >= 0)
        {
            command[bytes] = '\0';

            if (strcmp(command, "ON") == 0)
            {
                blinking = 0;
                led_state = 1;
		gpio_set_led(1);
                have_last_toggle = 0;

                printf("[GPIO SIM] LED ON\n");
            }
            else if (strcmp(command, "OFF") == 0)
            {
                blinking = 0;
                led_state = 0;
		gpio_set_led(0);
                have_last_toggle = 0;

                printf("[GPIO SIM] LED OFF\n");
            }
            else if (strcmp(command, "START") == 0)
            {
                blinking = 1;
                led_state = 0;
		gpio_set_led(0);
                have_last_toggle = 0;

                reset_stats(&stats);

                clock_gettime(CLOCK_MONOTONIC, &next_toggle);
                add_ms(&next_toggle, interval_ms);

                printf("Blinking started. Interval: %ld ms\n",
                       interval_ms);
            }
            else if (strcmp(command, "STOP") == 0)
            {
                blinking = 0;
                led_state = 0;
		gpio_set_led(0);
                have_last_toggle = 0;

                printf("Blinking stopped. LED OFF\n");
            }
            else if (strncmp(command, "SET_INTERVAL:", 13) == 0)
            {
                char *end;
                errno = 0;

                long value = strtol(command + 13, &end, 10);

                if (errno == 0 &&
                    end != command + 13 &&
                    *end == '\0' &&
                    value >= MIN_INTERVAL_MS &&
                    value <= MAX_INTERVAL_MS)
                {
                    interval_ms = value;

                    printf("Blink interval set to %ld ms\n",
                           interval_ms);

                    if (blinking)
                    {
                        clock_gettime(
                            CLOCK_MONOTONIC,
                            &next_toggle
                        );

                        add_ms(&next_toggle, interval_ms);
                    }

                    have_last_toggle = 0;
                }
                else
                {
                    printf("Invalid interval.\n");
                    printf("Allowed range: %d-%d ms\n",
                           MIN_INTERVAL_MS,
                           MAX_INTERVAL_MS);
                }
            }
            else if (strcmp(command, "STATUS") == 0)
            {
                printf("\n========== CONTROLLER STATUS ==========\n");

                printf("LED Status: %s\n",
                       led_state ? "ON" : "OFF");

                printf("Blinking: %s\n",
                       blinking ? "YES" : "NO");

                printf("Configured interval: %ld ms\n",
                       interval_ms);

                print_stats(&stats);

                printf("=======================================\n");
            }
            else if (strcmp(command, "EXIT") == 0)
            {
                printf("Hardware Controller Stopped\n");
                break;
            }
            else
            {
                printf("Unknown command: %s\n", command);
            }

            fflush(stdout);
        }
        else if (errno != EAGAIN && errno != EINTR)
        {
            perror("mq_receive");
            break;
        }

        /*
         * Periodic simulated GPIO control.
         */
        if (blinking)
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);

            if (time_reached(&now, &next_toggle))
            {
                int64_t now_ns = to_ns(&now);

                led_state = !led_state;
		gpio_set_led(led_state);

                if (have_last_toggle)
                {
                    int64_t elapsed =
                        now_ns - to_ns(&last_toggle);

                    double actual_ms = ns_to_ms(elapsed);

                    record_interval(
                        &stats,
                        actual_ms,
                        interval_ms
                    );

                    printf(
                        "[GPIO SIM] LED %s | "
                        "Actual: %.3f ms | Error: %+.3f ms\n",
                        led_state ? "ON" : "OFF",
                        actual_ms,
                        actual_ms - (double)interval_ms
                    );
                }
                else
                {
                    printf(
                        "[GPIO SIM] LED %s | First transition\n",
                        led_state ? "ON" : "OFF"
                    );

                    have_last_toggle = 1;
                }

                last_toggle = now;

                /*
                 * Schedule from the actual transition time.
                 *
                 * This avoids rapid catch-up transitions if Linux
                 * wakes the process late. The trade-off is that
                 * occasional delays can shift the schedule.
                 */
                next_toggle = now;
                add_ms(&next_toggle, interval_ms);

                fflush(stdout);
            }
        }

        /*
         * Keep the polling delay short so commands remain responsive.
         */
        sleep_ns(POLL_DELAY_NS);
    }
    gpio_cleanup();

    mq_close(mq);
    mq_unlink(QUEUE_NAME);

    return EXIT_SUCCESS;
}
