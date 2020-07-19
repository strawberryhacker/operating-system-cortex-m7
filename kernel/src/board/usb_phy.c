#include "usb_phy.h"
#include "usbhs.h"
#include "nvic.h"
#include "clock.h"

/*
 * The USB pipe 0 is allways of type control and can not be used
 * with DMA. It has only one bank and a max size of 64 bytes.
 */

#define PIPE_COUNT 1

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

    usb_pipes[0].irq_freq = 1;
    usb_pipes[0].endpoint = 0;
    usb_pipes[0].type = USB_PIPE_TYPE_CTRL;
    usb_pipes[0].token = USB_PIPE_TOKEN_SETUP;
    usb_pipes[0].autosw = 0;
    usb_pipes[0].size = USB_PIPE_SIZE_64B;
    usb_pipes[0].banks = USB_PIPE_BANKS_1;
    usb_pipes[0].alloc = 1;

    peripheral_clock_enable(34);

    usbhs_enable();
    usbhs_set_operation(USB_HOST);

    /* Enable the USB UPLL clock at 480 MHz */
    upll_init(UPLL_x40);

    usbhs_init(&usb_host, usb_pipes, PIPE_COUNT);

    nvic_enable(34);
    nvic_set_prioriy(34, NVIC_PRI_3);
}
