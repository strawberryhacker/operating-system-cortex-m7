/* Copyright (C) StrawberryHacker */

#include "usbhc.h"
#include "hardware.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "cpu.h"
#include "usb_protocol.h"
#include "umalloc.h"
#include "pmalloc.h"
#include "memory.h"
#include "config.h"
#include <stddef.h>

static void usbhc_setup_in(struct usb_pipe* pipe);
static u8 usbhc_setup_out(struct usb_pipe* pipe, u8* setup);
static void usbhc_control_status_out(struct usb_pipe* pipe);

/* Read and write packages from the FIFO */
static void usbhc_in(struct usb_pipe* pipe, struct urb* urb);
static void usbhc_out(struct usb_pipe* pipe, struct urb* urb);

/* Reset */
static void usbhc_pipe_reset(struct usb_pipe* pipe);
static void usbhc_pipe_soft_reset(struct usb_pipe* pipe);

static inline u32 usbhc_pick_interrupted_pipe(u32 pipe_mask);
static void usbhc_start_urb(struct urb* urb, struct usb_pipe* pipe);
static void usbhc_end_urb(struct urb* urb, struct usb_pipe* pipe,
    enum urb_status status);

/* Root-hub event handlers */
static void usbhc_handle_root_hub_connect(u32 isr, struct usbhc* usbhc);
static void usbhc_handle_root_hub_reset(u32 isr, struct usbhc* usbhc);

/* Pipe event handlers */
static void usbhc_handle_setup_sent(struct usb_pipe* pipe);
static void usbhc_handle_receive_in(struct usb_pipe* pipe, u32 isr);
static void usbhc_handle_transmit_out(struct usb_pipe* pipe);

/* Common interrupt handlers derived form the main interrupt handler */
static inline void usbhc_root_hub_exception(u32 isr, struct usbhc* usbhc);
static inline void usbhc_pipe_exception(u32 isr, struct usbhc* usbhc);
static inline void usbhc_sof_exception(u32 isr, struct usbhc* usbhc);

/* String representation of the different pipe states */
static const char* pipe_states[] = {
    [0x00] = "DISABLED",
    [0x01] = "IDLE",
    [0x02] = "SETUP",
    [0x03] = "SETUP IN",
    [0x04] = "SETUP OUT",
    [0x05] = "ZLP IN",
    [0x06] = "ZLP OUT",
    [0x07] = "IN",
    [0x08] = "OUT",
    [0x09] = "STATUS",
    [0x0A] = "ERROR"
};

/*
 * The USB host controller uses one main structure to manage internal state 
 * and pointers to the pipes. This will be made by the caller and the the
 * pointer will be stored here
 */
struct usbhc* usbhc_private = NULL;

/*
 * All URBs are allocated using a fast bitmap allocator. These will be allocated
 * for allmost every usb transfer, so the allocater needs to be really fast.
 */
struct umalloc_desc urb_allocator;

/* 
 * After reset the datasheet says that all pipes are reset, but it s still
 * recomended to perform a manual reset after this
 */
