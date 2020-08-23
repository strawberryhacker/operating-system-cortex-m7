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

static void usbhc_send_in(struct usb_pipe* pipe);
static u8 usbhc_setup_out(struct usb_pipe* pipe, u8* setup);
static void usbhc_control_status_out(struct usb_pipe* pipe);

/* Read and write packages from the FIFO */
static u8 usbhc_in(struct usb_pipe* pipe, struct urb* urb);
static void usbhc_out(struct usb_pipe* pipe, struct urb* urb);

/* Reset */
static void usbhc_pipe_init(struct usb_pipe* pipe);
static void usbhc_pipe_soft_reset(struct usb_pipe* pipe);

static inline u32 usbhc_pick_interrupted_pipe(u32 pipe_mask);
static void usbhc_start_urb(struct urb* urb, struct usb_pipe* pipe);
static void usbhc_end_urb(struct urb* urb, struct usb_pipe* pipe,
    enum urb_status status);

/* Root-hub event handlers */
static void usbhc_root_hub_connect(u32 isr, struct usbhc* usbhc);
static void usbhc_root_hub_disconnect(u32 isr, struct usbhc* usbhc);
static void usbhc_root_hub_reset(u32 isr, struct usbhc* usbhc);

/* Pipe event handlers */
static void usbhc_naked(struct usb_pipe* pipe);
static void usbhc_pipe_setup_sent(struct usb_pipe* pipe);
static void usbhc_pipe_receive_in(struct usb_pipe* pipe, u32 isr);
static void usbhc_transmit_out(struct usb_pipe* pipe);

/* Common interrupt handlers derived form the main interrupt handler */
static inline void usbhc_root_hub_exception(u32 isr, struct usbhc* usbhc);
static inline void usbhc_pipe_exception(u32 isr, struct usbhc* usbhc);
static inline void usbhc_sof_exception(u32 isr, struct usbhc* usbhc);

static u8 usbhc_size_to_pipe_size(u32 size, u8* pipe_size);
static u8 usbhc_pipe_to_index(struct usb_pipe* pipe, struct usbhc* hc, u8* index);
static u8 usbhc_solve_pipe_conflicts(struct usb_pipe* pipe, struct usbhc* hc);
static u8 usbhc_alloc_pipe(struct usb_pipe* pipe, u32 cfg);

/* String representation of the different pipe states, defined in usbhc.h */
static const char* pipe_states[] = {
    [0x00] = "FREE",
    [0x01] = "CLAIMED",
    [0x02] = "IDLE",
    [0x03] = "SETUP",
    [0x04] = "SETUP IN",
    [0x05] = "SETUP OUT",
    [0x06] = "ZLP IN",
    [0x07] = "ZLP OUT",
    [0x08] = "IN",
    [0x09] = "OUT",
    [0x0A] = "STATUS",
    [0x0B] = "ERROR"
};

