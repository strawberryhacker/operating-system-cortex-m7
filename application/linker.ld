/* Copyright (C) StrawberryHacker */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm","elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

MEMORY {
    ram (rwx) : ORIGIN = 0x00000000, LENGTH = 0x00010000 /* 65 KiB */
}

ENTRY(main)

SECTIONS {
    /* Application information */
    .app_info : {
        KEEP(*(.app_info))
    } > ram


    /* Code */
    .text : {
        . = ALIGN(4);
        _text_s = .;

        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.gnu.linkonce.t.*)
        *(.gnu.linkonce.r.*)

        . = ALIGN(4);
        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _text_e = .;
    } > ram


    /* Black C magic */
    .ARM.extab : {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ram

    .ARM.exidx : {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ram

    .preinit_array : {
        KEEP(*(.preinit_array))
    } > ram

    .init_array : {
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
    } > ram

    .fini_array : {
        KEEP(*(.fini_array))
		KEEP(*(SORT(.fini_array.*)))
    } > ram


    /* Global offset table */
    .got : {
        . = ALIGN(4);
        _got_s = .;
        KEEP(*(.got))
        KEEP(*(.got*))
        _got_e = .;
        . = ALIGN(4);
    } > ram


    /* Procedure linkage table */
    .plt : {
        . = ALIGN(4);
        _plt_s = .;
        KEEP(*(.got.plt))
        KEEP(*(.got.plt*))
        _plt_e = .;
        . = ALIGN(4);
    }


    /* Constant data */
    .rodata : {
        . = ALIGN(4);
        _rodata_s = .;
        KEEP(*(.rodata))
        KEEP(*(.rodata*))
        . = ALIGN(4);
        _rodata_e = .;
    } > ram


    /* Initialized data and ramfunctions */
    .data : {
        . = ALIGN(4);
        _data_s = .;
        KEEP(*(.data))
        KEEP(*(.data*))
        KEEP(*(.ramfunc))
        KEEP(*(.ramfunc*))
        . = ALIGN(4);
        _data_e = .;
    } > ram

    /* Uninitialized data */
    .bss (NOLOAD) : {
        . = ALIGN(4);
        _bss_s = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _bss_e = .;
    } > ram

    _end = .;
}
