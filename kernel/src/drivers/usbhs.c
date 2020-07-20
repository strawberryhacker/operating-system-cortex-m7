#include "usbhs.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "cpu.h"
#include "usb_protocol.h"

/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger 
 * and interrupt. The CPU can only read/write FRZCLK and USBE when
 * this but is set
 */
void usbhs_freeze_clock(void)
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 14);
    USBHS->CTRL = reg;
}

/*
 * Unfreezes the USB clock
 */
void usbhs_unfreeze_clock(void)
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 14);
    USBHS->CTRL = reg; 
}

/*
 * Enable the USB interface
 */
void usbhs_enable(void)
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 15);
    USBHS->CTRL = reg;
}

/*
 * Disables the USB interface. This act as a hardware reset, thus 
 * resetting USB interface, disables the USB tranceiver and disables
 * the USB clock inputs. This does not reset FRZCLK and UIMOD
 */
void usbhs_disable(void)
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 15);
    USBHS->CTRL = reg;
}

/*
 * Sets the USB operating mode; host or device
 */
void usbhs_set_mode(enum usb_mode mode)
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */

    if (mode == USB_HOST) {
        reg &= ~(1 << 25);
    } else {
        reg |= (1 << 25);
    }
    USBHS->CTRL = reg;
}

/*
 * Enables the remote connection error interrupt
 */
static inline void usbhs_enable_connection_error(void) 
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 4);
    USBHS->CTRL = reg;
}

/*
 * Disables the remote connection error interrupt
 */
static inline void usbhs_disable_connection_error(void) 
{
    u32 reg = USBHS->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 4);
    USBHS->CTRL = reg;
}

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if
 * the clock is usable, 0 if not
 */
