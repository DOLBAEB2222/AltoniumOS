/* Linker script for the kernel */

ENTRY(_start)

SECTIONS
{
    /* Load at 1MB physical address (standard for multiboot) */
    . = 0x100000;
    
    /* Kernel code and data */
    .text ALIGN(4096) : {
        *(.multiboot)
        *(.text)
    }
    
    .rodata ALIGN(4096) : {
        *(.rodata*)
    }
    
    .data ALIGN(4096) : {
        *(.data)
    }
    
    .bss ALIGN(4096) : {
        *(COMMON)
        *(.bss)
    }
    
    /* Discard symbols that aren't needed */
    /DISCARD/ : {
        *(.comment)
        *(.gnu*)
    }
}
