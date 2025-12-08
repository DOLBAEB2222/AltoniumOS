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

    ; Initialize bootlog at 0x500 with magic
    mov dword [0x500], 0x424F4F54  ; "BOOT" magic
    mov byte [0x500 + 2], 0  ; boot_method (0=CHS, 1=EDD, 2=error)
    mov byte [0x500 + 3], 0  ; retry_count
    
    ; Detect memory size using INT 15h E820
    call detect_memory
    
    ; Check for INT 13h extensions
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, 0x80
    int 0x13
    jc .no_edd
    cmp bx, 0xAA55
    jne .no_edd
    
    ; EDD is supported - store in memory at 0x500
    mov byte [0x500 + 4], 1
    
    ; Check for edd=off parameter in kernel command line
    ; For now, skip this as GRUB passes command line to kernel only
    ; We'll use the full EDD path
    jmp .load_with_edd

.no_edd:
    ; EDD not supported, use CHS fallback
    mov byte [0x500 + 4], 0
    jmp .load_with_chs

.load_with_edd:
    ; Load kernel using INT 13h extensions (AH=42h)
    ; DAP structure at 0x600
    mov byte [0x600], 0x10      ; DAP size
    mov byte [0x601], 0         ; Reserved
    mov word [0x602], 64        ; Number of sectors to read
    mov dword [0x604], 0x10000  ; Transfer buffer (segment 0x1000:0x0000)
    mov dword [0x608], 2        ; Starting LBA (sector 2)
    mov dword [0x60C], 0        ; Starting LBA upper 32 bits
    
    mov ah, 0x42
    mov dl, 0x80
    mov si, 0x600
    int 0x13
    
    ; Add small delay after INT 13h
    mov cx, 0x1000
.delay_edd:
    loop .delay_edd
    
    jc .disk_error
    
    ; Record success status and boot method
    mov byte [0x500 + 6], 1  ; boot_method = EDD
    mov byte [0x500 + 5], 0  ; int13_status = success
    jmp .switch_to_pm

.load_with_chs:
    ; Load kernel from disk to 0x10000 using CHS
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
    
    ; Add small delay after INT 13h
    mov cx, 0x1000
.delay_chs:
    loop .delay_chs
    
    jc .disk_error
    
    ; Record success status and boot method
    mov byte [0x500 + 6], 0  ; boot_method = CHS
    mov byte [0x500 + 5], 0  ; int13_status = success
    jmp .switch_to_pm

.disk_error:
    ; Record error status
    mov byte [0x500 + 6], 2  ; boot_method = error
    mov byte [0x500 + 5], ah ; int13_status = error code
    inc byte [0x500 + 7]     ; increment retry count

.switch_to_pm:
    ; Enable A20 using port 0x92
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ; Also enable A20 using 8042 controller
    call enable_a20_8042
    
    ; Load GDT and switch to protected mode
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:pm_start

detect_memory:
    ; Use INT 15h E820 to get memory size
    mov edx, 0x534D4150  ; "SMAP" signature
    mov ebx, 0           ; Continuation value
    mov edi, 0x620       ; Buffer at 0x620
    
    mov dword [0x500 + 0x0C], 0  ; Initialize memory field to 0
    
.memory_loop:
    mov eax, 0xE820
    mov ecx, 24          ; Structure size
    int 0x15
    
    jc .memory_done
    
    ; Check entry type (offset 20)
    cmp dword [edi + 20], 1  ; Type 1 = usable memory
    jne .memory_next
    
    ; Add to total: [edi+8] = length
    mov eax, [edi + 8]   ; Get length in eax
    shr eax, 20          ; Convert to MB
    mov edx, [0x500 + 0x0C]  ; Get current total
    add edx, eax         ; Add this entry
    mov [0x500 + 0x0C], edx  ; Store back
    
.memory_next:
    cmp ebx, 0
    je .memory_done
    
    ; Move buffer forward for next entry
    add edi, 24
    jmp .memory_loop

.memory_done:
    ret

enable_a20_8042:
    ; Wait for input buffer to be empty
    mov ecx, 0x10000
.wait_input:
    in al, 0x64
    test al, 2
    jz .input_empty
    loop .wait_input
.input_empty:
    
    ; Send 0xD1 command to 8042
    mov al, 0xD1
    out 0x64, al
    
    ; Wait for input buffer to be empty again
    mov ecx, 0x10000
.wait_input2:
    in al, 0x64
    test al, 2
    jz .input_empty2
    loop .wait_input2
.input_empty2:
    
    ; Send 0xDF to enable A20
    mov al, 0xDF
    out 0x60, al
    
    ; Wait a bit
    mov cx, 0x1000
.delay_a20:
    loop .delay_a20
    
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

; Pad to end of sector
times 512-($-$$) db 0
