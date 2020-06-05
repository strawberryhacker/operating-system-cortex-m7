#include "interrupt.h"
#include "hardware.h"

void interrupt_enable(u8 irq_number) {
    NVIC->ISER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

void interrupt_disable(u8 irq_number) {

}

void interrupt_set_prioriy(u8 irq_number) {

}

void interrupt_pend(u8 irq_number) {

}