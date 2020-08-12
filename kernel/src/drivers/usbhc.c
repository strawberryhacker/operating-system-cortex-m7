/* Copyright (C) StrawberryHacker */

#include "usbhc.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "cpu.h"
#include "usb_protocol.h"
#include "bmalloc.h"
#include "pmalloc.h"

#include <stddef.h>

/* Pointer to the USB core structure used in the callbacks */
struct usb_core* usbhc_core = NULL;

struct bmalloc_desc urb_allocator;

/* General USB host core stuff */
static inline void usbhc_enable_connection_error(void);
static inline void usbhc_disable_connection_error(void);
static inline u8   usbhc_check_conenction_error(void);
static inline enum usb_device_speed usbhc_get_speed_status(void);
static inline void usbhc_clear_connection_error_flag(void);
static inline void usbhc_set_connection_error_flag(void);
static inline void usbhc_vbus_request_enable(void);
static inline void usbhc_send_resume(void);
static inline void usbhc_clear_reset(void);
static inline void usbhc_sof_enable(void);
static inline void usbhc_sof_disable(void);
static inline void usbhc_set_host_speed(enum usb_host_speed speed);

/* Core interrupts */
static inline u32  usbhc_get_global_status(void);
static inline u32  usbhc_get_global_interrupt_mask(void);
static inline void usbhc_clear_global_status(u32 mask);
static inline void usbhc_force_global_status(u32 mask);
static inline void usbhc_disable_global_interrupt(u32 mask);
static inline void usbhc_enable_global_interrupt(u32 mask);

/* Pipe interrupts */
static inline u32  usbhc_pipe_get_status(u8 pipe);
static inline u32  usbhc_pipe_get_interrupt_mask(u8 pipe);
static inline void usbhc_pipe_clear_status(u8 pipe, u32 mask);
static inline void usbhc_pipe_force_status(u8 pipe, u32 mask);
static inline void usbhc_pipe_enable_interrupt(u8 pipe, u32 mask);
static inline void usbhc_pipe_disable_interrupt(u8 pipe, u32 mask);

/* Frame */
static inline u32 usbhc_get_frame_number(void);
static inline u8  usbhc_get_microframe_number(void);
static inline void usbhc_clear_frame_number(void);

/* Pipes */
static void usbhc_set_pipe_addr(u8 pipe, u8 addr);
static inline void usbhc_pipe_reset_assert(u8 pipe);
static inline void usbhc_pipe_reset_deassert(u8 pipe);
static inline void usbhc_pipe_enable(u8 pipe);
static inline void usbhc_pipe_disable(u8 pipe);
static inline void usbhc_pipe_in_request_defined(u8 pipe, u8 count);
static inline void usbhc_pipe_in_request_continous(u8 pipe, u8 count);
static inline void usbhc_pipe_set_configuration(u8 pipe, u32 cfg);
static inline void usbhc_pipe_allocate_set(u8 pipe);
static inline void usbhc_pipe_allocate_clear(u8 pipe);
static inline u32  usbhc_pipe_get_configuration(u8 pipe);
static inline u8   usbhc_pipe_check_configuration(u8 pipe);
static inline void usbhc_pipe_set_token(u8 pipe, enum pipe_token token);
static inline void usbhc_pipe_set_freq(u8 pipe, u8 irq_freq);
static inline u32  usbhc_get_fifo_byte_count(u8 pipe);

/*
 * Enables the remote connection error interrupt
 */
static inline void usbhc_enable_connection_error(void) 
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 4);
    USBHC->CTRL = reg;
}

/*
 * Disables the remote connection error interrupt
 */
static inline void usbhc_disable_connection_error(void) 
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 4);
    USBHC->CTRL = reg;
}

/*
 * Checks if a connection error has occured on the USB bus. Returns 1 if an
 * error has occured, 0 if not
 */
