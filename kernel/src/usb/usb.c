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

static void usbc_enumerate(struct urb* urb);
static void usbc_add_device(struct usb_core* usbc);

static u8 usbc_new_address(struct usb_core* usbc);
static void usbc_delete_address(struct usb_core* usbc, u8 address);

/* Enumeration stages */
static void usbc_get_dev_desc(struct urb* urb, struct usb_core* usbc, u8 full);
static void usbc_set_dev_addr(struct urb* urb, struct usb_core* usbc);
static void usbc_get_cfg_desc(struct urb* urb, struct usb_core* usbc);
static void usbc_get_all_desc(struct urb* urb, struct usb_core* usbc);

/* Enumeration stages complete */
static void usbc_device_desc_done(struct urb* urb, struct usb_dev* device);
static void usbc_ep0_size_done(struct urb* urb, struct usb_dev* device);
static void usbc_address_done(struct urb* urb, struct usb_dev* device);
static void usbc_desc_length_done(struct urb* urb, struct usb_dev* device);
static void usbc_get_all_desc_done(struct urb* urb, struct usb_dev* device);

static u8 usbc_verify_descriptors(u8* data, u32 size, u32* configs, u32* ifaces, 
    u32* eps);
static void usbc_parse_descriptors(struct usb_dev* dev, u8* data, u32 size);

/*
 * This will take in a URB an perform a get device descriptor request. Full 
 * specifies if the full descriptor is fetched or only the first 8 bytes. This
 * is due to unknown size of EP0 
 */
static void usbc_get_dev_desc(struct urb* urb, struct usb_core* usbc, u8 full)
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
    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usbc_enumerate);
    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usb_get_string_desc(struct urb* urb, struct usb_core* usbc,
    u8 desc_index, u16 lang_id)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_STRING;
    setup.bDescriptorIndex = desc_index;
    setup.wIndex           = lang_id;
    setup.wLength          = ENUM_BUFFER_SIZE;

    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usbc_enumerate);
    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usbc_set_dev_addr(struct urb* urb, struct usb_core* usbc)
{
    setup.bmRequestType = USB_HOST_TO_DEVICE;
    setup.bRequest      = USB_REQ_SET_ADDRESS;
    setup.wValue        = usbc_new_address(usbc);
    setup.wIndex        = 0;
    setup.wLength       = 0;

    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usbc_enumerate);
    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usbc_get_cfg_desc(struct urb* urb, struct usb_core* usbc)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_CONFIGURATION;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;
    setup.wLength          = 9;

    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usbc_enumerate);
    usbhc_submit_urb(urb, usbc->pipe0);
}

static void usbc_get_all_desc(struct urb* urb, struct usb_core* usbc)
{
    setup.bmRequestType    = USB_DEVICE_TO_HOST;
    setup.bRequest         = USB_REQ_GET_DESCRIPTOR;
    setup.bDescriptorType  = USB_DESC_CONFIGURATION;
    setup.bDescriptorIndex = 0;
    setup.wIndex           = 0;
    setup.wLength          = usbc->enum_dev->desc_total_size;

    usbhc_fill_control_urb(urb, (u8 *)&setup, enum_buffer, &usbc_enumerate);
    usbhc_submit_urb(urb, usbc->pipe0);
}


/*
 * The URB will contain the first 8 bytes of the device decriptor. This should
 * be enough to read the default EP0 size
 */
static void usbc_ep0_size_done(struct urb* urb, struct usb_dev* device)
{
    /* Update the endpoint zero size */
    printl("EP0 size done");
    struct usb_dev_desc* dev_desc = (struct usb_dev_desc *)urb->transfer_buffer;
    u32 packet_size = dev_desc->bMaxPacketSize;
    if ((packet_size < 8) || (packet_size > 1024)) {
        panic("Abort enumeration");
    }
    device->ep0_size = packet_size;
    print("Max packet => %d\n", device->ep0_size);
}

static void usbc_device_desc_done(struct urb* urb, struct usb_dev* device)
{
    printl("Device descriptor done");
    struct usb_setup_desc* desc = (struct usb_setup_desc *)urb->setup_buffer;
    u32 size = desc->wLength;
    print("Size: %d\n", size);

    u8* src = urb->transfer_buffer;
    u8* dest = (u8 *)&device->desc;

    memory_copy(src, dest, size);
}

static void usbc_address_done(struct urb* urb, struct usb_dev* device)
{
    printl("Address done");
    struct usb_setup_desc* setup = (struct usb_setup_desc *)urb->setup_buffer;
    device->address = setup->wValue;
    print("DEVICE ADDRESS => %d\n", device->address);

    /* Update the pipe address */
    struct usb_core* usbc = (struct usb_core *)urb->context;
    usbhc_set_address(usbc->pipe0, device->address);
}

static void usbc_desc_length_done(struct urb* urb, struct usb_dev* device)
{
    printl("Descriptor length done");
    struct usb_config_desc* cfg_desc = (struct usb_config_desc *)urb->transfer_buffer;
    print("Bytes recieved => %d\n", urb->acctual_length);
    if (urb->acctual_length != 9) {
        panic("Error");
    }
    device->desc_total_size = cfg_desc->wTotalLength;
    print("Total length => %d\n", device->desc_total_size);
}

