#include "usb_phy.h"
#include "usbhs.h"
#include "nvic.h"
#include "clock.h"

#define PIPE_COUNT 3

/* Allocate pipes */
struct usb_pipe usb_pipes[PIPE_COUNT];

/* Main USB host instance */
struct usb_host usb_host;

/*
 * Initializes the USB PHY including the interrupt and clock controller
 */
void usb_phy_init(void)
{
    /*
     * The reccomended start up sequence can be found in the 
     * datasheet at page 752
     */

    peripheral_clock_enable(34);

    usbhs_enable();
    usbhs_set_operation(USB_HOST);

    /* Enable the USB UPLL clock at 480 MHz */
    upll_init(UPLL_x40);

    usbhs_init(&usb_host, usb_pipes, PIPE_COUNT);

    nvic_enable(34);
    nvic_set_prioriy(34, NVIC_PRI_1);
}
