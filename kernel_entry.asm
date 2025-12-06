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
extern memory_enable_pae

global _start
global halt_cpu
global multiboot_magic_storage
global multiboot_info_ptr_storage
global enable_pae_from_asm

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

; Enable PAE from assembly (called from C if needed)
enable_pae_from_asm:
    ; Check if we can enable PAE
    mov eax, 1
    cpuid
    test edx, 0x20        ; Check PAE bit
    jz .pae_failed
    
    ; Enable PAE in CR4
    mov eax, cr4
    or eax, 0x20          ; Set PAE bit
    mov cr4, eax
    
    ; Return success
    xor eax, eax
    ret
    
.pae_failed:
    mov eax, 1            ; Return error
    ret

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