static void usbc_get_all_desc_done(struct urb* urb, struct usb_dev* device)
{
    printl("All descriptors received");
    usbc_parse_descriptors(device, urb->transfer_buffer, urb->acctual_length);

    print("CFGS => %d\n", device->num_configs);
    print("IFACE => %d\n", device->configs[0].num_ifaces);

    for (u32 c = 0; c < device->num_configs; c++) {
        struct usb_config_desc* c_ptr = &device->configs[c].desc;
        usb_print_config_desc(c_ptr);
        for (u32 i = 0; i < device->configs[c].num_ifaces; i++) {
            struct usb_iface_desc* i_ptr = &device->configs[c].ifaces[i].desc;
            usb_print_iface_desc(i_ptr);
            for (u32 e = 0; e < device->configs[c].ifaces[i].num_eps; e++) {
                struct usb_ep_desc* e_ptr = &device->configs[c].ifaces[i].eps[e].desc;
                usb_print_ep_desc(e_ptr);
            }
        }
    }
}

static void usbc_get_string_done(struct urb* urb, struct usb_dev* device)
{
    print("Done => %d\n", urb->acctual_length);
    print("Language ID => %2h\n", *(u16 *)(urb->transfer_buffer + 2));
    char* data = (char *)urb->transfer_buffer;
    for (u32 i = 0; i < urb->acctual_length; i++) {
        print("%c ", *data++);
    }
}

/*
 * This is called when the URB does not return OK. This is most of the times
 * due to NAK or STALL from the device
 */
static void usb_handle_urb_fail(struct urb* urb)
{
    panic("Enumeration URB failed");
}

/*
 * This will perform the enumeration of a newly attatched devices. Everything
 * will happend asynchronously since this is a URB callback. The entire
 * enumeration will only use one URB. A new device will be added in this
 * process
 */
static void usbc_enumerate(struct urb* urb)
{
    if (urb->status != URB_STATUS_OK) {
        usb_handle_urb_fail(urb);
        return;
    }
    struct usb_core* usbc = (struct usb_core *)urb->context;

    print("Enumerate handler => ");
    switch(usbc->enum_state) {
        case USB_ENUM_IDLE : {
            break;
        }
        case USB_ENUM_GET_EP0_SIZE : {
            usbc_ep0_size_done(urb, usbc->enum_dev);
            usbhc_set_ep_size(usbc->pipe0, usbc->enum_dev->ep0_size);
            usbc->enum_state = USB_ENUM_GET_DEV_DESC;
            usbc_get_dev_desc(urb, usbc, 1);
            break;
        }
        case USB_ENUM_GET_DEV_DESC : {
            usbc_device_desc_done(urb, usbc->enum_dev);
            usbc->enum_state = USB_ENUM_SET_ADDRESS;
            usbc_set_dev_addr(urb, usbc);
            break;
        }
        case USB_ENUM_SET_ADDRESS : {
            usbc_address_done(urb, usbc->enum_dev);
            usbc->enum_state = USB_ENUM_GET_DESC_LENGTH;
            usbc_get_cfg_desc(urb, usbc);
            break;
        }
        case USB_ENUM_GET_DESC_LENGTH : {
            usbc_desc_length_done(urb, usbc->enum_dev);
            usbc->enum_state = USB_ENUM_GET_DESCRIPTORS;
            usbc_get_all_desc(urb, usbc);
            break;
        }
        case USB_ENUM_GET_DESCRIPTORS : {
            usbc_get_all_desc_done(urb, usbc->enum_dev);
            usbc->enum_state = USB_ENUM_GET_STRINGS;
            usb_get_string_desc(urb, usbc, 2, 0);
            break;
        }
        case USB_ENUM_GET_STRINGS : {
            printl("Strings done");
            usbc_get_string_done(urb, usbc->enum_dev);
            usb_print_dev_desc(&usbc->enum_dev->desc);
        }
    }
}

/*
 * This returns a new free address. An address is dynamically assigned to a 
 * device during enumeration. If the SET_ADDRESS command fails the address should
 * still be set. Only after a disconnection should the address be deleted!
 */
static u8 usbc_new_address(struct usb_core* usbc)
{
    u16 bitmask = usbc->dev_addr_bm;
    for (u8 i = 1; i < MAX_PIPES; i++) {
        if ((bitmask & (1 << i)) == 0) {
            usbc->dev_addr_bm |= (1 << i);
            return i;
        }
    }
    return 0;
}

static void usbc_delete_address(struct usb_core* usbc, u8 address)
{
    usbc->dev_addr_bm &= ~(1 << address);
}

/*
 * This will add a new device to the USB core layer. This will be available in
 * the device list as well as from the enum device pointer in the USB host core
 */
static void usbc_add_device(struct usb_core* usbc)
{
    /* Allocate a new device */
    struct usb_dev* device = (struct usb_dev *)
        bmalloc(sizeof(struct usb_dev), BMALLOC_SRAM);

    /* Insert it into the list of devices */
    list_add_first(&device->node, &usbc_private->dev_list);
    usbc->enum_dev = device;
}

