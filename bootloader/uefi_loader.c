#include <efi.h>
#include <efilib.h>
#include <efiprot.h>

#define ALTONIUM_GRUB_PATH   L"EFI\\ALTONIUM\\GRUBX64.EFI"
#define ALTONIUM_KERNEL_PATH L"boot\\x86\\kernel.elf"

static const CHAR16 *status_string(EFI_STATUS status) {
    switch (status) {
        case EFI_SUCCESS: return L"success";
        case EFI_LOAD_ERROR: return L"load error";
        case EFI_INVALID_PARAMETER: return L"invalid parameter";
        case EFI_UNSUPPORTED: return L"unsupported";
        case EFI_BAD_BUFFER_SIZE: return L"bad buffer size";
        case EFI_BUFFER_TOO_SMALL: return L"buffer too small";
        case EFI_NOT_READY: return L"not ready";
        case EFI_DEVICE_ERROR: return L"device error";
        case EFI_WRITE_PROTECTED: return L"write protected";
        case EFI_OUT_OF_RESOURCES: return L"out of resources";
        case EFI_NOT_FOUND: return L"not found";
        case EFI_ACCESS_DENIED: return L"access denied";
        case EFI_NO_MEDIA: return L"no media";
        case EFI_MEDIA_CHANGED: return L"media changed";
        default: return NULL;
    }
}

static VOID print_ascii(const CHAR16 *msg) {
    if (!msg || !ST || !ST->ConOut) {
        return;
    }
    uefi_call_wrapper(ST->ConOut->OutputString, 2, ST->ConOut, (CHAR16 *)msg);
}

static VOID print_newline(void) {
    print_ascii(L"\r\n");
}

static VOID print_uint(UINT64 value) {
    CHAR16 buffer[32];
    INTN pos = 31;
    buffer[pos] = L'\0';
    if (value == 0) {
        buffer[--pos] = L'0';
    } else {
        while (value > 0 && pos > 0) {
            buffer[--pos] = (CHAR16)(L'0' + (value % 10));
            value /= 10;
        }
    }
    print_ascii(&buffer[pos]);
}

static VOID print_hex(UINT64 value, UINTN width) {
    static const CHAR16 digits[] = L"0123456789ABCDEF";
    CHAR16 buffer[32];
    INTN pos = 31;
    UINTN printed = 0;
    buffer[pos] = L'\0';
    do {
        buffer[--pos] = digits[value & 0xF];
        value >>= 4;
        printed++;
    } while ((value != 0 || printed < width) && pos > 0);
    print_ascii(&buffer[pos]);
}

static VOID print_status(EFI_STATUS status, const CHAR16 *context) {
    print_ascii(L"[UEFI] ");
    if (context) {
        print_ascii(context);
    } else {
        print_ascii(L"status");
    }
    print_ascii(L": 0x");
    print_hex((UINT64)status, 8);
    const CHAR16 *name = status_string(status);
    if (name) {
        print_ascii(L" (");
        print_ascii(name);
        print_ascii(L")");
    }
    print_newline();
}

static VOID wait_for_key(void) {
    if (!ST || !ST->ConIn) {
        return;
    }
    print_ascii(L"\r\nPress any key to continue...\r\n");
    UINTN index = 0;
    EFI_INPUT_KEY key;
    uefi_call_wrapper(BS->WaitForEvent, 3, 1, &ST->ConIn->WaitForKey, &index);
    uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
}

static EFI_STATUS open_volume(EFI_HANDLE device_handle, EFI_FILE_PROTOCOL **root) {
    if (!root) {
        return EFI_INVALID_PARAMETER;
    }
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, device_handle, &fs_guid, (VOID **)&fs);
    if (EFI_ERROR(status)) {
        print_status(status, L"HandleProtocol (FileSystem) failed");
        return status;
    }
    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, root);
    if (EFI_ERROR(status)) {
        print_status(status, L"OpenVolume failed");
    }
    return status;
}

static EFI_STATUS read_file_into_buffer(EFI_FILE_PROTOCOL *root, const CHAR16 *path, VOID **buffer, UINTN *buffer_size) {
    if (!root || !path || !buffer || !buffer_size) {
        return EFI_INVALID_PARAMETER;
    }

    print_ascii(L"[UEFI] Opening file: ");
    print_ascii(path);
    print_newline();

    EFI_FILE_PROTOCOL *file = NULL;
    EFI_STATUS status = uefi_call_wrapper(root->Open, 5, root, &file, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        print_status(status, L"File open failed");
        return status;
    }

    EFI_FILE_INFO *info = NULL;
    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    UINTN info_size = 0;

    status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL) {
        print_status(status, L"GetInfo (size) failed");
        uefi_call_wrapper(file->Close, 1, file);
        return status;
    }

    info = AllocatePool(info_size);
    if (!info) {
        print_ascii(L"[UEFI] Unable to allocate file info buffer\r\n");
        uefi_call_wrapper(file->Close, 1, file);
        return EFI_OUT_OF_RESOURCES;
    }

    status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_size, info);
    if (EFI_ERROR(status)) {
        print_status(status, L"GetInfo (data) failed");
        FreePool(info);
        uefi_call_wrapper(file->Close, 1, file);
        return status;
    }

    UINTN size = (UINTN)info->FileSize;
    print_ascii(L"[UEFI] File size: ");
    print_uint(size);
    print_ascii(L" bytes\r\n");
    FreePool(info);

    VOID *data = AllocatePool(size);
    if (!data) {
        print_ascii(L"[UEFI] Unable to allocate file buffer\r\n");
        uefi_call_wrapper(file->Close, 1, file);
        return EFI_OUT_OF_RESOURCES;
    }

    UINTN read_size = size;
    status = uefi_call_wrapper(file->Read, 3, file, &read_size, data);
    uefi_call_wrapper(file->Close, 1, file);

    if (EFI_ERROR(status) || read_size != size) {
        print_status(status, L"File read failed");
        FreePool(data);
        return EFI_DEVICE_ERROR;
    }

    print_ascii(L"[UEFI] File read complete\r\n");
    *buffer = data;
    *buffer_size = size;
    return EFI_SUCCESS;
}

