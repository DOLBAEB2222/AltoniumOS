; Kernel entry point with Multiboot header
[BITS 32]

; Multiboot header constants
MULTIBOOT_MAGIC     equ 0x1BADB002
MULTIBOOT_FLAGS     equ 0x00000003
MULTIBOOT_CHECKSUM  equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

[SECTION .multiboot]
align 4
multiboot_header:
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

[SECTION .text]

extern kernel_main

global _start
global halt_cpu
global multiboot_magic_storage
global multiboot_info_ptr_storage

_start:
    ; Preserve Multiboot registers for the kernel
    mov [multiboot_magic_storage], eax
    mov [multiboot_info_ptr_storage], ebx

    ; Set up stack (place it at 8MB for safety)
    mov esp, 0x800000
    
    ; Call kernel main function (in C)
    call kernel_main
    
    ; Should never reach here, but halt if we do
    call halt_cpu
    jmp $

; CPU halt function
halt_cpu:
    cli
    hlt
    jmp halt_cpu

section .bss
align 4
multiboot_magic_storage:
    dd 0
multiboot_info_ptr_storage:
    dd 0

; Mark stack as non-executable (fixes linker warning)
section .note.GNU-stack noalloc noexec nowrite progbits
