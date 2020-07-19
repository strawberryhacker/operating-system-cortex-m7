# USB progress

**19.07.2020**

I am able to get the USB exceptions working. Currently it is divided up into tre handlers

- SOF interrupts
- Pipe interrupts
- Root hub interrupts

The SOF interrupt will trigger every millisecond when microframes are disabled in high-speed. Currently this is not doing anything. The pipe interrupts calls a handler which picks the first "interrupted" pipe and handles that pipe. This includes reading the pipe interrupt status register; USBHS->HSTPIPISR[x] where x is the pipe number. The root hub interrupt takes care of everything that has to do with wakeup, resume and connect. 

Notes:

I made a small functions which sends the USB get descriptor frame (first frame in the device enumeration prosess) which send 8 bytes. This is called by the root hub interrupt after a successful bus reset. It seems that a bus reset DISABLES both the pipes and the configuration, so I had to configure the pipe again. I read the status register and found that the pipe registerd that i wrote the ist assigned DPRAM memory. Still the number of bytes written were wrong and varies after multiple warm bus resets. 
