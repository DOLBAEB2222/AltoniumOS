/* linker.ld - Linker script for the kernel */

ENTRY(_start)

SECTIONS
{
    /* Kernel will be loaded at 1 MB mark */
    . = 0x00100000;
    
    kernel_start = .;
    
    /* Multiboot header must be in the first 8 KB */
    .multiboot : ALIGN(4K)
    {
        *(.multiboot)
    }
    
    /* Read-only code and data */
    .text : ALIGN(4K)
    {
        *(.text)
        *(.text.*)
    }
    
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Read-write data */
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }
    
    /* Uninitialized data (BSS) */
    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }
    
    kernel_end = .;
}
