/* Copyright (C) StrawberryHacker */

#include "usbhc.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "cpu.h"
#include "usb_protocol.h"

#include <stddef.h>

/* Pointer to the USB core structure used in the callbacks */
struct usb_core* usbhc_core = NULL;

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

/*
 * Configures and allocates a pipe. It takes in a pointer to the USB core
 * object, the pipe number, the pipe configuration register and the pipe target
 * device address. Returns 1 if the allocation was successfull
 */
static u8 usbhc_pipe_configure(struct usb_core* core, u8 pipe, u32 cfg, u8 addr)
{
    /* Pipe configuration algorithm is explanied at page 768 */
    usbhc_pipe_enable(pipe);

    /* Perform deallocation then allocation */
    usbhc_pipe_set_configuration(pipe, cfg);
    cfg |= (1 << 1);
    usbhc_pipe_set_configuration(pipe, cfg);

    /* Check if the configuration parameters is ok */
    if (!usbhc_pipe_check_configuration(pipe)) {
        return 0;
    }

    usbhc_set_pipe_addr(pipe, addr);

    return 1;
}

/*
 * Finds the first free pipe starting from the specified index. Returns the
 * pipe index, -1 if not found
 */
static i8 usbhc_find_free_pipe(struct usb_core* core, u8 start_index) 
{
    u8 max_pipes = core->hw->pipe_count;

    if (start_index >= max_pipes) {
        return -1;
    }
    for (u8 i = start_index; i < max_pipes; i++) {
        if (core->hw->pipes[i].status == PIPE_STATUS_FREE) {
            return i;
        }
    }
    return -1;
}

/*
 * Returns the index of the specified pipe. Returns -1 if not found
 */
static u8 usbhc_pipe_get_index(struct usb_core* core, struct usb_pipe* pipe)
{
    for (u8 i = 0; i < core->hw->pipe_count; i++) {
        if (pipe == (core->hw->pipes + i)) {
            return i;
        }
    }
    
    panic("Pipe index wrong");

    return 0;
}

/*
 * Tries to allocate a pipe. This will handle any DPRAM conflicts and reallocate
 * conflicting pipes. Returns the allocated pipe if success, else NULL
 */
struct usb_pipe* usbhc_pipe_allocate(struct usb_core* core, u32 cfg,
                                     u8 addr, u8 pipe0)
{
    /* Try to find the first free pipe except the control pipe*/
    u8 pipe = usbhc_find_free_pipe(core, 1);
    if (pipe < 0) {
        return NULL;
    }

    if (pipe0) {
        pipe = 0;
    }

    if (!usbhc_pipe_configure(core, pipe, cfg, addr)) {
        return NULL;
    }

    /* 
     * Pipe has been allocated so we must resolve any memory conflics
     * The pipe above the allocated pipe will be able to move, but
     * pipe + 2 will allways not move. Check datasheet page 756
     */
    for (u8 i = pipe + 2; i < core->hw->pipe_count; i++) {
        
        /* Only reallocate allocated pipes */
        if (usbhc_pipe_get_configuration(i) & (1 << 1)) {
            usbhc_pipe_allocate_clear(i);
            usbhc_pipe_allocate_set(i);
        }
    }

    /* Enable ERROR, OVERFLOW and STALL interrupts */
    usbhc_pipe_clear_status(pipe, (1 << 3) | (1 << 5) | (1 << 6));
    usbhc_pipe_enable_interrupt(pipe, (1 << 3) | (1 << 5) | (1 << 6));
    usbhc_enable_global_interrupt(1 << (pipe + 8));

    struct usb_pipe* ret_pipe = &core->hw->pipes[pipe];
    ret_pipe->status = PIPE_STATUS_IDLE;

    print("Pipe allocation on pipe %d - %32b\n", pipe, USBHC->HSTPIPCFG[pipe]);

    return ret_pipe;
}

/*
 * Performs a hard reset on all pipes. This means deallocating them
 * and updating the status
 */
void usbhc_pipe_hard_reset(struct usb_core* core)
{
    for (u8 i = 0; i < core->hw->pipe_count; i++) {

        /* Clear the alloc flag */
        usbhc_pipe_set_configuration(i, 0);

        /* Reset the internal pipe state machine */
        usbhc_pipe_reset_assert(i);
        usbhc_pipe_reset_deassert(i);

        core->hw->pipes[i].status = PIPE_STATUS_FREE;
    }
}

/*
 * Performs a soft reset on all pipes. This means configuring the
 * pipes, allocating them and updating the status
 */
