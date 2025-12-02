AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -nostdlib -nostdinc -fno-builtin -fno-stack-protector -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld

OBJS = boot.o idt_asm.o kernel.o console.o idt.o pic.o keyboard.o shell.o

all: kernel.bin

kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

boot.o: boot.asm
	$(AS) $(ASFLAGS) -o $@ $<

idt_asm.o: idt_asm.asm
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o kernel.bin

.PHONY: all clean
