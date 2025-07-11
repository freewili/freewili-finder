#include "gtest/gtest.h"
#include <gtest/gtest.h>
#include <cfwfinder.h>

TEST(CFwFinderCAPI, FindAllDevices_InvalidParams) {
    fw_error_t err;
    // Null devices and count
    err = fw_device_find_all(nullptr, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Null devices
    uint32_t count = 0;
    err = fw_device_find_all(nullptr, &count, nullptr, nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Null count
    fw_freewili_device_t* devices[1] = { nullptr };
    err = fw_device_find_all(devices, nullptr, nullptr, nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

TEST(CFwFinderCAPI, DeviceIsValid_Null) {
    ASSERT_FALSE(fw_device_is_valid(nullptr));
}

TEST(CFwFinderCAPI, DeviceFree_InvalidParams) {
    // Null devices
    fw_error_t err = fw_device_free(nullptr, 1);
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Zero count
    fw_freewili_device_t* devices[1] = { nullptr };
    err = fw_device_free(devices, 0);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

TEST(CFwFinderCAPI, DeviceGetStr_InvalidParams) {
    char buf[32];
    fw_error_t err;

    // Null device
    err = fw_device_get_str(nullptr, fw_stringtype_name, buf, sizeof(buf));
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Null buffer
    fw_freewili_device_t* dummy = nullptr;
    err = fw_device_get_str(dummy, fw_stringtype_name, nullptr, sizeof(buf));
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Zero buffer size
    err = fw_device_get_str(dummy, fw_stringtype_name, buf, 0);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

TEST(CFwFinderCAPI, FindAllDevices_ActualDevice) {
    char error_message[256] = { 0 };
    uint32_t error_message_size = sizeof(error_message) / sizeof(error_message[0]);
    fw_freewili_device_t* devices[32] = { 0 };
    uint32_t device_count = 32;

    fw_error_t err = fw_device_find_all(devices, &device_count, error_message, &error_message_size);
    ASSERT_EQ(err, fw_error_success);
    if (device_count == 0) {
        GTEST_SKIP() << "No Free-Wili devices found. Skipping test.";
    }

    for (uint32_t i = 0; i < device_count; ++i) {
        ASSERT_TRUE(fw_device_is_valid(devices[i])) << "Device at index " << i << " is not valid.";

        char name[64] = { 0 };
        err = fw_device_get_str(devices[i], fw_stringtype_name, name, sizeof(name));
        ASSERT_EQ(err, fw_error_success) << "Failed to get name for device at index " << i;
        ASSERT_GT(strlen(name), 0) << "Device at index " << i << " has an empty name.";

        char serial[64] = { 0 };
        err = fw_device_get_str(devices[i], fw_stringtype_serial, serial, sizeof(serial));
        ASSERT_EQ(err, fw_error_success) << "Failed to get serial for device at index " << i;
        ASSERT_GT(strlen(serial), 0) << "Device at index " << i << " has an empty serial.";
        printf("Device %u: Name: %s, Serial: %s\n", i, name, serial);
    }

    // Free the devices
    err = fw_device_free(devices, device_count);
    ASSERT_EQ(err, fw_error_success) << "Failed to free devices.";
    for (uint32_t i = 0; i < device_count; ++i) {
        ASSERT_EQ(devices[i], nullptr) << "Device at index " << i << " was not freed properly.";
    }
}

TEST(CFwFinderCAPI, UsbDeviceEnum_InvalidParams) {
    fw_error_t err;
    // Null device
    err = fw_usb_device_begin(nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);
    err = fw_usb_device_next(nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);
    err = fw_usb_device_count(nullptr, nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);
    fw_freewili_device_t* dummy = nullptr;
    uint32_t count = 0;
    err = fw_usb_device_count(dummy, &count);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

TEST(CFwFinderCAPI, UsbDeviceGetStr_InvalidParams) {
    fw_error_t err;
    char buf[32];
    // Null device
    err = fw_usb_device_get_str(nullptr, fw_stringtype_name, buf, sizeof(buf));
    ASSERT_EQ(err, fw_error_invalid_parameter);
    // Null buffer
    fw_freewili_device_t* dummy = nullptr;
    err = fw_usb_device_get_str(dummy, fw_stringtype_name, nullptr, sizeof(buf));
    ASSERT_EQ(err, fw_error_invalid_parameter);
    // Zero buffer size
    err = fw_usb_device_get_str(dummy, fw_stringtype_name, buf, 0);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

TEST(CFwFinderCAPI, UsbDeviceGetStr_TypeString_InvalidParams) {
    fw_error_t err;
    char buf[64];

    // Test with null device
    err = fw_usb_device_get_str(nullptr, fw_stringtype_type, buf, sizeof(buf));
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Test with null buffer
    fw_freewili_device_t* dummy = nullptr;
    err = fw_usb_device_get_str(dummy, fw_stringtype_type, nullptr, sizeof(buf));
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Test with zero buffer size
    err = fw_usb_device_get_str(dummy, fw_stringtype_type, buf, 0);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

TEST(CFwFinderCAPI, UsbDeviceGetType_InvalidParams) {
    fw_error_t err;
    fw_usbdevicetype_t type;

    // Null device
    err = fw_usb_device_get_type(nullptr, &type);
    ASSERT_EQ(err, fw_error_invalid_parameter);

    // Null type pointer
    fw_freewili_device_t* dummy = nullptr;
    err = fw_usb_device_get_type(dummy, nullptr);
    ASSERT_EQ(err, fw_error_invalid_parameter);
}

// Optionally, add a test for valid USB device enumeration if devices are found
TEST(CFwFinderCAPI, UsbDeviceEnumeration_IfDeviceFound) {
    char error_message[256] = { 0 };
    uint32_t error_message_size = sizeof(error_message) / sizeof(error_message[0]);
    fw_freewili_device_t* devices[8] = { 0 };
    uint32_t device_count = 8;
    fw_error_t err = fw_device_find_all(devices, &device_count, error_message, &error_message_size);
    ASSERT_EQ(err, fw_error_success);
    if (device_count == 0) {
        GTEST_SKIP() << "No Free-Wili devices found. Skipping USB device enumeration test.";
    }
    for (uint32_t i = 0; i < device_count; ++i) {
        err = fw_usb_device_begin(devices[i]);
        ASSERT_EQ(err, fw_error_success);
        uint32_t usb_count = 0;
        err = fw_usb_device_count(devices[i], &usb_count);
        ASSERT_EQ(err, fw_error_success);
        ASSERT_GT(usb_count, 0) << "Device at index " << i << " has 0 USB device count.";
        for (uint32_t j = 0; j < usb_count; ++j) {
            char usb_name[64] = { 0 };
            err = fw_usb_device_get_str(devices[i], fw_stringtype_name, usb_name, sizeof(usb_name));
            ASSERT_EQ(err, fw_error_success);
            // Optionally print or check values
            printf("Device %u USB %u: Name: %s\n", i, j, usb_name);
            // Get the USB device serial
            char usb_serial[64] = { 0 };
            err = fw_usb_device_get_str(
                devices[i],
                fw_stringtype_serial,
                usb_serial,
                sizeof(usb_serial)
            );
            ASSERT_EQ(err, fw_error_success);
            printf("Device %u USB %u: Serial: %s\n", i, j, usb_serial);
            // Optionally, you can check other properties like VID, PID, etc.
            // For example, you can retrieve the VID and PID if needed
            uint32_t vid = 0, pid = 0, location = 0;
            err = fw_usb_device_get_int(devices[i], fw_inttype_vid, &vid);
            ASSERT_EQ(err, fw_error_success);
            err = fw_usb_device_get_int(devices[i], fw_inttype_pid, &pid);
            ASSERT_EQ(err, fw_error_success);
            err = fw_usb_device_get_int(devices[i], fw_inttype_location, &location);
            ASSERT_EQ(err, fw_error_success);
            // Print VID and PID and location
            printf(
                "Device %u USB %u: VID: 0x%04X, PID: 0x%04X Location: %d\n",
                i,
                j,
                vid,
                pid,
                location
            );

            // Test getting USB device type
            fw_usbdevicetype_t usb_type;
            err = fw_usb_device_get_type(devices[i], &usb_type);
            ASSERT_EQ(err, fw_error_success);
            ASSERT_LT(usb_type, fw_usbdevicetype__maxvalue);
            printf("Device %u USB %u: Type: %u\n", i, j, usb_type);

            // Test getting USB device type as string
            char usb_type_str[64] = { 0 };
            err = fw_usb_device_get_str(
                devices[i],
                fw_stringtype_type,
                usb_type_str,
                sizeof(usb_type_str)
            );
            ASSERT_EQ(err, fw_error_success);
            ASSERT_GT(strlen(usb_type_str), 0) << "USB device type string should not be empty";
            printf("Device %u USB %u: Type String: %s\n", i, j, usb_type_str);

            // Move to the next USB device
            err = fw_usb_device_next(devices[i]);
            ASSERT_EQ(err, fw_error_success);
        }
    }
    // Free the devices
    err = fw_device_free(devices, device_count);
    ASSERT_EQ(err, fw_error_success);
}
