; boot/boot.asm - Multiboot-compliant 32-bit bootstrap
; Provides minimal initialization and transfers control to kernel_main

bits 32

; Multiboot header constants
MULTIBOOT_MAGIC        equ 0x1BADB002
MULTIBOOT_PAGE_ALIGN   equ 1 << 0
MULTIBOOT_MEMORY_INFO  equ 1 << 1
MULTIBOOT_FLAGS        equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
MULTIBOOT_CHECKSUM     equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Stack size (16 KB)
STACK_SIZE equ 0x4000

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb STACK_SIZE
stack_top:

section .data
; GDT entries
gdt_start:
    ; Null descriptor
    dq 0x0000000000000000
    
    ; Code segment descriptor (base=0, limit=0xFFFFF, 4KB granularity, 32-bit)
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x9A         ; Access byte (present, ring 0, code, executable, readable)
    db 0xCF         ; Flags (4KB granularity, 32-bit) + Limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)
    
    ; Data segment descriptor (base=0, limit=0xFFFFF, 4KB granularity, 32-bit)
    dw 0xFFFF       ; Limit (bits 0-15)
    dw 0x0000       ; Base (bits 0-15)
    db 0x00         ; Base (bits 16-23)
    db 0x92         ; Access byte (present, ring 0, data, writable)
    db 0xCF         ; Flags (4KB granularity, 32-bit) + Limit (bits 16-19)
    db 0x00         ; Base (bits 24-31)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Size
    dd gdt_start                 ; Offset

CODE_SEG equ 0x08
DATA_SEG equ 0x10

section .text
global _start
extern kernel_main

_start:
    ; Disable interrupts during initialization
    cli
    
    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Update segment registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Perform a far jump to set CS
    jmp CODE_SEG:.flush_cs
    
.flush_cs:
    ; Set up the stack
    mov esp, stack_top
    mov ebp, esp
    
    ; Zero the .bss section
    extern __bss_start
    extern __bss_end
    
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb
    
    ; Push Multiboot magic and info structure pointer for kernel_main
    ; EAX contains magic number (0x2BADB002 if loaded by Multiboot bootloader)
    ; EBX contains physical address of Multiboot information structure
    push ebx
    push eax
    
    ; Call kernel_main
    call kernel_main
    
    ; If kernel_main returns, halt the system
    cli
.hang:
    hlt
    jmp .hang

; Global size directive for the entry point
global _start:function (_start.end - _start)
.end:

; Mark stack as non-executable
section .note.GNU-stack noalloc noexec nowrite progbits