static u8 usbc_verify_descriptors(u8* data, u32 size, u32* configs, u32* ifaces, 
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
                usb_print_config_desc((struct usb_config_desc *)(data + pos));
                if (size != sizeof(struct usb_config_desc)) {
                    return 0;
                }
                (*configs)++;
                break;
            }
            case USB_DESC_INTERFACE : {
                usb_print_iface_desc((struct usb_iface_desc *)(data + pos));
                if (size != sizeof(struct usb_iface_desc)) {
                    return 0;
                }
                (*ifaces)++;
                break;
            }
            case USB_DESC_ENDPOINT : {
                usb_print_ep_desc((struct usb_ep_desc *)(data + pos));
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
        offset += index * sizeof(struct usb_ep);
    }
    return offset;
}

static void usbc_parse_descriptors(struct usb_dev* dev, u8* data, u32 size)
{
    u32 configs;
    u32 ifaces;
    u32 eps;

    if (!usbc_verify_descriptors(data, size, &configs, &ifaces, &eps)) {
        print("Descriptor verification failed\n");
    }
    print("Configs => %d\n", configs);
    print("Ifaces => %d\n",ifaces);
    print("EPs => %d\n", eps);

    /* Allocate the memory */
    u32 desc_mem_size = 0;
    desc_mem_size += configs * sizeof(struct usb_config);
    desc_mem_size += ifaces * sizeof(struct usb_iface);
    desc_mem_size += eps * sizeof(struct usb_ep);

    

    u8* desc_ptr = (u8 *)bmalloc(desc_mem_size, BMALLOC_SRAM);
    dev->configs = (struct usb_config *)desc_ptr;
    
    if (dev->configs == NULL) {
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

            dev->num_configs++;
            cfg->num_ifaces = 0;
            last_cfg = cfg;
            config_index++;

        } else if (type == USB_DESC_INTERFACE) {
            print("IFACE\n");
            u32 offset = usb_get_desc_offset(configs, ifaces,
                USB_DESC_INTERFACE, iface_index);
            print("Offset => %d\n", offset);
            struct usb_iface* iface = (struct usb_iface *)(desc_ptr + offset);
            iface->num_eps = 0;
            last_iface = iface;
            
            memory_copy(data + pos, &iface->desc, sizeof(struct usb_iface_desc));

            if (last_cfg == NULL) {
                panic("Error");
            }
            if (last_cfg->num_ifaces == 0) {
                last_cfg->ifaces = iface;
                print("LINKING INTERFACE\n");
            }
            last_cfg->num_ifaces++;
            iface_index++;

        } else if (type == USB_DESC_ENDPOINT) {
            print("EP\n");
            u32 offset = usb_get_desc_offset(configs, ifaces,
                USB_DESC_ENDPOINT, ep_index);
            print("Offset => %d\n", offset);
            struct usb_ep* ep = (struct usb_ep *)(desc_ptr + offset);

            memory_copy(data + pos, &ep->desc, sizeof(struct usb_ep));

            if (last_iface == NULL) {
                panic("Error");
            }
            if (last_iface->num_eps == 0) {
                last_iface->eps = ep;
                print("LINKING ENDPOINT\n");
            }
            last_iface->num_eps++;
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
        struct pipe_config cfg = {
            .banks = PIPE_BANKS_1,
            .autoswitch = 0,
            .device = 0,
            .frequency = 0,
            .pipe = 0,
            .size = PIPE_SIZE_64,
            .token = PIPE_TOKEN_SETUP,
            .type = PIPE_TYPE_CTRL
        };
        usbhc_alloc_pipe(&usbhc->pipes[0], &cfg);
        usbhc_set_address(&usbhc->pipes[0], 0);
        usbhc_set_ep_size(&usbhc->pipes[0], 64);

        usbc_add_device(usbc_private);

        struct urb* urb = usbhc_alloc_urb();

        /* Context for callback routine */
        urb->context = usbc_private;

        /* Start the enumeration */
        usbc_private->enum_state = USB_ENUM_GET_EP0_SIZE;
        usbc_get_dev_desc(urb, usbc_private, 0);
        printl("Enumeration has started");
    }
}

static void sof_event(struct usbhc* usbhc)
{

}

void usbc_init(struct usb_core* usbc, struct usbhc* usbhc)
{
    usbc_private = usbc;
    usbc_private->enum_state = USB_ENUM_IDLE;

    usbc->usbhc = usbhc;
    usbc->pipe0 = &usbhc->pipes[0];

    /* Initialize the device list */
    list_init(&usbc->dev_list);
    usbc->dev_addr_bm = 1;

    /* Initialize the driver list */
    list_init(&usbc->driver_list);

    usbhc_add_root_hub_callback(usbhc, &root_hub_event);
    usbhc_add_sof_callback(usbhc, &sof_event);
}

void usbc_add_driver(struct usb_driver* driver, struct usb_core* usbc)
{
    list_add_first(&driver->node, &usbc->driver_list);
}
