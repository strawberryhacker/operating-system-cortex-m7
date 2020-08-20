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
struct usb_core* usbc_private;

static void usb_enumerate(struct urb* urb);
static void usb_add_device(struct usb_core* usbc);

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

static u8 usb_verify_descriptors(u8* data, u32 size, u32* configs, u32* ifaces, 
    u32* eps);
static void usb_parse_descriptors(struct usb_device* dev, u8* data, u32 size);

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
}

static void usb_device_desc_done(struct urb* urb, struct usb_device* device)
{
    struct usb_setup_desc* desc = (struct usb_setup_desc *)urb->setup_buffer;
    u32 size = desc->wLength;
    print("Size: %d\n", size);

    u8* src = urb->transfer_buffer;
    u8* dest = (u8 *)&device->desc;

    memory_copy(src, dest, size);
}

static void usb_address_done(struct urb* urb, struct usb_device* device)
{
    struct usb_setup_desc* setup = (struct usb_setup_desc *)urb->setup_buffer;
    device->address = setup->wValue;
    print("DEVICE ADDRESS => %d\n", device->address);

    /* Update the pipe address */
    struct usb_core* usbc = (struct usb_core *)urb->context;
    usbhc_set_address(usbc->pipe0, device->address);
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
}

static void usb_get_all_desc_done(struct urb* urb, struct usb_device* device)
{
    usb_parse_descriptors(device, urb->transfer_buffer, urb->acctual_length);

    print("CFGS => %d\n", device->num_configurations);
    print("IFACE => %d\n", device->configurations[0].num_interfaces);

    for (u32 c = 0; c < device->num_configurations; c++) {
        struct usb_config_desc* c_ptr = &device->configurations[c].desc;
        usb_debug_print_config_desc(c_ptr);
        for (u32 i = 0; i < device->configurations[c].num_interfaces; i++) {
            struct usb_iface_desc* i_ptr = &device->configurations[c].interfaces[i].desc;
            usb_debug_print_iface_desc(i_ptr);
            for (u32 e = 0; e < device->configurations[c].interfaces[i].num_endpoints; e++) {
                struct usb_ep_desc* e_ptr = &device->configurations[c].interfaces[i].endpoints[e].desc;
                usb_debug_print_ep_desc(e_ptr);
            }
        }
    }

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
            usbc->enum_state = USB_ENUM_GET_DEV_DESC;
            usb_enum_get_dev_desc(urb, usbc, 1);
            break;
        }
        case USB_ENUM_GET_DEV_DESC : {
            printl("Device descriptor done");
            usb_device_desc_done(urb, usbc->enum_device);
            usbc->enum_state = USB_ENUM_SET_ADDRESS;
            usb_enum_set_dev_addr(urb, usbc);
            break;
        }
        case USB_ENUM_SET_ADDRESS : {
            printl("Address setup done");
            usb_address_done(urb, usbc->enum_device);
            usbc->enum_state = USB_ENUM_GET_DESC_LENGTH;
            usb_enum_get_cfg_desc(urb, usbc);
            break;
        }
        case USB_ENUM_GET_DESC_LENGTH : {
            print("Configuration decriptor done");
            usb_desc_length_done(urb, usbc->enum_device);
            usbc->enum_state = USB_ENUM_GET_DESCRIPTORS;
            usb_enum_get_all_desc(urb, usbc);
            break;
        }
        case USB_ENUM_GET_DESCRIPTORS : {
            print("All the descriptors are read\n");
            usb_get_all_desc_done(urb, usbc->enum_device);

            /* We have a new device with descriptor so we must seach for a driver */
            struct list_node* it;
            list_iterate(it, &usbc->driver_list) {

                struct usb_driver* driver = 
                    list_get_entry(it, struct usb_driver, node);
                struct usb_iface* iface = &usbc->enum_device->configurations[0].interfaces[0];
                u8 status = driver->probe(iface);
            
                if (status) {
                    printl("Found a driver that supports the device");
                    usbhc_assign_driver(driver, iface);
                    driver->start(iface);
                    break;
                }
            }
            break;
        }
    }
}

/*
 * This returns a new free address. An address is dynamically assigned to a 
 * device during enumeration. If the SET_ADDRESS command fails the address should
 * still be set. Only after a disconnection should the address be deleted!
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
 * This will add a new device to the USB core layer. This will be available in
 * the device list as well as from the enum device pointer in the USB host core
 */
static void usb_add_device(struct usb_core* usbc)
{
    /* Allocate a new device */
    struct usb_device* device = (struct usb_device *)
        bmalloc(sizeof(struct usb_device), BMALLOC_SRAM);

    /* Insert it into the list of devices */
    list_add_first(&device->node, &usbc_private->device_list);
    usbc->enum_device = device;

    /* Add a name for debugging */
    string_copy("Lime", device->name);
}

static u8 usb_verify_descriptors(u8* data, u32 size, u32* configs, u32* ifaces, 
    u32* eps)
{ 
    *configs = 0;
    *eps = 0;
    *ifaces = 0;

    u32 pos = 0;

    while (pos < size) {
        u32 type = data[pos + 1];
        u32 size = data[pos];

        switch(type) {
            case USB_DESC_CONFIGURATION : {
                usb_debug_print_config_desc((struct usb_config_desc *)(data + pos));
                if (size != sizeof(struct usb_config_desc)) {
                    return 0;
                }
                (*configs)++;
                break;
            }
            case USB_DESC_INTERFACE : {
                usb_debug_print_iface_desc((struct usb_iface_desc *)(data + pos));
                if (size != sizeof(struct usb_iface_desc)) {
                    return 0;
                }
                (*ifaces)++;
                break;
            }
            case USB_DESC_ENDPOINT : {
                usb_debug_print_ep_desc((struct usb_ep_desc *)(data + pos));
                if (size != sizeof(struct usb_ep_desc)) {
                    return 0;
                }
                (*eps)++;
                break;
            }
            default : {
                break;
            }
        }
        print("SIZE => %d\n", size);
        pos += size;
    }
    
    if (pos != size) {
        printl("Decriptor verification error");
        return 0;
    } else {
        return 1;
    }
}

