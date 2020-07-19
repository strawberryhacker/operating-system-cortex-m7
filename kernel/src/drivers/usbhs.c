#include "usbhs.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "cpu.h"

struct usb_pipe enum_pipe = {
    .irq_freq = 0,
    .endpoint = 0,
    .type = USB_PIPE_TYPE_CTRL,
    .token = USB_PIPE_TOKEN_SETUP,
    .autosw = 0,
    .size = USB_PIPE_SIZE_64B,
    .banks = USB_PIPE_BANKS_1,
    .alloc = 1
};

/*
 * Sets the host speed capability
 */
static inline void usbhs_set_host_speed(enum usb_host_speed speed)
{
    u32 reg = USBHS->HSTCTRL;
    reg &= ~(0b11 << 12);
    reg |= (speed << 12);
    USBHS->HSTCTRL = reg;
}

/*
 * Sends a USB resume. NOTE this should only be called when SOF are
 * generated on the bus SOFE = 1
 */
static inline void usbhs_send_resume(void)
{
    USBHS->HSTCTRL |= (1 << 10);
}

/*
 * Send a USB reset on the bus
 */
static inline void usbhs_send_reset(void)
{
    USBHS->HSTCTRL |= (1 << 9);
}

/*
 * Enables SOF generation on the bus
 */
static inline void usbhs_sof_enable(void)
{
    USBHS->HSTCTRL |= (1 << 8);
}

/*
 * Disables SOF generation on the bus
 */
