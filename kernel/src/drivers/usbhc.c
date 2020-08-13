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
#include "memory.h"

#include <stddef.h>

/*
 * The USB host controller uses one main structure to manage internal state 
 * and pointers to the pipes. This will be made from the caller the the pointer
 * will be stored here
 */
struct usbhc* usbhc_private = NULL;

/*
 * All URBs are allocated using a fast bitmap allocator. These will be allocated
 * for allmost every usb transfer, so the allocater needs to be really quick.
 */
struct bmalloc_desc urb_allocator;

/* General USB host core stuff */
static inline void usbhw_connection_error_enable(void);
static inline void usbhw_connection_error_disable(void);
static inline u8   usbhw_connection_error_check(void);
static inline void usbhw_connection_error_clear(void);
static inline void usbhw_connection_error_force(void);

static inline u32  usbhw_pipe_get_error_reg(u8 pipe);
static inline void usbhw_vbus_request_enable(void);
static inline void usbhw_send_resume(void);
static inline void usbhw_reset_clear(void);
static inline void usbhw_sof_enable(void);
static inline void usbhw_sof_disable(void);
static inline enum usb_device_speed usbhw_get_device_speed(void);
static inline void usbhw_set_host_speed(enum usb_host_speed speed);

/* Global USB host controller interrupts */
static inline u32  usbhw_global_get_status(void);
static inline u32  usbhw_global_get_interrupt_mask(void);
static inline void usbhw_global_clear_status(u32 mask);
static inline void usbhw_global_force_status(u32 mask);
static inline void usbhw_global_disable_interrupt(u32 mask);
static inline void usbhw_global_enable_interrupt(u32 mask);

/* Pipe interrupts */
static inline u32  usbhw_pipe_get_status(u8 pipe);
static inline u32  usbhw_pipe_get_interrupt_mask(u8 pipe);
static inline void usbhw_pipe_clear_status(u8 pipe, u32 mask);
static inline void usbhw_pipe_force_status(u8 pipe, u32 mask);
static inline void usbhw_pipe_disable_interrupt(u8 pipe, u32 mask);
static inline void usbhw_pipe_enable_interrupt(u8 pipe, u32 mask);

/* Frame info */
static inline u32  usbhw_get_frame_number(void);
static inline void usbhw_clear_frame_number(void);
static inline u8   usbhw_get_microframe_number(void);

/* Pipes maintainance */
static inline void usbhw_pipe_reset_assert(u8 pipe);
static inline void usbhw_pipe_reset_deassert(u8 pipe);
static inline void usbhw_pipe_allocate_set(u8 pipe);
static inline void usbhw_pipe_allocate_clear(u8 pipe);
static inline void usbhw_pipe_enable(u8 pipe);
static inline void usbhw_pipe_disable(u8 pipe);
static inline void usbhw_pipe_set_addr(u8 pipe, u8 addr);
static inline void usbhw_pipe_in_request_defined(u8 pipe, u8 count);
static inline void usbhw_pipe_in_request_continous(u8 pipe);
static inline void usbhw_pipe_set_configuration(u8 pipe, u32 cfg);
static inline u32  usbhw_pipe_get_configuration(u8 pipe);
static inline u8   usbhw_pipe_check_configuration(u8 pipe);
static inline void usbhw_pipe_set_token(u8 pipe, enum pipe_token token);
static inline void usbhw_pipe_set_freq(u8 pipe, u8 irq_freq);
static inline u32  usbhw_pipe_get_byte_count(u8 pipe);

static inline void usbhw_connection_error_enable(void) 
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 4);
    USBHC->CTRL = reg;
}

static inline void usbhw_connection_error_disable(void) 
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 4);
    USBHC->CTRL = reg;
}

static inline u8 usbhw_connection_error_check(void)
{
    if (USBHC->SR & (1 << 4)) {
        return 1;
    } else {
        return 0;
    }
}

static inline u32 usbhw_pipe_get_error_reg(u8 pipe)
{
    return USBHC->HSTPIPERR[pipe];
}

