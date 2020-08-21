/* Copyright (C) StrawberryHacker */

#include "usb_debug.h"
#include "print.h"

/* String representation of the different recipient fields */
static const char* usb_debug_direction[] = {
    [0x00] = "host -> device",
    [0x01] = "device -> host"
};

/* String representation of the different type fields */
#define USB_DEBUG_TYPE_MAX 3
static const char* usb_debug_type[] = {
    [0x00] = "standard",
    [0x01] = "class",
    [0x02] = "vendor",
    [0x03] = "reserved"
};

/* String representation of the different recipient fields */
#define USB_DEBUG_RECIPIENT_MAX 3
static const char* usb_debug_recipient[] = {
    [0x00] = "device",
    [0x01] = "interface",
    [0x02] = "endpoint",
    [0x03] = "other"
};

/* String representation of the standard device requests */
#define USB_DEBUG_DEVICE_REQUEST_MAX 9
static const char* usb_debug_device_requests[] = {
    [0x00] = "GET_STATUS",
    [0x01] = "CLEAR_FEATURE",
    [0x02] = "RESERVED",
    [0x03] = "SET_FEATURE",
    [0x04] = "RESERVED",
    [0x05] = "SET_ADDRESS",
    [0x06] = "GET_DESCRIPTOR",
    [0x07] = "SET_DESCRIPTOR",
    [0x08] = "GET_CONFIGURATION",
    [0x09] = "SET_CONFIGURATION"
};

/* String representation of the different descriptor types */
#define USB_DEBUG_DESC_TYPE_MAX 6
static const char* usb_debug_desc_type[] = {
    [0x00] = "RESERVED",
    [0x01] = "DEVICE",
    [0x02] = "CONFIGURATION",
    [0x03] = "STRING",
    [0x04] = "INTERFACE",
    [0x05] = "ENDPOINT",
    [0x06] = "DEVICE_QUALIFIER"
};

void usb_debug_print_setup(struct usb_setup_desc* desc)
{
    /* Data direction */
    u8 field = (desc->bmRequestType >> 7) & 0b1;
    print("Setup packet => Direction:  ");
    print("%s\n", usb_debug_direction[field]);
    
    /* Setup type field */
    field = (desc->bmRequestType >> 5) & 0b11;
    print("\t\tType:       ");
    if (field <= USB_DEBUG_TYPE_MAX) {
        print("%s\n", usb_debug_type[field]);
    }

    /* Recepient */
    field = (desc->bmRequestType & 0b11111);
    print("\t\tRecipient:  ");
    if (field <= USB_DEBUG_RECIPIENT_MAX) {
        print("%s\n", usb_debug_recipient[field]);
    }

    /* Print the request type */
    u8 bRequest = desc->bRequest;
    print("\t\tRequest:    ");
    if (bRequest <= USB_DEBUG_DEVICE_REQUEST_MAX) {
        print("%s\n", usb_debug_device_requests[bRequest]);
    }

    /* We have two different cases related to the wValue field */
    if ((bRequest == 6) || (bRequest == 7)) {
        print("\t\tDesc type:  ");
        u8 type = (desc->wValue >> 8) & 0xFF;
        if (type <= USB_DEBUG_DESC_TYPE_MAX) {
            print("%s\n", usb_debug_desc_type[type]);
        } else {
            print("%d\n", type);
        }
        print("\t\tDesc index: %d\n", desc->wValue & 0xFF);
    } else {
        print("\t\twValue:     %d\n", desc->wValue);
    }
    /* Print the two last fields */
    print("\t\twIndex:     %d\n", desc->wIndex);
    print("\t\twLength:    %d\n", desc->wLength);

    print("\n");
}

void usb_print_ep_desc(struct usb_ep_desc* desc)
{
    print("Endpoint descriptor => ");
    print("\taddress:      %8b\n", desc->bEndpointAddress);
    print("\t\t\tattributes:   %8b\n", desc->bmAttributes);
    print("\t\t\tmax pkt size: %d\n", desc->wMaxPacketSize);
    print("\t\t\tinterval:     %d\n", desc->bInterval);
}

void usb_print_iface_desc(struct usb_iface_desc* desc)
{
    print("Interface descriptor => ");
    print("iface number: %8b\n", desc->bInterfaceNumber);
    print("\t\t\talternate:    %8b\n", desc->bAlternateSetting);
    print("\t\t\tnum eps:      %d\n", desc->bNumEndpoints);
    print("\t\t\tiface class:  %1h\n", desc->bInterfaceClass);
    print("\t\t\tsubclass:     %d\n", desc->bInterfaceSubClass);
    print("\t\t\tprotocol:     %1h\n", desc->bInterfaceProtocol);
    print("\t\t\tinnterface:   %d\n", desc->iInterface);
}

void usb_print_config_desc(struct usb_config_desc* desc)
{
    print("Config descriptor => ");
    print("\tnum ifaces:  %8b\n", desc->bNumInterfaces);
    print("\t\t\tcfg value:   %8b\n", desc->bConfigurationValue);
    print("\t\t\tcfg:         %d\n", desc->iConfiguration);
    print("\t\t\tattrib:      %d\n", desc->bmAttributes);
    print("\t\t\tmax power:   %d\n", desc->bMaxPower);
}

void usb_print_dev_desc(struct usb_dev_desc* desc)
{
    printl("Device descriptor");
    printl("-------------------------------");
    printl("iManufacturer: %d", desc->iManufacturer);
    printl("iProduct: %d", desc->iProduct);
    printl("iSerialNumber: %d", desc->iSerialNumber);
    printl("bDeviceClass: %d", desc->bDeviceClass);
    printl("-------------------------------");
}