static void usbhc_pipe_reset(struct usb_pipe* pipe) 
{
    /* Initialize the URB queue linked to the pipe */
    list_init(&pipe->urb_list);

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
 * Performs a soft reset of a pipe. 
 */
static void usbhc_pipe_soft_reset(struct usb_pipe* pipe)
{
    spinlock_aquire(&pipe->lock);
    pipe->state = PIPE_STATE_DISABLED;
    spinlock_release(&pipe->lock);
}

/*
 * Default callback for root hub changes passed to USB host core. If no 
 * callback is assigned, this function will run. 
 */
void default_root_hub_callback(struct usbhc* usbhc, enum root_hub_event event)
{
    (void)usbhc;
    (void)event;
}

/*
 * Default callback for SOF
 */
void default_sof_callback(struct usbhc* usbhc)
{
    (void)usbhc;
}

/*
 * According to page 752 in the datasheet the USB host controller must be 
 * anabled and unfrozen before any clocks are enabled. The USB host controller
 * configuration (below) will be reconfigured after the clocks are enabled. 
 */
void usbhc_early_init(void)
{
    usbhw_unfreeze_clock();
    usbhw_set_mode(USB_HOST);
    usbhw_enable();
}

/*
 * Initializes the USB hardware
 */
void usbhc_init(struct usbhc* usbhc, struct usb_pipe* pipe, u32 pipe_count)
{
    /* Disable all global interrupts */
    usbhw_global_disable_interrupt(0xFFFFFFFF);
    usbhw_global_clear_status(0xFFFFFFFF);

    usbhw_unfreeze_clock();
    usbhw_set_mode(USB_HOST);
    usbhw_enable();
    
    /* This must be called in order to detect device connection or dissconnection */
    usbhw_vbus_request_enable();

    /* Make a new bitmap allocator. This is used for allocating URBs */
    umalloc_new(&urb_allocator, sizeof(struct urb),
        URB_MAX_COUNT, URB_ALLOCATOR_BANK);

    /* Link the pipes to the USB host controller */
    usbhc->pipe_base = pipe;
    usbhc->pipe_count = pipe_count;
    usbhc->root_hub_callback = &default_root_hub_callback;

    /* Assign the private USBHC pointer */
    usbhc_private = usbhc;

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
    usbhw_global_clear_status(USBHW_CONN | USBHW_WAKEUP);
    usbhw_global_enable_interrupt(USBHW_CONN | USBHW_WAKEUP);
}

u8 usbhc_clock_usable(void)
{
    return usbhw_clock_usable();
}

void usbhc_send_reset(void)
{
    /* Clear the host speed configuration */
    usbhw_set_host_speed(USB_HOST_SPEED_NORMAL);
    usbhw_send_reset();
}

static void usbhc_in(struct usb_pipe* pipe, struct urb* urb)
{
    /* Read the number of bytes in the FIFO */
    volatile u32 fifo_count = usbhw_pipe_get_byte_count(pipe->number);
    print("FIFO count => %d\n", fifo_count);
    volatile u8* dest = (volatile u8 *)urb->transfer_buffer;
    volatile u8* src = usbhc_get_fifo_ptr(pipe->number);

    urb->receive_length += fifo_count;
    while (fifo_count--) {
        *dest++ = *src++;
    }
}

static void usbhc_out(struct usb_pipe* pipe, struct urb* urb)
{
    printl("USB OUT FIFO write");
}

/*
 * This functions performs a setup request. Every USB setup request
 * is 8 bytes. This function will not return eny status. This will 
 * entirly be handled by the interrupt.
 */
static u8 usbhc_setup_out(struct usb_pipe* pipe, u8* setup)
{
    u8 pipe_number = pipe->number;
    if (usbhw_pipe_get_byte_count(pipe_number)) {
        return 0;
    }
    print("=> => pipe status => %32b\n", usbhw_pipe_get_status(pipe_number));
    /* Clear the transmitted SETUP packet bit */
    usbhw_pipe_clear_status(pipe_number, USBHW_TXSETUP);
    usbhw_pipe_set_token(pipe_number, PIPE_TOKEN_SETUP);
    
    volatile u8* fifo_ptr = usbhc_get_fifo_ptr(pipe_number);
    for (u8 i = 0; i < 8; i++) {
        *fifo_ptr++ = *setup++;
    }
    u32 fifo_count = usbhw_pipe_get_byte_count(pipe_number);

    /* Enable the interrupt */
    usbhw_pipe_enable_interrupt(pipe_number, USBHW_TXSETUP);

    /* Enable the USB hardware to access the FIFO and start transfer */
    usbhw_pipe_disable_interrupt(pipe_number, USBHW_PFREEZE | USBHW_FIFO_CTRL);
    return 1;
}

static void usbhc_control_status_out(struct usb_pipe* pipe)
{
    print("Sending ZLP out\n");
    u32 fifo_count = usbhw_pipe_get_byte_count(pipe->number);
    if (fifo_count) {
        panic("FIFO not zero");
    }
    usbhw_pipe_clear_status(pipe->number, USBHW_TXOUT);
    usbhw_pipe_set_token(pipe->number, PIPE_TOKEN_OUT);
    usbhw_pipe_enable_interrupt(pipe->number, USBHW_TXOUT);
    usbhw_pipe_disable_interrupt(pipe->number, USBHW_FIFO_CTRL | USBHW_PFREEZE);
}

static void usbhc_setup_in(struct usb_pipe* pipe)
{
    usbhw_pipe_clear_status(pipe->number, USBHW_RXIN | USBHW_SHORTPKT);
    usbhw_pipe_in_request_defined(pipe->number, 1);
    usbhw_pipe_set_token(pipe->number, PIPE_TOKEN_IN);
    usbhw_pipe_enable_interrupt(pipe->number, USBHW_RXIN);
    usbhw_pipe_disable_interrupt(pipe->number, USBHW_FIFO_CTRL | USBHW_PFREEZE);
}

/*
 * Starts the execution of a URB
 */
static void usbhc_start_urb(struct urb* urb, struct usb_pipe* pipe)
{
    print("\n\nStarting URB => %s => pipe state: %s\n", urb->name, pipe_states[pipe->state]);
    if (urb->flags & URB_FLAGS_SETUP) {
        pipe->state = PIPE_STATE_CTRL_OUT;
        usbhc_setup_out(pipe, urb->setup_buffer);
    }
}

static void usbhc_end_urb(struct urb* urb, struct usb_pipe* pipe,
    enum urb_status status)
{
    /* Reset the state of the pipe */
    pipe->state = PIPE_STATE_IDLE;

    /*
     * Delting the URB from the URB queue must happend before the callback, in
     * case the caller is using the same URB to enqueue a new request
     */
    list_delete_node(&urb->node);

    /* Update status and perform callback */
    urb->status = status;
    urb->callback(urb);

    /* Check if we can start another transfer directly */
    if ((pipe->urb_list.next != &pipe->urb_list) && pipe->state == PIPE_STATE_IDLE) {
        struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
        usbhc_start_urb(urb, pipe);
    } else {
        print("Cannot start next trasfer directly\n");
    }
}

/*
 * This finds the first pipe which has generatet an interrupt. This will also 
 * change the local starting point in cases where a pipe demands high
 * interrupt usage. This makes sure that all pipes will be handled, and also 
 * speeds up search in cases where multiple interrupt happends at once
 */
static inline u32 usbhc_pick_interrupted_pipe(u32 pipe_mask)
{
    static u32 pipe_start = 0;
    u32 pipe_number = pipe_start;
    for (u32 i = 0; i < MAX_PIPES; i++) {
        if (pipe_mask & (1 << pipe_number)) {
            break;
        }
        pipe_number++;
        if (pipe_number >= MAX_PIPES) {
            pipe_number = 0;
        }
    }

    /* Change the starting point */
    pipe_start++;
    if (pipe_start >= MAX_PIPES) {
        pipe_start = 0;
    }
    return pipe_number;
}

static void usbhc_handle_setup_sent(struct usb_pipe* pipe)
{
    if (pipe->state != PIPE_STATE_CTRL_OUT) {
        panic("State error");
    }
    /* Clear the SETUP sent interrupt flag */
    usbhw_pipe_clear_status(pipe->number, USBHW_TXSETUP);
    printl("Setup is sent");

    /* Firgure out wether to perform an SETUP IN or a SETUP OUT traansaction */   
    struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
    u8 req_type = urb->setup_buffer[0];

    if (urb->transfer_length) {
        if ((req_type & USB_DEVICE_TO_HOST) == USB_DEVICE_TO_HOST) {
            pipe->state = PIPE_STATE_SETUP_IN;
            usbhc_setup_in(pipe);
        } else {
            pipe->state = PIPE_STATE_SETUP_OUT;
            usbhc_out(pipe, urb);
        }
    } else {
        /* No data stage */
        if ((req_type & USB_DEVICE_TO_HOST) == USB_DEVICE_TO_HOST) {
            /* ZLP OUT stage */
            pipe->state = PIPE_STATE_ZLP_OUT;
            usbhw_pipe_disable_interrupt(pipe->number, USBHW_RXIN);
            usbhc_control_status_out(pipe);
        } else {
            /* ZLP in stage */
            pipe->state = PIPE_STATE_ZLP_IN;
            usbhc_setup_in(pipe);
        }
    }
}

static void usbhc_handle_receive_in(struct usb_pipe* pipe, u32 isr)
{
    usbhw_pipe_clear_status(pipe->number, USBHW_RXIN | USBHW_SHORTPKT);

    struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);

    /* Read the data recieved using the URB only */
    usbhc_in(pipe, urb);

    if (pipe->state == PIPE_STATE_ZLP_IN) {
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
        return;
    }

    if (isr & USBHW_SHORTPKT) {
        /* This is a short packet and marks the end of the data stage */
        printl("(short packet)");
        if (urb->setup_buffer[0] & USB_HOST_TO_DEVICE) {
            pipe->state = PIPE_STATE_ZLP_IN;
        } else {
            /* ZLP OUT stage */
            pipe->state = PIPE_STATE_ZLP_OUT;
            usbhw_pipe_disable_interrupt(pipe->number, USBHW_RXIN);
            usbhc_control_status_out(pipe);
        }
    }
}