static EFI_STATUS verify_kernel_file(EFI_HANDLE device_handle) {
    print_ascii(L"[UEFI] Verifying kernel.elf presence\r\n");
    EFI_FILE_PROTOCOL *root = NULL;
    EFI_STATUS status = open_volume(device_handle, &root);
    if (EFI_ERROR(status)) {
        return status;
    }

    VOID *buffer = NULL;
    UINTN size = 0;
    status = read_file_into_buffer(root, ALTONIUM_KERNEL_PATH, &buffer, &size);
    uefi_call_wrapper(root->Close, 1, root);
    if (EFI_ERROR(status)) {
        print_ascii(L"[UEFI] Failed to read kernel.elf\r\n");
        return status;
    }

    FreePool(buffer);
    print_ascii(L"[UEFI] kernel.elf verified (" );
    print_uint(size);
    print_ascii(L" bytes)\r\n");
    return EFI_SUCCESS;
}

static EFI_STATUS probe_block_io(EFI_HANDLE device_handle) {
    print_ascii(L"[UEFI] Probing block I/O protocol\r\n");
    EFI_BLOCK_IO_PROTOCOL *block = NULL;
    EFI_GUID block_guid = EFI_BLOCK_IO_PROTOCOL_GUID;
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, device_handle, &block_guid, (VOID **)&block);
    if (EFI_ERROR(status)) {
        print_status(status, L"HandleProtocol (BlockIo) failed");
        return status;
    }

    if (!block->Media || !block->Media->MediaPresent) {
        print_ascii(L"[UEFI] No media present in boot device\r\n");
        return EFI_NO_MEDIA;
    }

    print_ascii(L"[UEFI] Block size: ");
    print_uint(block->Media->BlockSize);
    print_ascii(L" bytes\r\n");

    print_ascii(L"[UEFI] Last block: ");
    print_uint(block->Media->LastBlock);
    print_newline();

    VOID *buffer = AllocatePool(block->Media->BlockSize);
    if (!buffer) {
        print_ascii(L"[UEFI] Unable to allocate block buffer\r\n");
        return EFI_OUT_OF_RESOURCES;
    }

    status = uefi_call_wrapper(block->ReadBlocks, 5, block, block->Media->MediaId, 0, block->Media->BlockSize, buffer);
    FreePool(buffer);

    if (EFI_ERROR(status)) {
        print_status(status, L"ReadBlocks failed");
        return status;
    }

    print_ascii(L"[UEFI] Block device read test passed\r\n");
    return EFI_SUCCESS;
}

static EFI_STATUS dump_memory_map(void) {
    print_ascii(L"[UEFI] Capturing memory map\r\n");
    UINTN map_size = 0;
    UINTN map_key = 0;
    UINTN descriptor_size = 0;
    UINT32 descriptor_version = 0;

    EFI_STATUS status = uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, NULL, &map_key, &descriptor_size, &descriptor_version);
    if (status != EFI_BUFFER_TOO_SMALL) {
        print_status(status, L"GetMemoryMap (size) failed");
        return status;
    }

    map_size += descriptor_size * 2;
    EFI_MEMORY_DESCRIPTOR *map = AllocatePool(map_size);
    if (!map) {
        print_ascii(L"[UEFI] Unable to allocate memory map buffer\r\n");
        return EFI_OUT_OF_RESOURCES;
    }

    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &map_size, map, &map_key, &descriptor_size, &descriptor_version);
    if (EFI_ERROR(status)) {
        FreePool(map);
        print_status(status, L"GetMemoryMap (data) failed");
        return status;
    }

    UINTN entry_count = map_size / descriptor_size;
    UINT64 conventional_pages = 0;

    for (UINTN i = 0; i < entry_count; i++) {
        EFI_MEMORY_DESCRIPTOR *desc = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)map + (i * descriptor_size));
        if (desc->Type == EfiConventionalMemory) {
            conventional_pages += desc->NumberOfPages;
        }
    }

    print_ascii(L"[UEFI] Memory map entries: ");
    print_uint(entry_count);
    print_newline();

    print_ascii(L"[UEFI] Conventional memory: ");
    print_uint(conventional_pages / 256); /* pages -> MB */
    print_ascii(L" MB\r\n");

    FreePool(map);
    return EFI_SUCCESS;
}

