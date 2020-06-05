#ifndef USART_H
#define USART_H

#include "types.h"
#include "hardware.h"

enum usart_parity {
    USART_PARITY_EVEN,
    USART_PARITY_ODD,
    USART_PARITY_SPACE,
    USART_PARITY_MARK,
    USART_PARITY_NO,
    USART_PARITY_MULTIDROP = 6
};

enum usart_sb {
    USART_SB_ONE,
    USART_SB_TWO = 2
};

enum usart_data {
    USART_DATA_5_BIT,
    USART_DATA_6_BIT,
    USART_DATA_7_BIT,
    USART_DATA_8_BIT
};

struct usart_desc {
    u32 buad_rate;
    enum usart_data data_bits;
    enum usart_sb stop_bits;
    enum usart_parity parity;
};

#define USART_IRQ_RXRDY (1 << 0)
#define USART_IRQ_TXRDY (1 << 1)

void usart_init(usart_reg* reg, const struct usart_desc* desc);

void usart_deinit(usart_reg* reg);

void usart_interrupt_enable(usart_reg* reg, u32 mask);

static inline void usart_write(usart_reg* reg, u8 data) {
    // Check that the THR is empty
    while (!(reg->CSR & (1 << 1)));
    reg->THR = data;
}

static inline u8 usart_read(usart_reg* reg) {
    return reg->RHR;
}

#endif