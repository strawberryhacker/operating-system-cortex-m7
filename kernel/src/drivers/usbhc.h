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

/* USB transfer flags */
#define URB_FLAGS_SETUP  (u32)(1 << 0)
#define URB_FLAGS_IN     (u32)(1 << 1)
#define URB_FLAGS_OUT    (u32)(1 << 2)

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

enum pipe_state {
    PIPE_STATE_DISABLED,
    PIPE_STATE_IDLE,
    PIPE_STATE_SETUP,
    PIPE_STATE_SETUP_IN,
    PIPE_STATE_SETUP_OUT,
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
    URB_STATUS_NAK
};

/*
 * Holds all neccesary fields needed to configure a pipe
 */
struct pipe_cfg {
    u8 frequency;
    /* Sets the address and pipe */
    u8 device;
    u8 pipe : 4;

    u8 autosw;

    enum pipe_type type;
    enum pipe_token token;

    enum pipe_banks banks;
    enum pipe_size size;
};

/*
 * USB request block. The entire data stream from the upper layers to the USB 
 * ohost controller happends trough URBs. A URB will contain all nessecary dat
 * to perform any USB transfer and deliver the status back.
 */
struct urb {
    char name[16];

    /* Transfer flags */
    u32 flags;

    /* Buffer for setup data */
    u8* setup_buffer;

    /* Buffer for IN and OUT requests */
    u8* transfer_buffer;
    u32 buffer_lenght;
    u32 buffer_count; /* Maybe add the current recieved size */

    /* For error and status reporting */
    enum urb_status status;

    /* 
     * URB complete callback. The context field allows the user to add in a 
     * context for the callback. This has nothing to do with the callback, but
     * can be read directly from the struct
     */
    void (*callback)(struct urb*);
    void* context;

    /* URB queue node */
    struct list_node node;
};

/*
 * The USB driver will include one pipe descriptor per pipe. This will hold all
 * relevant information about that pipe. Most important is the configuration and
 * the transfer state. 
 */
struct usb_pipe {
    u8* dpram;
    u32 number;
    struct list_node urb_list;

    /* The pipe state applies to any transaction */
    struct spinlock lock;
    enum pipe_state state;

    /* Hold the current pipe configuration */
    struct pipe_cfg cfg;
};

/*
 * Main USB host controller structure. This will contain all the pipes, and 
 * therefore all the URB trnasations in the system, pending or not. 
 */
struct usbhc {
    struct usb_pipe* pipe_base;
    u32 pipe_count;

    /* Callback for the root-hub */
    void (*root_hub_callback)(struct usbhc*, enum root_hub_event);
    void (*sof_callback)(struct usbhc*);
};

/*
 * Bit endian load and store
 */
static inline void usb_store_be16(u16 value, u8* addr)
{
    *addr++ = (u8)(value >> 8);
    *addr = (u8)value;
}

static inline u16 usb_load_be16(u8* addr)
{
    u16 value = 0;
    value |= (*addr << 8);
    value |= *addr;
    return value;
}

/*
 * Initialization of the USB host controller. The early init function should be
 * called before any USB clocks are enabled. If not the result is unpredictable. 
 * The normal intit functions will setup the host controller 
 */
void usbhc_early_init(void);
void usbhc_init(struct usbhc* hc, struct usb_pipe* pipe, u32 pipe_count);

/* Control */
u8 usbhc_clock_usable(void);
void usbhc_send_reset(void);

void usbhc_add_root_hub_callback(struct usbhc* hc,
    void (*callback)(struct usbhc* , enum root_hub_event));

void usbhc_add_sof_callback(struct usbhc* hc, void (*callback)(struct usbhc*));

u8 usbhc_alloc_pipe(struct usb_pipe* pipe, struct pipe_cfg* cfg);
void usbhc_set_address(struct usb_pipe* pipe, u8 addr);

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

void usbhc_fill_control_urb(struct urb* urb, u8* setup, u8* transfer_buffer,
    u32 buffer_lenght, void (*callback)(struct urb*), const char* name);
    
void print_urb_list(struct usb_pipe* pipe);

#endif