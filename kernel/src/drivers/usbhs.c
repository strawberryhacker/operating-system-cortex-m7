#include "usbhs.h"
#include "hardware.h"
#include "print.h"

/*
 * Enables the USB interface and un-freezes the clock. This must 
 * be called prior to enabling the USB clock.
 */
void usbhs_enable(void)
{
    u32 reg = USB->CTRL;
    reg |= (1 << 15);
    reg |= (1 << 24);   /* Must be set */
    reg &= ~(1 << 14);
    USB->CTRL = reg;
}

/*
 * Disables the USB interface. This is mandatory before disabling
 * USB clock source to avoid freezing the USB in an undefined state
 */
void usbhs_disable(void)
{
    /*
     * The USBHS disable can be called even though the clock is
     * freezed. The USBHS transceiver is disabled, clock inputs
     * are disabled, peripheral is reset, and all registers become
     * read only.
     * 
     * This does NOT disable or reset the DPRAM
     */
    u32 reg = USB->CTRL;
    reg &= ~(1 << 15);
    reg |= (1 << 24);     /* Must be set */
    USB->CTRL = reg;
}

/*
 * Sets the main USB operation; either host or device
 */
void usbhs_set_operation(enum usb_operation operation) 
{
    u32 reg = USB->CTRL;
    if (operation == USB_HOST) {
        reg &= ~(1 << 25);
    } else {
        reg |= (1 << 25);
    }
    USB->CTRL = reg;
}

/*
 * In host operation this returns the speed status
 */
enum usb_speed usbhs_get_speed_status(void)
{
    return (enum usb_speed)((USB->SR >> 12) & 0b11);
}

/*
 * Gets the device detection status. Returns 1 if a device is 
 * connected and 0 if not. 
 */
u8 usbhs_get_connection_status(void)
{

}

/*
 * Initialized the USB host interface
 */
u8 usbhs_init(void)
{
    /* Set up descriptor */

    /* Reset pipes */

    /* Freeze the USB clock */
    
}
