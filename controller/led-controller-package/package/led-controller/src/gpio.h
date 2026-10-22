#ifndef GPIO_H
#define GPIO_H

int gpio_init(void);
void gpio_set_led(int state);
void gpio_cleanup(void);

#endif
