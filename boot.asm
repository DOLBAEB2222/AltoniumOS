; Stage 1 bootloader for AltoniumOS (BIOS)
; Minimal 512-byte boot sector
; Loads stage 2 bootloader from sector 1

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

    ; Setup VGA mode 3, page 0, attribute 0x07
    mov ah, 0x00
    mov al, 0x03
    int 0x10
    
    ; Set page 0
    mov ah, 0x05
    xor al, al
    int 0x10

    ; Print loading message
    mov si, msg_stage1
    call print

    ; Load stage 2 (512 bytes) from sector 1 to 0x7E00
    mov ah, 0x02
    mov al, 1             ; 1 sector (512 bytes)
    mov ch, 0             ; Cylinder 0
    mov cl, 1             ; Sector 1
    mov dh, 0             ; Head 0
    mov dl, 0x80          ; Drive 0x80
    mov bx, 0x7E0        ; Segment (0x7E0 * 16 = 0x7E00)
    mov es, bx
    xor bx, bx           ; Offset 0
    int 0x13
    
    ; Add small delay after disk INT
    mov cx, 0x1000
.delay_boot:
    loop .delay_boot
    
    jc error

    ; Print success message
    mov si, msg_stage1_ok
    call print

    ; Jump to stage 2 at 0x7E00
    jmp 0x7E00

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
    
    ; Mirror to QEMU serial (port 0xE9)
    mov dx, 0xE9
    out dx, al
    
    jmp .loop
.done:
    ret

[BITS 16]

msg_stage1: db 'Stage1: Loading AltoniumOS...', 13, 10, 0
msg_stage1_ok: db 'Stage1: OK', 13, 10, 0
err_msg: db 'Stage1: Disk error!', 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
