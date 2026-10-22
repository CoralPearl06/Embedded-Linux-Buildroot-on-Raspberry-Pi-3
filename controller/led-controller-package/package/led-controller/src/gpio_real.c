#include <stdio.h>
#include <gpiod.h>
#include "gpio.h"

#define GPIO_CHIP_NAME "/dev/gpiochip0"
#define LED_GPIO_PIN  17

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *led_line = NULL;

int gpio_init(void)
{
    chip = gpiod_chip_open(GPIO_CHIP_NAME);

    if (chip == NULL)
    {
        perror("Failed to open GPIO chip");
        return -1;
    }

    led_line = gpiod_chip_get_line(chip, LED_GPIO_PIN);

    if (led_line == NULL)
    {
        perror("Failed to get GPIO line");
        gpiod_chip_close(chip);
        chip = NULL;
        return -1;
    }

    if (gpiod_line_request_output(led_line, "embedded-controller", 0) < 0)
    {
        perror("Failed to request GPIO output");
        gpiod_chip_close(chip);
        chip = NULL;
        led_line = NULL;
        return -1;
    }

    printf("[GPIO REAL] GPIO initialized; LED uses GPIO%d\n", LED_GPIO_PIN);
    return 0;
}

void gpio_set_led(int state)
{
    if (led_line == NULL)
    {
        fprintf(stderr, "[GPIO REAL] Error: GPIO not initialized\n");
        return;
    }

    if (gpiod_line_set_value(led_line, state ? 1 : 0) < 0)
        perror("Failed to set LED state");
}

void gpio_cleanup(void)
{
    if (led_line != NULL)
    {
        gpiod_line_set_value(led_line, 0);
        gpiod_line_release(led_line);
        led_line = NULL;
    }

    if (chip != NULL)
    {
        gpiod_chip_close(chip);
        chip = NULL;
    }

    printf("[GPIO REAL] GPIO cleanup complete\n");
}