static void usbhc_handle_transmit_out(struct usb_pipe* pipe)
{
    usbhw_pipe_clear_status(pipe->number, USBHW_TXOUT);
    
    if (pipe->state == PIPE_STATE_ZLP_OUT) {
        /* Setup transfer is done */
        struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
        printl("ZLP out done => status: %32b\n", usbhw_pipe_get_status(pipe->number));
        usbhw_pipe_enable_interrupt(pipe->number, USBHW_PFREEZE | USBHW_FIFO_CTRL);
        usbhw_pipe_in_request_defined(pipe->number, 1);
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
    }
}

static void usbhc_handle_root_hub_connect(u32 isr, struct usbhc* usbhc)
{
    /* Clear flags and disable interrupt on connect*/
    usbhw_global_clear_status(USBHW_CONN);
    usbhw_global_disable_interrupt(USBHW_CONN);

    /* Listen for disconnection, reset sent and SOF */
    usbhw_global_clear_status(USBHW_RST | USBHW_DCONN | USBHW_SOF);
    usbhw_global_enable_interrupt(USBHW_RST | USBHW_DCONN | USBHW_SOF);

    /* Callback to upper layer i.e. USB host core */
    usbhc->root_hub_callback(usbhc, RH_EVENT_CONNECTION);
}

