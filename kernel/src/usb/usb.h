/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhc.h"
#include "usb_protocol.h"

enum usb_enum_state {
    USB_ENUM_IDLE,
    USB_ENUM_GET_DEV_DESC,
    USB_ENUM_SET_ADDRESS,
    USB_ENUM_GET_CONFIG_DESC
};

/*
 * This 
 */
struct usb_endpoint {
    struct usb_ep_desc desc;
    struct usbhc* hc;
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

struct usb_device {
    struct usb_configuration* configurations;
    u32 num_configurations;

    u8 address;
};

/*
 * A USB driver is linked to an interface, NOT a device. This will contain
 * fields which all drivers must implement. Note that only callbacks for root
 * hub and bus changes are defined. The drivers has to monitor all the transfers
 * using dynamic URB callbacks
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

    struct usb_device* device;
};

void usb_init(struct usb_core* core, struct usbhc* hc);

#endif
