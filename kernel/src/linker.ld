/* Copyright (C) StrawberryHacker */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm","elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

MEMORY {
    image_info      (r) : ORIGIN = 0x00404000, LENGTH = 0x000200  /* 512 B  */
    flash          (rx) : ORIGIN = 0x00404200, LENGTH = 0x1FBE00  /* 2 MiB  */
    boot_signature (rw) : ORIGIN = 0x20400000, LENGTH = 0x000020  /* 32 B   */
    sram          (rwx) : ORIGIN = 0x20400020, LENGTH = 0x05DBE0  /* 384 kB */
}

STACK_SIZE = 8192;
HEAP_SIZE = 262144;

SECTIONS {

    /* Kernel header */
    .image_info : {
        KEEP(*(.image_info))
    } > image_info


    /* Cortex-M7 vector table */
    .vector_table : {
        . = ALIGN(4);
        _vector_table_s = .;
        KEEP(*(.vector_table))
        . = ALIGN(4);
        _vector_table_e = .;
    } > flash


    /* Code */
    .text : {
        . = ALIGN(4);
        _text_s = .;

        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)

        . = ALIGN(4);
        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _text_e = .;
    } > flash


    /* Black C magic */
    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > flash

    .ARM.exidx : {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > flash

    .preinit_array : {
        KEEP(*(.preinit_array))
    } > flash

    .init_array : {
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
    } > flash

    .fini_array : {
        KEEP(*(.fini_array))
		KEEP(*(SORT(.fini_array.*)))
    } > flash


    /* Constant data */
    .rodata : {
        . = ALIGN(4);
        _rodata_s = .;
        KEEP(*(.rodata))
        KEEP(*(.rodata*))
        . = ALIGN(4);
        _rodata_e = .;
    } > flash


    /* Initialized data and ramfunctions */
    . = ALIGN(4);
    _relocate_s = LOADADDR(.data);
    .data : {
        . = ALIGN(4);
        _data_s = .;
        KEEP(*(.data))
        KEEP(*(.data*))
        KEEP(*(.ramfunc))
        KEEP(*(.ramfunc*))
        . = ALIGN(4);
        _data_e = .;
    } > sram AT > flash

    
    /* Boot signature */
    .boot_signature (NOLOAD) : {
        KEEP(*(.boot_signature))
    } > boot_signature


    /* Uninitialized data */
    .bss (NOLOAD) : {
        . = ALIGN(4);
        _bss_s = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _bss_e = .;
    } > sram


    /* Heap section */
    .heap (NOLOAD) : {
        . = ALIGN(8);
        _heap_s = .;
        . = . + HEAP_SIZE;
        . = ALIGN(8);
        _heap_e = .;
    } > sram


    /* Stack */
    .stack (NOLOAD) : {
        . = ALIGN(8);
        _stack_s = .;
        . = . + STACK_SIZE;
        . = ALIGN(8);
        _stack_e = .;
    } > sram

    _end = .;
}
