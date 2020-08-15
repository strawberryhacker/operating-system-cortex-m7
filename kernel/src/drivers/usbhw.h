/* Copyright (C) StrawberryHacker */

#ifndef USBHW_H
#define USBHW_H

#include "types.h"
#include "panic.h"
#include "hardware.h"

/* USB host controller registers bitmasks */
#define USBHW_RXIN      (u32)(1 << 0)
#define USBHW_TXOUT     (u32)(1 << 1)
#define USBHW_TXSETUP   (u32)(1 << 2)
#define USBHW_PERROR    (u32)(1 << 3)
#define USBHW_NAKED     (u32)(1 << 4)
#define USBHW_OVERFLOW  (u32)(1 << 5)
#define USBHW_STALL     (u32)(1 << 6)
#define USBHW_SHORTPKT  (u32)(1 << 7)

#define USBHW_CONN      (u32)(1 << 0)
#define USBHW_DCONN     (u32)(1 << 1)
#define USBHW_RST       (u32)(1 << 2)
#define USBHW_DOWN_RES  (u32)(1 << 3)
#define USBHW_UP_RES    (u32)(1 << 4)
#define USBHW_SOF       (u32)(1 << 5)
#define USBHW_WAKEUP    (u32)(1 << 6)

#define USBHW_FIFO_CTRL (u32)(1 << 14)
#define USBHW_PFREEZE   (u32)(1 << 17)

#define USBHW_PIPE_OFFSET 8
#define USBHW_DMA_OFFSET  25

enum usb_mode {
    USB_HOST,
    USB_DEVICE
};

enum usb_device_speed {
    USB_DEVICE_FS,
    USB_DEVICE_HS,
    USB_DEVICE_LS
};

enum usb_host_speed {
    USB_HOST_SPEED_NORMAL,
    USB_HOST_SPEED_LS,
    USB_HOST_SPEED_HS,
    USB_HOST_SPEED_FS
};

enum pipe_token {
    PIPE_TOKEN_SETUP,
    PIPE_TOKEN_IN,
    PIPE_TOKEN_OUT
};

enum pipe_type {
    PIPE_TYPE_CTRL,
    PIPE_TYPE_ISO,
    PIPE_TYPE_BLK,
    PIPE_TYPE_INT
};

enum pipe_banks {
    PIPE_BANKS_1,
    PIPE_BANKS_2,
    PIPE_BANKS_3
};

enum pipe_size {
    PIPE_SIZE_8,
    PIPE_SIZE_16,
    PIPE_SIZE_32,
    PIPE_SIZE_64,
    PIPE_SIZE_128,
    PIPE_SIZE_256,
    PIPE_SIZE_512,
    PIPE_SIZE_1024
};

/* General USB host core stuff */
static inline void usbhw_connection_error_enable(void);
static inline void usbhw_connection_error_disable(void);
static inline u8   usbhw_connection_error_check(void);
static inline void usbhw_connection_error_clear(void);
static inline void usbhw_connection_error_force(void);

static inline void usbhw_vbus_request_enable(void);
static inline void usbhw_set_mode(enum usb_mode mode);
static inline enum usb_device_speed usbhw_get_device_speed(void);
static inline void usbhw_set_host_speed(enum usb_host_speed speed);
static inline void usbhw_freeze_clock(void);
static inline void usbhw_unfreeze_clock(void);
static inline u8   usbhw_clock_usable(void);
static inline void usbhw_enable(void);
static inline void usbhw_disable(void);

static inline void usbhw_send_resume(void);
static inline void usbhw_send_reset(void);
static inline void usbhw_clear_reset(void);
static inline void usbhw_sof_enable(void);
static inline void usbhw_sof_disable(void);

/* Global USB host controller interrupts */
static inline u32  usbhw_global_get_status(void);
static inline u32  usbhw_global_get_interrupt_mask(void);
static inline void usbhw_global_clear_status(u32 mask);
static inline void usbhw_global_force_status(u32 mask);
static inline void usbhw_global_disable_interrupt(u32 mask);
static inline void usbhw_global_enable_interrupt(u32 mask);