static inline u8 usbhc_check_conenction_error(void)
{
    if (USBHC->SR & (1 << 4)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Returns the speed status. This should be checked at the end of a reset
 * request. In host mode two pulldowns are connected. The speed status is based
 * on which line the device pulls up. 
 */
static inline enum usb_device_speed usbhc_get_speed_status(void)
{
    u32 reg = (USBHC->SR >> 12) & 0b11;
    return (enum usb_device_speed)reg;
}


/*
 * Clears the connection error flag
 */
static inline void usbhc_clear_connection_error_flag(void)
{
    USBHC->SCR = (1 << 4);
}

/*
 * Sets the connection error flag
 */
static inline void usbhc_set_connection_error_flag(void)
{
    USBHC->SFR = (1 << 4);
}

/*
 * Enables the VBUS request. The host includes two weak pulldowns on the D+
 * and D- lines. When a devices is connected one of the host pulldown resistors
 * is overpowered by one of the devices pullups. Which line is pulled high
 * determines the speed of the deivce. To enable this detection VBS request
 * must be enabled.
 */
static inline void usbhc_vbus_request_enable(void)
{
    USBHC->SFR = (1 << 9);
}

/*
 * Sends a USB resume on the bus. A downstream USB resume is issued by the host
 * in order to wake up a suspended device. The USB resume is a K-state for at
 * least 20ms, followed by a low speed EOP. 
 */
static inline void usbhc_send_resume(void)
{
    /* According to the datasheet SOF must be enabled */
    if (!(USBHC->HSTCTRL & (1 << 8))) {
        panic("Attempted resume without SOF enabled");
    }

    USBHC->HSTCTRL |= (1 << 10);
}

/*
 * Clears the reset bit in the configuration register. This is said to have no
 * effect, but right above it, they reccomended clearing it. This should be 
 * done after a device disconnection to avoid any unintentional reset. 
 */
static inline void usbhc_clear_reset(void)
{
    USBHC->HSTCTRL &= ~(1 << 9);
}

/*
 * Starts generating SOFs and uSOFs. This will be automatically generated after
 * a bus reset if the host was in suspend state (SOFEN zero). This also
 * sets the WAKEUP flag.
 */
static inline void usbhc_sof_enable(void)
{
    USBHC->HSTCTRL |= (1 << 8);
}

/*
 * Disables SOF and uSOF generation
 */
static inline void usbhc_sof_disable(void)
{
    USBHC->HSTCTRL &= ~(1 << 8);
}

/*
 * Sets the host speed capability
 */
static inline void usbhc_set_host_speed(enum usb_host_speed speed)
{
    u32 reg = USBHC->HSTCTRL;
    reg &= ~(0b11 << 12);
    reg |= (speed << 12);
    USBHC->HSTCTRL = reg;
}

/*
 * Interrupt section
 * 
 * The USB interface has a quite complicated interrupt structure. A overview
 * can be found at page 754 in the datasheet. The host controller has ONE global
 * status register used when detecting interrupts. This register includes some
 * basic flags used for the root hub. In addition it includes one flag per pipe
 * used to indicate that a pipe interrupt has happended. A pipe interrupt has an
 * independent status register used to report events. The same goes for DMA.
 * If the pipe flag is cleared no pipe interrupt is generated.  
 */

/*
 * Returns the global USB host status register
 */
static inline u32 usbhc_get_global_status(void)
{
    return USBHC->HSTISR;
}

/*
 * Clear the mask in the global status register
 */
static inline void usbhc_clear_global_status(u32 mask)
{
    USBHC->HSTICR = mask;
}

/*
 * Sets the mask in the global status register (debugging purpose)
 */
static inline void usbhc_force_global_status(u32 mask)
{
    USBHC->HSTIFR = mask;
}

/*
 * Returns the global interrupt mask. This indicates which events will trigger
 * an interrupt
 */
static inline u32 usbhc_get_global_interrupt_mask(void)
{
    return USBHC->HSTIMR;
}

/*
 * Disables the interrupts corresponding to the input mask
 */
static inline void usbhc_disable_global_interrupt(u32 mask)
{
    USBHC->HSTIDR = mask;
}

/*
 * Enables the interrupts corresponding to the input mask
 */
static inline void usbhc_enable_global_interrupt(u32 mask)
{
    USBHC->HSTIER = mask;
}

/*
 * Returns the current frame number
 */
static inline u32 usbhc_get_frame_number(void)
{
    return (u32)((USBHC->HSTFNUM >> 3) & 0x7FF);
}

/*
 * Returns the micro-frame number
 */
static inline u8  usbhc_get_microframe_number(void)
{
    return (u8)(USBHC->HSTFNUM & 0x7);
}

/*
 * Clears the frame number
 */
static inline void usbhc_clear_frame_number(void)
{
    /* Perform a write operation to the FNUM field */
    USBHC->HSTFNUM &= ~(0x7FF << 3);
}

/*
 * Sets the pipe endpoint address. It takes in the endpoint number between
 * 0 and 9, and a 7 bit address identifying the USB device on the bus. Note that
 * pipe 0 should have its address set to one
 */
static void usbhc_set_pipe_addr(u8 pipe, u8 addr)
{
    /* I don't know if byte or halfword access are allowed */
    u32* reg_ptr = (u32 *)&USBHC->HSTADDR1 + (pipe >> 2);

    /* Compute the offset in the current register */
    u8 offset = ((pipe & 0x3) << 0x3);

    /* Update the address */
    u32 reg = *reg_ptr;
    reg &= ~(0x7F << offset);
    reg |= ((addr & 0x7F) << offset);
    *reg_ptr = reg;
}

/*
 * Assert reset on a pipe
 */
static inline void usbhc_pipe_reset_assert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP |= (1 << (pipe + 16));
}

/*
 * Deassert reset on a pipe
 */
static inline void usbhc_pipe_reset_deassert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP &= ~(1 << (pipe + 16));
}