void usbhc_pipe_soft_reset(struct usb_core* core)
{
    struct usb_pipe* pipe;
    for (u8 i = 0; i < core->hw->pipe_count; i++) {
        
        pipe = &core->hw->pipes[i];

        /* Used pipe */
        if (pipe->status != PIPE_STATUS_FREE) {
            
            /* Update the configuration mask and configure the pipe */
            u32 cfg_mask = 0;

            cfg_mask |= (pipe->type << 12);
            cfg_mask |= (pipe->token << 8);
            cfg_mask |= (pipe->auto_bank_switch << 10);
            cfg_mask |= (pipe->endpoint << 16);
            cfg_mask |= (pipe->interval << 24);
            cfg_mask |= (pipe->size << 4);
            cfg_mask |= (pipe->bank_count << 2);
            cfg_mask |= (pipe->ping_enable << 20);

            /* Configure the pipe whith interrupt disabled */
            usbhc_disable_global_interrupt(1 << (i + 8));
            usbhc_pipe_configure(core, i, cfg_mask, pipe->addr);
            usbhc_enable_global_interrupt(1 << (i + 8));
        }
    }
}

/*
 * Adds a pipe callback
 */
void usbhc_add_pipe_callback(struct usb_pipe* pipe,
                             void (*cb)(struct usb_pipe *))
{
    if (cb == NULL) {
        panic("Callback pointer error");
    }
    pipe->callback = cb;
}

/*
 * Stops a transfer on a pipe
 */
static void usbhc_stop_transfer(struct usb_core* core, struct usb_pipe* pipe,
                                enum usb_x_status status)
{
    pipe->x_status = status;

    u8 pipe_index = usbhc_pipe_get_index(core, pipe);

    /* Disable transfer interrupts and the STALL flag */
    usbhc_pipe_disable_interrupt(pipe_index, 0b111 | (1 << 6) | (1 << 7));

    if (pipe_index < 0) {
        panic("Wrong pipe index");
    }

    /* Reset pipe to stop all transfers */
    usbhc_pipe_reset_assert((u8)pipe_index);
    usbhc_pipe_reset_deassert((u8)pipe_index);
}

/*
 * Processes raw data from the receive buffer
 */
static void usbhc_read_raw(struct usb_core* core, struct usb_pipe* pipe)
{
    u32 pipe_number = usbhc_pipe_get_index(core, pipe);
    u32 byte_count = usbhc_get_fifo_byte_count(pipe_number);

    print("Byte count: %d\n", byte_count);

    volatile u8* dest = pipe->x.control.data;
    volatile u8* src = usbhc_get_fifo_ptr(pipe_number);

    /* Read the raw data */
    for (u8 i = 0; i < byte_count; i++) {
        *dest++ = *src++;
    }
}

/*
 * Sends a zero length packet out
 */
static void usb_zpl_out(struct usb_core* core, struct usb_pipe* pipe)
{
    u8 pipe_number = usbhc_pipe_get_index(core, pipe);

    if (usbhc_get_fifo_byte_count(pipe_number)) {
        panic("Fifo contains data");
    }

    usbhc_pipe_set_token(pipe_number, PIPE_TOKEN_OUT);
    usbhc_pipe_clear_status(pipe_number, 1 << 1);
    usbhc_pipe_enable_interrupt(pipe_number, 1 << 1);
    usbhc_pipe_disable_interrupt(pipe_number, (1 << 14) | (1 << 17));
}

/*
 * Performs a control IN request
 */
static void usbhc_control_in(struct usb_core* core, struct usb_pipe* pipe)
{
    u8 pipe_number = usbhc_pipe_get_index(core, pipe);

    usbhc_pipe_set_token(pipe_number, PIPE_TOKEN_IN);

    /* Clear SHORTPACKET and RXIN and disable FIFO control and PFREEZE */
    usbhc_pipe_clear_status(pipe_number, (1 << 7) | (1 << 0));
    usbhc_pipe_enable_interrupt(pipe_number, 1 << 0);
    usbhc_pipe_disable_interrupt(pipe_number, (1 << 14) | (1 << 17));
}

/*
 * Sends a control SETUP packet. Evert SETUP packet has an 8 
 * byte payload. 
 */
static void usbhc_control_setup_out(struct usb_core* core, struct usb_pipe* pipe)
{
    print("Setup out\n");
    u8 pipe_number = usbhc_pipe_get_index(core, pipe);

    /* Get the pointer to the data and to the FIFO pipe */
    volatile u8* src = (volatile u8 *)pipe->x.control.setup;
    volatile u8* dest = usbhc_get_fifo_ptr(pipe_number);

    usbhc_pipe_set_token(pipe_number, PIPE_TOKEN_SETUP);
    /* Clear the control transmitted flag */
    usbhc_pipe_clear_status(pipe_number, 1 << 2);

    for (u8 i = 0; i < 8; i++) {
        print("%1h,", *src);
        *dest++ = *src++;
    }
    
    print("Bytes: %d\n", usbhc_get_fifo_byte_count(pipe_number));
    usbhc_pipe_enable_interrupt(pipe_number, 1 << 2);
    usbhc_pipe_disable_interrupt(pipe_number, (1 << 14) | (1 << 17));
}