/* Pipe interrupts */
static inline u32  usbhw_pipe_get_status(u8 pipe);
static inline u32  usbhw_pipe_get_interrupt_mask(u8 pipe);
static inline void usbhw_pipe_clear_status(u8 pipe, u32 mask);
static inline void usbhw_pipe_force_status(u8 pipe, u32 mask);
static inline void usbhw_pipe_disable_interrupt(u8 pipe, u32 mask);
static inline void usbhw_pipe_enable_interrupt(u8 pipe, u32 mask);

/* Frame info */
static inline u32  usbhw_get_frame_number(void);
static inline void usbhw_clear_frame_number(void);
static inline u8   usbhw_get_microframe_number(void);

/* Pipes maintainance */
static inline void usbhw_pipe_reset_assert(u8 pipe);
static inline void usbhw_pipe_reset_deassert(u8 pipe);
static inline void usbhw_pipe_allocate_set(u8 pipe);
static inline void usbhw_pipe_allocate_clear(u8 pipe);
static inline void usbhw_pipe_enable(u8 pipe);
static inline void usbhw_pipe_disable(u8 pipe);
static inline void usbhw_pipe_set_addr(u8 pipe, u8 addr);
static inline void usbhw_pipe_in_request_defined(u8 pipe, u8 count);
static inline void usbhw_pipe_in_request_continous(u8 pipe);
static inline void usbhw_pipe_set_configuration(u8 pipe, u32 cfg);
static inline u32  usbhw_pipe_get_configuration(u8 pipe);
static inline u8   usbhw_pipe_check_configuration(u8 pipe);
static inline void usbhw_pipe_set_token(u8 pipe, enum pipe_token token);
static inline void usbhw_pipe_set_freq(u8 pipe, u8 irq_freq);
static inline u32  usbhw_pipe_get_byte_count(u8 pipe);
static inline u32  usbhw_pipe_get_error_reg(u8 pipe);

static inline void usbhw_connection_error_enable(void) 
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 4);
    USBHC->CTRL = reg;
}

static inline void usbhw_connection_error_disable(void) 
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 4);
    USBHC->CTRL = reg;
}

static inline u8 usbhw_connection_error_check(void)
{
    if (USBHC->SR & (1 << 4)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Returns the speed status. This should be checked at the end of a reset
 * request. In host mode two pulldowns are connected. The speed status is based
 * on which line the device pulls up. 
 */
static inline enum usb_device_speed usbhw_get_device_speed(void)
{
    u32 reg = (USBHC->SR >> 12) & 0b11;
    return (enum usb_device_speed)reg;
}

static inline void usbhw_connection_error_clear(void)
{
    USBHC->SCR = (1 << 4);
}

static inline void usbhw_connection_error_force(void)
{
    USBHC->SFR = (1 << 4);
}

/*
 * Enables the VBUS request. The host includes two weak pulldowns on the D+
 * and D- lines. When a devices is connected one of the host pulldown resistors
 * is overpowered by one of the devices pullups. Which line is pulled high
 * determines the speed of the deivce. To enable this detection VBS request
 * must be enabled.
 */
static inline void usbhw_vbus_request_enable(void)
{
    USBHC->SFR = (1 << 9);
}

/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger an interrupt.
 * The CPU can only read/write FRZCLK and USBE when this bit is set
 */
static inline void usbhw_freeze_clock(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 14);
    USBHC->CTRL = reg;
}

static inline void usbhw_unfreeze_clock(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 14);
    USBHC->CTRL = reg; 
}

static inline void usbhw_enable(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 15);
    USBHC->CTRL = reg;
}

/*
 * Disables the USB interface. This act as a hardware reset, thus resetting USB
 * interface, disables the USB tranceiver and disables the USB clock inputs.
 * This does not reset FRZCLK and UIMOD
 */
