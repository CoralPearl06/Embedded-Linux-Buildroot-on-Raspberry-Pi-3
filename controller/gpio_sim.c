#include <stdio.h>
#include "gpio.h"

static int initialized = 0;

int gpio_init(void)
{
    initialized = 1;

    printf("[GPIO SIM] GPIO initialized\n");
    printf("[GPIO SIM] LED ready\n");

    return 0;
}

void gpio_set_led(int state)
{
    if (!initialized)
    {
        fprintf(stderr, "[GPIO SIM] Error: GPIO not initialized\n");
        return;
    }

    if (state)
        printf("[GPIO SIM] LED ON\n");
    else
        printf("[GPIO SIM] LED OFF\n");
}

void gpio_cleanup(void)
{
    if (initialized)
    {
        printf("[GPIO SIM] LED OFF\n");
        printf("[GPIO SIM] GPIO cleanup complete\n");
        initialized = 0;
    }
}
