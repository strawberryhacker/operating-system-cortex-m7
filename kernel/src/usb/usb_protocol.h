/* Copyright (C) StrawberryHacker */

#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

#include "types.h"

/*
 * This file defines the hardware driver layer for the USB host stack. Note that
 * some coding conventions will not be followed in the USB stack due the USB
 * protocol. This way searching on the internet will become easier.
 * 
 * Please refer to the USB 2.0 spesification while reading this;
 * http://sdphca.ucsd.edu/lab_equip_manuals/usb_20.pdf
 * references in this USB stack will be related to this document.
 */

/*
 * Defines the bitmask fields in the SETUP packet request type
 */
#define USB_REQ_TYPE_HOST_TO_DEVICE 0x00
#define USB_REQ_TYPE_DEVICE_TO_HOST 0x80

#define USB_REQ_TYPE_STANDARD       0x00
#define USB_REQ_TYPE_CLASS          0x20
#define USB_REQ_TYPE_VENDOR         0x40

#define USB_REQ_TPYE_DEVICE         0x00
#define USB_REQ_TPYE_INTERFACE      0x01
#define USB_REQ_TPYE_ENDPOINT       0x02
#define USB_REQ_TPYE_OTHER          0x03

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
 * Structure describing the SETUP packet sent first in ALL control
 * transfers. This takes up eigth bytes.
 */
struct usb_setup {
    u8 bmRequestType;
    u8 bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
};

/*
 * Device descriptor. USB 2.0 specification page 262 
 */
struct usb_dev_desc {
    u8  bLength;
    u8  bDescriptorType;
    u16 bcdUSB;
    u8  bDeviceClass;
    u8  bDeviceSubClass;
    u8  bDeviceProtocol;
    u8  bMaxPacketSize;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8  iManufacturer;
    u8  iProduct;
    u8  iSerialNumber;
    u8  bNumConfigurations;    
};

/*
 * Device qualifier descriptor. USB 2.0 specification page 264
 */
struct usb_qualifier_desc {
    u8  bLength;
    u8  bDescriptorType;
    u16 bcdUSB;
    u8  bDeviceClass;
    u8  bDeviceSubClass;
    u8  bDeviceProtocol;
    u8  bMaxPacketSize;
    u8  bNumConfigurations;
    u8  bReserved;
};

/*
 * Configuration descriptor. USB 2.0 specification page 265
 */
struct usb_config_desc {
    u8  bLength;
    u8  bDescriptorType;
    u16 wTotalLength;
    u8  bNumInterfaces;
    u8  bConfigurationValue;
    u8  iConfiguration;
    u8  bmAttributes;
    u8  bMaxPower;
};

/*
 * Interface descriptor. USB 2.0 specification page 268
 */
struct usb_interface_desc {
    u8  bLength;
    u8  bDescriptorType;
    u8  bInterfaceNumber;
    u8  bAlternateSetting;
    u8  bNumEndpoints;
    u8  bInterfaceClass;
    u8  bInterfaceSubClass;
    u8  bInterfaceProtocol;
    u8  iInterface;
};

/*
 * Endpoint descriptor. USB 2.0 specification page 269
 */
struct usb_ep_desc {
    u8  bLength;
    u8  bDescriptorType;
    u8  bEndpointAddress;
    u8  bmAttributes;
    u16 wMaxPacketSize;
    u8  bInterval;
};

/*
 * String descriptor header. USB 2.0 specification page 273.
 * This will only contain the first two fields of either the 
 * string descriptor zero or the normal string descriptor. This
 * is because the size is varying
 */
struct usb_string_desc {
    u8  bLength;
    u8  bDescriptorType;
};

#endif
