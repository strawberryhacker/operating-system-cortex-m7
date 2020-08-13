/* Copyright (C) StrawberryHacker */

#ifndef usbhc_H
#define usbhc_H

#include "types.h"
#include "dlist.h"

/* Defines the maximum number of pipes supported by hardware */
#define MAX_PIPES 10

#define MAX_URBS 256

#define USBHC_DPRAM_ADDR 0xA0100000
#define USBHC_DPRAM_EP_SIZE 0x8000

/*
 * Returns an 8 bit pointer to the DPRAM base address associated with a pipe
 */
#define usbhc_get_fifo_ptr(pipe) \
	(volatile u8 *)(USBHC_DPRAM_ADDR + (USBHC_DPRAM_EP_SIZE * pipe))

enum usb_mode {
    USB_HOST,
    USB_DEVICE
};

enum usb_device_speed {
    USB_DEVICE_FS,
    USB_DEVICE_HS,
    USB_DEVICE_LS
};

enum usb_host_speed {
    USB_HOST_SPEED_NORMAL,
    USB_HOST_SPEED_LS,
    USB_HOST_SPEED_HS,
    USB_HOST_SPEED_FS
};

enum pipe_token {
    PIPE_TOKEN_SETUP,
    PIPE_TOKEN_IN,
    PIPE_TOKEN_OUT
};

enum root_hub_event {
    RH_EVENT_CONNECTION,
    RH_EVENT_DISCONNECTION,
    RH_EVENT_RESET_SENT
};

enum pipe_state {
    PIPE_STATE_DISABLED,
    PIPE_STATE_IDLE,
    PIPE_STATE_SETUP,
    PIPE_STATE_IN,
    PIPE_STATE_OUT,
    PIPE_STATE_STATUS,
    PIPE_STATE_ERROR
};

enum pipe_type {
    PIPE_TYPE_CTRL,
    PIPE_TYPE_ISO,
    PIPE_TYPE_BLK,
    PIPE_TYPE_INT
};

enum pipe_banks {
    PIPE_BANKS_1,
    PIPE_BANKS_2,
    PIPE_BANKS_3
};

enum pipe_size {
    PIPE_SIZE_8,
    PIPE_SIZE_16,
    PIPE_SIZE_32,
    PIPE_SIZE_64,
    PIPE_SIZE_128,
    PIPE_SIZE_256,
    PIPE_SIZE_512,
    PIPE_SIZE_1024
};

enum urb_tx_flags {
    URB_TX_SETUP_OUT,
    URB_TX_SETUP_IN
};

/*
 * Holds all neccesary filds 
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
 * USB request block. This will be able to perform all kinds of USB transfers.
 */
struct urb {
    char name[16];
    /* Transfer flags */
    enum urb_tx_flags tx_flags;

    struct dlist_node node;

    /* Buffer for setup data */
    u8* setup_buffer;

    /* Buffer for IN and OUT requests */
    u8* data_buffer;
    u32 data_size;

    /* For error and status reporting */
    u8 status;
    u8 state;
};

/*
 * The USB driver will include one pipe descriptor per pipe
 */
struct usb_pipe {
    u8* dpram;
    u32 number;

    struct dlist urb_queue;

    /* The pipe state applies to any transaction */
    enum pipe_state state;

    /* Hold the current pipe configuration */
    struct pipe_cfg cfg;
};

/*
 * 
 */
struct usbhc {
    struct usb_pipe* pipe_base;
    u32 pipe_count;

    /* Callback for the root-hub */
    void (*root_hub_callback)(struct usbhc*, enum root_hub_event);
};

/*
 * Some of the API calles to the hardware is exposed due to the USB state 
 * requirement when enabling cloks. These will only be called by the usb_phy
 * file in the board config. After clocks are enabled everything will be
 * handled by the USBHC
 */
void usbhw_freeze_clock(void);
void usbhw_unfreeze_clock(void);
void usbhw_enable(void);
void usbhw_disable(void);
void usbhw_set_mode(enum usb_mode mode);


u8 usbhw_clock_usable(void);
void usbhw_send_reset(void);

void usbhc_init(struct usbhc* hc, struct usb_pipe* pipe, u32 pipe_count);

void usbhc_add_root_hub_callback(struct usbhc* hc,
    void (*callback)(struct usbhc* , enum root_hub_event));

u8 usbhc_pipe_allocate(struct usb_pipe* pipe, struct pipe_cfg* cfg);
void usbhc_set_address(struct usb_pipe* pipe, u8 addr);

u8 usbhc_send_setup_raw(struct usb_pipe* pipe, u8* setup);

/*
 * URB (USB request blocks) is the main communication channel between the 
 * USB host core, device drivers, and the lower level host controller. All 
 * messages; iso, bulk, interrupt and control, should be issued with URBs. 
 * This structure will contain all information to submit any transaction and 
 * retport back the status.
 */
struct urb* usbhc_urb_new(void);
u8 usbhc_urb_cancel(struct urb* urb, struct usb_pipe* pipe);
void usbhc_urb_submit(struct urb* urb, struct usb_pipe* pipe);
void print_urb_list(struct usb_pipe* pipe);

#endif