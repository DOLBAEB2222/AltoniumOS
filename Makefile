.PHONY: all clean build run iso iso-bios iso-uefi img run-iso run-iso-uefi run-img

AS = nasm
CC = gcc
LD = ld

ASFLAGS = -f elf32
CFLAGS = -m32 -std=c99 -ffreestanding -fno-stack-protector -Wall -Wextra -nostdlib -Iinclude
LDFLAGS = -m elf_i386 -T arch/x86/linker.ld
UEFI_CFLAGS = -fshort-wchar -ffreestanding -fno-stack-protector -fPIC -mno-red-zone -maccumulate-outgoing-args -Wall -Wextra -Iinclude -I/usr/include/efi -I/usr/include/efi/x86_64 -I/usr/include/efi/protocol
UEFI_LDFLAGS = -nostdlib -znocombreloc -shared -Bsymbolic -L/usr/lib -L/usr/lib64 -T /usr/lib/elf_x86_64_efi.lds
UEFI_LIBS = -lgnuefi -lefi

BUILD_DIR = build
DIST_DIR = dist
UEFI_STAGE_DIR = $(DIST_DIR)/uefi_iso
ISO_BIOS = $(DIST_DIR)/os.iso
ISO_UEFI = $(DIST_DIR)/os-uefi.iso

all: build

build: $(DIST_DIR)/kernel.elf $(DIST_DIR)/kernel.bin

dirs:
	@mkdir -p $(BUILD_DIR) $(DIST_DIR)

$(BUILD_DIR)/kernel_entry.o: arch/x86/kernel_entry.asm dirs
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/kernel.o: kernel/main.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/disk.o: drivers/ata/disk.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/fat12.o: filesystem/fat12/fat12.c dirs
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/uefi_loader.o: bootloader/uefi/uefi_loader.c dirs
	$(CC) $(UEFI_CFLAGS) -c -o $@ $<

$(DIST_DIR)/kernel.elf: $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/disk.o $(BUILD_DIR)/fat12.o dirs
	$(LD) $(LDFLAGS) -o $@ $(BUILD_DIR)/kernel_entry.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/disk.o $(BUILD_DIR)/fat12.o

$(DIST_DIR)/kernel.bin: $(DIST_DIR)/kernel.elf
	objcopy -O binary $< $@

$(DIST_DIR)/boot.bin: bootloader/bios/boot.asm dirs
	$(AS) -f bin -o $@ $<

$(DIST_DIR)/EFI/BOOT/BOOTX64.EFI: $(BUILD_DIR)/uefi_loader.o dirs
	@mkdir -p $(DIST_DIR)/EFI/BOOT
	$(LD) $(UEFI_LDFLAGS) -o $(BUILD_DIR)/uefi_loader.so /usr/lib/crt0-efi-x86_64.o $(BUILD_DIR)/uefi_loader.o $(UEFI_LIBS)
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $(BUILD_DIR)/uefi_loader.so $@

$(DIST_DIR)/EFI/ALTONIUM/GRUBX64.EFI: grub-uefi.cfg $(DIST_DIR)/kernel.elf dirs
	@mkdir -p $(DIST_DIR)/EFI/ALTONIUM
	grub-mkstandalone -O x86_64-efi -o $@ "boot/grub/grub.cfg=$(CURDIR)/grub-uefi.cfg"

bootable: img

iso: iso-bios iso-uefi

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

img: $(DIST_DIR)/boot.bin $(DIST_DIR)/kernel.bin scripts/build_fat12_image.py
	@echo "Building FAT12 disk image..."
	@python3 scripts/build_fat12_image.py --boot $(DIST_DIR)/boot.bin --kernel $(DIST_DIR)/kernel.bin --output $(DIST_DIR)/os.img
	@echo "FAT12 disk image created at $(DIST_DIR)/os.img"

run-iso: iso-bios
	@echo "Run BIOS ISO with: qemu-system-i386 -cdrom $(ISO_BIOS)"

run-iso-uefi: iso-uefi
	@echo "Run UEFI ISO with: qemu-system-x86_64 -bios /usr/share/OVMF/OVMF_CODE_4M.fd -cdrom $(ISO_UEFI)"

run-img: img
	@echo "Run IMG with: qemu-system-i386 -hda $(DIST_DIR)/os.img"

run: $(DIST_DIR)/kernel.elf
	@echo "Run with: qemu-system-i386 -kernel $(DIST_DIR)/kernel.elf"

clean:
	rm -rf $(BUILD_DIR) $(DIST_DIR)

help:
	@echo "Targets:"
	@echo "  all          - Build the kernel (default)"
	@echo "  build        - Build the kernel"
	@echo "  iso          - Create both BIOS and UEFI ISO images"
	@echo "  iso-bios     - Create a BIOS-only ISO image"
	@echo "  iso-uefi     - Create a UEFI-only ISO image"
	@echo "  img          - Create a raw disk image"
	@echo "  run-iso      - Show how to run the BIOS ISO in QEMU"
	@echo "  run-iso-uefi - Show how to run the UEFI ISO in QEMU"
	@echo "  run-img      - Show how to run IMG in QEMU"
	@echo "  clean        - Remove build artifacts"
	@echo "  run          - Show how to run with QEMU direct kernel boot"
	@echo "  help         - Show this help message"