/*
 * Enables a pipe
 */
static inline void usbhc_pipe_enable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP |= (1 << pipe);
}

/*
 * Disables a pipe
 */
static inline void usbhc_pipe_disable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP &= ~(1 << pipe);
}

/*
 * Returns the pipe status register
 */
static inline u32 usbhc_pipe_get_status(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPISR[pipe];
}

/*
 * Clears the input mask in the pipe status register
 */
static inline void usbhc_pipe_clear_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPICR[pipe] = mask;
}

/*
 * Sets the input mask in the pipe status register
 */
static inline void usbhc_pipe_force_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIFR[pipe] = mask;
}

/*
 * Returns the pipe interrupt mask
 */
static inline u32 usbhc_pipe_get_interrupt_mask(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPIMR[pipe];
}

/*
 * Enables the interrupt corresponding to the pipe interrupt mask
 */
static inline void usbhc_pipe_enable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIER[pipe] = mask;
}

/*
 * Disables the interrupt corresponding to the pipe interrupt mask
 */
static inline void usbhc_pipe_disable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIDR[pipe] = mask;
}

/*
 * Performs a predefined number of IN request before the pipe is frozen. This
 * makes the device send IN packets. 
 */
static inline void usbhc_pipe_in_request_defined(u8 pipe, u8 count)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPINRQ[pipe] = count;
}

/*
 * Performs IN requests continously on the given pipe until the pipe is frozen
 */
static inline void usbhc_pipe_in_request_continous(u8 pipe, u8 count)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPINRQ[pipe] = (1 << 8);
}

/*
 * Sets a pipe configuration
 */
static inline void usbhc_pipe_set_configuration(u8 pipe, u32 cfg)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] = cfg;
}

/*
 * Set the pipe allocate flag
 */
static inline void usbhc_pipe_allocate_set(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] |= (1 << 1);
}

/*
 * Clear the pipe allocate flag
 */
static inline void usbhc_pipe_allocate_clear(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] &= ~(1 << 1);
}

/*
 * Writes the given configuration mask to the given pipe
 */
static inline u32 usbhc_pipe_get_configuration(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPCFG[pipe];
}

/*
 * Checks the given pipe configuration status. This indicates if the
 * configurations fields are set according to the pipe capabilities.
 */
static inline u8 usbhc_pipe_check_configuration(u8 pipe) 
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }

    if (USBHC->HSTPIPISR[pipe] & (1 << 18)) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Sets the pipe token
 */
static inline void usbhc_pipe_set_token(u8 pipe, enum pipe_token token)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHC->HSTPIPCFG[pipe];
    reg &= ~(0b11 << 8);
    reg |= (token << 8);
    USBHC->HSTPIPCFG[pipe] = reg;
}

/*
 * Sets the pipe interrupt frequency. Only useful for interrupt pipes
 */
static inline void usbhc_pipe_set_freq(u8 pipe, u8 irq_freq)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHC->HSTPIPCFG[pipe];
    reg &= ~(0xFF << 24);
    reg |= (irq_freq << 24);
    USBHC->HSTPIPCFG[pipe] = reg;
}

/*
 * Returns the number of bytes in a pipe FIFO
 */
static inline u32 usbhc_get_fifo_byte_count(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }

    /* Delay so that the hardware can update the size */
    for (volatile u8 i = 0; i < 2; i++) {
        asm volatile ("nop");
    }

    return (USBHC->HSTPIPISR[pipe] >> 20) & 0x7FF;
}

/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger an interrupt.
 * The CPU can only read/write FRZCLK and USBE when this bit is set
 */
void usbhc_freeze_clock(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 14);
    USBHC->CTRL = reg;
}

/*
 * Unfreezes the USB clock
 */
void usbhc_unfreeze_clock(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 14);
    USBHC->CTRL = reg; 
}

/*
 * Enable the USB interface
 */
void usbhc_enable(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 15);
    USBHC->CTRL = reg;
}

/*
 * Disables the USB interface. This act as a hardware reset, thus resetting USB
 * interface, disables the USB tranceiver and disables the USB clock inputs.
 * This does not reset FRZCLK and UIMOD
 */
void usbhc_disable(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 15);
    USBHC->CTRL = reg;
}

/*
 * Sets the USB operating mode; host or device
 */
void usbhc_set_mode(enum usb_mode mode)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */

    if (mode == USB_HOST) {
        reg &= ~(1 << 25);
    } else {
        reg |= (1 << 25);
    }
    USBHC->CTRL = reg;
}

