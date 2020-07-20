#ifndef USBHS_H
#define USBHS_H

#include "types.h"

#define USBHS_DPRAM_ADDR 0xA0100000
#define USBHS_DPRAM_EP_SIZE 0x8000

#define usbhs_get_fifo_ptr(epn, scale)                                                                         \
	(volatile uint##scale##_t *)(USBHS_DPRAM_ADDR + USBHS_DPRAM_EP_SIZE * epn)

#define _usbhs_get_fifo_ptr(epn, scale)                                                                         \
	(((volatile uint##scale##_t(*)[USBHS_DPRAM_EP_SIZE / ((scale) / 8)]) USBHS_DPRAM_ADDR)[(epn)])

/*
 * Defines the USB mode of operation 
 */
enum usb_mode {
    USB_HOST,
    USB_DEVICE
};

/*
 * This enum indicates the deivce speed status in host mode
 */
enum usb_device_speed {
    USB_DEVICE_FS,
    USB_DEVICE_HS,
    USB_DEVICE_LS
};

/*
 * This enum specifies the host speed capability
 */
enum usb_host_speed {
    USB_HOST_SPEED_NORMAL,
    USB_HOST_SPEED_LS,
    USB_HOST_SPEED_HS,
    USB_HOST_SPEED_FS
};

/*
 * Defines the tokens available for a pipe
 */
enum pipe_token {
    PIPE_TOKEN_SETUP,
    PIPE_TOKEN_IN,
    PIPE_TOKEN_OUT
};

/*
 * Initializes the USB interface
 */
void usbhs_init(void);

/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger 
 * and interrupt. The CPU can only read/write FRZCLK and USBE when
 * this but is set
 */
void usbhs_freeze_clock(void);

/*
 * Unfreezes the USB clock
 */
void usbhs_unfreeze_clock(void);

/*
 * Enable the USB interface
 */
void usbhs_enable(void);

/*
 * Disables the USB interface. This act as a hardware reset, thus 
 * resetting USB interface, disables the USB tranceiver and disables
 * the USB clock inputs. This does not reset FRZCLK and UIMOD
 */
void usbhs_disable(void);

/*
 * Sets the USB operating mode; host or device
 */
void usbhs_set_mode(enum usb_mode mode);

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if
 * the clock is usable, 0 if not
 */
u8 usbhs_clock_usable(void);

#endif