static u32 usb_get_desc_offset(u32 configs, u32 ifaces, u8 type, u32 index)
{
    u32 offset = 0;
    if (type == USB_DESC_CONFIGURATION) {
        offset += index * sizeof(struct usb_config);
    } else if (type == USB_DESC_INTERFACE) {
        offset += configs * sizeof(struct usb_config);
        offset += index * sizeof(struct usb_iface);
    } else if (type == USB_DESC_ENDPOINT) {
        offset += configs * sizeof(struct usb_config);
        offset += ifaces * sizeof(struct usb_iface);
        offset += index * sizeof(struct usb_endpoint);
    }
    return offset;
}

static void usb_parse_descriptors(struct usb_device* dev, u8* data, u32 size)
{
    u32 configs;
    u32 ifaces;
    u32 eps;

    if (!usb_verify_descriptors(data, size, &configs, &ifaces, &eps)) {
        print("Descriptor verification failed\n");
    }
    print("Configs => %d\n", configs);
    print("Ifaces => %d\n",ifaces);
    print("EPs => %d\n", eps);

    /* Allocate the memory */
    u32 desc_mem_size = 0;
    desc_mem_size += configs * sizeof(struct usb_config);
    desc_mem_size += ifaces * sizeof(struct usb_iface);
    desc_mem_size += eps * sizeof(struct usb_endpoint);

    

    u8* desc_ptr = (u8 *)bmalloc(desc_mem_size, BMALLOC_SRAM);
    dev->configurations = (struct usb_config *)desc_ptr;
    
    if (dev->configurations == NULL) {
        panic("Error");
    }
    print("Allocated => %d\n", desc_mem_size);

    /* Variable bank */
    u32 config_index = 0;
    u32 iface_index = 0;
    u32 ep_index = 0;

    struct usb_config* last_cfg = NULL;
    struct usb_iface* last_iface = NULL;

    u32 pos = 0;
    while (pos < size) {
        
        u32 type = data[pos + 1];
        u32 size = data[pos];
        if (type == USB_DESC_CONFIGURATION) {
            print("CFG\n");
            u32 offset = usb_get_desc_offset(configs, ifaces,
                USB_DESC_CONFIGURATION, config_index);
            print("Offset => %d\n", offset);
            struct usb_config* cfg = (struct usb_config *)(desc_ptr + offset);

            memory_copy(data + pos, &cfg->desc, sizeof(struct usb_config_desc));

            dev->num_configurations++;
            cfg->num_interfaces = 0;
            last_cfg = cfg;
            config_index++;

        } else if (type == USB_DESC_INTERFACE) {
            print("IFACE\n");
            u32 offset = usb_get_desc_offset(configs, ifaces,
                USB_DESC_INTERFACE, iface_index);
            print("Offset => %d\n", offset);
            struct usb_iface* iface = (struct usb_iface *)(desc_ptr + offset);
            iface->num_endpoints = 0;
            last_iface = iface;
            
            memory_copy(data + pos, &iface->desc, sizeof(struct usb_iface_desc));

            if (last_cfg == NULL) {
                panic("Error");
            }
            if (last_cfg->num_interfaces == 0) {
                last_cfg->interfaces = iface;
                print("LINKING INTERFACE\n");
            }
            last_cfg->num_interfaces++;
            iface_index++;

        } else if (type == USB_DESC_ENDPOINT) {
            print("EP\n");
            u32 offset = usb_get_desc_offset(configs, ifaces,
                USB_DESC_ENDPOINT, ep_index);
            print("Offset => %d\n", offset);
            struct usb_endpoint* ep = (struct usb_endpoint *)(desc_ptr + offset);

            memory_copy(data + pos, &ep->desc, sizeof(struct usb_ep_desc));

            if (last_iface == NULL) {
                panic("Error");
            }
            if (last_iface->num_endpoints == 0) {
                last_iface->endpoints = ep;
                print("LINKING ENDPOINT\n");
            }
            last_iface->num_endpoints++;
            ep_index++;
        }
        pos += size;
    }
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

        usb_add_device(usbc_private);

        struct urb* urb = usbhc_alloc_urb();

        /* Context for callback routine */
        urb->context = usbc_private;

        /* Start the enumeration */
        usbc_private->enum_state = USB_ENUM_GET_EP0_SIZE;
        usb_enum_get_dev_desc(urb, usbc_private, 0);
        printl("Enumeration has started");
    }
}

static void sof_event(struct usbhc* usbhc)
{

}

void usb_init(struct usb_core* usbc, struct usbhc* usbhc)
{
    usbc_private = usbc;
    usbc_private->enum_state = USB_ENUM_IDLE;

    usbc->usbhc = usbhc;
    usbc->pipe0 = &usbhc->pipe_base[0];

    /* Initialize the device list */
    list_init(&usbc->device_list);
    usbc->device_addr_bm = 1;

    /* Initialize the driver list */
    list_init(&usbc->driver_list);

    usbhc_add_root_hub_callback(usbhc, &root_hub_event);
    usbhc_add_sof_callback(usbhc, &sof_event);
}

void usb_add_driver(struct usb_driver* driver, struct usb_core* usbc)
{
    list_add_first(&driver->node, &usbc->driver_list);
}
