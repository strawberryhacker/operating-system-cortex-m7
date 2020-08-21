/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhc.h"
#include "usb_protocol.h"

/*
 * This will hold all the states the USB host can have during enumeration. Some
 * states is deviced up into substates, even though they essentially do the same
 * thing
 */
enum usb_enum_state {
    USB_ENUM_IDLE,
    USB_ENUM_GET_EP0_SIZE,
    USB_ENUM_GET_DEV_DESC,
    USB_ENUM_SET_ADDRESS,
    USB_ENUM_GET_DESC_LENGTH,
    USB_ENUM_GET_DESCRIPTORS
};

/*
 * Contains the enpo
 */
struct usb_ep {
    struct usb_ep_desc desc;

    /* Corresponding physical pipe */
    struct usb_pipe* pipe;

    struct usbhc* usbhc;
    struct usb_device* device;
};


struct usb_driver;

struct usb_iface {
    struct usb_iface_desc desc;

    /* Contains a list of all endpoint descriptors */
    struct usb_ep* eps;
    u32 num_eps;

    /* Pointer to the assigned driver */
    struct usb_driver* driver;
};

struct usb_config {
    struct usb_config_desc desc;

    /* Pointer to an array of all the interfaces on the given configuration */
    struct usb_iface* ifaces;
    u32 num_ifaces;

    /* Only one interface can be active at a time */
    struct usb_iface* curr_iface;
};

/*
 * This struct will contain all necessary imformation about a device. The number
 * of devices is not known, hence a linked list is needed. 
 */
struct usb_device {
    char name[8];
    struct usb_dev_desc desc;

    /* Hold the total size of all descriptors */
    u32 desc_total_size;

    /* Contains all subconfigurations */
    struct usb_config* configs;
    u32 num_configs;

    struct usb_config* curr_config;

    struct list_node node;

    u8 address;
    u16 ep0_size;
};

/*
 * A USB driver is linked to an interface, NOT a device. This will contain
 * fields which all drivers must implement. Note that only callbacks for root
 * hub changes and bus changes are defined. The drivers has to monitor the
 * transfers separately using dynamic URB callbacks
 */
struct usb_driver {
    const char* name;

    /* Driver callbacks */
    u8 (*probe)(struct usb_iface* iface);
    u8 (*start)(struct usb_iface* iface);
    u8 (*connect)(struct usb_iface* iface);
    u8 (*disconnect)(struct usb_iface* iface);
    u8 (*suspend)(struct usb_iface* iface);
    u8 (*resume)(struct usb_iface* iface);

    struct list_node node;
};

struct usb_core {
    /* Enumeration state */
    enum usb_enum_state enum_state;
    
    /* This will allways point to the device currently being enumerated */
    struct usb_device* enum_device;

    /* Pointer to the USB host controller */
    struct usbhc* usbhc;

    /* Pointer to the control pipe */
    struct usb_pipe* pipe0;

    /* Contains a list of all the devices */
    struct list_node device_list;
    u16 device_addr_bm;

    /* Contains a list of all possible drivers */
    struct list_node driver_list;
};

void usb_init(struct usb_core* usbc, struct usbhc* usbhc);

void usb_add_driver(struct usb_driver* driver, struct usb_core* usbc);

#endif
