/* Copyright (C) StrawberryHacker */

#include "usb.h"
#include "print.h"
#include "list.h"
#include "usb_protocol.h"
#include "memory.h"
#include "bmalloc.h"
#include "thread.h"
#include "usb_debug.h"

static void usb_enumerate(struct urb* urb);
static void usb_add_enumerating_device(struct usb_core* core);
static void usb_add_configurations(struct usb_core* core, u32 count);

static u8 usb_get_free_address(struct usb_core* core);

/* Enumeration stages */
static void usb_get_dev_desc(struct urb* urb, struct usb_core* core, u8 full);
static void usb_set_dev_addr(struct urb* urb, struct usb_core* core);
static void usb_get_config_desc(struct urb* urb, struct usb_core* core);
static void usb_get_all_descriptors(struct urb* urb, struct usb_core* core);

/* Enumeration stages complete */
static void usb_device_desc_done(struct urb* urb, struct usb_device* device);
static void usb_ep0_size_done(struct urb* urb, struct usb_device* device);
static void usb_address_done(struct urb* urb, struct usb_device* device);
static void usb_desc_length_done(struct urb* urb, struct usb_device* device);
static void usb_get_all_desc_done(struct urb* urb, struct usb_device* device);

static struct usb_setup_desc setup;
static u8 transfer_buffer[512];

struct usb_core* private_core;

/*
 * This returns a new free address, and marks that address as used
 */
static u8 usb_get_free_address(struct usb_core* core)
{
    u16 bitmask = core->device_addr_bm;
    for (u8 i = 1; i < MAX_PIPES; i++) {
        if ((bitmask & (1 << i)) == 0) {
            return i;
        }
    }
    return 0;
}

static void usb_get_dev_desc(struct urb* urb, struct usb_core* core, u8 full)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.wIndex           = 0;
    setup.bDescriptorIndex = 0;
    setup.bDescriptorType  = USB_DESC_DEVICE;
    if (full) {
        setup.wLength = 18;
    } else {
        setup.wLength = 8;
    }
    usb_debug_print_setup(&setup);
    usbhc_fill_control_urb(urb, (u8 *)&setup, transfer_buffer, 512, 
        &usb_enumerate, setup.wLength, "Hello");

    usbhc_submit_urb(urb, core->pipe0);
}

static void usb_set_dev_addr(struct urb* urb, struct usb_core* core)
{
    setup.bmRequestType = USB_HOST_TO_DEVICE;
    setup.bRequest      = USB_REQ_SET_ADDRESS;
    setup.wIndex        = 0;
    setup.wLength       = 0;
    setup.wValue        = usb_get_free_address(core);

    print("NEW ADDRESS: %d\n", setup.wValue);

    usb_debug_print_setup(&setup);
    usbhc_fill_control_urb(urb, (u8 *)&setup, transfer_buffer, 512, 
        &usb_enumerate, 0, "Coco");
    
    usbhc_submit_urb(urb, core->pipe0);
}

static void usb_get_config_desc(struct urb* urb, struct usb_core* core)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_CONFIGURATION;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;
    setup.wLength          = 9;

    usb_debug_print_setup(&setup);
    usbhc_fill_control_urb(urb, (u8 *)&setup, transfer_buffer, 512, 
        &usb_enumerate, 9, "Coco");
    
    usbhc_submit_urb(urb, core->pipe0);
}

static void usb_get_all_descriptors(struct urb* urb, struct usb_core* core)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_CONFIGURATION;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;
    setup.wLength          = core->enum_device->desc_total_size;

    usb_debug_print_setup(&setup);
    usbhc_fill_control_urb(urb, (u8 *)&setup, transfer_buffer, 512, 
        &usb_enumerate, setup.wLength, "Peach");
    
    usbhc_submit_urb(urb, core->pipe0);
}

/*
 * This funtions will be called with a URB containing 18 bytes of data. 
 */
static void usb_device_desc_done(struct urb* urb, struct usb_device* device)
{
    struct usb_setup_desc* desc = (struct usb_setup_desc *)urb->setup_buffer;
    u32 size = desc->wLength;
    print("Size: %d\n", size);

    u8* src = urb->transfer_buffer;
    u8* dest = (u8 *)&device->desc;

    memory_copy(src, dest, size);

    struct usb_core* core = (struct usb_core *)urb->context;
    print("Vendor ID: %2h\n", core->enum_device->desc.idVendor);
    print("Configuration: %d\n", core->enum_device->desc.bNumConfigurations);

    core->enum_state = USB_ENUM_SET_ADDRESS;
    usb_set_dev_addr(urb, core);
}

static void usb_ep0_size_done(struct urb* urb, struct usb_device* device)
{
    /* Update the endpoint zero size */
    struct usb_dev_desc* dev_desc = (struct usb_dev_desc *)urb->transfer_buffer;
    device->ep0_size = dev_desc->bMaxPacketSize;

    print("Max packet => %d\n", device->ep0_size);

    /* Send the full device descriptor request */
    struct usb_core* core = (struct usb_core *)urb->context;
    core->enum_state = USB_ENUM_GET_DEV_DESC;
    usb_get_dev_desc(urb, core, 1);
}

static void usb_address_done(struct urb* urb, struct usb_device* device)
{
    struct usb_setup_desc* setup = (struct usb_setup_desc *)urb->setup_buffer;
    device->address = setup->wValue;
    print("DEVICE ADDRESS => %d\n", device->address);

    /* Update the pipe address */
    struct usb_core* core = (struct usb_core *)urb->context;
    usbhc_set_address(core->pipe0, device->address);

    /* Get the entire device descriptor */
    core->enum_state = USB_ENUM_GET_DESC_LENGTH;
    usb_get_config_desc(urb, core);
}

