#include "usbhs.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"

/*
 * Resets, configures and allocates all the pipes
 */
void usbshs_init_pipes(struct usb_pipe* pipe, u32 pipe_count)
{
    for (u32 i = 0; i < pipe_count; i++) {
        /* Reset the pipe */
        USBHS->HSTPIP |= (1 << (16 + i));
        USBHS->HSTPIP &= ~(1 << (16 + i));

        /* Enable the pipe */
        USBHS->HSTPIP |= (1 << i);

        /* Configure the pipe */
        u32 reg = 0;
        reg |= (pipe->irq_freq << 24);
        reg |= (pipe->endpoint << 16);
        reg |= (pipe->type << 12);
        reg |= (pipe->token << 8);
        reg |= (pipe->autosw << 10);
        reg |= (pipe->size << 4);
        reg |= (pipe->banks << 2);
        USBHS->HSTPIPCGF = reg;

        if (!(USBHS->HSTPIPISR & (1 << 18))) {
            panic("Pipe error");
        }
    }
}

/*
 * Enables the USB interface and un-freezes the clock. This must 
 * be called prior to enabling the USB clock.
 */
void usbhs_enable(void)
{
    u32 reg = USBHS->CTRL;
    reg |= (1 << 15);
    reg |= (1 << 24);   /* Must be set */
    reg &= ~(1 << 14);
    USBHS->CTRL = reg;
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
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 15);
    reg |= (1 << 24);     /* Must be set */
    USBHS->CTRL = reg;
}

/*
 * Sets the main USB operation; either host or device
 */
void usbhs_set_operation(enum usb_operation operation) 
{
    u32 reg = USBHS->CTRL;
    if (operation == USB_HOST) {
        reg &= ~(1 << 25);
    } else {
        reg |= (1 << 25);
    }
    USBHS->CTRL = reg;
}

/*
 * In host operation this returns the speed status
 */
enum usb_speed usbhs_get_speed_status(void)
{
    return (enum usb_speed)((USBHS->SR >> 12) & 0b11);
}

/*
 * Initialized the USB host interface
 */
u8 usbhs_init(struct usb_host* host_desc, struct usb_pipe* pipes, u32 pipe_count)
{
    /* Check if the clock is usable */
    while (!(USBHS->SR & (1 << 14)));
    print("Clock usable\n");

    /* Un-freeze the USB clock */
    USBHS->CTRL &= ~(1 << 14);

    /* Set high-speed operation */
    u32 reg = USBHS->HSTCTRL;
    reg &= ~(0b11 << 12);
    reg |= (2 << 12);
    USBHS->HSTCTRL = reg;

    /* Disable VBUS hardware control */
    USBHS->CTRL |= (1 << 8);

    /* Enable device detection */
    USBHS->SFR = (1 << 9);

    /* Enable interrupts */
    USBHS->HSTIER = (1 << 0) | (1 << 2) | (1 << 5) | (1 << 6) | (1 << 1);

    usbshs_init_pipes(pipes, pipe_count);
}

/*
 * USB host exception
 */
void usb_exception(void)
{
    u32 status = USBHS->HSTISR;
    USBHS->HSTICR = 0b1111111;

    print("Speed: %d\n", usbhs_get_speed_status());

    
    print("Status: %32b\n", status);
}
