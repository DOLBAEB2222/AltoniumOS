; Stage 2 bootloader for AltoniumOS
; Loads kernel from disk using EDD (INT 13h extensions) or CHS fallback
; Placed right after boot sector (sector 1+)

[BITS 16]
[ORG 0x7E00]

stage2_start:
    ; Print "Stage2 OK" to serial port 0xE9
    mov dx, 0xE9
    mov al, 'S'
    out dx, al
    mov al, 't'
    out dx, al
    mov al, 'g'
    out dx, al
    mov al, '2'
    out dx, al
    mov al, 13
    out dx, al
    mov al, 10
    out dx, al
    
    ; Continue with the real code
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00    ; Stack below stage2 code
    sti

    ; Check for INT 13h extensions
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    int 0x13
    jc .no_edd
    cmp bx, 0xAA55
    jne .no_edd
    
    ; EDD is supported - store in memory at 0x500
    mov byte [0x500 + 1], 1
    jmp .load_kernel

.no_edd:
    ; EDD not supported, use CHS fallback
    mov byte [0x500 + 1], 0

.load_kernel:
    ; Load kernel from disk to 0x10000
    ; Kernel is at sector 2 (after boot + stage2), 64 sectors
    mov ah, 0x02
    mov al, 64            ; 64 sectors (32KB)
    mov ch, 0             ; Cylinder 0
    mov cl, 2             ; Sector 2
    mov dh, 0             ; Head 0
    mov dl, 0x80          ; Drive 0x80
    mov bx, 0x1000        ; Segment 0x1000:0x0000 = 0x10000
    mov es, bx
    xor bx, bx
    int 0x13
    jc .disk_error
    
    ; Record success status
    mov byte [0x500 + 2], 0x00
    jmp .switch_to_pm

.disk_error:
    ; Record error status
    mov byte [0x500 + 2], ah

.switch_to_pm:
    ; Initialize bootlog at 0x500 with magic
    mov word [0x500], 0x4F4F
    mov word [0x502], 0x5442
    
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

; Pad to end of sector
times 512-($-$$) db 0
