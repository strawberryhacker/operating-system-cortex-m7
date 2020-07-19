#ifndef USBHS_H
#define USBHS_H

#include "types.h"

/*
 * CUrrenlty this driver will only support host operating mode. The
 * different USB host states are; IDLE, READY and SUSPEND. When the 
 * device is first plugged in the USB interface sees a connection and 
 * moves to READY state, since SOF enable is not set the USB interface 
 * will move directly to suspend state. If SOF are then generated the 
 * tranceiver goes into READY state again. 
 */

enum usb_operation {
    USB_HOST,
    USB_DEVICE
};

enum usb_speed {
    USB_FULL_SPEED,
    USB_HIGH_SPEED,
    USB_LOW_SPEED
};

#define USB_PIPE_TYPE_CTRL      0
#define USB_PIPE_TYPE_ISO       1
#define USB_PIPE_TYPE_BULK      2
#define USB_PIPE_TYPE_INTERRUPT 3

#define USB_PIPE_TOKEN_SETUP    0
#define USB_PIPE_TOKEN_IN       1
#define USB_PIPE_TOKEN_OUT      2

#define USB_PIPE_SIZE_8B        0
#define USB_PIPE_SIZE_16B       1
#define USB_PIPE_SIZE_32B       2
#define USB_PIPE_SIZE_64B       3
#define USB_PIPE_SIZE_128B      4
#define USB_PIPE_SIZE_256B      5
#define USB_PIPE_SIZE_512B      6
#define USB_PIPE_SIZE_1024B     7

#define USB_PIPE_BANKS_1        0
#define USB_PIPE_BANKS_2        1
#define USB_PIPE_BANKS_3        2

/*
 * Pipe structure
 */
struct usb_pipe {
    /* Pipe configuration */
    u8 irq_freq : 8;
    u8 endpoint : 4;
    u8 type     : 2;
    u8 token    : 2;
    u8 autosw   : 1;
    u8 size     : 3;
    u8 banks    : 2;
    u8 alloc    : 1;

    /* Pipe transfer done callback */
    void (*pipe_callback)(struct usb_pipe*);
};

/*
 * Main USB host instance
 */
struct usb_host {
    /* Pointer to the pipes */
    struct usb_pipe* pipes;
    u32 pipe_count;
};

/*
 * Enables the USB interface and un-freezes the clock. This must 
 * be called prior to enabling the USB clock.
 */
void usbhs_enable(void);

/*
 * Disables the USB interface. This is mandatory before disabling
 * USB clock source to avoid freezing the USB in an undefined state
 */
void usbhs_disable(void);

/*
 * Sets the main USB operation; either host or device
 */
void usbhs_set_operation(enum usb_operation operation);

/*
 * In host operation this returns the speed status
 */
enum usb_speed usbhs_get_speed_status(void);

/*
 * Initialized the USB host interface
 */
u8 usbhs_init(struct usb_host* host_desc, struct usb_pipe* pipes,
    u32 pipe_count);

#endif