/*
 * Starts a control transfer
 */
void usbhc_control_transfer(struct usb_core* core, struct usb_pipe* pipe,
                            u8* data, u8* setup, u8 req_size)
{
    pipe->x.control.data = data;
    pipe->x.control.setup = setup;
    pipe->x.control.receive_size = req_size;

    usbhc_control_setup_out(core, pipe);
}

/*
 * Default handler for SOF
 */
void usbhc_sof_default_handler(struct usb_core* core)
{
    (void)core;
}

/*
 * Default handler for root hub change
 */
void usbhc_rh_default_handler(struct usb_core* core, enum root_hub_event event)
{
    (void)core;
    (void)event;
}

/*
 * Default pipe handler
 */
void usbhc_pipe_default_handler(struct usb_pipe* pipe)
{
    (void)pipe;
}

/*
 * Initializes the USB interface. This takes in the main USB core 
 * structure, a list of pipes and the pipe count
 */
void usbhc_init(struct usb_core* core, struct usb_hardware* hw,
                struct usb_pipe* pipes, u8 pipe_count)
{
    /* The USB should be enabled because of the clock config */
    usbhc_unfreeze_clock();
    usbhc_set_mode(USB_HOST);
    usbhc_enable();

    /* Disable all interrupts that might have been set */
    for (u32 i = 0; i < 10; i++) {
        usbhc_pipe_disable_interrupt(i, 0xFFFFFFFF);

        /* Set the pipe to free */
        pipes[i].status = PIPE_STATUS_FREE;
        pipes[i].callback = usbhc_pipe_default_handler;
    }
    usbhc_disable_global_interrupt(0xFFFFFFFF);

    /* Configure the hardware layer */
    hw->pipes = pipes;
    hw->pipe_count = pipe_count;
    hw->active_pipes = 0;
    hw->dpram_used = 0;

    /* Configure the core layer */
    core->rh_callback = usbhc_rh_default_handler;
    core->sof_callback = usbhc_sof_default_handler;
    core->hw = hw;

    usbhc_core = core;

    /* 
     * The VBUS enable request must be enabled to detect
     * connection events
     */
    usbhc_vbus_request_enable();

    /* Enable wakeup and connection interrupt */
    usbhc_enable_global_interrupt((1 << 6) | (1 << 0));
}

/*
 * Root hub handler. This is called when a root hub change occurs.
 * This routine must clear the corresponding interrupt flag(s)
 */
static void usbhc_root_hub_handler(struct usb_core* core, u32 global_status)
{  
    usbhc_clear_global_status(0x5F);

    /* Decive disconnection */
    if (global_status & (1 << 1)) {
        usbhc_clear_global_status((1 << 0) | (1 << 1));

        /* 
         * According to datasheet page 862 the reset bit should be
         * cleared to avoid an undefined reset state when no device
         * is connected to the bus
         */
        usbhc_clear_reset();

        /* Enable CONNECT and WAKEUP interrupts */
        usbhc_clear_global_status((1 << 0) | (1 << 6));
        usbhc_enable_global_interrupt((1 << 0) | (1 << 6));
        usbhc_core->rh_callback(core, RH_EVENT_DISCONNECT);

    } else if (global_status & (1 << 0)) {

        /* Enabling the reset sent and disconnect interrupt */
        usbhc_clear_global_status(1 << 2);
        usbhc_enable_global_interrupt((1 << 2) | (1 << 1));

        /* After the reset is sent the CPU will start sending SOF */
        usbhc_enable_global_interrupt(1 << 5);
        //usbhc_sof_enable();
        usbhc_core->rh_callback(core, RH_EVENT_CONNECT);
    }

    /* 
     * Wakeup. This occurs if the USBHC is in suspend mode and a peripheral 
     * disconnection or a upstream resume is detected 
     */
    if (global_status & (1 << 6)) {
        
        usbhc_disable_global_interrupt(1 << 6);
        usbhc_clear_global_status(1 << 6);

        /* Check if the clock is usable */
        while (!usbhc_clock_usable());

        usbhc_unfreeze_clock();
    }

    /* Reset sent */
    if (global_status & (1 << 2)) {
        /* Disable reset sent interrupt */
        usbhc_clear_global_status(1 << 2);
        usbhc_disable_global_interrupt(1 << 2);
        usbhc_core->rh_callback(core, RH_EVENT_RESET);
    }
}

