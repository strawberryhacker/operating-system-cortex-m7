#include "sd.h"
#include "gpio.h"
#include "clock.h"

void sd_init(void) {
    // Initialize all the data and command pins used for the SD card
    gpio_set_function(GPIOA, 25, GPIO_FUNC_D);
    gpio_set_function(GPIOA, 28, GPIO_FUNC_C);
    gpio_set_function(GPIOA, 30, GPIO_FUNC_C);
    gpio_set_function(GPIOA, 31, GPIO_FUNC_C);
    gpio_set_function(GPIOA, 26, GPIO_FUNC_C);
    gpio_set_function(GPIOA, 27, GPIO_FUNC_C);

    // Enable the MMC clock
    peripheral_clock_enable(18);

    // Initialize the card connected pin
    peripheral_clock_enable(12);
    gpio_set_function(GPIOC, 16, GPIO_FUNC_OFF);
    gpio_set_direction(GPIOC, 16, GPIO_INPUT); 
}

u8 sd_is_connected(void) {
    if (gpio_get_pin_status(GPIOC, 16)) {
        return 0;
    } else {
        return 1;
    }
}
