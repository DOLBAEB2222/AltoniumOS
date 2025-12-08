.PHONY: all clean build run iso iso-bios iso-uefi iso-hybrid img run-iso run-iso-uefi run-iso-hybrid run-img

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -std=c99 -ffreestanding -fno-stack-protector -Wall -Wextra -nostdlib
LDFLAGS = -m elf_i386 -T linker.ld
UEFI_CFLAGS = -fshort-wchar -ffreestanding -fno-stack-protector -fPIC -mno-red-zone -maccumulate-outgoing-args -Wall -Wextra -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol
UEFI_LDFLAGS = -nostdlib -znocombreloc -shared -Bsymbolic -L/usr/lib -L/usr/lib64 -T /usr/lib/elf_x86_64_efi.lds
UEFI_LIBS = -lgnuefi -lefi

BUILD_DIR = build
DIST_DIR = dist
UEFI_STAGE_DIR = $(DIST_DIR)/uefi_iso
HYBRID_STAGE_DIR = $(DIST_DIR)/hybrid_iso
ISO_BIOS = $(DIST_DIR)/os.iso
ISO_UEFI = $(DIST_DIR)/os-uefi.iso
ISO_HYBRID = $(DIST_DIR)/os-hybrid.iso

KERNEL_OBJS = $(BUILD_DIR)/kernel_entry.o \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/string.o \
	$(BUILD_DIR)/vga_console.o \
	$(BUILD_DIR)/keyboard.o \
	$(BUILD_DIR)/prompt.o \
	$(BUILD_DIR)/commands.o \
	$(BUILD_DIR)/nano.o \
	$(BUILD_DIR)/disk.o \
	$(BUILD_DIR)/fat12.o \
	$(BUILD_DIR)/bootlog.o \
	$(BUILD_DIR)/pci.o \
	$(BUILD_DIR)/ata_pio.o \
	$(BUILD_DIR)/ahci.o \
	$(BUILD_DIR)/nvme.o \
	$(BUILD_DIR)/storage_manager.o \
	$(BUILD_DIR)/pcspeaker.o

all: build

build: $(DIST_DIR)/kernel.elf $(DIST_DIR)/kernel.bin

dirs:
	@mkdir -p $(BUILD_DIR) $(DIST_DIR)

$(BUILD_DIR)/kernel_entry.o: kernel_entry.asm dirs
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/main.o: kernel/main.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/string.o: lib/string.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/vga_console.o: drivers/console/vga_console.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/keyboard.o: drivers/input/keyboard.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/prompt.o: shell/prompt.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/commands.o: shell/commands.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/nano.o: shell/nano.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/disk.o: disk.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/fat12.o: fat12.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/bootlog.o: kernel/bootlog.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pci.o: drivers/bus/pci.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ata_pio.o: drivers/storage/ata_pio.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/ahci.o: drivers/storage/ahci.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/nvme.o: drivers/storage/nvme.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/storage_manager.o: drivers/storage/storage_manager.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pcspeaker.o: drivers/audio/pcspeaker.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/uefi_loader.o: bootloader/uefi_loader.c dirs
	$(CC) $(UEFI_CFLAGS) -c -o $@ $<

$(DIST_DIR)/kernel.elf: $(KERNEL_OBJS) dirs
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

$(DIST_DIR)/kernel.bin: $(DIST_DIR)/kernel.elf
	objcopy -O binary $< $@

$(DIST_DIR)/kernel64.elf: kernel64_stub.c dirs
	$(CC) -m64 -ffreestanding -fno-stack-protector -nostdlib -o $@ $< -Wl,--oformat=elf64-x86-64 -Wl,-Ttext=0x100000 -Wl,--build-id=none

$(DIST_DIR)/boot.bin: boot.asm dirs
	$(AS) -f bin -o $@ $<

$(DIST_DIR)/stage2.bin: bootloader/stage2.asm dirs
	$(AS) -f bin -o $@ $<

$(DIST_DIR)/EFI/BOOT/BOOTX64.EFI: $(BUILD_DIR)/uefi_loader.o dirs
	@mkdir -p $(DIST_DIR)/EFI/BOOT
	$(LD) $(UEFI_LDFLAGS) -o $(BUILD_DIR)/uefi_loader.so /usr/lib/crt0-efi-x86_64.o $(BUILD_DIR)/uefi_loader.o $(UEFI_LIBS)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $(BUILD_DIR)/uefi_loader.so $@

$(DIST_DIR)/EFI/ALTONIUM/GRUBX64.EFI: grub-uefi.cfg $(DIST_DIR)/kernel.elf dirs
	@mkdir -p $(DIST_DIR)/EFI/ALTONIUM
	grub-mkstandalone -O x86_64-efi -o $@ "boot/grub/grub.cfg=$(CURDIR)/grub-uefi.cfg"

bootable: img

iso: iso-hybrid

iso-bios: $(ISO_BIOS)

$(ISO_BIOS): $(DIST_DIR)/kernel.elf
	@echo "Creating BIOS bootable ISO image..."
	@rm -rf $(DIST_DIR)/iso
	@mkdir -p $(DIST_DIR)/iso/boot/grub
	@cp $(DIST_DIR)/kernel.elf $(DIST_DIR)/iso/boot/
	@cp grub.cfg $(DIST_DIR)/iso/boot/grub/
	@grub-mkrescue -o $@ $(DIST_DIR)/iso 2>/dev/null
	@echo "BIOS ISO image created at $@"

iso-uefi: $(ISO_UEFI)