/*
 * Returns the speed status. This should be checked at the end of a reset
 * request. In host mode two pulldowns are connected. The speed status is based
 * on which line the device pulls up. 
 */
static inline enum usb_device_speed usbhw_get_device_speed(void)
{
    u32 reg = (USBHC->SR >> 12) & 0b11;
    return (enum usb_device_speed)reg;
}

static inline void usbhw_connection_error_clear(void)
{
    USBHC->SCR = (1 << 4);
}

static inline void usbhw_connection_error_force(void)
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
static inline void usbhw_vbus_request_enable(void)
{
    USBHC->SFR = (1 << 9);
}

/*
 * Sends a USB resume on the bus. A downstream USB resume is issued by the host
 * in order to wake up a suspended device. The USB resume is a K-state for at
 * least 20ms, followed by a low speed EOP. 
 */
static inline void usbhw_send_resume(void)
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
static inline void usbhw_reset_clear(void)
{
    USBHC->HSTCTRL &= ~(1 << 9);
}

/*
 * Starts generating SOFs and uSOFs. This will be automatically generated after
 * a bus reset if the host was in suspend state (SOFEN zero). This also
 * sets the WAKEUP flag.
 */
static inline void usbhw_sof_enable(void)
{
    USBHC->HSTCTRL |= (1 << 8);
}

static inline void usbhw_sof_disable(void)
{
    USBHC->HSTCTRL &= ~(1 << 8);
}

static inline void usbhw_set_host_speed(enum usb_host_speed speed)
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

static inline u32 usbhw_global_get_status(void)
{
    return USBHC->HSTISR;
}

static inline void usbhw_global_clear_status(u32 mask)
{
    USBHC->HSTICR = mask;
}

static inline void usbhw_global_force_status(u32 mask)
{
    USBHC->HSTIFR = mask;
}

static inline u32 usbhw_global_get_interrupt_mask(void)
{
    return USBHC->HSTIMR;
}

static inline void usbhw_global_disable_interrupt(u32 mask)
{
    USBHC->HSTIDR = mask;
}

static inline void usbhw_global_enable_interrupt(u32 mask)
{
    USBHC->HSTIER = mask;
}

static inline u32 usbhw_get_frame_number(void)
{
    return (u32)((USBHC->HSTFNUM >> 3) & 0x7FF);
}

static inline u8  usbhw_get_microframe_number(void)
{
    return (u8)(USBHC->HSTFNUM & 0x7);
}

static inline void usbhw_clear_frame_number(void)
{
    /* Perform a write operation to the FNUM field */
    USBHC->HSTFNUM &= ~(0x7FF << 3);
}

/*
 * Sets the pipe endpoint address. It takes in the endpoint number between
 * 0 and 9, and a 7 bit address identifying the USB device on the bus. Note that
 * pipe 0 should have its address set to one
 */
static inline void usbhw_pipe_set_addr(u8 pipe, u8 addr)
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

static inline void usbhw_pipe_reset_assert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP |= (1 << (pipe + 16));
}

static inline void usbhw_pipe_reset_deassert(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP &= ~(1 << (pipe + 16));
}

static inline void usbhw_pipe_enable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP |= (1 << pipe);
}

static inline void usbhw_pipe_disable(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIP &= ~(1 << pipe);
}

static inline u32 usbhw_pipe_get_status(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPISR[pipe];
}

static inline void usbhw_pipe_clear_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPICR[pipe] = mask;
}

static inline void usbhw_pipe_force_status(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIFR[pipe] = mask;
}

static inline u32 usbhw_pipe_get_interrupt_mask(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    return USBHC->HSTPIPIMR[pipe];
}

static inline void usbhw_pipe_enable_interrupt(u8 pipe, u32 mask)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPIER[pipe] = mask;
}

static inline void usbhw_pipe_disable_interrupt(u8 pipe, u32 mask)
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
static inline void usbhw_pipe_in_request_defined(u8 pipe, u8 count)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPINRQ[pipe] = count;
}

/*
 * Performs IN requests continously on the given pipe until the pipe is frozen
 */
