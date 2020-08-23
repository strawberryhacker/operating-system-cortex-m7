/* Copyright (C) StrawberryHacker */

#ifndef usbhc_H
#define usbhc_H

#include "types.h"
#include "list.h"
#include "usbhw.h"
#include "spinlock.h"

/* Defines the maximum number of pipes supported by hardware */
#define MAX_PIPES 10

#define USBHC_DPRAM_ADDR 0xA0100000
#define USBHC_DPRAM_EP_SIZE 0x8000
/*
 * Returns an 8 bit pointer to the DPRAM base address associated with a pipe
 */
#define usbhc_get_fifo_ptr(pipe) \
	(volatile u8 *)(USBHC_DPRAM_ADDR + (USBHC_DPRAM_EP_SIZE * pipe))

enum root_hub_event {
    RH_EVENT_CONNECTION,
    RH_EVENT_DISCONNECTION,
    RH_EVENT_RESET_SENT
};

/*
 * Active
 * Halted
 * Aborting
 * Resetting
 * Clearing a halted
 * Halting
 */
enum pipe_state {
    PIPE_STATE_FREE,
    PIPE_STATE_CLAIMED,
    PIPE_STATE_IDLE,
    PIPE_STATE_CTRL_OUT,
    PIPE_STATE_ZLP_IN,
    PIPE_STATE_ZLP_OUT,
    PIPE_STATE_IN,
    PIPE_STATE_OUT,
    PIPE_STATE_STATUS,
    PIPE_STATE_ERROR
};

enum urb_status {
    URB_STATUS_OK,
    URB_STATUS_ERROR,
    URB_STATUS_STALL,
    URB_STATUS_NAK,
    URB_STATUS_ABORT
};

/*
 * Holds all neccesary fields needed to configure a pipe
 */
struct pipe_config {
    u8 dev_addr;
    u8 ep_addr;

    /* Requested pipe size */
    u32 size;

    u8 frequency;

    u8 banks : 2;
    u8 bank_switch : 1;

    enum pipe_type type;
};

/* USB transfer flags */
#define URB_FLAGS_SETUP        (u32)(1 << 0)
#define URB_FLAGS_IN           (u32)(1 << 1)
#define URB_FLAGS_OUT          (u32)(1 << 2)
#define URB_FLAGS_INTERRUPT_IN (u32)(1 << 3)

/*
 * Main USB request block (URB) structure. A URB will contain all nessecary data
 * to perform any USB transfer and deliver the status back. Every transfer on
 * the USB bus will be done through these blocks. They are exposed both to the 
 * USB host core (enumeration) as well as the USB drivers. 
 */
struct urb {
    /* URB queue node */
    struct list_node node;

    /* Transfer flags indicating what transfer to perform */
    u32 flags;

    /* Buffer for setup data */
    u8* setup_buffer;

    /* Buffer and transfer length for IN and OUT requests */
    u8* transfer_buffer;
    u32 transfer_length;
    u32 acctual_length;

    /* For error and status reporting */
    enum urb_status status;

    /* 
     * URB complete callback. The context field allows the user to add in a 
     * context for the callback. This has nothing to do with the callback, but
     * can be read directly from the structure in the 
     */
    void (*callback)(struct urb*);
    void* context;
};

/*
 * The USB driver will include one pipe descriptor per pipe. This will hold all
 * relevant information about that pipe. Most important is the configuration and
 * the transfer state. 
 */
struct usb_pipe {
    /*
     * Each USB pipe has assigned a fixed number. This corresponds to the pipe
     * number in the USB hardware. Threrefore the FIFO access address is fixed
     */
    u32 num;
    volatile u8* fifo;
    
    struct list_node urb_list;

    /* The pipe state applies to any transaction */
    struct spinlock lock;
    enum pipe_state state;
    enum pipe_type type;

    u32 ep_size;

    /* Hold the current pipe configuration */
    struct pipe_config config;
};

/*
 * Main USB host controller structure. This will contain all the pipes, and 
 * therefore all the URB trnasations in the system, pending or not. 
 */
struct usbhc {
    struct usb_pipe* pipes;
    u32 num_pipes;

    /* Callback for the root-hub */
    void (*root_hub_callback)(struct usbhc*, enum root_hub_event);
    void (*sof_callback)(struct usbhc*);
};

/*
 * Initialization of the USB host controller. The early init function should be
 * called before any USB clocks are enabled. If not the result is unpredictable. 
 * The normal intit functions will setup the host controller 
 */
void usbhc_early_init(void);

void usbhc_init(struct usbhc* usbhc, struct usb_pipe* pipe, u32 pipe_count);

u8 usbhc_clock_usable(void);

void usbhc_send_reset(void);

void usbhc_add_root_hub_callback(struct usbhc* usbhc,
    void (*callback)(struct usbhc* , enum root_hub_event));

void usbhc_add_sof_callback(struct usbhc* usbhc, void (*callback)(struct usbhc*));

struct usb_pipe* usbhc_request_pipe(void);

u8 usbhc_pipe_configure(struct usb_pipe* pipe, struct pipe_config* cfg);

void usbhc_set_address(struct usb_pipe* pipe, u8 addr);

void usbhc_set_ep_size(struct usb_pipe* pipe, u32 ep_size);

/*
 * URB (USB request blocks) is the main communication channel between the 
 * USB host core, device drivers, and the lower level host controller. All 
 * messages; iso, bulk, interrupt and control, should be issued with URBs. 
 * This structure will contain all information to submit any transaction and 
 * retport back the status.
 */
struct urb* usbhc_alloc_urb(void);

u8 usbhc_cancel_urb(struct urb* urb, struct usb_pipe* pipe);

void usbhc_submit_urb(struct urb* urb, struct usb_pipe* pipe);

void usbhc_set_urb_context(struct urb* urb, void* context);

void usbhc_fill_control_urb(struct urb* urb, u8* setup, u8* transfer_buffer,
                            void (*callback)(struct urb*));
    
void print_urb_list(struct usb_pipe* pipe);

#endif