static EFI_STATUS load_grub_image(EFI_HANDLE image_handle, EFI_HANDLE device_handle, EFI_HANDLE *loaded_image) {
    print_ascii(L"[UEFI] Loading GRUB chainloader\r\n");

    EFI_FILE_PROTOCOL *root = NULL;
    EFI_STATUS status = open_volume(device_handle, &root);
    if (EFI_ERROR(status)) {
        return status;
    }

    VOID *buffer = NULL;
    UINTN buffer_size = 0;
    status = read_file_into_buffer(root, ALTONIUM_GRUB_PATH, &buffer, &buffer_size);
    uefi_call_wrapper(root->Close, 1, root);
    if (EFI_ERROR(status)) {
        print_ascii(L"[UEFI] Unable to read GRUBX64.EFI\r\n");
        return status;
    }

    EFI_HANDLE grub_image = NULL;
    status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, image_handle, NULL, buffer, buffer_size, &grub_image);
    FreePool(buffer);

    if (EFI_ERROR(status)) {
        print_status(status, L"LoadImage failed");
        return status;
    }

    print_ascii(L"[UEFI] GRUB image loaded into memory\r\n");
    *loaded_image = grub_image;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);

    if (!system_table || !system_table->ConOut || !system_table->BootServices) {
        return EFI_INVALID_PARAMETER;
    }

    EFI_STATUS status = uefi_call_wrapper(system_table->ConOut->Reset, 2, system_table->ConOut, TRUE);
    if (EFI_ERROR(status)) {
        print_status(status, L"Console reset failed");
    }
    uefi_call_wrapper(system_table->ConOut->ClearScreen, 1, system_table->ConOut);

    print_ascii(L"======================================\r\n");
    print_ascii(L"AltoniumOS UEFI Bootstrap v1.0\r\n");
    print_ascii(L"AMD E1-7010 Compatible\r\n");
    print_ascii(L"======================================\r\n\r\n");

    print_ascii(L"[UEFI] Firmware vendor: ");
    if (system_table->FirmwareVendor) {
        print_ascii(system_table->FirmwareVendor);
    } else {
        print_ascii(L"(unknown)");
    }
    print_newline();

    print_ascii(L"[UEFI] Firmware revision: 0x");
    print_hex(system_table->FirmwareRevision, 8);
    print_newline();

    print_ascii(L"[UEFI] System table at 0x");
    print_hex((UINT64)(UINTN)system_table, 16);
    print_newline();

    print_ascii(L"[UEFI] Boot services at 0x");
    print_hex((UINT64)(UINTN)system_table->BootServices, 16);
    print_ascii(L" (rev 0x");
    print_hex(system_table->BootServices->Hdr.Revision, 8);
    print_ascii(L")\r\n");

    if (system_table->RuntimeServices) {
        print_ascii(L"[UEFI] Runtime services at 0x");
        print_hex((UINT64)(UINTN)system_table->RuntimeServices, 16);
        print_newline();
    }

    EFI_STATUS map_status = dump_memory_map();
    if (EFI_ERROR(map_status)) {
        print_status(map_status, L"Memory map capture failed");
    }

    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &loaded_image_guid, (VOID **)&loaded_image);
    if (EFI_ERROR(status)) {
        print_status(status, L"HandleProtocol (LoadedImage) failed");
        wait_for_key();
        return status;
    }

    print_ascii(L"[UEFI] Boot device handle: 0x");
    print_hex((UINT64)(UINTN)loaded_image->DeviceHandle, 16);
    print_newline();

    status = probe_block_io(loaded_image->DeviceHandle);
    if (EFI_ERROR(status)) {
        wait_for_key();
        return status;
    }

    status = verify_kernel_file(loaded_image->DeviceHandle);
    if (EFI_ERROR(status)) {
        wait_for_key();
        return status;
    }

    EFI_HANDLE grub_image = NULL;
    status = load_grub_image(image_handle, loaded_image->DeviceHandle, &grub_image);
    if (EFI_ERROR(status)) {
        print_ascii(L"\r\n[UEFI] FATAL: Unable to load GRUB bootloader\r\n");
        print_ascii(L"Verify that \EFI\\ALTONIUM\\GRUBX64.EFI exists on the USB media.\r\n");
        wait_for_key();
        return status;
    }

    print_ascii(L"\r\n[UEFI] Launching GRUB bootloader\r\n");
    print_ascii(L"[UEFI] Transferring control to GRUB\r\n");
    print_ascii(L"======================================\r\n\r\n");

    status = uefi_call_wrapper(BS->StartImage, 3, grub_image, NULL, NULL);
    if (EFI_ERROR(status)) {
        print_ascii(L"\r\n[UEFI] GRUB execution failed\r\n");
        print_status(status, L"StartImage failed");
        wait_for_key();
    }

    return status;
}
