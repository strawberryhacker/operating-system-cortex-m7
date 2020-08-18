/* Copyright (C) StrawberryHacker */

#include "usb.h"
#include "print.h"
#include "list.h"
#include "usb_protocol.h"
#include "memory.h"
#include "bmalloc.h"
#include "thread.h"
#include "usb_debug.h"
#include "config.h"

/* Private valiables used for device enumeration */
static struct usb_setup_desc setup;
static u8 enum_buffer[ENUM_BUFFER_SIZE];

/* Private USB core instance */
struct usb_core* usbc_priv;

static void usb_enumerate(struct urb* urb);
static void usb_add_device(struct usb_core* usbc);
static void usb_add_configurations(struct usb_core* usbc, u32 count);

static u8 usb_new_address(struct usb_core* usbc);
static void usb_delete_address(struct usb_core* usbc, u8 address);

/* Enumeration stages */
static void usb_enum_get_dev_desc(struct urb* urb, struct usb_core* usbc, u8 full);
static void usb_enum_set_dev_addr(struct urb* urb, struct usb_core* usbc);
static void usb_enum_get_cfg_desc(struct urb* urb, struct usb_core* usbc);
static void usb_enum_get_all_desc(struct urb* urb, struct usb_core* usbc);

/* Enumeration stages complete */
static void usb_device_desc_done(struct urb* urb, struct usb_device* device);
static void usb_ep0_size_done(struct urb* urb, struct usb_device* device);
static void usb_address_done(struct urb* urb, struct usb_device* device);
static void usb_desc_length_done(struct urb* urb, struct usb_device* device);
static void usb_get_all_desc_done(struct urb* urb, struct usb_device* device);

/*
 * This returns a new free address. An address is dynamically assigned to a 
 * device during enumeration. If the SET_ADDRESS fails the address should still
 * be set. Only after a disconnection should the address be deleted!
 */
static u8 usb_new_address(struct usb_core* usbc)
{
    u16 bitmask = usbc->device_addr_bm;
    for (u8 i = 1; i < MAX_PIPES; i++) {
        if ((bitmask & (1 << i)) == 0) {
            usbc->device_addr_bm |= (1 << i);
            return i;
        }
    }
    return 0;
}

static void usb_delete_address(struct usb_core* usbc, u8 address)
{
    usbc->device_addr_bm &= ~(1 << address);
}

/*
 * This will take in a URB an perform a get device descriptor request. Full 
 * specifies if the full descriptor is fetched or only the first 8 bytes. This
 * is due to unknown size of EP0 
 */
static void usb_enum_get_dev_desc(struct urb* urb, struct usb_core* usbc, u8 full)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_DEVICE;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;

    if (full) {
        setup.wLength = 18;
    } else {
        setup.wLength = 8;
    }
    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usb_enumerate,
        "GET DEV");

    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usb_enum_set_dev_addr(struct urb* urb, struct usb_core* usbc)
{
    setup.bmRequestType = USB_HOST_TO_DEVICE;
    setup.bRequest      = USB_REQ_SET_ADDRESS;
    setup.wValue        = usb_new_address(usbc);
    setup.wIndex        = 0;
    setup.wLength       = 0;

    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usb_enumerate,
        "SET ADDR");
    
    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usb_enum_get_cfg_desc(struct urb* urb, struct usb_core* usbc)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_CONFIGURATION;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;
    setup.wLength          = 9;

    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usb_enumerate,
        "GET CFG");
    
    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usb_enum_get_all_desc(struct urb* urb, struct usb_core* usbc)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_CONFIGURATION;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;
    setup.wLength          = usbc->enum_device->desc_total_size;

    usb_debug_print_setup(&setup);
    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usb_enumerate,
        "GET ALL");
    
    usbhc_submit_urb(urb, usbc->pipe0);
}

/*
 * The URB will contain the first 8 bytes of the device decriptor. This should
 * be enough to read the default EP0 size
 */
static void usb_ep0_size_done(struct urb* urb, struct usb_device* device)
{
    /* Update the endpoint zero size */
    struct usb_dev_desc* dev_desc = (struct usb_dev_desc *)urb->transfer_buffer;
    device->ep0_size = dev_desc->bMaxPacketSize;

    print("Max packet => %d\n", device->ep0_size);

    urb->packet_size = device->ep0_size;

    /* Send the full device descriptor request */
    struct usb_core* usbc = (struct usb_core *)urb->context;
    usbc->enum_state = USB_ENUM_GET_DEV_DESC;
    usb_enum_get_dev_desc(urb, usbc, 1);
}

static void usb_device_desc_done(struct urb* urb, struct usb_device* device)
{
    struct usb_setup_desc* desc = (struct usb_setup_desc *)urb->setup_buffer;
    u32 size = desc->wLength;
    print("Size: %d\n", size);

    u8* src = urb->transfer_buffer;
    u8* dest = (u8 *)&device->desc;

    memory_copy(src, dest, size);

    struct usb_core* usbc = (struct usb_core *)urb->context;
    print("Vendor ID: %2h\n", usbc->enum_device->desc.idVendor);
    print("Configuration: %d\n", usbc->enum_device->desc.bNumConfigurations);

    usbc->enum_state = USB_ENUM_SET_ADDRESS;
    usb_enum_set_dev_addr(urb, usbc);
}