static void usbhc_handle_root_hub_reset(u32 isr, struct usbhc* usbhc)
{
    usbhw_global_clear_status(USBHW_RST);
    usbhw_global_disable_interrupt(USBHW_RST);

    /* Read the device speed status */
    enum usb_device_speed speed = usbhw_get_device_speed();

    if (speed == USB_DEVICE_FS) {
        print("Full speed device connected\n");
    }

    usbhc->root_hub_callback(usbhc, RH_EVENT_RESET_SENT);
}

/*
 * This functions handles all root-hub changes, including connection, dis-
 * connection, wakeup etc. This will perform the necessary changes to the 
 * USB host controller interrup registers and perform callbacks to the upper
 * USB layers
 */
static inline void usbhc_root_hub_exception(u32 isr, struct usbhc* usbhc)
{
    print("Root hub ISR => %32b\n", isr);
    /* Check for device connection */
    if (isr & USBHW_CONN) {
        usbhc_handle_root_hub_connect(isr, usbhc);
    }

    /* Check for a wakeup interrupt */
    if (isr & USBHW_WAKEUP) {
        printl("Print wakeup");
        while (1);
    }

    /* Reset has been sent on the port */
    if (isr & USBHW_RST) {
        usbhc_handle_root_hub_reset(isr, usbhc);
    }
}