static inline void usbhw_pipe_in_request_continous(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPINRQ[pipe] = (1 << 8);
}

static inline void usbhw_pipe_set_configuration(u8 pipe, u32 cfg)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] = cfg;
}

static inline void usbhw_pipe_allocate_set(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] |= (1 << 1);
}

static inline void usbhw_pipe_allocate_clear(u8 pipe)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    USBHC->HSTPIPCFG[pipe] &= ~(1 << 1);
}

static inline u32 usbhw_pipe_get_configuration(u8 pipe)
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
static inline u8 usbhw_pipe_check_configuration(u8 pipe) 
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

static inline void usbhw_pipe_set_token(u8 pipe, enum pipe_token token)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHC->HSTPIPCFG[pipe];
    reg &= ~(0b11 << 8);
    reg |= (token << 8);
    USBHC->HSTPIPCFG[pipe] = reg;
}

static inline void usbhw_pipe_set_freq(u8 pipe, u8 irq_freq)
{
    if (pipe >= 10) {
        panic("Pipe out of bound");
    }
    
    u32 reg = USBHC->HSTPIPCFG[pipe];
    reg &= ~(0xFF << 24);
    reg |= (irq_freq << 24);
    USBHC->HSTPIPCFG[pipe] = reg;
}

static inline u32 usbhw_pipe_get_byte_count(u8 pipe)
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

static inline void usbhw_pipe_freeze(u8 pipe)
{
    USBHC->HSTPIPIER[pipe] = (1 << 17);
}

static inline void usbhw_pipe_unfreeze(u8 pipe)
{
    USBHC->HSTPIPIDR[pipe] = (1 << 17);
}

static inline void usbhw_pipe_fifoctrl_enable(u8 pipe) 
{
    USBHC->HSTPIPIER[pipe] = (1 << 14);
}

static inline void usbhw_pipe_fifoctrl_disable(u8 pipe) 
{
    USBHC->HSTPIPIDR[pipe] = (1 << 14);
}

/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger an interrupt.
 * The CPU can only read/write FRZCLK and USBE when this bit is set
 */
void usbhw_freeze_clock(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg |= (1 << 14);
    USBHC->CTRL = reg;
}

void usbhw_unfreeze_clock(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 14);
    USBHC->CTRL = reg; 
}

void usbhw_enable(void)
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
void usbhw_disable(void)
{
    u32 reg = USBHC->CTRL;
    reg &= ~(1 << 24);      /* Clear UID */
    reg |= (1 << 8);        /* Set VBUSHWC */
    reg &= ~(1 << 15);
    USBHC->CTRL = reg;
}

void usbhw_set_mode(enum usb_mode mode)
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
void usbhw_send_reset(void)
{
    usbhw_set_host_speed(USB_HOST_SPEED_NORMAL);
    USBHC->HSTIER = (1 << 2);
    USBHC->HSTCTRL |= (1 << 9);
}

void usbhw_interrupt_disable(void)
{
    USBHC->HSTIDR = 0xFFFFFFFF;
    USBHC->HSTICR = 0xFFFFFFFF;
}

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if the clock is
 * usable, 0 if not
 */
u8 usbhw_clock_usable(void)
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
    usbhw_pipe_disable(pipe->number);

    /* Reset the pipe */
    usbhw_pipe_reset_assert(pipe->number);
    usbhw_pipe_reset_deassert(pipe->number);

    /* De-allocate all pipes by writing 1 to ALLOC */
    usbhw_pipe_set_configuration(pipe->number, 0);

    pipe->state = PIPE_STATE_DISABLED;
}

/*
 * Performs a soft reset of a pipe. After reset the datasheet says that all 
 * pipes are reset, but it s still recomended to perform a manual reset after
 * this. 
 */
void usbhc_pipe_soft_reset(struct usb_pipe* pipe)
{
    pipe->state = PIPE_STATE_DISABLED;
}

/*
 * Allocates a pipe
 */
