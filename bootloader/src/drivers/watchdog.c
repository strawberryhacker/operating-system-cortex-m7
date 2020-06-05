#include "watchdog.h"
#include "hardware.h"

void watchdog_disable(void) {
    WATCHDOG->MR = (1 << 15);
}