static inline void usbhw_disable(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 15);
    USBHC->CTRL = reg;
}

static inline void usbhw_set_mode(enum usb_mode mode)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */

    if (mode == USB_HOST) {
        reg &= ~(1 << 25);
    } else {
        reg |= (1 << 25);
    }
    USBHC->CTRL = reg;
}

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if the clock is
 * usable, 0 if not
 */
static inline u8 usbhw_clock_usable(void)
{
    if (USBHC->SR & (1 << 14)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Sends a USB resume on the bus. A downstream USB resume is issued by the host
 * in order to wake up a suspended device. The USB resume is a K-state for at
 * least 20ms, followed by a low speed EOP. 
 */
static inline void usbhw_send_resume(void)
{
    /* According to the datasheet SOF must be enabled */
    if (!(USBHC->HSTCTRL & (1 << 8))) {
        panic("Attempted resume without SOF enabled");
    }

    USBHC->HSTCTRL |= (1 << 10);
}

/*
 * Sends a USB reset. It might be useful to write this bit to zero when a device
 * disconnection is detected. This sets any connected device to its default
 * unconfigured state. It sends the reset by pulling both data lines low for at
 * least 10 ms (SE0)
 */
static inline void usbhw_send_reset(void)
{
    USBHC->HSTCTRL |= (1 << 9);
}

/*
 * Clears the reset bit in the configuration register. This is said to have no
 * effect, but right above it, they reccomended clearing it. This should be 
 * done after a device disconnection to avoid any unintentional reset. 
 */
static inline void usbhw_clear_reset(void)
{
    USBHC->HSTCTRL &= ~(1 << 9);
}

/*
 * Starts generating SOFs and uSOFs. This will be automatically generated after
 * a bus reset if the host was in suspend state (SOFEN zero). This also
 * sets the WAKEUP flag.
 */
static inline void usbhw_sof_enable(void)
{
    USBHC->HSTCTRL |= (1 << 8);
}

static inline void usbhw_sof_disable(void)
{
    USBHC->HSTCTRL &= ~(1 << 8);
}

static inline void usbhw_set_host_speed(enum usb_host_speed speed)
{
    u32 reg = USBHC->HSTCTRL;
    reg &= ~(0b11 << 12);
    reg |= (speed << 12);
    USBHC->HSTCTRL = reg;
}

/*
 * Interrupt section
 * 
 * The USB interface has a quite complicated interrupt structure. A overview
 * can be found at page 754 in the datasheet. The host controller has ONE global
 * status register used when detecting interrupts. This register includes some
 * basic flags used for the root hub. In addition it includes one flag per pipe
 * used to indicate that a pipe interrupt has happended. A pipe interrupt has an
 * independent status register used to report events. The same goes for DMA.
 * If the pipe flag is cleared no pipe interrupt is generated.  
 */

static inline u32 usbhw_global_get_status(void)
{
    return USBHC->HSTISR;
}

static inline void usbhw_global_clear_status(u32 mask)
{
    USBHC->HSTICR = mask;
}

static inline void usbhw_global_force_status(u32 mask)
{
    USBHC->HSTIFR = mask;
}

static inline u32 usbhw_global_get_interrupt_mask(void)
{
    return USBHC->HSTIMR;
}

static inline void usbhw_global_disable_interrupt(u32 mask)
{
    USBHC->HSTIDR = mask;
}

static inline void usbhw_global_enable_interrupt(u32 mask)
{
    USBHC->HSTIER = mask;
}

static inline u32 usbhw_get_frame_number(void)
{
    return (u32)((USBHC->HSTFNUM >> 3) & 0x7FF);
}

static inline u8  usbhw_get_microframe_number(void)
{
    return (u8)(USBHC->HSTFNUM & 0x7);
}

static inline void usbhw_clear_frame_number(void)
{
    /* Perform a write operation to the FNUM field */
    USBHC->HSTFNUM &= ~(0x7FF << 3);
}

/*
 * Sets the pipe endpoint address. It takes in the endpoint number between
 * 0 and 9, and a 7 bit address identifying the USB device on the bus. Note that
 * pipe 0 should have its address set to one
 */
static inline void usbhw_pipe_set_addr(u8 pipe, u8 addr)
{
    /* I don't know if byte or halfword access are allowed */
    u32* reg_ptr = (u32 *)&USBHC->HSTADDR1 + (pipe >> 2);

    /* Compute the offset in the current register */
    u8 offset = ((pipe & 0x3) << 0x3);

    /* Update the address */
    u32 reg = *reg_ptr;
    reg &= ~(0x7F << offset);
    reg |= ((addr & 0x7F) << offset);
    *reg_ptr = reg;
}

static inline void usbhw_pipe_reset_assert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP |= (1 << (pipe + 16));
}

static inline void usbhw_pipe_reset_deassert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP &= ~(1 << (pipe + 16));
}

