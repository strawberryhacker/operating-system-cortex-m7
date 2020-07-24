/* Copyright (C) StrawberryHacker */

#ifndef usbhc_H
#define usbhc_H

#include "types.h"
#include "dlist.h"

/* Defines the maximum number of pipes supported by hardware */
#define MAX_PIPES 10

#define usbhc_DPRAM_ADDR 0xA0100000
#define usbhc_DPRAM_EP_SIZE 0x8000

/*
 * Returns an 8 bit pointer to the DPRAM base address
 * associated with a pipe
 */
#define usbhc_get_fifo_ptr(pipe) \
	(volatile u8 *)(usbhc_DPRAM_ADDR + (usbhc_DPRAM_EP_SIZE * pipe))

/*
 * Defines the USB mode of operation 
 */
enum usb_mode {
    USB_HOST,
    USB_DEVICE
};

/*
 * This enum indicates the deivce speed status in host mode
 */
enum usb_device_speed {
    USB_DEVICE_FS,
    USB_DEVICE_HS,
    USB_DEVICE_LS
};

/*
 * This enum specifies the host speed capability
 */
enum usb_host_speed {
    USB_HOST_SPEED_NORMAL,
    USB_HOST_SPEED_LS,
    USB_HOST_SPEED_HS,
    USB_HOST_SPEED_FS
};

/*
 * Defines the tokens available for a pipe
 */
enum pipe_token {
    PIPE_TOKEN_SETUP,
    PIPE_TOKEN_IN,
    PIPE_TOKEN_OUT
};

/*
 * Root hub event. This enum describes a root hub event
 */
enum root_hub_event {
    RH_EVENT_CONNECT,
    RH_EVENT_DISCONNECT,
    RH_EVENT_RESET,
    RH_EVENT_WAKEUP,
    RH_EVENT_DOWNSTREAM_RESUME,
    RH_EVENT_UPSTREAM_RESUME
};

/*
 * Holds the pipe status
 */
enum pipe_status {
    PIPE_STATUS_FREE,
    PIPE_STATUS_IDLE
};

/*
 * Transfer status
 */
enum usb_x_status {
    USB_X_STATUS_OK,
    USB_X_STATUS_STOP,
    USB_X_STATUS_STALL,
    USB_X_STATUS_RESET,
    USB_X_STATUS_SETUP,
    USB_X_STATUS_CONTROL_IN,
    USB_X_STATUS_CONTROL_OUT,
    USB_X_STATUS_ZLP_IN,
    USB_X_STATUS_ZLP_OUT,
    USB_X_STATUS_DATA_IN,
    USB_X_STATUS_DATA_OUT
};

/*
 * Describes all fields needed for a control transfer. A control
 * transfer consist of three stages; a setup packet, an IN data 
 * packet, and a ZLP OUT acknowledgment packet. Each of these
 * consist of a token, data and a handshake. 
 */
struct usb_ctrl_transfer {
    /* Control transfer setup packet is allways 8 bytes long */
    u8* setup;

    /* Contains a pointer to the receive buffer */
    u8* data;
    u32 receive_size;

    /* Holds the timeout between packages */
    i32 timeout;


};

/*
 * Structure that hold pipe configuration information
 */
struct usb_pipe {
    void (*callback)(struct usb_pipe*);

    /*
     * This field has two meanings depending upon the use. If
     * a high-speed bulk OUT pipe is used this contains the 
     * bit-interval between PING / OUT request, dependning if
     * the PINGEN is set. This is per microframe. If an
     * interrupt pipe is configured this holds the number of
     * milliseconds between interrupt requests.
     */
    u8 interval;

    /* Endpoint targeted by the pipe */
    u8 endpoint;

    /* Device address targeted by the pipe */
    u8 addr;

    u8 type             : 2;
    u8 token            : 2;
    u8 auto_bank_switch : 1;
    u8 size             : 3;
    u8 bank_count       : 2;

    /* Only for high-speed bulk OUT pipes USB 2.0 */
    u8 ping_enable;

    /* General transfer state */
    enum pipe_status status;
    enum usb_x_status x_status;

    union {
        struct usb_ctrl_transfer control;
    } x;
};


/*
 * This structure hold private data that the USB hardware layer will 
 * use to maintain pipes and schedule events. This is also referenced
 * in teh usb_core structure. 
 */
struct usb_hardware {
    /* Continous list of pipe descriptors */
    struct usb_pipe* pipes;
    u8 pipe_count;

    u8 active_pipes;
    u32 dpram_used;
};

/*
 * Core USB structure
 */
struct usb_core {
    struct usb_hardware* hw;

    /* Callbacks for SOF */
    void (*sof_callback)(struct usb_core*);

    /* Callback for root hub change taking in the event */
    void (*rh_callback)(struct usb_core*, enum root_hub_event);
};

/*
 * Initializes the USB interface. This takes in the main USB core 
 * structure, a list of pipes and the pipe count
 */
void usbhc_init(struct usb_core* core, struct usb_hardware* hw,
                struct usb_pipe* pipes, u8 pipe_count);


/*
 * Freezes the USB clock. Only asynchronous interrupt can trigger 
 * and interrupt. The CPU can only read/write FRZCLK and USBE when
 * this but is set
 */
void usbhc_freeze_clock(void);

/*
 * Unfreezes the USB clock
 */
void usbhc_unfreeze_clock(void);

/*
 * Enable the USB interface
 */
void usbhc_enable(void);

/*
 * Disables the USB interface. This act as a hardware reset, thus 
 * resetting USB interface, disables the USB tranceiver and disables
 * the USB clock inputs. This does not reset FRZCLK and UIMOD
 */
void usbhc_disable(void);

/*
 * Sets the USB operating mode; host or device
 */
void usbhc_set_mode(enum usb_mode mode);

/*
 * Checks if the USB UTMI 30MHz clock is usable. Returns 1 if
 * the clock is usable, 0 if not
 */
u8 usbhc_clock_usable(void);

/*
 * Clears and disables all global interrupts
 */
void usbhc_interrupt_disable(void);

/*
 * Sends a USB reset. It might be useful to write this bit to 
 * zero when a device disconnection is detected.
 */
void usbhc_send_reset(void);

/*
 * Performs a soft reset on all pipes. This means configuring the
 * pipes, allocating them and updating the status
 */
void usbhc_pipe_soft_reset(struct usb_core* core);

/*
 * Performs a hard reset on all pipes. This means deallocating them
 * and updating the status
 */
void usbhc_pipe_hard_reset(struct usb_core* core);

/*
 * Adds a pipe callback
 */
void usbhc_add_pipe_callback(struct usb_pipe* pipe,
                             void (*cb)(struct usb_pipe *));

/*
 * Starts a control transfer
 */
void usbhc_control_transfer(struct usb_core* core, struct usb_pipe* pipe,
                            u8* data, u8* setup, u8 req_size);

/*
 * Tries to allocate a pipe. This will handle any DPRAM
 * conflicts and reallocate conflicting pipes. Returns the allocated
 * pipe if success, else NULL
 */
struct usb_pipe* usbhc_pipe_allocate(struct usb_core* core, u32 cfg, u8 addr, u8 pipe0);

#endif