#ifndef SECTIONS_H
#define SECTIONS_H

#define __ramfunc__ __attribute__((long_call, section(".ramfunc")))
#define __bootsig__ __attribute__((section(".boot_signature")))
#define __image_info__ __attribute__((section(".image_info")))

#endif