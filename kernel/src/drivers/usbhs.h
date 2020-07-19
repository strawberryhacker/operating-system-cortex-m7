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
 * Gets the device detection status. Returns 1 if a device is 
 * connected and 0 if not. 
 */
u8 usbhs_get_connection_status(void);

/*
 * Initialized the USB host interface
 */
u8 usbhs_init(void);

#endif