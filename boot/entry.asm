; Bootloader entry point for multiboot-compliant bootloader
; This file implements the boot protocol and transitions to the kernel

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000003
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
extern kernel_main

global _start
align 4
_start:
    mov esp, stack_top
    
    ; Clear EFLAGS
    xor ebp, ebp
    
    ; Preserve multiboot info for kernel
    push ebx
    push eax
    
    call kernel_main
    
    cli
.hang:
    hlt
    jmp .hang