$(ISO_UEFI): $(DIST_DIR)/kernel.elf $(DIST_DIR)/EFI/BOOT/BOOTX64.EFI $(DIST_DIR)/EFI/ALTONIUM/GRUBX64.EFI
	@echo "Creating UEFI bootable ISO image..."
	@rm -rf $(UEFI_STAGE_DIR)
	@mkdir -p $(UEFI_STAGE_DIR)/EFI/BOOT $(UEFI_STAGE_DIR)/EFI/ALTONIUM $(UEFI_STAGE_DIR)/boot
	@cp $(DIST_DIR)/EFI/BOOT/BOOTX64.EFI $(UEFI_STAGE_DIR)/EFI/BOOT/BOOTX64.EFI
	@cp $(DIST_DIR)/EFI/ALTONIUM/GRUBX64.EFI $(UEFI_STAGE_DIR)/EFI/ALTONIUM/GRUBX64.EFI
	@cp $(DIST_DIR)/kernel.elf $(UEFI_STAGE_DIR)/boot/kernel.elf
	@cp grub-uefi.cfg $(UEFI_STAGE_DIR)/boot/grub.cfg
	@xorriso -as mkisofs -R -J -l -e EFI/BOOT/BOOTX64.EFI -no-emul-boot -isohybrid-gpt-basdat -o $@ $(UEFI_STAGE_DIR) >/dev/null
	@echo "UEFI ISO image created at $@"

iso-hybrid: $(ISO_HYBRID)

$(ISO_HYBRID): $(DIST_DIR)/kernel.elf $(DIST_DIR)/kernel64.elf $(DIST_DIR)/EFI/BOOT/BOOTX64.EFI $(DIST_DIR)/EFI/ALTONIUM/GRUBX64.EFI
	@echo "Creating hybrid BIOS/UEFI bootable ISO image..."
	@rm -rf $(HYBRID_STAGE_DIR)
	@mkdir -p $(HYBRID_STAGE_DIR)/boot/grub
	@mkdir -p $(HYBRID_STAGE_DIR)/boot/x86
	@mkdir -p $(HYBRID_STAGE_DIR)/boot/x64
	@mkdir -p $(HYBRID_STAGE_DIR)/EFI/BOOT
	@mkdir -p $(HYBRID_STAGE_DIR)/EFI/ALTONIUM
	@cp $(DIST_DIR)/kernel.elf $(HYBRID_STAGE_DIR)/boot/x86/kernel.elf
	@cp $(DIST_DIR)/kernel64.elf $(HYBRID_STAGE_DIR)/boot/x64/kernel64.elf
	@cp $(DIST_DIR)/EFI/BOOT/BOOTX64.EFI $(HYBRID_STAGE_DIR)/EFI/BOOT/BOOTX64.EFI
	@cp $(DIST_DIR)/EFI/ALTONIUM/GRUBX64.EFI $(HYBRID_STAGE_DIR)/EFI/ALTONIUM/GRUBX64.EFI
	@cp grub/hybrid.cfg $(HYBRID_STAGE_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $@ $(HYBRID_STAGE_DIR) 2>/dev/null
	@echo "Hybrid ISO image created at $@"
	@echo "This ISO boots on both BIOS and UEFI systems"

img: $(DIST_DIR)/boot.bin $(DIST_DIR)/stage2.bin $(DIST_DIR)/kernel.bin scripts/build_fat12_image.py
	@echo "Building FAT12 disk image..."
	@python3 scripts/build_fat12_image.py --boot $(DIST_DIR)/boot.bin --stage2 $(DIST_DIR)/stage2.bin --kernel $(DIST_DIR)/kernel.bin --output $(DIST_DIR)/os.img
	@echo "FAT12 disk image created at $(DIST_DIR)/os.img"

run-iso: iso-bios
	@echo "Run BIOS ISO with: qemu-system-i386 -cdrom $(ISO_BIOS)"

run-iso-uefi: iso-uefi
	@echo "Run UEFI ISO with: qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom $(ISO_UEFI)"

run-iso-hybrid: iso-hybrid
	@echo "Run hybrid ISO in BIOS mode: qemu-system-i386 -cdrom $(ISO_HYBRID)"
	@echo "Run hybrid ISO in UEFI mode: qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom $(ISO_HYBRID)"

run-img: img
	@echo "Run IMG with: qemu-system-i386 -hda $(DIST_DIR)/os.img"

run: $(DIST_DIR)/kernel.elf
	@echo "Run with: qemu-system-i386 -kernel $(DIST_DIR)/kernel.elf"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

help:
	@echo "Targets:"
	@echo "  all            - Build the kernel (default)"
	@echo "  build          - Build the kernel"
	@echo "  iso            - Create hybrid BIOS/UEFI ISO (new default)"
	@echo "  iso-bios       - Create a BIOS-only ISO image"
	@echo "  iso-uefi       - Create a UEFI-only ISO image"
	@echo "  iso-hybrid     - Create a hybrid BIOS/UEFI ISO with x86/x64 kernels"
	@echo "  img            - Create a raw disk image"
	@echo "  run-iso        - Show how to run the BIOS ISO in QEMU"
	@echo "  run-iso-uefi   - Show how to run the UEFI ISO in QEMU"
	@echo "  run-iso-hybrid - Show how to run the hybrid ISO in QEMU"
	@echo "  run-img        - Show how to run IMG in QEMU"
	@echo "  clean          - Remove build artifacts"
	@echo "  run            - Show how to run with QEMU direct kernel boot"
	@echo "  help           - Show this help message"

