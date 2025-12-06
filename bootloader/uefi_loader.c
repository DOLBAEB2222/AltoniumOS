#include <efi.h>
#include <efilib.h>

#define ALTONIUM_GRUB_PATH L"EFI\\ALTONIUM\\GRUBX64.EFI"

static VOID print_status(EFI_STATUS status, CHAR16 *context) {
    Print(L"[UEFI] %s: %r\r\n", context, status);
}

static EFI_STATUS read_file_into_buffer(EFI_FILE_PROTOCOL *root, CHAR16 *path, VOID **buffer, UINTN *buffer_size) {
    if (!root || !path || !buffer || !buffer_size) {
        return EFI_INVALID_PARAMETER;
    }

    EFI_FILE_PROTOCOL *file = NULL;
    EFI_STATUS status = uefi_call_wrapper(root->Open, 5, root, &file, path, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) {
        return status;
    }

    EFI_GUID file_info_guid = EFI_FILE_INFO_ID;
    EFI_FILE_INFO *info = NULL;
    UINTN info_size = 0;

    status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_size, NULL);
    if (status != EFI_BUFFER_TOO_SMALL) {
        uefi_call_wrapper(file->Close, 1, file);
        return status;
    }

    info = AllocatePool(info_size);
    if (!info) {
        uefi_call_wrapper(file->Close, 1, file);
        return EFI_OUT_OF_RESOURCES;
    }

    status = uefi_call_wrapper(file->GetInfo, 4, file, &file_info_guid, &info_size, info);
    if (EFI_ERROR(status)) {
        FreePool(info);
        uefi_call_wrapper(file->Close, 1, file);
        return status;
    }

    UINTN size = (UINTN)info->FileSize;
    FreePool(info);

    VOID *data = AllocatePool(size);
    if (!data) {
        uefi_call_wrapper(file->Close, 1, file);
        return EFI_OUT_OF_RESOURCES;
    }

    UINTN read_size = size;
    status = uefi_call_wrapper(file->Read, 3, file, &read_size, data);
    uefi_call_wrapper(file->Close, 1, file);

    if (EFI_ERROR(status) || read_size != size) {
        FreePool(data);
        return EFI_DEVICE_ERROR;
    }

    *buffer = data;
    *buffer_size = size;
    return EFI_SUCCESS;
}

static EFI_STATUS load_grub_image(EFI_HANDLE image_handle, EFI_HANDLE device_handle, EFI_HANDLE *loaded_image) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs = NULL;
    EFI_GUID fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, device_handle, &fs_guid, (VOID **)&fs);
    if (EFI_ERROR(status)) {
        return status;
    }

    EFI_FILE_PROTOCOL *root = NULL;
    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root);
    if (EFI_ERROR(status)) {
        return status;
    }

    VOID *buffer = NULL;
    UINTN buffer_size = 0;
    status = read_file_into_buffer(root, ALTONIUM_GRUB_PATH, &buffer, &buffer_size);
    uefi_call_wrapper(root->Close, 1, root);
    if (EFI_ERROR(status)) {
        return status;
    }

    EFI_HANDLE grub_image = NULL;
    status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, image_handle, NULL, buffer, buffer_size, &grub_image);
    FreePool(buffer);

    if (EFI_ERROR(status)) {
        return status;
    }

    *loaded_image = grub_image;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
    InitializeLib(image_handle, system_table);
    Print(L"AltoniumOS UEFI bootstrap starting...\r\n");

    EFI_LOADED_IMAGE_PROTOCOL *loaded_image = NULL;
    EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_STATUS status = uefi_call_wrapper(BS->HandleProtocol, 3, image_handle, &loaded_image_guid, (VOID **)&loaded_image);
    if (EFI_ERROR(status)) {
        print_status(status, L"HandleProtocol (LoadedImage) failed");
        return status;
    }

    EFI_HANDLE grub_image = NULL;
    status = load_grub_image(image_handle, loaded_image->DeviceHandle, &grub_image);
    if (EFI_ERROR(status)) {
        print_status(status, L"Failed to load GRUB image");
        return status;
    }

    Print(L"Launching GRUB chainloader...\r\n");
    status = uefi_call_wrapper(BS->StartImage, 3, grub_image, NULL, NULL);
    if (EFI_ERROR(status)) {
        print_status(status, L"GRUB failed to start");
    }
    return status;
}