static inline void usbhw_pipe_enable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP |= (1 << pipe);
}

static inline void usbhw_pipe_disable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP &= ~(1 << pipe);
}

static inline u32 usbhw_pipe_get_status(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPISR[pipe];
}

static inline void usbhw_pipe_clear_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPICR[pipe] = mask;
}

static inline void usbhw_pipe_force_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIFR[pipe] = mask;
}

static inline u32 usbhw_pipe_get_interrupt_mask(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPIMR[pipe];
}

static inline void usbhw_pipe_enable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIER[pipe] = mask;
}

static inline void usbhw_pipe_disable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIDR[pipe] = mask;
}

/*
 * Performs a predefined number of IN request before the pipe is frozen. This
 * makes the device send IN packets. 
 */
static inline void usbhw_pipe_in_request_defined(u8 pipe, u8 count)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    if (count == 0) {
        panic("Cannot perform zero INs");
    }
    USBHC->HSTPIPINRQ[pipe] = count - 1;
}

/*
 * Performs IN requests continously on the given pipe until the pipe is frozen
 */
static inline void usbhw_pipe_in_request_continous(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPINRQ[pipe] = (1 << 8);
}

static inline void usbhw_pipe_set_configuration(u8 pipe, u32 cfg)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] = cfg;
}

static inline void usbhw_pipe_allocate_set(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] |= (1 << 1);
}

static inline void usbhw_pipe_allocate_clear(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] &= ~(1 << 1);
}

static inline u32 usbhw_pipe_get_configuration(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPCFG[pipe];
}

/*
 * Checks the given pipe configuration status. This indicates if the
 * configurations fields are set according to the pipe capabilities.
 */
static inline u8 usbhw_pipe_check_configuration(u8 pipe) 
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }

    if (USBHC->HSTPIPISR[pipe] & (1 << 18)) {
        return 1;
    } else {
        return 0;
    }
}

static inline void usbhw_pipe_set_token(u8 pipe, enum pipe_token token)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHC->HSTPIPCFG[pipe];
    reg &= ~(0b11 << 8);
    reg |= (token << 8);
    USBHC->HSTPIPCFG[pipe] = reg;
}

static inline void usbhw_pipe_set_freq(u8 pipe, u8 irq_freq)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHC->HSTPIPCFG[pipe];
    reg &= ~(0xFF << 24);
    reg |= (irq_freq << 24);
    USBHC->HSTPIPCFG[pipe] = reg;
}

static inline u32 usbhw_pipe_get_byte_count(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }

    /* Delay so that the hardware can update the size */
    for (volatile u8 i = 0; i < 2; i++) {
        asm volatile ("nop");
    }

    return (USBHC->HSTPIPISR[pipe] >> 20) & 0x7FF;
}

static inline u32 usbhw_pipe_get_error_reg(u8 pipe)
{
    return USBHC->HSTPIPERR[pipe];
}

#endif