u8 usbhc_pipe_allocate(struct usb_pipe* pipe, struct pipe_cfg* cfg)
{
    memory_copy(cfg, &pipe->cfg, sizeof(struct pipe_cfg));

    /* Configure and allocate the pipe */
    usbhw_pipe_reset_assert(pipe->number);
    usbhw_pipe_reset_deassert(pipe->number);

    /* The pipe should be enabled before the configuration set */
    usbhw_pipe_enable(pipe->number);

    u32 cfg_reg = 0;
    cfg_reg |= (cfg->frequency << 24);
    cfg_reg |= (cfg->pipe << 16);
    cfg_reg |= (cfg->autosw << 10);
    cfg_reg |= (cfg->type << 12);
    cfg_reg |= (cfg->token << 8);
    cfg_reg |= (cfg->size << 4);
    cfg_reg |= (cfg->banks << 2);
    
    usbhw_pipe_set_configuration(pipe->number, cfg_reg);
    cfg_reg |= (1 << 1);            /* Allocate bit */
    usbhw_pipe_set_configuration(pipe->number, cfg_reg);

    print("Pipe number: %d\n", pipe->number);

    if (!(usbhw_pipe_get_status(pipe->number) & (1 << 18))) {
        return 0;
    }

    /* Enable pipe interrupts */
    usbhw_pipe_clear_status(pipe->number, (1 << 3) | (1 << 5) | (1 << 6));
    usbhw_pipe_enable_interrupt(pipe->number, (1 << 3) | (1 << 5) | (1 << 6));
    usbhw_global_enable_interrupt(1 << (pipe->number + 8));
    return 1;
}

/*
 * Sets the address of a device. Pipe number zero should allways have 
 * its address field set to zero.
 */
void usbhc_set_address(struct usb_pipe* pipe, u8 addr)
{
    usbhw_pipe_set_addr(pipe->number, addr);
}

/*
 * Default callback for root hub changes passed to USB host core. If noe 
 * callback is assigned, this function will run. 
 */
void default_root_hub_callback(struct usbhc* hc, enum root_hub_event event)
{
    (void)hc;
    (void)event;
}

/*
 * Assigns a new root hub callback to the UBSHC
 */
void usbhc_add_root_hub_callback(struct usbhc* hc,
    void (*callback)(struct usbhc* , enum root_hub_event))
{
    hc->root_hub_callback = callback;
}

/*
 * Initializes the USB hardware
 */
void usbhc_init(struct usbhc* hc, struct usb_pipe* pipe, u32 pipe_count)
{
    /* Disable all interrupts */
    usbhw_interrupt_disable();

    usbhw_unfreeze_clock();
    usbhw_set_mode(USB_HOST);
    usbhw_enable();
    
    /* This must be called in order to detect device connection or dissconnection */
    usbhw_vbus_request_enable();

    /* Make a new bitmap allocator. This is used for allocating URBs */
    bmalloc_new(&urb_allocator, sizeof(struct urb), MAX_URBS, PMALLOC_BANK_2);

    /* Link the pipes to the USB host controller */
    hc->pipe_base = pipe;
    hc->pipe_count = pipe_count;
    hc->root_hub_callback = &default_root_hub_callback;

    /* Assign the private USBHC pointer */
    usbhc_private = hc;

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
    usbhw_global_clear_status((1 << 0) | (1 << 6));
    usbhw_global_enable_interrupt((1 << 0) | (1 << 6));
}

/*
 * This functions performs a setup request. Every USB setup request
 * is 8 bytes. This function will not return eny status. This will 
 * entirly be handled by the interrupt.
 */
u8 usbhc_send_setup_raw(struct usb_pipe* pipe, u8* setup)
{
    u8 pipe_number = pipe->number;
    if (usbhw_pipe_get_byte_count(pipe_number)) {
        return 0;
    }
    /* Clear the transmitted SETUP packet bit */
    usbhw_pipe_clear_status(pipe_number, (1 << 2));
    usbhw_pipe_set_token(pipe_number, PIPE_TOKEN_SETUP);
    
    u8* fifo_ptr = (u8 *)usbhc_get_fifo_ptr(pipe_number);
    for (u8 i = 0; i < 8; i++) {
        *fifo_ptr++ = *setup++;
    }
    u32 fifo_count = usbhw_pipe_get_byte_count(pipe_number);

    /* Enable the interrupt */
    usbhw_pipe_enable_interrupt(pipe_number, (1 << 2));

    /* Enable the USB hardware to access the FIFO */
    usbhw_pipe_unfreeze(pipe_number);
    usbhw_pipe_fifoctrl_disable(pipe_number);
    return 1;
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
    dlist_insert_last(&urb->node, &pipe->urb_queue);
}

