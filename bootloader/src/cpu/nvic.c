#include "nvic.h"
#include "hardware.h"

void nvic_enable(u8 irq_number) {
    NVIC->ISER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

void nvic_disable(u8 irq_number) {
	NVIC->ICER[irq_number >> 5] = (1 << (irq_number & 0b11111));
}

void nvic_set_prioriy(u8 irq_number) {

}

void nvic_pend(u8 irq_number) {

}

void nvic_clear_pending(u8 irq_number) {
	
}