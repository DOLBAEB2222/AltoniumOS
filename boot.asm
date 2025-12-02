; Simple bootloader for OS console commands
; This is a minimal 16-bit real mode bootloader that loads and executes the kernel

[BITS 16]
[ORG 0x7C00]

start:
    ; Clear screen
    mov ax, 0x0003
    int 0x10

    ; Print welcome message
    mov si, welcome_msg
    call print_string

    ; Load kernel from disk (simplified - assume it's at sector 2)
    mov ah, 0x02          ; Read sectors
    mov al, 10            ; Read 10 sectors
    mov ch, 0             ; Cylinder 0
    mov cl, 2             ; Starting sector 2
    mov dh, 0             ; Head 0
    mov dl, 0x80          ; Drive 0x80 (first hard disk)
    mov bx, 0x1000        ; Load to 0x1000
    mov es, bx
    xor bx, bx
    int 0x13

    ; Jump to kernel
    jmp 0x1000:0x0000

print_string:
    mov ah, 0x0E          ; Print character
.loop:
    lodsb                 ; Load byte from [si]
    cmp al, 0             ; Check for null terminator
    je .done
    int 0x10              ; Print character in al
    jmp .loop
.done:
    ret

welcome_msg:
    db 'Loading OS...', 0x0D, 0x0A, 0

; Padding to 510 bytes
times 510 - ($ - $$) db 0
; Boot signature
dw 0xAA55
