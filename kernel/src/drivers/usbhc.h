/* Copyright (C) StrawberryHacker */

#ifndef usbhc_H
#define usbhc_H

#include "types.h"
#include "dlist.h"

/* Defines the maximum number of pipes supported by hardware */
#define MAX_PIPES 10

#define MAX_URBS 256

#define USBHC_DPRAM_ADDR 0xA0100000
#define USBHC_DPRAM_EP_SIZE 0x8000

/*
 * Returns an 8 bit pointer to the DPRAM base address associated with a pipe
 */
#define usbhc_get_fifo_ptr(pipe) \
	(volatile u8 *)(USBHC_DPRAM_ADDR + (USBHC_DPRAM_EP_SIZE * pipe))

/*
 * Defines the USB mode of operation 
 */
enum usb_mode {
    USB_HOST,
    USB_DEVICE
};

/*
 * This enum indicates the device speed in host mode
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
 * USB request block. This will be able to perform all kinds of USB transfers.
 */
struct urb {
    char name[16];
    struct dlist_node node;

    /* Buffer for setup data */
    u8* setup_buffer;

    /* Buffer for IN and OUT requests */
    u8* data_buffer;
    u32 data_size;
    u8 status;
    u8 state;
};

/*
 * The USB driver will include one pipe descriptor per pipe
 */
struct usb_pipe {
    u8* dpram;
    u32 number;

    struct dlist urb_queue;
};

/*
 * 
 */
struct usbhc {
    struct usb_pipe* pipe_base;
    u32 pipe_count;
};

/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger 
 * and interrupt. The CPU can only read/write FRZCLK and USBE when
 * this but is set
 */
void usbhc_freeze_clock(void);

/*
 * Unfreezes the USB clock
 */
void usbhc_unfreeze_clock(void);

/*
 * Enable the USB interface
 */
void usbhc_enable(void);

/*
 * Disables the USB interface. This act as a hardware reset, thus 
 * resetting USB interface, disables the USB tranceiver and disables
 * the USB clock inputs. This does not reset FRZCLK and UIMOD
 */
void usbhc_disable(void);

/*
 * Sets the USB operating mode; host or device
 */
void usbhc_set_mode(enum usb_mode mode);

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if
 * the clock is usable, 0 if not
 */
u8 usbhc_clock_usable(void);

/*
 * Sends a USB reset. It might be useful to write this bit to 
 * zero when a device disconnection is detected.
 */
void usbhc_send_reset(void);

void usbhc_init(struct usbhc* hc, struct usb_pipe* pipe, u32 pipe_count);

/*************************************************************************/
/* URBs                                                                  */
/*************************************************************************/
struct urb* usbhc_urb_new(void);

u8 usbhc_urb_cancel(struct urb* urb, struct usb_pipe* pipe);

void usbhc_urb_submit(struct urb* urb, struct usb_pipe* pipe);

void print_urb_list(struct usb_pipe* pipe);

#endif