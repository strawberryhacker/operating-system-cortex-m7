# Copyright (C) StrawberryHacker

TOP = $(shell pwd)

BUILDDIR = $(TOP)/build
TARGET_NAME = bootloader

# Compilers
CC      = arm-none-eabi-gcc
OBJDUMP = arm-none-eabi-objdump
OBJCOPY = arm-none-eabi-objcopy
ARM_ASM = arm-none-eabi-as

#-------------------------------------------------------------------------------
# Add compiler, preprocessor and linker flags here 
#-------------------------------------------------------------------------------
CPFLAGS += -I$(TOP)/src

CFLAGS += -x c -mthumb -O0 -ffunction-sections -mlong-calls -g3 -Wall
CFLAGS += -std=gnu99 -mcpu=cortex-m7 -c

LDFLAGS += -mthumb -Wl,-Map="$(BUILDDIR)/$(TARGET_NAME).map" -Wl,--start-group
LDFLAGS += -Wl,--end-group -Wl,--gc-sections -mcpu=cortex-m7 -Tlinker.ld

ASMFLAGS += -mcpu=cortex-m7

#-------------------------------------------------------------------------------
# Include source files
#-------------------------------------------------------------------------------
SRC +=

include src/Makefile

#-------------------------------------------------------------------------------
# Rules
#-------------------------------------------------------------------------------
.SECONDARY: $(OBJ)
.PHONY: all start end elf bin lss hex
all: elf lss bin hex size

elf: $(BUILDDIR)/$(TARGET_NAME).elf
bin: $(BUILDDIR)/$(TARGET_NAME).bin
lss: $(BUILDDIR)/$(TARGET_NAME).lss
hex: $(BUILDDIR)/$(TARGET_NAME).hex


size: $(BUILDDIR)/$(TARGET_NAME).elf
	@echo
	@$(CC) $(LDFLAGS) -Wl,--print-memory-usage $(OBJ) -o $(TOP)/dummy.elf
	@rm $(TOP)/dummy.elf

$(BUILDDIR)/%.bin: $(BUILDDIR)/%.elf
	@echo Building: "\033[32m" $(notdir $@) "\033[0m"
	@$(OBJCOPY) -O binary $< $@

$(BUILDDIR)/%.lss: $(BUILDDIR)/%.elf
	@echo Building: "\033[32m" $(notdir $@) "\033[0m"
	@$(OBJDUMP) -h -S $< > $@

$(BUILDDIR)/%.hex: $(BUILDDIR)/%.elf
	@echo Building: "\033[32m" $(notdir $@) "\033[0m"
	@$(OBJCOPY) -O ihex $< $@


# Linking stage
$(BUILDDIR)/%.elf: $(OBJ)
	@echo
	@echo Building: "\033[32m" $(notdir $@) "\033[0m"
	@$(CC) $(LDFLAGS) $^ -o $@

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
	@rm -r $(BUILDDIR)