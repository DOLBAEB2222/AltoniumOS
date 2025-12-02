bits 32

section .note.GNU-stack noalloc noexec nowrite progbits

section .multiboot
    dd 0x1BADB002
    dd 0x00
    dd -(0x1BADB002 + 0x00)

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    
    call kernel_main
    
    cli
.hang:
    hlt
    jmp .hang