static inline void usbhs_sof_disable(void)
{
    USBHS->HSTCTRL &= ~(1 << 8);
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
 * Gets the pipe byte count
 */
static u32 usbhs_get_pipe_byte_count(u8 pipe)
{
    u32 count = USBHS->HSTPIPISR[pipe];

    return (count >> 20) & 0x7FF;
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
 * Resets all the pipes
 */
static void usbhs_reset_pipes(void)
{
    USBHS->HSTPIP |= 0x1FF0000;
    USBHS->HSTPIP &= ~0x1FF0000;
}

/*
 * Sets the 7 bit address field of a pipe. Takes in the pipe number
 * and the pipe endpoint address
 */
static void usbhs_set_pipe_addr(u8 pipe, u8 addr)
{
    volatile u32* reg_ptr = (volatile u32 *)&USBHS->HSTADDR1;

    /* Get the right offset */
    u8 reg_offset = pipe >> 2;
    u8 bit_offset = ((pipe & 0x3) << 3); 

    u32 reg = *(reg_ptr + reg_offset);
    reg &= ~(0x7F << bit_offset);
    reg |= ((addr & 0x7F) << bit_offset);
    *(reg_ptr + reg_offset) = reg;
}

/*
 * Resets, configures and allocates the given list of pipes
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

        USBHS->HSTPIPCFG[i] = reg;

        reg |= (pipe->alloc << 1);

        USBHS->HSTPIPCFG[i] = reg;

        if (!(USBHS->HSTPIPISR[i] & (1 << 18))) {
            panic("Pipe error");
        }
        print("Pipe #%d ok\n", i);

        USBHS->HSTIER = (1 << (i + 8));
        USBHS->HSTPIPICR[i] = 0xFF;
        USBHS->HSTPIPIER[i] = 0xFF;

        usbhs_set_pipe_addr(i, 0);
    }
}

/*
 * Asks the device for its configuration descriptor
 */
static void _get_dev_desc(void)
{
    printl("Sending first frame");

    usbshs_init_pipes(&enum_pipe, 1);

    /* Enable pipe 0 */
    print("Pipe status: %32b\n", USBHS->HSTPIP);
    print("Pipe #0 cfg: %32b\n", USBHS->HSTPIPCFG[0]);

    volatile u8 request[8];
    request[0] = 0x80;
    request[1] = 0x06;
    request[2] = 0x00;
    request[3] = 0x01;

    request[4] = 0x00;
    request[5] = 0x00;
    request[6] = 0x12;
    request[7] = 0x00;

    /* (volatile u8 *)usbhs_get_fifo_ptr(0, 8) */
    volatile u8* dpram_dest = (volatile u8*)0xA0100000;
    volatile u8* src = (volatile u8 *)request;

    /* Set setup in pipcfg - ICR TXSTPI - copy - ier txstpe - idr fifocon pfreeze*/
    USBHS->HSTPIPCFG[0] &= ~(0b11 << 8);
    USBHS->HSTPIPICR[0] = (1 << 2);
    cpsid_i();
    print("Byte cnt: %d\n", usbhs_get_pipe_byte_count(0));
    for (u8 i = 0; i < 8; i++) {
        *dpram_dest++ = *src++;
    }
    *dpram_dest++ = *src++;
    cpsie_i();
    print("Byte cnt: %d\n", usbhs_get_pipe_byte_count(0));

    USBHS->HSTPIPIER[0] = 0xFF;
    USBHS->HSTPIPIDR[0] = (1 << 14) | (1 << 17);
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
    usbhs_set_host_speed(HOST_SPEED_NORMAL);

    /* Disable VBUS hardware control */
    USBHS->CTRL |= (1 << 8);

    /* Enable device detection */
    USBHS->SFR = (1 << 9);

    /* Enable interrupts */
    USBHS->HSTIER = (1 << 0) | (1 << 2) | (1 << 5) | (1 << 6) | (1 << 1);
    USBHS->HSTICR = 0xFFFFFFFF;

    usbshs_init_pipes(pipes, pipe_count);

    return 1;
}

/*
 * USB core pipe interrupt handler. This gets called when one of the 
 * pipes triggers an interrupt and must be serviced. This function
 * should clear all the pipe interrupt flags and take the appropriate
 * action
 */
static void usbhs_pipe_interrupt(u32 status)
{
    print("Pipe IRQ\n");
    /* Get the first pipe which indicates interrupt */
    u8 pipe_number = 0;
    for (pipe_number = 0; pipe_number < 10; pipe_number++) {
        if (status & (1 << (8 + pipe_number))) {
            break;
        }
    }

    u32 pipe_status = USBHS->HSTPIPISR[pipe_number];
    USBHS->HSTICR = (1 << (8 + pipe_number));
    USBHS->HSTPIPICR[pipe_number] = 0xFFFFFFFF;

    if (pipe_status & (1 << 2)) {
        print("Byte cnt: %d\n", usbhs_get_pipe_byte_count(0));
        print("Pipe ready");
    }

    if (pipe_status & (1 << 0)) {
        print("yey\n");
    }
}

/*
 * USB core SOF interrupt handler. This gets called when either a frame
 * or a microframe has been transmitted. 
 */
static void usbhs_sof_interrupt(u32 status)
{
    USBHS->HSTICR = (1 << 5);
}

/*
 * USB core root hub interrupt handler. This gets called when a root
 * hub change occurs and needs to be serviced. This can be; wakeup,
 * upstream resume, downstream resume, reset sent, device disconnection
 * or device connection. 
 */
static void usbhs_root_hub_interrupt(u32 status)
{
    USBHS->HSTICR = 0b1011111;

    /* Connect */
    if (status & (1 << 0)) {
        printl("Connect");
        usbhs_send_reset();
        USBHS->HSTICR = (1 << 0);
    }
    /* Disconnect */
    if (status & (1 << 1)) {
        USBHS->HSTICR = (1 << 1);
        usbhs_sof_disable();
        printl("Disconnect");
    }
    /* Reset sent */
    if (status & (1 << 2)) {
        USBHS->HSTICR = (1 << 2);
        printl("Getting device descriptor");
        usbhs_reset_pipes();
        _get_dev_desc();
    }
    /* Wakeup */
    if (status & (1 << 6)) {
        USBHS->HSTICR = (1 << 6);
        printl("Wakeup");
    }
}

/*
 * USB host exception
 */
void usb_exception(void)
{
    u32 status = USBHS->HSTISR;

    /* Pipe interrupt handler */
    if (status & 0x3FF00) {
        usbhs_pipe_interrupt(status);
    }

    /* SOF interrupt handler */
    if (status & 0x20) {
        usbhs_sof_interrupt(status);
    }

    /* Root hub interrupt handler */
    if (status & 0x5F) {
        usbhs_root_hub_interrupt(status);
    }
    gpio_toggle(GPIOC, 8);
}