u8 usbhs_clock_usable(void)
{
    if (USBHS->SR & (1 << 14)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Checks if a connection error has occured on the USB bus.
 * Returns 1 if an error has occured, 0 if not
 */
static inline u8 usbhs_check_conenction_error(void)
{
    if (USBHS->SR & (1 << 4)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Returns the speed status. This should be checked at the end 
 * of a reset request.
 */
static inline enum usb_device_speed usbhs_get_speed_status(void)
{
    u32 reg = (USBHS->SR >> 12) & 0b11;
    return (enum usb_device_speed)reg;
}


/*
 * Clears the connection error flag
 */
static inline void usbhs_clear_connection_error_flag(void)
{
    USBHS->SCR = (1 << 4);
}

/*
 * Sets the connection error flag
 */
static inline void usbhs_set_connection_error_flag(void)
{
    USBHS->SFR = (1 << 4);
}

/*
 * Enables the VBUS request. The host includes two weak pulldowns
 * on the D+ and D- lines. When a devices is connected one of
 * the pulldowns os overpowered by one of the devices pullups. 
 * Which line is pulled high determines the speed of the deivce. 
 * To enable this detection VBS request must be enabled.
 */
static inline void usbhs_vbus_request_enable(void)
{
    USBHS->SFR = (1 << 9);
}

/*
 * Sends a USB resume on the bus
 */
static inline void usbhs_send_resume(void)
{
    USBHS->HSTCTRL |= (1 << 10);
}

/*
 * Sends a USB reset. It might be useful to write this bit to 
 * zero when a device disconnection is detected.
 */
static inline void usbhs_send_reset(void)
{
    USBHS->HSTCTRL |= (1 << 9);
}

/*
 * Clears the reset bit in the configuration register. This is
 * said to have no effect, but right above it, it is reccomended
 */
static inline void usbhs_clear_reset(void)
{
    USBHS->HSTCTRL &= ~(1 << 9);
}

/*
 * Starts generating SOFs and uSOFs. This will be automatically 
 * generated after a bus reset if the host was in suspend state
 * (SOF enable zero). This also sets the WAKEUP flag.
 */
static inline void usbhs_sof_enable(void)
{
    USBHS->HSTCTRL |= (1 << 8);
}

/*
 * Disables SOF and uSOF generation
 */
static inline void usbhs_sof_disable(void)
{
    USBHS->HSTCTRL &= ~(1 << 8);
}

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
 * Interrupt section
 * 
 * The USB interface has a quite complicated interrupt structure.
 * A overview can be found at page 754 in the datasheet. The 
 * host controller has ONE global status register used when
 * detecting interrupts. This register includes some basic flags
 * used for the root hub. In addition it includes one flag per pipe
 * used to indicate that a pipe interrupt has happended. A pipe 
 * interrupt has an independent status register used to report
 * events. The same thing with DMA. This way, if the pipe flag
 * is cleared not pipe interrupt is generated.  
 */

/*
 * Returns the global USB host status register
 */
static inline u32 usbhs_get_global_status(void)
{
    return USBHS->HSTISR;
}

/*
 * Clear the mask in the global status register
 */
static inline void usbhs_clear_global_status(u32 mask)
{
    USBHS->HSTICR = mask;
}

/*
 * Sets the mask in the global status register (debugging purpose)
 */
static inline void usbhs_force_global_status(u32 mask)
{
    USBHS->HSTIFR = mask;
}

/*
 * Returns the global interrupt mask. This indicates which events
 * will trigger an interrupt
 */
static inline u32 usbhs_get_global_interrupt_mask(void)
{
    return USBHS->HSTIMR;
}

/*
 * Disables the interrupts corresponding to the input mask
 */
static inline void usbhs_disable_global_interrupt(u32 mask)
{
    USBHS->HSTIDR = mask;
}

/*
 * Enables the interrupts corresponding to the input mask
 */
static inline void usbhs_enable_global_interrupt(u32 mask)
{
    USBHS->HSTIER = mask;
}

/*
 * Returns the current frame number
 */
static inline u32 usbhs_get_frame_number(void)
{
    return (u32)((USBHS->HSTFNUM >> 3) & 0x7FF);
}

/*
 * Clears the frame number
 */
static inline void usbhs_clear_frame_number(void)
{
    /* Perform a write operation to the FNUM field */
    USBHS->HSTFNUM &= ~(0x7FF << 3);
}

/*
 * Sets the pipe endpoint address. It takes in the endpoint
 * number between 0 and 9, and a 7 bit address.
 */
static void usbhs_set_pipe_addr(u8 pipe, u8 addr)
{
    /* I don't know if byte or halfword access are allowed */
    u32* reg_ptr = (u32 *)&USBHS->HSTADDR1 + (pipe >> 2);

    /* Compute the offset in the current register */
    u8 offset = ((pipe & 0x3) << 0x3);

    /* Update the address */
    u32 reg = *reg_ptr;
    reg &= ~(0x7F << offset);
    reg |= ((addr & 0x7F) << offset);
    *reg_ptr = reg;
}

/*
 * Assert reset on the specified pipe
 */
static inline void usbhs_pipe_reset_assert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIP |= (1 << (pipe + 16));
}

/*
 * Deassert reset on the specified pipe
 */
static inline void usbhs_pipe_reset_deassert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIP &= ~(1 << (pipe + 16));
}

/*
 * Enables the specifed pipe
 */
static inline void usbhs_pipe_enable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIP |= (1 << pipe);
}

/*
 * Disables the specifed pipe
 */
static inline void usbhs_pipe_disable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIP &= ~(1 << pipe);
}

/*
 * Returns the specified pipes status register
 */
static inline u32 usbhs_pipe_get_status(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHS->HSTPIPISR[pipe];
}

/*
 * Clears the input mask in the specified pipes status register
 */
static inline void usbhs_pipe_clear_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPICR[pipe] = mask;
}

/*
 * Sets the input mask in the specified pipes status register
 */
static inline void usbhs_pipe_force_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPIFR[pipe] = mask;
}

/*
 * Return the specified pipe interrupt mask
 */
static inline u32 usbhs_pipe_get_interrupt_mask(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHS->HSTPIPIMR[pipe];
}

/*
 * Enables the interrupt corresponding to the specified
 * pipes interrupt mask
 */
static inline void usbhs_pipe_enable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPIER[pipe] = mask;
}

/*
 * Disables the interrupt corresponding to the specified
 * pipes interrupt mask
 */
