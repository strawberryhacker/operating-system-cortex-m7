/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhc.h"
#include "usb_protocol.h"

/*
 * This will hold all the states the USB host can have during enumeration. Some
 * states is deviced up into substates, even though they do the same thing
 */
enum usb_enum_state {
    USB_ENUM_IDLE,
    USB_ENUM_GET_EP0_SIZE,
    USB_ENUM_GET_DEV_DESC,
    USB_ENUM_SET_ADDRESS,
    USB_ENUM_GET_DESC_LENGTH,
    USB_ENUM_GET_DESCRIPTORS
};

struct usb_endpoint {
    struct usb_ep_desc desc;
    struct usbhc* usbhc;
    struct usb_device* device; 
};

struct usb_interface {
    /* Points to the acctual interface descriptor */
    struct usb_interface_desc desc;

    /* Contains a list of all endpoint descriptors */
    struct usb_endpoint* endpoints;
    u32 num_endpoints;
};

struct usb_configuration {
    /* Device configuration descriptor */
    struct usb_config_desc desc;

    /* Pointer to an array of all the interfaces on the given configuration */
    struct usb_host_interface* interfaces;
    u32 num_interfaces;

    /* Only one interface can be active at a time */
    struct usb_host_interface* curr_interface;
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
    struct usb_configuration* configurations;
    u32 num_configurations;

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

    u8 (*connect)(struct usb_interface* iface);
    u8 (*disconnect)(struct usb_interface* iface);
    u8 (*suspend)(struct usb_interface* iface);
    u8 (*resume)(struct usb_interface* iface);
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
};

void usb_init(struct usb_core* usbc, struct usbhc* usbhc);

#endif
