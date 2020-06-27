#include "ethernet.h"
#include "gpio.h"
#include "clock.h"
#include "nvic.h"

void eth_init(void) {

    // PHY management signals
    gpio_set_function(GPIOD, 8, GPIO_FUNC_A);
    gpio_set_function(GPIOD, 9, GPIO_FUNC_A);

    // PHY reference clock
    gpio_set_function(GPIOD, 0, GPIO_FUNC_A);

    // PHY reset
    gpio_set_function(GPIOC, 10, GPIO_FUNC_OFF);
    gpio_set(GPIOC, 10);
    gpio_set_direction(GPIOC, 10, GPIO_OUTPUT);

    // Ethernet signals
    gpio_set_function(GPIOD, 2, GPIO_FUNC_A);
    gpio_set_function(GPIOC, 3, GPIO_FUNC_A);
    gpio_set_function(GPIOD, 1, GPIO_FUNC_A);
    gpio_set_function(GPIOD, 5, GPIO_FUNC_A);
    gpio_set_function(GPIOD, 6, GPIO_FUNC_A);
    gpio_set_function(GPIOD, 7, GPIO_FUNC_A);
    gpio_set_function(GPIOD, 4, GPIO_FUNC_A);

    // Enable the GMAC clock
    peripheral_clock_enable(39);

    // Configure the NVIC
    nvic_enable(39);
}

/// Assert the PHY reset line
void eth_phy_reset_assert(void) {
    gpio_clear(GPIOC, 10);
}

/// Deassert the PHY reset line
void eth_phy_reset_deassert(void) {
    gpio_set(GPIOC, 10);
}