static inline void usbhs_pipe_disable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPIDR[pipe] = mask;
}

/*
 * Performs a predefined number of IN request before the pipe 
 * is frozen. This makes the device send IN packets. 
 */
static inline void usbhs_pipe_in_request_defined(u8 pipe, u8 count)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPINRQ[pipe] = count;
}

/*
 * Performs IN requests on the given pipe untill the pipe is frozen
 */
static inline void usbhs_pipe_in_request_continous(u8 pipe, u8 count)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPINRQ[pipe] = (1 << 8);
}

/*
 * Writes the given configuration mask to the given pipe
 */
static inline void usbhs_pipe_set_configuration(u8 pipe, u32 cfg)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHS->HSTPIPCFG[pipe] = cfg;
}

/*
 * Writes the given configuration mask to the given pipe
 */
static inline u32 usbhs_pipe_get_configuration(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHS->HSTPIPCFG[pipe];
}

/*
 * Checks the given pipe configuration status. This indicates
 * if the configurations fields are set according to the
 * pipe capabilities.
 */
static inline u8 usbhs_pipe_check_configuration(u8 pipe) 
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }

    if (USBHS->HSTPIPISR[pipe] & (1 << 18)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Sets the pipe token
 */
static inline void usbhs_pipe_set_token(u8 pipe, enum pipe_token token)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHS->HSTPIPCFG[pipe];
    reg &= ~(0b11 << 8);
    reg |= (token << 8);
    USBHS->HSTPIPCFG[pipe] = reg;
}

/*
 * Sets the pipe interrupt frequency. Only useful for interrupt pipes
 */
static inline void usbhs_pipe_set_freq(u8 pipe, u8 irq_freq)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHS->HSTPIPCFG[pipe];
    reg &= ~(0xFF << 24);
    reg |= (irq_freq << 24);
    USBHS->HSTPIPCFG[pipe] = reg;
}

/*
 * Initializes the USB interface
 */
void usbhs_init(void)
{
    /* The USB should be enabled because of the clock config */
    usbhs_unfreeze_clock();
    usbhs_set_mode(USB_HOST);
    usbhs_enable();

    /* We disable all interrupts that might have been set */
    for (u32 i = 0; i < 10; i++) {
        usbhs_pipe_disable_interrupt(i, 0xFFFFFFFF);
    }
    usbhs_disable_global_interrupt(0xFFFFFFFF);

    /* Enable VBUS request to start monitoring connect status */
    usbhs_vbus_request_enable();

    /* Enable wakeup and connection interrupt */
    usbhs_enable_global_interrupt((1 << 6) | (1 << 0));
}

/*
 * This functions gets called when a reset has been sent on the bus
 */
static void reset_callback(void)
{
    /* Configure the control pipe */
    u32 cfg = (3 << 4) | (1 << 1);

    usbhs_pipe_enable(0);
    usbhs_pipe_set_configuration(0, cfg);
    if (usbhs_pipe_check_configuration(0) == 0) {
        panic("Pipe configuration not ok");
    }

    printl("Pipe configuration ok");

    usbhs_pipe_set_token(0, PIPE_TOKEN_SETUP);
    usbhs_pipe_clear_status(0, 1 << 2);

    u8 setup[8];
    setup[0] = 0x80;
    setup[1] = 0x06;
    setup[2] = 0x00;
    setup[3] = 0x01;

    setup[4] = 0x00;
    setup[5] = 0x00;
    setup[6] = 0x12;
    setup[7] = 0x00;
    volatile u8* dest = usbhs_get_fifo_ptr(0, 8);
    volatile u8* src = setup;

    for (u8 i = 0; i < 8; i++) {
        *dest++ = *src++;
    }

    print("Error: %32b\n", USBHS->HSTPIPERR[0]);
    print("Size: %d\n", (usbhs_pipe_get_status(0) >> 20) & 0b11111111111);
    print("Status: %32b\n", usbhs_pipe_get_status(0));

    usbhs_enable_global_interrupt(0xFF << 8);
    usbhs_pipe_enable_interrupt(0, 0xFF);
    usbhs_pipe_disable_interrupt(0, (1 << 14) | (1 << 17));
}

