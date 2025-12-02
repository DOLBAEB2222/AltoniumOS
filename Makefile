.PHONY: all clean kernel.bin iso run

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -Wall -Wextra -O2
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

BOOT_OBJ = boot/boot.o
KERNEL_OBJ = kernel/main.o

all: kernel.bin

boot/boot.o: boot/boot.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(BOOT_OBJ) $(KERNEL_OBJ)
	$(LD) $(LDFLAGS) -o $@ $^

iso: kernel.bin
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo '' >> iso/boot/grub/grub.cfg
	echo 'menuentry "MyOS" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	echo '    boot' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o myos.iso iso

run: kernel.bin
	qemu-system-i386 -kernel kernel.bin

clean:
	rm -f boot/*.o kernel/*.o kernel.bin
	rm -rf iso myos.iso
