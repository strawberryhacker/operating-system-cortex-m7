#include "usart.h"

void usart_init(usart_reg* reg, const struct usart_desc* desc) {
    // Disable and reset transmitter and receiver
    reg->CR = (1 << 5) | (1 << 7) | (1 << 2) | (1 << 3);

    // Write the mode register
    reg->RTOR = 0;
	reg->TTGR = 0;
    reg->MR = (1 << 20) | (desc->data_bits << 6) | (desc->stop_bits << 12) |
        (desc->parity << 9);
    
    // Update the buad rate
    reg->BRGR = (u16)(150000000/(16 * desc->buad_rate));

    // Enable transmitter and receiver
    reg->CR = (1 << 4) | (1 << 6);
}

void usart_deinit(usart_reg* reg) {
    // Disable and reset the USART
    reg->CR = (1 << 4) | (1 << 6) | (1 << 2) | (1 << 3);
}

void usart_interrupt_enable(usart_reg* reg, u32 mask) {
    reg->IER = mask;
}