/*
 * Root hub handler. This is called when a root hub change
 * occurs and it should clear the corresponding interrupt flag
 */
static void usbhs_root_hub_handler(u32 global_status)
{  
    usbhs_clear_global_status(0x5F);

    if (global_status & (1 << 0)) {
        printl("Device connected");
        usbhs_clear_global_status(1 << 2);
        usbhs_enable_global_interrupt(1 << 2);
        usbhs_send_reset();
    }

    if (global_status & (1 << 1)) {
        printl("Device disconnected");
    }

    if (global_status & (1 << 6)) {
        printl("Wakeup");
    }

    if (global_status & (1 << 2)) {
        reset_callback();
        /* Disable reset send interrupt */
        usbhs_disable_global_interrupt(1 << 2);
    }
    print("\n");
}

/*
 * Pipe handler. This gets called when a pipe interrupt occurs. 
 * This code should clear the corresponding interrupt flag
 */
static void usbhs_pipe_handler(u32 global_status)
{
    u8 pipe;

    /* Go through all the pipes and find the first interrupted one */
    for (pipe = 0; pipe < 10; pipe++) {
        if (global_status & (1 << (pipe + 8))) {
            break;
        }
    }
    /* Get the first interrupted pipe status */
    u32 pipe_status = usbhs_pipe_get_status(pipe);

    /* Clear the pipe status flags [0..7] and the global pipe flag */
    usbhs_pipe_clear_status(pipe, 0xFF);
    usbhs_clear_global_status((1 << (pipe + 8)));

    /* Transmitted setup packet */
    if (pipe_status & (1 << 2)) {
        printl("Setup transmitted on pipe %d\n", pipe);
        print("cfg: %32b\n", usbhs_pipe_get_configuration(pipe));

        usbhs_pipe_set_token(0, PIPE_TOKEN_IN);
        usbhs_pipe_clear_status(0, (1 << 7) | (1 << 0));
        usbhs_pipe_enable_interrupt(0, 1 << 0);
        usbhs_pipe_disable_interrupt(0, (1 << 14) | (1 << 17));
    }

    if (pipe_status & (1 << 0)) {
        u8 buffer[32];
        print("Size: %d\n", (usbhs_pipe_get_status(0) >> 20) & 0b11111111111);
        u8 size = (usbhs_pipe_get_status(0) >> 20) & 0b11111111111;
        volatile u8* src = usbhs_get_fifo_ptr(0, 8);
        volatile u8* dest = buffer;

        for (u8 i = 0; i < size; i++) {
            *dest++ = *src++;
        }

        struct usb_dev_desc* desc = (struct usb_dev_desc*)buffer;

        print("Length: %d\n", desc->length);
        print("Desc type: %1h\n", desc->descriptor_type);
        print("BCD USB: %2h\n", desc->bcd_usb);
        print("Dev class: %1h\n", desc->device_class);
        print("Dev subclass: %d\n", desc->device_subclass);
        print("Dev protocol: %d\n", desc->device_protocol);
        print("Max packet: %d\n", desc->max_packet_size);
        print("Vendor: %2h\n", desc->vendor_id);
        print("Product: %2h\n", desc->product_id);
        print("BCD device: %2d\n", desc->bcd_device);
        print("Num config: %d\n", desc->num_configurations);

        print("\nPacket received\n");
    }
}

/*
 * SOF handler. This gets called when a SOF and a  uSOF has
 * been sent on the line. This code should clear the SOF flag.
 */
static void usbhs_sof_handler(u32 global_status)
{
    usbhs_clear_global_status(1 << 5);
}

/*
 * USB core exception
 */
void usb_exception(void)
{
    u32 global_status = usbhs_get_global_status();
    
    if (global_status & 0x3FF00) {
        usbhs_pipe_handler(global_status);
    }
     
    if (global_status & 0x20) {
        usbhs_sof_handler(global_status);
    }

    if (global_status & 0x5FF) {
        usbhs_root_hub_handler(global_status);
    }
}