/*
 * This function handles all pipe exceptions on a pipe. Only the first pipe 
 * with an interrupt is handled due to interrupt latency. For example if the 
 * kernel or any peripherals has a pending interrupt, this can be executed 
 * between two consecutive pipe interrupts. If not other interrupt must wait 
 * for this to complete
 */
static inline void usbhc_pipe_exception(u32 isr, struct usbhc* usbhc)
{
    /* We only handle one pipe at a time */
    u32 pipe_mask = 0x3FF & (isr >> USBHW_PIPE_OFFSET);
    u32 pipe_number = usbhc_pick_interrupted_pipe(pipe_mask);

    /* Clear the global flag on the current pipe */
    usbhw_global_clear_status(1 << (pipe_number + USBHW_PIPE_OFFSET));

    u32 pipe_isr = usbhw_pipe_get_status(pipe_number);
    u32 pipe_imr = usbhw_pipe_get_interrupt_mask(pipe_number);

    //print("Pipe status => %32b\n", pipe_isr);
    struct usb_pipe* pipe = &usbhc->pipe_base[pipe_number];

    /* Check for any errors */
    if (pipe_isr & USBHW_PERROR) {
        print("Error on pipe %d => %32b\n", pipe_number, usbhw_pipe_get_error_reg(pipe_number));
        panic("");
        usbhw_pipe_clear_status(pipe_number, USBHW_PERROR);
        usbhw_pipe_set_error_reg(pipe_number, 0);
    }

    /* NAKed by device */
    if (pipe_isr & USBHW_NAKED) {
        print("NAKed\n");
    }
    
    /* Check for transmittet setup */
    if (pipe_isr & USBHW_TXSETUP) {
        usbhc_handle_setup_sent(pipe);
    }
    /* Check for receive IN interrupt */
    if (pipe_isr & pipe_imr & USBHW_RXIN) {
        print("Pipe excpetion => RX IN\n");

        /* 
         * The receive IN has to take in the interrupt status since its
         * operation depends on the short packet flag
         */
        usbhc_handle_receive_in(pipe, pipe_isr);
    }
    /* Check for transmitted out */
    if (pipe_isr & pipe_imr & USBHW_TXOUT) {
        print("Pipe excpetion => TX OUT\n");
        usbhc_handle_transmit_out(pipe);
    }
}

/*
 * This functions handles the SOF exception, manages all SOF events, and 
 * performs a callback to the upper USB layers if necessary (not micro-frame)
 */
static inline void usbhc_sof_exception(u32 isr, struct usbhc* usbhc)
{
    /* Clear the SOF interrupt flag */
    usbhw_global_clear_status(USBHW_SOF);
}

/*
 * Main USB host controller interrupt handler. This handles every interrupt
 * related to the USB on the system. This will not clear the interrupt status
 * flags, so this has to be done in the sub-routines
 */
void usb_exception(void)
{
    u32 isr = usbhw_global_get_status();

    /* SOF interrupt */
    if (isr & 0x20) {
        usbhc_sof_exception(isr, usbhc_private);
    }

    /* Pipe interrupts */
    if (isr & 0x3FF00) {
        usbhc_pipe_exception(isr, usbhc_private);
    }

    /* Root-hub interrupts */
    if (isr & 0x5F) {
        usbhc_root_hub_exception(isr, usbhc_private);
    }  
    /* During development this will make sure that  */
    usbhw_global_clear_status(0xFFFFFFFF);  
}

