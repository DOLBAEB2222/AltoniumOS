; Kernel entry point
[BITS 32]
[SECTION .text]

extern kernel_main

global _start
global halt_cpu

_start:
    ; Set up stack
    mov esp, 0xFFFFF
    
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
