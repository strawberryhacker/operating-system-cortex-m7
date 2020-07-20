#include "usb_phy.h"
#include "usbhs.h"
#include "nvic.h"
#include "clock.h"
#include "panic.h"
#include "print.h"

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

    usbhs_unfreeze_clock();
    usbhs_set_mode(USB_HOST);
    usbhs_enable();

    /* Enable the USB UPLL clock at 480 MHz */
    upll_init(UPLL_x40);

    /* Check if the USB clock is usable */
    if (usbhs_clock_usable() == 0) {
        panic("USB clock not usable");
    }
    
    usbhs_init();

    printl("USB ready");

    nvic_enable(34);
    nvic_set_prioriy(34, NVIC_PRI_3);
}
