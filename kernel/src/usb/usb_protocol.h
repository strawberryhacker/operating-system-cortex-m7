#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

#include "types.h"

/*
 * Defines the fields in the SETUP packet request type
 */
#define USB_REQ_TYPE_HOST_TO_DEVICE (u8)(0 << 7)
#define USB_REQ_TYPE_DEVICE_TO_HOST (u8)(1 << 7)

#define USB_REQ_TYPE_STANDARD       (u8)(0 << 5)
#define USB_REQ_TYPE_CLASS          (u8)(1 << 5)
#define USB_REQ_TYPE_VENDOR         (u8)(2 << 5)

#define USB_REQ_TPYE_DEVICE         (u8)(0 << 0)
#define USB_REQ_TPYE_INTERFACE      (u8)(1 << 0)
#define USB_REQ_TPYE_ENDPOINT       (u8)(2 << 0)
#define USB_REQ_TPYE_OTHER          (u8)(3 << 0)

/*
 * Defines the different setup requests
 */
#define USB_REQ_CLEAR_FEATURE     1
#define USB_REQ_GET_CONFIGURATION 8
#define USB_REQ_GET_DESCRIPTOR    6
#define USB_REQ_GET_INTERFACE     10
#define USB_REQ_GET_STATUS        0
#define USB_REQ_SET_ADDRESS       5
#define USB_REQ_SET_CONFIGURATION 9
#define USB_REQ_SET_DESCRIPTOR    7
#define USB_REQ_SET_FEATURE       3
#define USB_REQ_SET_INTERFACE     11
#define USB_REQ_SYNC_FRAME        12

/*
 * Data packet for the USB control trasfer
 */
struct usb_setup {
    u8 request_type;
    u8 request;
    u16 value;
    u16 index;
    u16 length;
};

/*
 * Device descriptor
 */
struct usb_dev_desc {
    u8  length;           /* Size of descriptor in bytes */
    u8  descriptor_type;  /* Allways set to 1 */
    u16 bcd_usb;          /* USB spec relase number */
    u8  device_class;
    u8  device_subclass;
    u8  device_protocol;
    u8  max_packet_size;  /* Defines max packet size EP0 */
    u16 vendor_id;
    u16 product_id;
    u16 bcd_device;
    u8  manufacturer;
    u8  product;
    u8  serial_number;
    u8  num_configurations;    
};

#endif
