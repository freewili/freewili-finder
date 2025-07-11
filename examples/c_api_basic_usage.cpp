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

        char name[64] = { 0 };
        char serial[64] = { 0 };

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

        printf("Device %u: %s\n", i + 1, name);
        printf("  Serial: %s\n", serial);

        // Begin USB device enumeration
        err = fw_usb_device_begin(devices[i]);
        if (err != fw_error_success) {
            printf("  Failed to begin USB device enumeration\n");
            continue;
        }

        // Get USB device count
        uint32_t usb_count = 0;
        err = fw_usb_device_count(devices[i], &usb_count);
        if (err != fw_error_success) {
            printf("  Failed to get USB device count\n");
            continue;
        }

        printf("  Total USB Devices: %u\n", usb_count);
        printf("  USB Devices:\n");

        // Enumerate USB devices
        for (uint32_t j = 0; j < usb_count; ++j) {
            char usb_name[64] = { 0 };
            char usb_serial[64] = { 0 };
            uint32_t vid = 0, pid = 0, location = 0;
            bool device_info_valid = true;

            // Get USB device strings
            err = fw_usb_device_get_str(devices[i], fw_stringtype_name, usb_name, sizeof(usb_name));
            if (err != fw_error_success) {
                printf("    USB Device %u: Failed to get name\n", j + 1);
                device_info_valid = false;
            }

            if (device_info_valid) {
                err = fw_usb_device_get_str(
                    devices[i],
                    fw_stringtype_serial,
                    usb_serial,
                    sizeof(usb_serial)
                );
                if (err != fw_error_success) {
                    printf("    USB Device %u: Failed to get serial\n", j + 1);
                    device_info_valid = false;
                }
            }

            // Get USB device integers (VID, PID, Location)
            if (device_info_valid) {
                err = fw_usb_device_get_int(devices[i], fw_inttype_vid, &vid);
                if (err != fw_error_success) {
                    printf("    USB Device %u: Failed to get VID\n", j + 1);
                    device_info_valid = false;
                }
            }

            if (device_info_valid) {
                err = fw_usb_device_get_int(devices[i], fw_inttype_pid, &pid);
                if (err != fw_error_success) {
                    printf("    USB Device %u: Failed to get PID\n", j + 1);
                    device_info_valid = false;
                }
            }

            if (device_info_valid) {
                err = fw_usb_device_get_int(devices[i], fw_inttype_location, &location);
                if (err != fw_error_success) {
                    printf("    USB Device %u: Failed to get location\n", j + 1);
                    device_info_valid = false;
                }
            }

            if (device_info_valid) {
                printf("    USB Device %u: %s\n", j + 1, usb_name);
                printf("      Serial: %s\n", usb_serial);
                printf("      VID: 0x%04X, PID: 0x%04X, Location: %u\n", vid, pid, location);

                // Try to get port information (for serial devices)
                char port[64] = { 0 };
                if (fw_usb_device_get_str(devices[i], fw_stringtype_port, port, sizeof(port))
                    == fw_error_success)
                {
                    printf("      Port: %s\n", port);
                }

                // Try to get path information (for mass storage devices)
                char path[256] = { 0 };
                if (fw_usb_device_get_str(devices[i], fw_stringtype_path, path, sizeof(path))
                    == fw_error_success)
                {
                    printf("      Path: %s\n", path);
                }
            }

            // Move to the next USB device
            if (j < usb_count - 1) {
                err = fw_usb_device_next(devices[i]);
                if (err != fw_error_success) {
                    printf("    Failed to move to next USB device\n");
                    break;
                }
            }
        }

        printf("\n");
    }

    // Free the devices
    err = fw_device_free(devices, device_count);
    if (err != fw_error_success) {
        fprintf(stderr, "Failed to free devices\n");
    }

    return 0;
}