static const char* usb_speed[] = {
    [0x00] = "full speed",
    [0x01] = "high speed",
    [0x02] = "low speed"
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
 * This resets the specified pipe. After reset the datasheet says that all pipes
 * are reset, but it is still recomended to perform a manual reset after this. 
 * This must ONLY be called on initialization, since it initializes the URB
 * list
 */
static void usbhc_pipe_init(struct usb_pipe* pipe) 
{
    /* Initialize the URB queue linked to the pipe */
    list_init(&pipe->urb_list);

    /* De-allocate all pipes by writing 1 to ALLOC */
    usbhw_pipe_set_configuration(pipe->num, 0);

    /* Reset the pipe */
    usbhw_pipe_reset_assert(pipe->num);
    usbhw_pipe_reset_deassert(pipe->num);

    /* Disable the pipe */
    usbhw_pipe_disable(pipe->num);

    pipe->state = PIPE_STATE_FREE;
}

/*
 * Performs a soft reset of a pipe. This resets the pipe, deallocates it and 
 * disables it. The pipe will be available for allocation again. 
 */
static void usbhc_pipe_soft_reset(struct usb_pipe* pipe)
{
    spinlock_aquire(&pipe->lock);
    pipe->state = PIPE_STATE_FREE;
    spinlock_release(&pipe->lock);
}

/*
 * Default callback for root hub changes passed to USB host core. The USB 
 * core should assign a callback. So this should not run.
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
 * enabled and unfrozen before any clocks are enabled. The USB host controller
 * configuration (below) will be reconfigured after the clocks are enabled. This
 * must be called proir to the configuration of any USB clock. 
 */
void usbhc_early_init(void)
{
    usbhw_unfreeze_clock();
    usbhw_set_mode(USB_HOST);
    usbhw_enable();
}

/*
 * Initializes the USB host controller
 */
void usbhc_init(struct usbhc* usbhc, struct usb_pipe* pipe, u32 pipe_count)
{
    usbhw_unfreeze_clock();
    usbhw_set_mode(USB_HOST);
    usbhw_enable();

    /* Disable all global interrupts */
    usbhw_global_disable_interrupt(0xFFFFFFFF);
    usbhw_global_clear_status(0xFFFFFFFF);
    
    /* This must be called in order to detect device connection or dissconnection */
    usbhw_vbus_request_enable();

    /* Initialize a bitmap allocator for URBs */
    umalloc_new(&urb_allocator, sizeof(struct urb),
        URB_MAX_COUNT, URB_ALLOCATOR_BANK);

    /* Link the pipes to the USB host controller */
    usbhc->pipes = pipe;
    usbhc->num_pipes = pipe_count;
    usbhc->root_hub_callback = &default_root_hub_callback;

    /* Assign the private USBHC pointer */
    usbhc_private = usbhc;

    /*
     * Initialize the pipes. The pipes are disabled and de-allocated after reset
     * but their configuration still remains 
     */
    for (u32 i = 0; i < pipe_count; i++) {
        pipe[i].num = i;
        pipe[i].fifo = usbhc_get_fifo_ptr(i);
        usbhc_pipe_init(&pipe[i]);
    }

    /* Listen for wakeup or connection interrupts */
    usbhw_global_clear_status(USBHW_CONN | USBHW_WAKEUP);
    usbhw_global_enable_interrupt(USBHW_CONN | USBHW_WAKEUP);
}

/*
 * Returns 1 if the USB 30 MHz clock is usable. I do not know what this 30 MHz
 * clock is, or how to enable it. 
 */
u8 usbhc_clock_usable(void)
{
    return usbhw_clock_usable();
}

/*
 * Sends a USB reset on the root hub port. This will reset the only device 
 * connected to the system (often a HUB)
 */
void usbhc_send_reset(void)
{
    /* Clear the host speed configuration */
    usbhw_set_host_speed(USB_HOST_SPEED_NORMAL);
    usbhw_send_reset();
}

/*
 * Read the data in the FIFO until end of FIFO or end of transfer. If a short 
 * packet has been received it returns 1
 */
static u8 usbhc_in(struct usb_pipe* pipe, struct urb* urb)
{
    /* Read the number of bytes in the FIFO */
    volatile u32 fifo_count = usbhw_pipe_get_byte_count(pipe->num);
    print("FIFO count => %d\n", fifo_count);
    volatile u8* dest = (volatile u8 *)urb->transfer_buffer;
    volatile u8* src = usbhc_get_fifo_ptr(pipe->num);

    dest += urb->acctual_length;
    print("acctual length %d current length %d\n", urb->transfer_length, urb->acctual_length);

    u8 short_pkt = (fifo_count != pipe->ep_size) ? 1 : 0;
    while (fifo_count) {
        *dest++ = *src++;
        urb->acctual_length++;
        fifo_count--;
        if (urb->acctual_length == urb->transfer_length) {
            break;
        }
    }
    usbhw_pipe_disable_interrupt(pipe->num, USBHW_FIFO_CTRL);
    if (fifo_count != 0) {
        printl("WARNING");
    }
    if (short_pkt) {
        printl("SHORT PACKET");
    }
    return short_pkt;
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
    u32 pipe_number = pipe->num;
    if (usbhw_pipe_get_byte_count(pipe_number)) {
        return 0;
    }
    usbhw_pipe_clear_status(pipe_number, USBHW_TXSETUP);
    usbhw_pipe_set_token(pipe_number, PIPE_TOKEN_SETUP);
    
    volatile u8* fifo_ptr = usbhc_get_fifo_ptr(pipe_number);
    for (volatile u8 i = 0; i < 8; i++) {
        *fifo_ptr++ = *setup++;
    }
    usbhw_pipe_enable_interrupt(pipe_number, USBHW_TXSETUP);
    usbhw_pipe_disable_interrupt(pipe_number, USBHW_PFREEZE | USBHW_FIFO_CTRL);
    return 1;
}

static void usbhc_control_status_out(struct usb_pipe* pipe)
{
    print("Sending ZLP out\n");
    u32 fifo_count = usbhw_pipe_get_byte_count(pipe->num);
    if (fifo_count) {
        panic("FIFO not zero");
    }
    usbhw_pipe_clear_status(pipe->num, USBHW_TXOUT);
    usbhw_pipe_set_token(pipe->num, PIPE_TOKEN_OUT);
    usbhw_pipe_enable_interrupt(pipe->num, USBHW_TXOUT);
    usbhw_pipe_disable_interrupt(pipe->num, USBHW_FIFO_CTRL | USBHW_PFREEZE);
}

static void usbhc_send_in(struct usb_pipe* pipe)
{
    usbhw_pipe_clear_status(pipe->num, USBHW_RXIN | USBHW_SHORTPKT);
    usbhw_pipe_in_request_defined(pipe->num, 1);
    usbhw_pipe_set_token(pipe->num, PIPE_TOKEN_IN);
    usbhw_pipe_enable_interrupt(pipe->num, USBHW_RXIN);
    usbhw_pipe_disable_interrupt(pipe->num, USBHW_FIFO_CTRL | USBHW_PFREEZE);
}

/*
 * Starts the execution of a URB
 */
static void usbhc_start_urb(struct urb* urb, struct usb_pipe* pipe)
{
    print("\n\nStarting URB => pipe state: %s\n", pipe_states[pipe->state]);

    urb->acctual_length = 0;

    if (urb->flags & URB_FLAGS_SETUP) {
        pipe->state = PIPE_STATE_CTRL_OUT;
        usbhc_setup_out(pipe, urb->setup_buffer);
    } else if (urb->flags & URB_FLAGS_INTERRUPT_IN) {
        usbhw_pipe_reset_assert(pipe->num);
        usbhw_pipe_reset_deassert(pipe->num);
        print("Pipe number => %d\n", pipe->config.ep_addr);
        print("Pipe number => %d\n", pipe->config.dev_addr);
        pipe->state = PIPE_STATE_IN;
        usbhw_pipe_in_request_continous(pipe->num);
        usbhw_pipe_disable_interrupt(pipe->num, USBHW_PFREEZE);
        usbhw_pipe_enable_interrupt(pipe->num, 0xFF);
        
        print("IN REQUEST\n");
    }
}

/*
 * This functions ends the current URB and performs the callback. If any more
 * URBs are being enqueued it will start the execution of the first one. This
 * reusults in a problem when only one URB is in the system.
 * 
 * Both the callback and this function can start a new trasfer, so we have to 
 * make sure it does not happend twice. Therefore the pipe status is checked 
 * before executing the URB to determine if the callback has allready started
 * the new transfer
 */
static void usbhc_end_urb(struct urb* urb, struct usb_pipe* pipe,
    enum urb_status status)
{
    if (urb->flags & URB_FLAGS_INTERRUPT_IN) {
        urb->status = status;
        urb->callback(urb);
        return;
    }
    pipe->state = PIPE_STATE_IDLE;
    list_delete_node(&urb->node);

    urb->status = status;
    urb->callback(urb);

    u8 list_empty = list_is_empty(&pipe->urb_list);
    if (!list_empty && (pipe->state == PIPE_STATE_IDLE)) {

        /* A new block can be started right away */
        struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
        usbhc_start_urb(urb, pipe);
    } else {
        print("Cannot start next trasfer directly\n");
    }
}

/*
 * Aborts the transfer on a pipe. This will happend if the pipe needs to be
 * reallocated since this will corrupt the DPRAM memory. This function does not
 * report the event to the USB core layer
 */
static void usbhc_abort_transfer(struct usb_pipe* pipe)
{
    usbhw_pipe_reset_assert(pipe->num);
    usbhw_pipe_reset_deassert(pipe->num);
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

static void usbhc_naked(struct usb_pipe* pipe)
{
    print("NAKed");
    usbhw_pipe_clear_status(pipe->num, USBHW_NAKED);
    /* Reset the pipe */
    //usbhc_end_urb(&pipe->urb_list.next, pipe, URB_STATUS_NAK);
}

static void usbhc_pipe_setup_sent(struct usb_pipe* pipe)
{
    if (pipe->state != PIPE_STATE_CTRL_OUT) {
        panic("State error");
    }
    /* Clear the SETUP sent interrupt flag */
    usbhw_pipe_clear_status(pipe->num, USBHW_TXSETUP);
    printl("Setup is sent");

    /* Firgure out wether to perform an SETUP IN or a SETUP OUT traansaction */   
    struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
    u8 req_type = urb->setup_buffer[0];

    if (urb->transfer_length) {
        if ((req_type & USB_DEVICE_TO_HOST) == USB_DEVICE_TO_HOST) {
            pipe->state = PIPE_STATE_IN;
            usbhc_send_in(pipe);
        } else {
            pipe->state = PIPE_STATE_OUT;
            usbhc_out(pipe, urb);
        }
    } else {
        /* No data stage */
        if ((req_type & USB_HOST_TO_DEVICE) == USB_HOST_TO_DEVICE) {
            /* ZLP in stage */
            pipe->state = PIPE_STATE_ZLP_IN;
            usbhc_send_in(pipe);
        } else {
            panic("");
        }
        
    }
}

static void usbhc_pipe_receive_in(struct usb_pipe* pipe, u32 isr)
{
    usbhw_pipe_clear_status(pipe->num, USBHW_RXIN | USBHW_SHORTPKT);
    struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);

    if (pipe->state == PIPE_STATE_ZLP_IN) {
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
        return;
    }

    /* Read the data received */
    u8 short_pkt = usbhc_in(pipe, urb);

    /* PIPE0 => 64   EP0 => 8 */
    /* Short packet received */
    if (short_pkt) {
        printl("DISABLING");
        usbhw_pipe_disable_interrupt(pipe->num, USBHW_RXIN);
        if (pipe->type == PIPE_TYPE_CTRL) {
            pipe->state = PIPE_STATE_ZLP_OUT;
            usbhc_control_status_out(pipe);
            return;
        }
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
        return;
    }

    /* Full packet received */
    if (urb->acctual_length >= urb->transfer_length) {
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
        return;
    }
    /* Have not received all data and no ZLP */
    usbhc_send_in(pipe);
}

static void usbhc_transmit_out(struct usb_pipe* pipe)
{
    usbhw_pipe_clear_status(pipe->num, USBHW_TXOUT);
    
    if (pipe->state == PIPE_STATE_ZLP_OUT) {
        /* Setup transfer is done */
        struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
        printl("ZLP out done => status: %32b\n", usbhw_pipe_get_status(pipe->num));
        usbhw_pipe_enable_interrupt(pipe->num, USBHW_PFREEZE | USBHW_FIFO_CTRL);
        usbhw_pipe_in_request_defined(pipe->num, 1);
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
    }
}

static void usbhc_root_hub_connect(u32 isr, struct usbhc* usbhc)
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

static void usbhc_root_hub_disconnect(u32 isr, struct usbhc* usbhc)
{
    usbhw_global_disable_interrupt(USBHW_DCONN);
    printl("Root hub disconnected");
}

static void usbhc_root_hub_reset(u32 isr, struct usbhc* usbhc)
{
    usbhw_global_clear_status(USBHW_RST);
    usbhw_global_disable_interrupt(USBHW_RST);

    /* Read the device speed status */
    enum usb_device_speed speed = usbhw_get_device_speed();

    printl("Device speed => %s", usb_speed[speed]);

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
    /* Check for device connection */
    if (isr & USBHW_CONN) {
        usbhc_root_hub_connect(isr, usbhc);
    }

    if (isr & USBHW_DCONN) {
        usbhc_root_hub_disconnect(isr, usbhc);
    }

    /* Check for a wakeup interrupt */
    if (isr & USBHW_WAKEUP) {
        printl("Print wakeup");
        while (1);
    }

    /* Reset has been sent on the port */
    if (isr & USBHW_RST) {
        usbhc_root_hub_reset(isr, usbhc);
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
    struct usb_pipe* pipe = &usbhc->pipes[pipe_number];

    print("Pipe number => %d\n", pipe->num);
    if (pipe->num == 1) {
        print("PIP ISR => %32\n", pipe_isr);
    }

    /* Check for stall */
    if (pipe_isr & USBHW_STALL) {
        print("Stalled on pipe %d targeting EP %d\n", pipe->num, pipe->config.ep_addr);
    }
    /* Check for any errors */
    if (pipe_isr & USBHW_PERROR) {
        print("Error on pipe %d => %32b\n", pipe_number, usbhw_pipe_get_error_reg(pipe_number));
        panic("");
        usbhw_pipe_clear_status(pipe_number, USBHW_PERROR);
        usbhw_pipe_set_error_reg(pipe_number, 0);
    }

    /* NAKed by device */
    if (pipe_isr & USBHW_NAKED) {
        usbhc_naked(pipe);
    }

    
    /* Check for transmittet setup */
    if (pipe_isr & USBHW_TXSETUP) {
        usbhc_pipe_setup_sent(pipe);
    }

    /* Check for receive IN interrupt */
    if (pipe_isr & pipe_imr & USBHW_RXIN) {
        print("Pipe excpetion => RX IN\n");

        /* 
         * The receive IN has to take in the interrupt status since its
         * operation depends on the short packet flag
         */
        usbhc_pipe_receive_in(pipe, pipe_isr);
    }

    /* Check for transmitted out */
    if (pipe_isr & pipe_imr & USBHW_TXOUT) {
        print("Pipe excpetion => TX OUT\n");
        usbhc_transmit_out(pipe);
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
    usbhc->sof_callback(usbhc);
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
static u8 usbhc_alloc_pipe(struct usb_pipe* pipe, u32 cfg)
{
    /* The pipe should be enabled before resetting and changing config */
    usbhw_pipe_enable(pipe->num);
    usbhw_pipe_reset_assert(pipe->num);
    usbhw_pipe_reset_deassert(pipe->num);

    cfg &= ~(1 << 1);    
    usbhw_pipe_set_configuration(pipe->num, cfg);
    cfg |= (1 << 1);
    usbhw_pipe_set_configuration(pipe->num, cfg);

    if (!(usbhw_pipe_get_status(pipe->num) & (1 << 18))) {
        return 0;
    }

    /* Enable pipe interrupts */
    usbhw_pipe_clear_status(pipe->num, 0xFF);
    usbhw_pipe_enable_interrupt(pipe->num,
        USBHW_PERROR | USBHW_OVERFLOW | USBHW_STALL | USBHW_NAKED);

    usbhw_global_enable_interrupt(1 << (pipe->num + USBHW_PIPE_OFFSET));

    usbhw_pipe_enable_interrupt(pipe->num, USBHW_PFREEZE);

    print("Status reg => %32b\n", usbhw_pipe_get_status(pipe->num));
    pipe->state = PIPE_STATE_IDLE;
    return 1;
}

/*
 * Tries to allocate a free pipe and returns it
 */
struct usb_pipe* usbhc_request_pipe(void)
{
    for (u8 i = 0; i < usbhc_private->num_pipes; i++) {
        struct usb_pipe* pipe = &usbhc_private->pipes[i];

        if (pipe->state == PIPE_STATE_FREE) {
            pipe->state = PIPE_STATE_CLAIMED;
            return pipe;
        }
    }
    printl("Shit, no more pipes");
    return NULL;
}

/*
 * This function converts a physical pipe size to the acctual pipe size. If the 
 * pipe size requested is not supported, it tries to increase the pipe size 
 * so that it supports it. Returns 1 if the returned pipe size is valid
 */
static u8 usbhc_size_to_pipe_size(u32 size, u8* pipe_size)
{
    u32 i;
    for (i = 32; i --> 0;) {
        if (size & (1 << i)) {
            break;
        }
    }
    u32 num = (1 << i);
    if (size & (num - 1)) {
        /* Size is not accurate, multiply by 2 */
        i++;
    }
    if ((i >= 3) && (i <= 10)) {
        *pipe_size = (i - 3);
        return 1;
    }
    return 0;
}

/*
 * Reallocates a pipe and performs the callback
 */
static u8 usbhc_pipe_reallocate(struct usb_pipe* pipe)
{
    if (pipe->state <= PIPE_STATE_CLAIMED) {
        /* Pipe is not active */
        return 1;
    }
    usbhc_abort_transfer(pipe);

    /* Reallocate the memory */
    u32 cfg = usbhw_pipe_get_configuration(pipe->num);
    cfg &= ~(1 << 1);
    usbhw_pipe_set_configuration(pipe->num, cfg);
    cfg |= (1 << 1);
    usbhw_pipe_set_configuration(pipe->num, cfg);

    if (!(usbhw_pipe_get_status(pipe->num) & (1 << 18))) {
        return 0;
    }

    /*
     * We have to end the URB after the reallocation. This is bacuse the pipe
     * needs to be ready to execute a new URB after the callback if either
     * a new URB has been submitted or if the queue is not empty. This pipe
     * will allways be functional after the allocation, but might mess up the 
     * memory of upper pipes
     */
    struct urb* urb = list_get_entry(pipe->urb_list.next, struct urb, node);
    usbhc_end_urb(urb, pipe, URB_STATUS_ABORT);

    printl("PIPE REALLOCATED");
    return 1;
}

/*
 * This will solve all the memory conflicts on the pipes with a higher muber 
 * than the given pipe
 */
static u8 usbhc_solve_pipe_conflicts(struct usb_pipe* pipe, struct usbhc* hc)
{
    /* Reallocate all pipes from index + 1 to num_pipes */
    for (u8 i = pipe->num + 1; i < hc->num_pipes; i++) {
        if (!usbhc_pipe_reallocate(&hc->pipes[i])) {
            return 0;
        }
    }
    return 1;
}

/*
 * Configures the specified pipe with the specified configuration. This will 
 * handle eventual memory conflicts in the DPRAM
 */
u8 usbhc_pipe_configure(struct usb_pipe* pipe, struct pipe_config* cfg)
{
    /* Default control pipe has a max size of 64 bytes, page 750 */
    if ((pipe->num) && (cfg->size > 64)) {
        return 0;
    }

    /* Pipe has to be claimed first, see usbhc_request_pipe */
    if (pipe->state != PIPE_STATE_CLAIMED) {
        return 0;
    }

    u8 size;
    if (!usbhc_size_to_pipe_size(cfg->size, &size)) {
        return 0;
    }

    if (cfg->banks < 1) {
        return 0;
    }

    /* Configuration is OK */
    u32 cfg_reg = 0;
    cfg_reg |= (cfg->frequency << 24);
    cfg_reg |= (cfg->ep_addr << 16);
    cfg_reg |= (cfg->type << 12);
    cfg_reg |= (cfg->bank_switch << 10);
    cfg_reg |= (size << 4);
    cfg_reg |= ((cfg->banks - 1) << 2);

    if (!usbhc_alloc_pipe(pipe, cfg_reg)) {
        return 0;
    }
    usbhc_set_address(pipe, cfg->dev_addr);
    pipe->ep_size = cfg->size;

    /* Allocation of pipe was successful */
    memory_copy(cfg, &pipe->config, sizeof(struct pipe_config));
    pipe->state = PIPE_STATE_IDLE;

    /*
     * Due the the stupid way the USB DPRAM is organized (see page 756) all 
     * pipes with a higher number will be reallocated to solve any memory
     * conflicts. If the size to be allocated is the same size as the pipe had
     * in its previous state, the reallocation is not necessary. When a pipe is
     * reallocated the current transfer on that buffer has to abort. This will
     * again trigger the callback, so extra execution time might occur
     */
    usbhc_solve_pipe_conflicts(pipe, usbhc_private);

    print("Pipe allocation success\n");
    return 1;
}

/*
 * Sets the address of a device. Pipe number zero should allways have 
 * its address field set to zero.
 */
void usbhc_set_address(struct usb_pipe* pipe, u8 addr)
{
    usbhw_pipe_set_addr(pipe->num, addr);
}

void usbhc_set_ep_size(struct usb_pipe* pipe, u32 ep_size)
{
    /*
     * The ep_size cannot be changed while a URB is being processed. In this 
     * case the current trasfer if aborted and the endpoint size is updated 
     * after
     */
    pipe->ep_size = ep_size;
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
    print("PIPE => %32b\n", usbhw_pipe_get_status(pipe->num));
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
 * Helps filling a URB with control transfer data
 */
void usbhc_fill_control_urb(struct urb* urb, u8* setup, u8* transfer_buffer,
                            void (*callback)(struct urb*))
{
    urb->setup_buffer    = setup;
    urb->transfer_buffer = transfer_buffer;
    urb->callback        = callback;
    urb->flags           = URB_FLAGS_SETUP;
    urb->transfer_length = *(u16 *)(urb->setup_buffer + 6);
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
