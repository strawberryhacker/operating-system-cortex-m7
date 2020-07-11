# Copyright (C) StrawberryHacker

TOP = $(shell pwd)

BUILDDIR = $(TOP)/build
TARGET_NAME = kernel

# Compilers
CC      = arm-none-eabi-gcc
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy
ARM_ASM = arm-none-eabi-as

#-------------------------------------------------------------------------------
# Add compiler, preprocessor and linker flags here 
#-------------------------------------------------------------------------------
CPFLAGS += -I$(TOP)/src

CFLAGS += -x c -mthumb -O3 -ffunction-sections -mlong-calls -g3 -Wall
CFLAGS += -std=gnu99 -mcpu=cortex-m7 -c -fdata-sections

LDFLAGS += -mthumb -Wl,-Map="$(BUILDDIR)/$(TARGET_NAME).map" -Wl,--start-group
LDFLAGS += -Wl,--end-group -Wl,--gc-sections -mcpu=cortex-m7 -Tlinker.ld

ASMFLAGS += -mcpu=cortex-m7 -mthumb -g

#-------------------------------------------------------------------------------
# Include source files
#-------------------------------------------------------------------------------
SRC +=

obj-y += /src/main.c
obj-y += /src/bootloader.c

kernel-y += /src/kernel/kernel_entry.c
kernel-y += /src/kernel/fault.c
kernel-y += /src/kernel/scheduler.c
kernel-y += /src/kernel/thread.c
kernel-y += /src/kernel/list.c
kernel-y += /src/kernel/dlist.c
kernel-y += /src/kernel/rt.c
kernel-y += /src/kernel/app.c
kernel-y += /src/kernel/background.c
kernel-y += /src/kernel/idle.c
kernel-y += /src/kernel/syscall.c
kernel-y += /src/kernel/dynamic_linker.c

driver-y += /src/drivers/systick.c
driver-y += /src/drivers/clock.c
driver-y += /src/drivers/watchdog.c
driver-y += /src/drivers/flash.c
driver-y += /src/drivers/usart.c
driver-y += /src/drivers/gpio.c
driver-y += /src/drivers/hash.c
driver-y += /src/drivers/dram.c
driver-y += /src/drivers/matrix.c
driver-y += /src/drivers/mmc.c
driver-y += /src/drivers/gmac.c

disk-y += /src/disk/sd_protocol.c
disk-y += /src/disk/disk_io.c
disk-y += /src/disk/fat32.c

board-y += /src/board/serial.c
board-y += /src/board/print.c
board-y += /src/board/sd.c
board-y += /src/board/ethernet.c

generic-y += /src/generic/sprint.c
generic-y += /src/generic/crc.c
generic-y += /src/generic/memory.c
generic-y += /src/generic/panic.c

net-y += /src/net/ip.c

mm-y += /src/mm/mm.c
mm-y += /src/mm/mpu.c
mm-y += /src/mm/cache.c

asm-y += /src/entry/entry.s
asm-y += /src/kernel/context.s
asm-y += /src/kernel/svc.s
asm-y += /src/kernel/fault_asm.s

ASM_OBJ := $(patsubst %.s, %.o, $(asm-y))
KERNEL_OBJ := $(patsubst %.c, %.o, $(kernel-y))
GENERIC_OBJ := $(patsubst %.c, %.o, $(generic-y))
BOARD_OBJ := $(patsubst %.c, %.o, $(board-y))
DRIVER_OBJ := $(patsubst %.c, %.o, $(driver-y))
MM_OBJ := $(patsubst %.c, %.o, $(mm-y))
OBJ_OBJ := $(patsubst %.c, %.o, $(obj-y))
DISK_OBJ := $(patsubst %.c, %.o, $(disk-y))
NET_OBJ := $(patsubst %.c, %.o, $(net-y))

OBJ += $(addprefix $(BUILDDIR), $(KERNEL_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(DRIVER_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(ASM_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(BOARD_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(GENERIC_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(MM_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(OBJ_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(DISK_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(NET_OBJ))

# Add include paths
CPFLAGS += -I$(TOP)/src/drivers
CPFLAGS += -I$(TOP)/src/board
CPFLAGS += -I$(TOP)/src/generic
CPFLAGS += -I$(TOP)/src/cpu
CPFLAGS += -I$(TOP)/src/mm
CPFLAGS += -I$(TOP)/src/kernel
CPFLAGS += -I$(TOP)/src/disk
CPFLAGS += -I$(TOP)/src/net
CPFLAGS += -I$(TOP)

#-------------------------------------------------------------------------------
# Rules
#-------------------------------------------------------------------------------
.SECONDARY: $(OBJ)
.PHONY: all elf bin lss hex
all: elf lss bin hex program

elf: $(BUILDDIR)/$(TARGET_NAME).elf
bin: $(BUILDDIR)/$(TARGET_NAME).bin
lss: $(BUILDDIR)/$(TARGET_NAME).lss
hex: $(BUILDDIR)/$(TARGET_NAME).hex

program:
	@python3 ../tools/kernel_prog.py -c /dev/ttyS3 -f build/kernel.bin

$(BUILDDIR)/%.bin: $(BUILDDIR)/%.elf
	@$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/%.lss: $(BUILDDIR)/%.elf
	@$(OBJDUMP) -h -S $< > $@

$(BUILDDIR)/%.hex: $(BUILDDIR)/%.elf
	@$(OBJCOPY) -O ihex $< $@

$(BUILDDIR)/%.elf: $(OBJ)
	@echo
	@$(CC) $(LDFLAGS) -Wl,--print-memory-usage $^ -o $@
	@echo


# Generate object files from .c files
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo " >" $<
	@$(CC) $(CFLAGS) $(CPFLAGS) -c $< -o $@

# Generate object files from .s files
$(BUILDDIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo " >" $<
	@$(ARM_ASM) $(ASMFLAGS) -c $< -o $@

clean:
	@rm -r -f $(BUILDDIR)
