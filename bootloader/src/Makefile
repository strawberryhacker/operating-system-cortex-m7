bootloader-y += /src/main.c

driver-y += /src/drivers/systick.c

asm-y += /src/boot/boot.s

BOOT_OBJ := $(patsubst %.c, %.o, $(bootloader-y))
DRIVER_OBJ := $(patsubst %.c, %.o, $(driver-y))
ASM_OBJ := $(patsubst %.s, %.o, $(asm-y))

OBJ += $(addprefix $(BUILDDIR), $(BOOT_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(DRIVER_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(ASM_OBJ))

# Add include paths
CPFLAGS += -I$(TOP)/src/drivers
CPFLAGS += -I$(TOP)/src/generic