u8 usbhc_urb_cancel(struct urb* urb, struct usb_pipe* pipe)
{
    /* Check the state */
    /* .... */

    /* Remove the URB from the URB queue */
    dlist_remove(&urb->node, &pipe->urb_queue);

    return 1;
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
static void usbhc_root_hub_exception(u32 isr, struct usbhc* hc)
{
    print("ROOT HUB ISR --> %32b\n", isr);
    /* Check for device connection */
    if (isr & (1 << 0)) {
        print("Device connection\n");

        if (usbhw_global_get_interrupt_mask() & (1 << 6)) {
        }

        /* Listen for disconnection and reset sent */
        usbhw_global_clear_status((1 << 1) | (1 << 2) | (1 << 5));
        usbhw_global_enable_interrupt((1 << 1) | (1 << 2) | (1 << 5));

        /* Callback to upper layer i.e. USB host core */
        hc->root_hub_callback(hc, RH_EVENT_CONNECTION);
    }

    /* Check for a wakeup interrupt */
    if (isr & (1 << 6)) {
        printl("Print wakeup");
        while (1);
    }

    /* Reset has been sent on the port */
    if (isr & (1 << 2)) {
        usbhw_global_clear_status(1 << 2);
        usbhw_global_disable_interrupt(1 << 2);

        /* Read the device speed status */
        enum usb_device_speed speed = usbhw_get_device_speed();

        if (speed == USB_DEVICE_FS) {
            print("Full speed device connected\n");
        }

        hc->root_hub_callback(hc, RH_EVENT_RESET_SENT);
    }
}

static void usbhc_handle_setup_sent(struct usb_pipe* pipe)
{
    /* Clear the SETUP sent interrupt flag */
    usbhw_pipe_clear_status(pipe->number, (1 << 2));

    print("Setup is sent");


}

/*
 * Pipe exceptions. This only handles the first pipe with an exception.
 */
static void usbhc_pipe_exception(u32 isr, struct usbhc* hc)
{
    u32 pipe_number;
    for (pipe_number = 0; pipe_number < MAX_PIPES; pipe_number++) {
        if (isr & (1 << (pipe_number + 8))) {
            break;
        }
    }
    /* Clear the global flag on the current pipe */
    usbhw_global_clear_status(1 << (pipe_number + 8));

    u32 pipe_status = usbhw_pipe_get_status(pipe_number);
    print("pipe status: %32b\n", pipe_status);
    
    /* Check for transmittet setup */
    if (pipe_status & (1 << 2)) {
        usbhc_handle_setup_sent(&hc->pipe_base[pipe_number]);
    }
}

/*
 * SOF exception
 */
static void usbhc_sof_exception(u32 isr, struct usbhc* hc)
{
    /* Clear the SOF interrupt flag */
    usbhw_global_clear_status((1 << 5));
}

/*
 * Main USBHC interrupt handler
 */
void usb_exception(void)
{
    u32 isr = usbhw_global_get_status();

    /* SOF interrupt */
    if (isr & 0x20) {
        //printl("Interrupt - SOF");
        usbhc_sof_exception(isr, usbhc_private);
    }

    /* Pipe interrupts */
    if (isr & 0x3FF00) {
        printl("Interrupt - pipe");
        usbhc_pipe_exception(isr, usbhc_private);
    }

    /* Root-hub interrupts */
    if (isr & 0x5F) {
        printl("Interrupt - root hub");
        usbhc_root_hub_exception(isr, usbhc_private);
    }  
    /* During development this will make sure that  */
    usbhw_global_clear_status(0xFFFFFFFF);  
}