static void usb_address_done(struct urb* urb, struct usb_device* device)
{
    struct usb_setup_desc* setup = (struct usb_setup_desc *)urb->setup_buffer;
    device->address = setup->wValue;
    print("DEVICE ADDRESS => %d\n", device->address);

    /* Update the pipe address */
    struct usb_core* usbc = (struct usb_core *)urb->context;
    usbhc_set_address(usbc->pipe0, device->address);

    /* Get the entire device descriptor */
    usbc->enum_state = USB_ENUM_GET_DESC_LENGTH;
    usb_enum_get_cfg_desc(urb, usbc);
}

static void usb_desc_length_done(struct urb* urb, struct usb_device* device)
{
    struct usb_config_desc* cfg_desc = (struct usb_config_desc *)urb->transfer_buffer;
    print("Bytes recieved => %d\n", urb->acctual_length);
    if (urb->acctual_length != 9) {
        panic("Error");
    }
    device->desc_total_size = cfg_desc->wTotalLength;
    print("Total length => %d\n", device->desc_total_size);

    /* Get every descriptor */
    struct usb_core* usbc = (struct usb_core *)urb->context;
    usbc->enum_state = USB_ENUM_GET_DESCRIPTORS;
    usb_enum_get_all_desc(urb, usbc);
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
 * This will perform the enumeration of a newly attatched devices. Everything
 * will happend asynchronously since this is a URB callback. The entire
 * enumeration will only use one URB. A new device will be added in this
 * process
 */
static void usb_enumerate(struct urb* urb)
{
    struct usb_core* usbc = (struct usb_core *)urb->context;
    print("Enumerate handler => ");

    switch(usbc->enum_state) {
        case USB_ENUM_IDLE : {
            break;
        }
        case USB_ENUM_GET_EP0_SIZE : {
            printl("EP0 size done");
            usb_ep0_size_done(urb, usbc->enum_device);
            break;
        }
        case USB_ENUM_GET_DEV_DESC : {
            printl("Device descriptor done");
            usb_device_desc_done(urb, usbc->enum_device);
            break;
        }
        case USB_ENUM_SET_ADDRESS : {
            printl("Address setup done");
            usb_address_done(urb, usbc->enum_device);
            break;
        }
        case USB_ENUM_GET_DESC_LENGTH : {
            print("Configuration decriptor done");
            usb_desc_length_done(urb, usbc->enum_device);
            break;
        }
        case USB_ENUM_GET_DESCRIPTORS : {
            print("All the descriptors are read\n");
            usb_get_all_desc_done(urb, usbc->enum_device);
            break;
        }
    }
}

/*
 * This will add a new device to the USB core layer. This will be available in
 * the device list as well as from the enum device pointer in the USB host core
 */
static void usb_add_device(struct usb_core* usbc)
{
    /* Allocate a new device */
    struct usb_device* device = (struct usb_device *)
        bmalloc(sizeof(struct usb_device), BMALLOC_SRAM);

    /* Insert it into the list of devices */
    list_add_first(&device->node, &usbc_priv->device_list);
    usbc->enum_device = device;

    /* Add a name for debugging */
    string_copy("Lime", device->name);
}

/*
 * Allocates all the configurations needed for a device and adds it to the
 * device structure
 */
static void usb_add_configurations(struct usb_core* usbc, u32 count)
{
    /* Allocate all the configuration structures */
    usbc->enum_device->configurations = (struct usb_configuration *)
        bmalloc(count * sizeof(struct usb_configuration), BMALLOC_SRAM);

    usbc->enum_device->num_configurations = count;
}

void root_hub_event(struct usbhc* usbhc, enum root_hub_event event)
{
    if (event == RH_EVENT_CONNECTION) {
        printl("USB core => connection");
        usbhc_send_reset();

    } else if (event == RH_EVENT_DISCONNECTION) {
        printl("USB core => disconnection");

    } else if (event == RH_EVENT_RESET_SENT) {
        printl("USB core => reset sent");

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
        usbhc_alloc_pipe(&usbhc->pipe_base[0], &cfg);
        usbhc_set_address(&usbhc->pipe_base[0], 0);

        usb_add_device(usbc_priv);

        struct urb* urb = usbhc_alloc_urb();

        /* Context for callback routine */
        urb->context = usbc_priv;

        /* Start the enumeration */
        usbc_priv->enum_state = USB_ENUM_GET_EP0_SIZE;
        usb_enum_get_dev_desc(urb, usbc_priv, 0);
        printl("Enumeration has started");
    }
}

static void sof_event(struct usbhc* usbhc)
{
    static i = 0;
    if (++i >= 1000) {
        i = 0;
        print("ok\n");
    }
}

void usb_init(struct usb_core* usbc, struct usbhc* usbhc)
{
    usbc_priv = usbc;
    usbc_priv->enum_state = USB_ENUM_IDLE;

    usbc->usbhc = usbhc;
    usbc->pipe0 = &usbhc->pipe_base[0];

    /* Initialize the device list */
    list_init(&usbc->device_list);
    usbc->device_addr_bm = 1;

    usbhc_add_root_hub_callback(usbhc, &root_hub_event);
    usbhc_add_sof_callback(usbhc, &sof_event);
}
