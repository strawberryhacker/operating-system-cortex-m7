bootloader-y += /src/main.c

driver-y += /src/drivers/systick.c
driver-y += /src/drivers/clock.c
driver-y += /src/drivers/watchdog.c
driver-y += /src/drivers/flash.c
driver-y += /src/drivers/usart.c
driver-y += /src/drivers/gpio.c

cpu-y += /src/cpu/interrupt.c

board-y += /src/board/serial.c

generic-y += /src/generic/print.c
generic-y += /src/generic/crc.c

asm-y += /src/boot/boot.s

BOOT_OBJ := $(patsubst %.c, %.o, $(bootloader-y))
CPU_OBJ := $(patsubst %.c, %.o, $(cpu-y))
GENERIC_OBJ := $(patsubst %.c, %.o, $(generic-y))
BOARD_OBJ := $(patsubst %.c, %.o, $(board-y))
DRIVER_OBJ := $(patsubst %.c, %.o, $(driver-y))
ASM_OBJ := $(patsubst %.s, %.o, $(asm-y))

OBJ += $(addprefix $(BUILDDIR), $(BOOT_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(DRIVER_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(ASM_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(BOARD_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(GENERIC_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(CPU_OBJ))

# Add include paths
CPFLAGS += -I$(TOP)/src/drivers
CPFLAGS += -I$(TOP)/src/board
CPFLAGS += -I$(TOP)/src/generic
CPFLAGS += -I$(TOP)/src/cpu