static void usb_desc_length_done(struct urb* urb, struct usb_device* device)
{
    struct usb_config_desc* cfg_desc = (struct usb_config_desc *)urb->transfer_buffer;
    if (urb->receive_length != 9) {
        panic("Error");
    }
    device->desc_total_size = cfg_desc->wTotalLength;
    print("Total length => %d\n", device->desc_total_size);

    /* Get every descriptor */
    struct usb_core* core = (struct usb_core *)urb->context;
    core->enum_state = USB_ENUM_GET_DESCRIPTORS;
    usb_get_all_descriptors(urb, core);
}

static void usb_get_all_desc_done(struct urb* urb, struct usb_device* device)
{
    u8* desc_ptr = urb->transfer_buffer;
    struct usb_config_desc* desc = (struct usb_config_desc *)desc_ptr;
    desc_ptr += sizeof(struct usb_config_desc);

    print("Number of interfaces => %d\n", desc->bNumInterfaces);
    struct usb_interface_desc* if_desc = (struct usb_interface_desc *)desc_ptr;
    desc_ptr += sizeof(struct usb_interface_desc);

    print("Number of endpoints => %d\n", if_desc->bNumEndpoints);
    print("Interface class => %1h\n", if_desc->bInterfaceClass);

    struct usb_ep_desc* ep1 = (struct usb_ep_desc *)desc_ptr;
    desc_ptr += sizeof(struct usb_ep_desc);
    struct usb_ep_desc* ep2 = (struct usb_ep_desc *)desc_ptr;
    desc_ptr += sizeof(struct usb_ep_desc);

    usb_debug_print_ep_desc(ep1);
    usb_debug_print_ep_desc(ep2);
}

/*
 * This will perform the enumeration of a newly attatched device. Everything
 * will happend asynchrounsly since this is a URB callback. The entire
 * enumeration will only use one URB and add a new device to the USB core. 
 */
static void usb_enumerate(struct urb* urb)
{
    struct usb_core* core = (struct usb_core *)urb->context;
    print("Enumerate handler\n");

    switch(core->enum_state) {
        case USB_ENUM_IDLE : {
            break;
        }
        case USB_ENUM_GET_EP0_SIZE : {
            printl("EP0 size done");
            usb_ep0_size_done(urb, core->enum_device);
            break;
        }
        case USB_ENUM_GET_DEV_DESC : {
            printl("Device descriptor done");
            usb_device_desc_done(urb, core->enum_device);
            break;
        }
        case USB_ENUM_SET_ADDRESS : {
            printl("Address setup done");
            usb_address_done(urb, core->enum_device);
            break;
        }
        case USB_ENUM_GET_DESC_LENGTH : {
            print("Configuration decriptor done");
            usb_desc_length_done(urb, core->enum_device);
            break;
        }
        case USB_ENUM_GET_DESCRIPTORS : {
            print("All the descriptors are read\n");
            usb_get_all_desc_done(urb, core->enum_device);
            break;
        }
    }
}

static void usb_add_enumerating_device(struct usb_core* core)
{
    /* Allocate a new device */
    struct usb_device* device = (struct usb_device *)
        bmalloc(sizeof(struct usb_device), BMALLOC_SRAM);

    /* Insert it into the list of devices */
    list_add_first(&device->node, &private_core->device_list);
    core->enum_device = device;

    /* name should be cpu */
    string_copy("Lime", device->name);
}

static void usb_add_configurations(struct usb_core* core, u32 count)
{
    /* Allocate all the configuration structures */
    core->enum_device->configurations = (struct usb_configuration *)
        bmalloc(count * sizeof(struct usb_configuration), BMALLOC_SRAM);

    core->enum_device->num_configurations = count;
}

void root_hub_event(struct usbhc* hc, enum root_hub_event event)
{
    if (event == RH_EVENT_CONNECTION) {
        printl("USB core - connection");
        usbhc_send_reset();

    } else if (event == RH_EVENT_DISCONNECTION) {
        printl("USB core - disconnection");

    } else if (event == RH_EVENT_RESET_SENT) {
        printl("USB core - reset sent");

        /* Set configuration for the control pipe */
        struct pipe_cfg cfg = {
            .banks = PIPE_BANKS_1,
            .autosw = 0,
            .device = 0,
            .frequency = 0,
            .pipe = 0,
            .size = PIPE_SIZE_64,
            .token = PIPE_TOKEN_SETUP,
            .type = PIPE_TYPE_CTRL
        };
        print("pipe number => %d\n", hc->pipe_base[0].number);
        usbhc_alloc_pipe(&hc->pipe_base[0], &cfg);
        usbhc_set_address(&hc->pipe_base[0], 0);

        usb_add_enumerating_device(private_core);
        print("Reach\n");
        struct list_node* node;
        list_iterate(node, &private_core->device_list) {
            struct usb_device* dev = list_get_entry(node, struct usb_device, node);
            print("Dev => %s\n", dev->name);
        }
        struct urb* urb = usbhc_alloc_urb();
        urb->context = private_core;
        private_core->enum_state = USB_ENUM_GET_EP0_SIZE;
        usb_get_dev_desc(urb, private_core, 0);
    }
}

void usb_init(struct usb_core* core, struct usbhc* hc)
{
    private_core = core;
    private_core->enum_state = USB_ENUM_IDLE;

    core->hc = hc;
    core->pipe0 = &hc->pipe_base[0];

    /* Initialize the device list */
    list_init(&core->device_list);
    core->device_addr_bm = 1;

    usbhc_add_root_hub_callback(hc, &root_hub_event);
}