/*
 * Setup packet sent handler. This is called from the pipe handler when the
 * TXSTPI flag is set. This functions will perform the next stage in the setup
 * packet. A general setup packet has three stages; the setup packet, the data
 * packet(s) and the handshake pakcet. Typically the first setup packet will not
 * be handled here since this packet acctually will trigger this function when
 * complete. This function takes in the pipe pointer to handle
 */
static void usbhc_setup_sent(struct usb_core* core, struct usb_pipe* pipe)
{
    /* 
     * The first byte in the setup packet indicates what the next
     * transaction will be. See USB spesification page 248
     */
    if (pipe->x.control.setup[0] & USB_REQ_TYPE_DEVICE_TO_HOST) {
        pipe->x_status = USB_X_STATUS_DATA_IN;
        usbhc_control_in(core, pipe);
    }
}

/*
 * Received packet handler. This is called when either a full or a partial data
 * has been received
 */
static void usbhc_received_packet(struct usb_core* core, struct usb_pipe* pipe)
{
    usbhc_read_raw(core, pipe);
}

/*
 * Transmitted data complete. This is called when the USB has transmitted a new
 * packet.
 */
static void usbhc_transmitt_complete(struct usb_core* core, struct usb_pipe* pipe)
{

}

/*
 * Pipe handler. This gets called when a pipe interrupt occurs. This code should
 * clear the corresponding interrupt flag(s)
 */
static void usbhc_pipe_handler(struct usb_core* core, u32 global_status)
{
    /* Clear the first pipe interrupt */
    u8 pipe;
    for (pipe = 0; pipe < MAX_PIPES; pipe++) {
        if (global_status & (1 << (pipe + 8))) {
            usbhc_clear_global_status(1 << (pipe + 8));
            break;
        }
    }

    /* Read the pipe interrupt status register */
    u32 pipe_isr = usbhc_pipe_get_status(pipe);
    u32 pipe_imr = usbhc_pipe_get_interrupt_mask(pipe);

    /* Get the pointer to the pipe to handler */
    struct usb_pipe* pipe_ptr = &core->hw->pipes[pipe];

    /* Check for USB stall */
    if (pipe_isr & pipe_imr & (1 << 6)) {
        panic("Stall");
    }

    /* Check for pipe overflow */
    if (pipe_isr & pipe_imr & (1 << 5)) {
        panic("Overflow");
    }

    /* Check for pipe error */
    if (pipe_isr & pipe_imr & (1 << 3)) {
        panic("Pipe error");
    }

    /* Transmitted setup packet */
    if (pipe_isr & pipe_imr & (1 << 2)) {
        /* 
         * Disable and clear the interrupt flag since two setup
         * packet will not follow eachother
         */
        usbhc_pipe_clear_status(pipe, 1 << 2);
        usbhc_pipe_disable_interrupt(pipe, 1 << 2);

        /* Call the handler for setup packet */
        usbhc_setup_sent(core, pipe_ptr);
    }

    /* Received full packet */
    if (pipe_isr & pipe_imr & 1) {
        usbhc_pipe_clear_status(pipe, 1 << 0);
        usbhc_pipe_disable_interrupt(pipe, 1 << 0);

        usbhc_received_packet(core, pipe_ptr);
    }

    /* Transmitted data complete */
    if (pipe_isr & pipe_imr & (1 << 1)) {
        usbhc_pipe_clear_status(pipe, 1 << 1);
        usbhc_pipe_disable_interrupt(pipe, 1 << 1);

        usbhc_transmitt_complete(core, pipe_ptr);
    }
}

/*
 * SOF handler. This gets called when a SOF and a  uSOF has been sent on the
 * line. This code should clear the SOF flag.
 */
static void usbhc_sof_handler(struct usb_core* core, u32 global_status)
{
    usbhc_clear_global_status(1 << 5);
}

/*
 * USB core exception handler. This breakes down the USB exception into three
 * groups; pipe interrupts, SOF interrupts and root hub interrupt. Each of these
 * has its own handler, in which are responsible for clearing any interrupt
 * flags.
 */
void usb_exception(void)
{
    u32 global_status = usbhc_get_global_status();
    
    if (global_status & 0x3FF00) {
        usbhc_pipe_handler(usbhc_core, global_status);
        return;
    }
     
    if (global_status & 0x20) {
        usbhc_sof_handler(usbhc_core, global_status);
        return;
    }

    if (global_status & 0x5F) {
        usbhc_root_hub_handler(usbhc_core, global_status);
        return;
    }
}

