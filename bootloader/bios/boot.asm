; Minimal bootloader for AltoniumOS
; Loads kernel from disk and switches to protected mode

[BITS 16]
[ORG 0x7C00]

    jmp short boot_start
    nop
    db 'ALTONOS '
    dw 512                ; Bytes per sector
    db 8                  ; Sectors per cluster
    dw 128                ; Reserved sectors
    db 2                  ; Number of FATs
    dw 256                ; Root directory entries
    dw 20480              ; Total sectors (16-bit)
    db 0xF8               ; Media descriptor
    dw 8                  ; Sectors per FAT
    dw 63                 ; Sectors per track
    dw 16                 ; Number of heads
    dd 0                  ; Hidden sectors
    dd 0                  ; Total sectors (32-bit)
    db 0x80               ; Drive number
    db 0                  ; Reserved
    db 0x29               ; Boot signature
    dd 0x20240101         ; Volume ID
    db 'ALTONIUMOS '      ; Volume label (11 bytes)
    db 'FAT12   '         ; File system type

boot_start:
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Print loading message
    mov si, msg
    call print

    ; Load kernel from disk to 0x10000
    mov ah, 0x02          ; BIOS read sectors
    mov al, 64            ; Read 64 sectors (32KB)
    mov ch, 0             ; Cylinder 0
    mov cl, 2             ; Starting sector 2
    mov dh, 0             ; Head 0
    mov dl, 0x80          ; Drive 0x80
    mov bx, 0x1000        ; Segment
    mov es, bx
    xor bx, bx            ; Offset 0
    int 0x13
    jc error

    ; Enable A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Load GDT and switch to protected mode
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

error:
    mov si, err_msg
    call print
    jmp $

print:
    mov ah, 0x0E
.loop:
    lodsb
    test al, al
    jz .done
    int 0x10
    jmp .loop
.done:
    ret

[BITS 32]
pm_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x10000

[BITS 16]

; GDT
gdt:
    dq 0
    dw 0xFFFF, 0, 0x9A00, 0xCF    ; Code segment
    dw 0xFFFF, 0, 0x9200, 0xCF    ; Data segment

gdt_desc:
    dw $ - gdt - 1
    dd gdt

msg: db 'Loading AltoniumOS...', 13, 10, 0
err_msg: db 'Disk error!', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