/*
 * Allocates a pipe
 */
u8 usbhc_alloc_pipe(struct usb_pipe* pipe, struct pipe_cfg* cfg)
{
    memory_copy(cfg, &pipe->cfg, sizeof(struct pipe_cfg));

    /* Resetting the pipe ends the eny previous transfers */
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
    cfg_reg |= (1 << 1);
    
    usbhw_pipe_set_configuration(pipe->number, cfg_reg);
    if (!(usbhw_pipe_get_status(pipe->number) & (1 << 18))) {
        return 0;
    }

    /* Enable pipe interrupts */
    usbhw_pipe_clear_status(pipe->number, 
        USBHW_PERROR | USBHW_OVERFLOW | USBHW_STALL);
    usbhw_pipe_enable_interrupt(pipe->number,
        USBHW_PERROR | USBHW_OVERFLOW | USBHW_STALL);

    usbhw_global_enable_interrupt(1 << (pipe->number + USBHW_PIPE_OFFSET));
    print("Pipe allocated => pipe %d\n", pipe->number);

    pipe->state = PIPE_STATE_IDLE;
    return 1;
}

/*
 * Allocates a USB request block
 */
struct urb* usbhc_alloc_urb(void)
{
    struct urb* urb = (struct urb *)umalloc(&urb_allocator);

    if (urb == NULL) {
        panic("URB alloc failed");
        return NULL;
    }
    list_node_init(&urb->node);
    return urb;
}

/*
 * Submits a URB into a pipe URB queue. This will do one of the following:
 *   - if the URB queue is empty it will start the transfer manually
 *   - if the URB queue is non-empty it will enqueue the URB intro the list
 */
void usbhc_submit_urb(struct urb* urb, struct usb_pipe* pipe)
{
    u32 urb_start = 0;
    if (pipe->urb_list.next == &pipe->urb_list) {
        urb_start = 1;
    }
    list_add_last(&urb->node, &pipe->urb_list);
    if (urb_start) {
        usbhc_start_urb(urb, pipe);
    }
}

/*
 * Attempts to cancel a URB that has not been precessed yet. Returns 1 if the
 * URB has been successfully canceled, and 0 if the URB cannot be canceled
 */
u8 usbhc_cancel_urb(struct urb* urb, struct usb_pipe* pipe)
{
    /* Check the state */
    /* .... */

    /* Remove the URB from the URB queue */
    list_delete_node(&urb->node);

    return 1;
}

/*
 * Fills a URB with control transfer data
 */
void usbhc_fill_control_urb(struct urb* urb, u8* setup, u8* transfer_buffer,
    u32 buffer_lenght, void (*callback)(struct urb*), u32 transfer_length,
    const char* name)
{
    urb->setup_buffer = setup;
    urb->transfer_buffer = transfer_buffer;
    urb->buffer_lenght = buffer_lenght;
    urb->callback = callback;
    urb->flags = URB_FLAGS_SETUP;
    urb->transfer_length = transfer_length;
    urb->receive_length = 0;

    string_copy(name, urb->name);
}

void print_urb_list(struct usb_pipe* pipe)
{
    struct list_node* list_node;
    list_iterate(list_node, &pipe->urb_list) {
        struct urb* urb = list_get_entry(list_node, struct urb, node);

        print("URB => %s\n", urb->name);
    }
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
 * Assigns a new root hub callback to the USB host controler
 */
void usbhc_add_root_hub_callback(struct usbhc* usbhc,
    void (*callback)(struct usbhc* , enum root_hub_event))
{
    usbhc->root_hub_callback = callback;
}

/*
 * Assigns a new SOF callback to the USB host controller. This will only 
 * be called on normal frames and not micro-frames. This is due to the overhead
 * in high-speed mode
 */
void usbhc_add_sof_callback(struct usbhc* usbhc, void (*callback)(struct usbhc*))
{
    usbhc->sof_callback = callback;
}