/*
 * Sends a USB reset. It might be useful to write this bit to zero when a device
 * disconnection is detected. This sets any connected device to its default
 * unconfigured state. It sends the reset by pulling both data lines low for at
 * least 10 ms (SE0)
 */
void usbhc_send_reset(void)
{
    usbhc_set_host_speed(USB_HOST_SPEED_NORMAL);
    USBHC->HSTIER = (1 << 2);
    USBHC->HSTCTRL |= (1 << 9);
}

/*
 * Clears and disables all global interrupts
 */
void usbhc_interrupt_disable(void)
{
    USBHC->HSTIDR = 0xFFFFFFFF;
    USBHC->HSTICR = 0xFFFFFFFF;
}

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if the clock is
 * usable, 0 if not
 */
u8 usbhc_clock_usable(void)
{
    if (USBHC->SR & (1 << 14)) {
        return 1;
    } else {
        return 0;
    }
}

static void usbhc_pipe_reset(struct usb_pipe* pipe) 
{
    /* Initialize the URB queue linked to the pipe */
    dlist_init(&pipe->urb_queue);

    /* Disable the pipe */
    usbhc_pipe_disable(pipe->number);

    /* Reset the pipe */
    usbhc_pipe_reset_assert(pipe->number);
    usbhc_pipe_reset_deassert(pipe->number);

    /* De-allocate all pipes by writing 1 to ALLOC */
    usbhc_pipe_set_configuration(pipe->number, (1 << 1));
}

/*
 * Initializes the USB hardware
 */
void usbhc_init(struct usbhc* hc, struct usb_pipe* pipe, u32 pipe_count)
{
    /* Disable all interrupts */
    usbhc_interrupt_disable();

    /* This must be called in order to detect device connection or dissconnection */
    usbhc_vbus_request_enable();

    /* Make a new bitmap allocator. This is used for allocating URBs */
    bmalloc_new(&urb_allocator, sizeof(struct urb), MAX_URBS, PMALLOC_BANK_2);

    /* Link the pipes to the USB host controller */
    hc->pipe_base = pipe;
    hc->pipe_count = pipe_count;

    /*
     * Initialize the pipes. The pipes are disabled and de-allocated after reset
     * but their configuration still remains 
     */
    for (u32 i = 0; i < pipe_count; i++) {
        /* Set the hardware endpoint targeted by this pipe */
        pipe[i].number = i;

        usbhc_pipe_reset(&pipe[i]);
    }

    /* Listen for wakeup or connection */
    usbhc_clear_global_status((1 << 0) | (1 << 6));
    usbhc_enable_global_interrupt((1 << 0) | (1 << 6));
}

/*
 * Allocates a USB request block
 */
struct urb* usbhc_urb_new(void)
{
    struct urb* urb = (struct urb *)bmalloc(&urb_allocator);

    if (urb == NULL) {
        panic("URB alloc failed");
        return NULL;
    }
    dlist_node_init(&urb->node);

    /* Link the node to this object */
    urb->node.obj = urb;

    return urb;
}

/*
 * Enqueues a URB into a pipe
 */
void usbhc_urb_submit(struct urb* urb, struct usb_pipe* pipe)
{
    dlist_insert_first(&urb->node, &pipe->urb_queue);
}

u8 usbhc_urb_cancel(struct urb* urb, struct usb_pipe* pipe)
{
    /* Check the state */
    /* .... */

    /* Remove the URB from the URB queue */
    dlist_remove(&urb->node, &pipe->urb_queue);
}

void print_urb_list(struct usb_pipe* pipe)
{
    struct dlist_node* node = pipe->urb_queue.first;
    struct dlist_node* tmp = node;
    while (node) {
        struct urb* urb = (struct urb *)node->obj;
        print("URB: %6s\n", urb->name);

        node = node->next;
    }
}

/*
 * Root hub exception
 */
static void usbhc_root_hub_exception(u32 isr)
{
    if (isr & (1 << 0)) {
        print("Device connection");
        /* Callback to upper layer i.e. USB host core */
    }
}

/*
 * Pipe exceptions
 */
static void usbhc_pipe_exception(u32 isr)
{

}

/*
 * SOF exception
 */
static void usbhc_sof_exception(u32 isr)
{

}

/*
 * Main USBHC interrupt handler
 */
void usb_exception(void)
{
    u32 isr = usbhc_get_global_status();

    /* SOF interrupt */
    if (isr & 0x20) {
        usbhc_sof_exception(isr);
    }

    /* Pipe interrupts */
    if (isr & 0x3FF00) {
        usbhc_pipe_exception(isr);
    }

    /* Root-hub interrupts */
    if (isr & 0x5F) {
        usbhc_root_hub_exception(isr);
    }

    usbhc_clear_global_status(0xFFFFFFFF);
}
