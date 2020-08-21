/* Copyright (C) StrawberryHacker */

#ifndef USB_H
#define USB_H

#include "types.h"
#include "usbhc.h"
#include "usb_protocol.h"
#include "config.h"

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
    USB_ENUM_GET_DESCRIPTORS,
    USB_ENUM_GET_PRODUCT_NAME,
    USB_ENUM_GET_MANUFACTURER_NAME
};

/*
 * Contains the enpo
 */
struct usb_ep {
    struct usb_ep_desc desc;

    /* Corresponding physical pipe */
    struct usb_pipe* pipe;

    struct usbhc* usbhc;
    struct usb_dev* device;
};


struct usb_driver;

/*
 * Holds all information about a interface. The pointer to the device and to
 * the driver is necessary in the process of assigning a driver. Drivers will
 * be assigned and connected to this structure. In case of composite devices
 * the driver will connect to the fist interface it supports and then claim 
 * other interfaces it feel it need
 */
struct usb_iface {
    struct usb_iface_desc desc;

    /* Contains a list of all endpoint descriptors */
    struct usb_ep* eps;
    u32 num_eps;

    /* Pointer to the assigned driver */
    struct usb_driver* driver;
    struct usb_dev* parent_dev;
    
    u8 assigned : 1;

    /* Pipes controlled by this interface driver */
    struct usb_pipe* pipes[MAX_PIPES];
    u32 pipe_bm;

    /* Link the interfaces together */
    struct list_node node;
};

/*
 * Holds the configuration descriptor together with a list of interfaces 
 * corresponding with that configuration
 */
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
struct usb_dev {
    struct usb_dev_desc desc;

    /* Hold the total size of all descriptors */
    u32 desc_total_size;

    /* Contains all subconfigurations */
    struct usb_config* configs;
    u32 num_configs;

    /* Points to an IAD is available */
    struct usb_iad_desc* iad;


    struct usb_config* curr_config;

    struct list_node node;

    /* Contains a list with all the interfaces */
    struct list_node iface_list;

    u8 address;
    u16 ep0_size;

    char product[USB_DEV_NAME_MAX_SIZE];
    char manufacturer[USB_DEV_NAME_MAX_SIZE];
};

/* Definitions of the different check flags */
#define USB_DEV_ID_VENDOR_MASK         (1 << 0)
#define USB_DEV_ID_PRODUCT_MASK        (1 << 1)
#define USB_DEV_ID_DEV_CLASS_MASK      (1 << 2)
#define USB_DEV_ID_DEV_SUBCLASS_MASK   (1 << 3)
#define USB_DEV_ID_DEV_PROTOCOL_MASK   (1 << 4)
#define USB_DEV_ID_IFACE_CLASS_MASK    (1 << 5)
#define USB_DEV_ID_IFACE_SUBCLASS_MASK (1 << 6)
#define USB_DEV_ID_IFACE_PROTOCOL_MASK (1 << 7)

/* Device ID structure used for matching drivers */
struct usb_dev_id {
    u32 flags;

    /* Matching device */
    u16 vendor_id;
    u16 product_id;
    u8 dev_class;
    u8 dev_sub_class;
    u8 dev_protocol;

    /* Matching interface */
    u8 iface_class;
    u8 iface_sub_class;
    u8 iface_protocol;
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
    u8 (*connect)(struct usb_iface* iface);
    u8 (*disconnect)(struct usb_iface* iface);
    u8 (*suspend)(struct usb_iface* iface);
    u8 (*resume)(struct usb_iface* iface);

    struct list_node node;

    /* Pointer to a list with devices IDs used for matching driver */
    const struct usb_dev_id* dev_ids;
    const u32 num_dev_ids;
};

/*
 * Definition of the core structure which will hold all necessary information
 * about driver, enumeration, the host controller and the hardware. 
 */
struct usb_core {
    /* Enumeration state */
    enum usb_enum_state enum_state;
    
    /* This will allways point to the device currently being enumerated */
    struct usb_dev* enum_dev;

    /* Pointer to the USB host controller */
    struct usbhc* usbhc;

    /* Pointer to the control pipe */
    struct usb_pipe* pipe0;

    /* Contains a list of all the devices */
    struct list_node dev_list;
    u16 dev_addr_bm;

    /* Contains a list of all possible drivers */
    struct list_node driver_list;
};

void usbc_init(struct usb_core* usbc, struct usbhc* usbhc);

void usbc_add_driver(struct usb_driver* driver, struct usb_core* usbc);

u8 usbc_iface_add_pipe(struct usb_pipe* pipe, struct usb_iface* iface);

u8 usbc_iface_remove_pipe(struct usb_pipe* pipe, struct usb_iface* iface);

#endif
