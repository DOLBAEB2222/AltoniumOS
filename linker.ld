ENTRY(_start)

SECTIONS {
    . = 0x10000;

    .multiboot : {
        *(.multiboot)
    }

    .text ALIGN(4096) : {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4096) : {
        *(.rodata)
        *(.rodata.*)
    }

    .eh_frame ALIGN(4096) : {
        *(.eh_frame)
    }

    .data ALIGN(4096) : {
        *(.data)
        *(.data.*)
        *(.got.plt)
    }

    .bss ALIGN(4096) : {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }

    /DISCARD/ : {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
    }
}
