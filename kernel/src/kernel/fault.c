#include "debug.h"
#include "panic.h"

void mem_fault_handler(void) {
    panic("Memory fault");
}

void bus_fault_handler(void) {
    panic("Bus fault");
}

void usage_fault_handler(void) {
    panic("Usage fault");
}

void hard_fault_handler(void) {
    panic("Hard fault");
}