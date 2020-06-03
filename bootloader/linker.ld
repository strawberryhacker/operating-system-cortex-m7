/* Copyright (C) StrawberryHacker */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm","elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

MEMORY {
    flash (rx)  : ORIGIN = 0x00400000, LENGTH = 0x02000  /* 8 KiB */
    sram  (rwx) : ORIGIN = 0x20400000, LENGTH = 0x5DC00 /* 384 kB */
}

STACK_SIZE = 0x500;

SECTIONS {
    .vector_table : {
        . = ALIGN(4);
        _vector_table_s = .;
        KEEP(*(.vector_table))
        . = ALIGN(4);
        _vector_table_e = .;
    } > flash

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

    .rodata : {
        . = ALIGN(4);
        _rodata_s = .;
        KEEP(*(.rodata))
        KEEP(*(.rodata*))
        . = ALIGN(4);
        _rodata_e = .;
    } > flash

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

    .bss (NOLOAD) : {
        . = ALIGN(4);
        _bss_s = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _bss_e = .;
    } > sram

    .stack (NOLOAD) : {
        . = ALIGN(8);
        _stack_s = .;
        . = . + STACK_SIZE;
        . = ALIGN(8);
        _stack_e = .;
    } > sram
}