/* Full packet received */
    if (urb->acctual_length >= urb->transfer_length) {
        usbhc_end_urb(urb, pipe, URB_STATUS_OK);
        return;
    }