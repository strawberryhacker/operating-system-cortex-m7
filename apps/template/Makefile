# Copyright (C) StrawberryHacker

TOP = $(shell pwd)

BUILDDIR = $(TOP)/build
TARGET_NAME = tamplate

# Compilers
CC      = arm-none-eabi-gcc
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy
ARM_ASM = arm-none-eabi-as

#-------------------------------------------------------------------------------
# Add compiler, preprocessor and linker flags here 
#-------------------------------------------------------------------------------
CPFLAGS += -I$(TOP)/src

#-ffunction-sections
CFLAGS += -x c -mthumb -O0  -mlong-calls -g3 -Wall
CFLAGS += -std=gnu99 -mcpu=cortex-m7 -c -fdata-sections -Wno-main -fpic
CFLAGS += -nostdlib

LDFLAGS += -mthumb -Wl,-Map="$(BUILDDIR)/$(TARGET_NAME).map" -Wl,--start-group
LDFLAGS += -Wl,--end-group -mcpu=cortex-m7 -Tlinker.ld --specs=nosys.specs
LDFLAGS += -Wl,--gc-sections -fpic

ASMFLAGS += -mcpu=cortex-m7 -mthumb -g

# FPU support
CFLAGS   += -mfpu=fpv5-sp-d16 -mfloat-abi=hard
LDFLAGS  += -mfpu=fpv5-sp-d16 -mfloat-abi=hard
ASMFLAGS += -mfpu=fpv5-sp-d16 -mfloat-abi=hard

#-------------------------------------------------------------------------------
# Include source files
#-------------------------------------------------------------------------------
SRC +=

obj-y += /src/main.c
obj-y += /src/application.c
obj-y += /src/generic/print.c
obj-y += /src/generic/sprint.c
obj-y += /src/generic/memory.c

system-y += /src/system/syscall.c

OBJ_OBJ := $(patsubst %.c, %.o, $(obj-y))
SYSTEM_OBJ := $(patsubst %.c, %.o, $(system-y))

OBJ += $(addprefix $(BUILDDIR), $(OBJ_OBJ))
OBJ += $(addprefix $(BUILDDIR), $(SYSTEM_OBJ))

# Add include paths
CPFLAGS += -I$(TOP)/src/generic
CPFLAGS += -I$(TOP)/src/system
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
	@python3 ../../tools/app_prog.py -c /dev/ttyS3 -f build/$(TARGET_NAME).bin

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
