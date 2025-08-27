/**
 * @file c_api_basic_usage.c
 * @brief Basic example demonstrating how to use the fwfinder C API
 *
 * This example shows how to:
 * - Find all connected FreeWili devices using the C API
 * - Display device information
 * - Enumerate USB devices for each FreeWili
 * - Handle errors properly
 */

#include <cfwfinder.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

fw_error_t print_usb_device(fw_freewili_device_t* device);

int main(void) {
    printf("FreeWili Device Finder C API Example\n");
    printf("====================================\n\n");

    char error_message[256] = { 0 };
    uint32_t error_message_size = sizeof(error_message);
    fw_freewili_device_t* devices[16] = { 0 };
    uint32_t device_count = 16;

    // Find all FreeWili devices
    fw_error_t err = fw_device_find_all(devices, &device_count, error_message, &error_message_size);

    if (err != fw_error_success) {
        fprintf(stderr, "Failed to find FreeWili devices: %s\n", error_message);
        return 1;
    }

    printf("Found %u FreeWili device(s):\n\n", device_count);

    // Iterate through each FreeWili device
    for (uint32_t i = 0; i < device_count; ++i) {
        if (!fw_device_is_valid(devices[i])) {
            printf("Device %u: Invalid device!\n", i + 1);
            continue;
        }

        char name[128] = { 0 };
        char serial[128] = { 0 };
        char type[128] = { 0 };
        bool is_standalone = false;
        uint64_t unique_id = 0;

        // Get device name and serial
        err = fw_device_get_str(devices[i], fw_stringtype_name, name, sizeof(name));
        if (err != fw_error_success) {
            printf("Device %u: Failed to get device name\n", i + 1);
            continue;
        }

        err = fw_device_get_str(devices[i], fw_stringtype_serial, serial, sizeof(serial));
        if (err != fw_error_success) {
            printf("Device %u: Failed to get device serial\n", i + 1);
            continue;
        }

        err = fw_device_get_str(devices[i], fw_stringtype_type, type, sizeof(type));
        if (err != fw_error_success) {
            printf("Device %u: Failed to get device type\n", i + 1);
            continue;
        }

        err = fw_device_is_standalone(devices[i], &is_standalone);
        if (err != fw_error_success) {
            printf("Device %u: Failed to check if device is standalone\n", i + 1);
            continue;
        }

        err = fw_device_unique_id(devices[i], &unique_id);
        if (err != fw_error_success) {
            printf("Device %u: Failed to get unique ID\n", i + 1);
            continue;
        }

        printf("Device %u: %s\n", i + 1, name);
        printf("  Serial: %s\n", serial);
        printf("  Type: %s\n", type);
        printf("  Standalone: %s\n", is_standalone ? "Yes" : "No");
        printf("  Unique ID: %" PRIu64 "\n", unique_id);

        // Get USB device count
        uint32_t usb_count = 0;
        err = fw_usb_device_count(devices[i], &usb_count);
        if (err != fw_error_success) {
            printf("  Failed to get USB device count\n");
            continue;
        }

        err = fw_usb_device_set(
            devices[i],
            fw_usbdevice_iter_main,
            error_message,
            &error_message_size
        );
        if (err != fw_error_success) {
            printf("  Failed to set USB device iterator: %s\n", error_message);
        } else {
            printf("  Main: ");
            print_usb_device(devices[i]);
        }

        err = fw_usb_device_set(
            devices[i],
            fw_usbdevice_iter_display,
            error_message,
            &error_message_size
        );
        if (err != fw_error_success) {
            printf("  Failed to set USB device iterator: %s\n", error_message);
        } else {
            printf("  Display: ");
            print_usb_device(devices[i]);
        }

        err = fw_usb_device_set(
            devices[i],
            fw_usbdevice_iter_fpga,
            error_message,
            &error_message_size
        );
        if (err != fw_error_success) {
            printf("  Failed to set USB device iterator: %s\n", error_message);
        } else {
            printf("  FPGA: ");
            print_usb_device(devices[i]);
        }

        err = fw_usb_device_set(
            devices[i],
            fw_usbdevice_iter_hub,
            error_message,
            &error_message_size
        );
        if (err != fw_error_success) {
            printf("  Failed to set USB device iterator: %s\n", error_message);
        } else {
            printf("  Hub: ");
            print_usb_device(devices[i]);
        }

        printf("  Total USB Devices: %u\n", usb_count);
        printf("  USB Devices:\n");

        // Begin USB device enumeration
        err = fw_usb_device_begin(devices[i]);
        if (err != fw_error_success) {
            printf("  Failed to begin USB device enumeration\n");
            continue;
        }
        do {
            err = print_usb_device(devices[i]);
            if (err != fw_error_success) {
                printf("    Failed to print USB device information\n");
                break;
            }
        } while ((err = fw_usb_device_next(devices[i])) == fw_error_success);
    }
    // Free the devices
    err = fw_device_free(devices, device_count);
    if (err != fw_error_success) {
        fprintf(stderr, "Failed to free devices\n");
    }

    return 0;
}

fw_error_t print_usb_device(fw_freewili_device_t* device) {
    if (device == NULL) {
        return fw_error_invalid_parameter;
    }

    char usb_name[128] = { 0 };
    char usb_serial[128] = { 0 };
    uint32_t vid = 0, pid = 0, location = 0;

    // Get USB device strings
    fw_error_t err = fw_usb_device_get_str(device, fw_stringtype_name, usb_name, sizeof(usb_name));
    if (err != fw_error_success) {
        return err;
    }

    err = fw_usb_device_get_str(device, fw_stringtype_serial, usb_serial, sizeof(usb_serial));
    if (err != fw_error_success) {
        return err;
    }

    // Get USB device integers (VID, PID, Location)
    err = fw_usb_device_get_int(device, fw_inttype_vid, &vid);
    if (err != fw_error_success) {
        return err;
    }

    err = fw_usb_device_get_int(device, fw_inttype_pid, &pid);
    if (err != fw_error_success) {
        return err;
    }

    err = fw_usb_device_get_int(device, fw_inttype_location, &location);
    if (err != fw_error_success) {
        return err;
    }

    fw_usbdevicetype_t usb_device_type;
    err = fw_usb_device_get_type(device, &usb_device_type);
    if (err != fw_error_success) {
        return err;
    }
    char usb_type_name[128] = { 0 };
    uint32_t usb_type_name_size = sizeof(usb_type_name);
    err = fw_usb_device_get_type_name(usb_device_type, usb_type_name, &usb_type_name_size);
    if (err != fw_error_success) {
        return err;
    }

    printf("    USB Device %u: %s\n", location, usb_name);
    printf("      Serial: %s\n", usb_serial);
    printf("      VID: 0x%04X, PID: 0x%04X, Location: %u\n", vid, pid, location);
    printf("      Type: %s\n", usb_type_name);

    // Try to get port information (for serial devices)
    char port[128] = { 0 };
    if (fw_usb_device_get_str(device, fw_stringtype_port, port, sizeof(port)) == fw_error_success) {
        printf("      Port: %s\n", port);
    }

    // Try to get path information (for mass storage devices)
    char path[256] = { 0 };
    if (fw_usb_device_get_str(device, fw_stringtype_path, path, sizeof(path)) == fw_error_success) {
        printf("      Path: %s\n", path);
    }

    return fw_error_success;
}
