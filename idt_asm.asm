bits 32

section .note.GNU-stack noalloc noexec nowrite progbits

global idt_load
idt_load:
    mov eax, [esp+4]
    lidt [eax]
    ret
