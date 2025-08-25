#include <cfwfinder.h>
#include <cfwfinder_internal.hpp>
#include <fwfinder.hpp>
#include <algorithm>
#include <memory>

using namespace Fw;

typedef struct fw_freewili_device_t {
    FreeWiliDevice device;

    // Iterator for USB devices.
    USBDevices::iterator usbDevicesIter = device.usbDevices.begin();

    bool operator==(const FreeWiliDevice& other) const noexcept {
        return device == other;
    }
} fw_freewili_device_t;

static std::vector<std::shared_ptr<fw_freewili_device_t>> fw_devices;

CFW_FINDER_API fw_error_t fw_device_find_all(
    fw_freewili_device_t** devices,
    uint32_t* count,
    char* const error_message,
    uint32_t* error_message_size
) {
    if (devices == nullptr || count == nullptr) {
        return fw_error_invalid_parameter;
    }

    auto found_fw_devices = find_all();
    if (!found_fw_devices.has_value()) {
        if (error_message == nullptr || error_message_size == nullptr) {
            return fw_error_invalid_parameter;
        }
        if (fixedStringCopy(error_message, error_message_size, found_fw_devices.error())
                .has_value())
        {
            return fw_error_internal_error;
        } else {
            return fw_error_memory;
        }
    }

    for (const auto& device: found_fw_devices.value()) {
        if (std::none_of(
                fw_devices.begin(),
                fw_devices.end(),
                [&device](const std::shared_ptr<fw_freewili_device_t> fw_device) {
                    return *fw_device == device;
                }
            ))
        {
            // Device not found in the existing list, add it
            fw_devices.push_back(std::make_shared<fw_freewili_device_t>(device));
        }
    }

    auto min_size = std::minmax(*count, static_cast<uint32_t>(fw_devices.size())).first;
    *count = min_size;

    for (uint32_t i = 0; i < min_size; ++i) {
        devices[i] = fw_devices[i].get();
    }
    return fw_error_success;
}

CFW_FINDER_API bool fw_device_is_valid(fw_freewili_device_t* device) {
    if (device == nullptr) {
        return false;
    }
    // Check if the device is in the list of known devices
    return std::any_of(
        fw_devices.begin(),
        fw_devices.end(),
        [&](const std::shared_ptr<fw_freewili_device_t>& fw_device) {
            return *fw_device == device->device;
        }
    );
}

CFW_FINDER_API fw_error_t fw_device_free(fw_freewili_device_t** devices, uint32_t count) {
    if (devices == nullptr || count == 0) {
        return fw_error_invalid_parameter;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (devices[i] == nullptr) {
            continue;
        }
        fw_devices.erase(
            std::remove_if(
                fw_devices.begin(),
                fw_devices.end(),
                [&](const std::shared_ptr<fw_freewili_device_t>& fw_device) {
                    return fw_device.get() == devices[i];
                }
            ),
            fw_devices.end()
        );
        devices[i] = nullptr; // Clear the pointer
    }
    return fw_error_success;
}

CFW_FINDER_API fw_error_t fw_device_get_str(
    fw_freewili_device_t* device,
    fw_stringtype_t str_type,
    char* const value,
    uint32_t value_size
) {
    if (device == nullptr || value == nullptr || value_size == 0) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    auto copy_value = [&value, &value_size](const std::string& str) {
        uint32_t size = value_size;
        if (fixedStringCopy(value, &size, str).has_value()) {
            return fw_error_success;
        } else {
            return fw_error_memory;
        }
    };

    switch (str_type) {
        case fw_stringtype_name:
            return copy_value(device->device.name);
            break;
        case fw_stringtype_serial:
            return copy_value(device->device.serial);
            break;
        case fw_stringtype_type:
            return copy_value(getDeviceTypeName(device->device.deviceType));
        case fw_stringtype_path:
        case fw_stringtype_port:
        case fw_stringtype_raw:
            return fw_error_invalid_parameter;
            break;
    }
    return fw_error_internal_error; // Should not reach here
}

CFW_FINDER_API fw_error_t
fw_device_get_type(fw_freewili_device_t* device, fw_devicetype_t* device_type) {
    if (device == nullptr || device_type == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }
    *device_type = static_cast<fw_devicetype_t>(device->device.deviceType);
    return fw_error_success;
}

CFW_FINDER_API fw_error_t
fw_device_get_type_name(fw_devicetype_t device_type, char* const name, uint32_t* name_size) {
    if (name == nullptr || name_size == nullptr) {
        return fw_error_invalid_parameter;
    }

    std::string type_name;
    switch (device_type) {
        case fw_devicetype_unknown:
            type_name = Fw::getDeviceTypeName(Fw::DeviceType::Unknown);
            break;
        case fw_devicetype_freewili:
            type_name = Fw::getDeviceTypeName(Fw::DeviceType::FreeWili);
            break;
        case fw_devicetype_defcon2024badge:
            type_name = Fw::getDeviceTypeName(Fw::DeviceType::DefCon2024Badge);
            break;
        case fw_devicetype_defcon2025fwbadge:
            type_name = Fw::getDeviceTypeName(Fw::DeviceType::DefCon2025FwBadge);
            break;
        case fw_devicetype_uf2:
            type_name = Fw::getDeviceTypeName(Fw::DeviceType::UF2);
            break;
        case fw_devicetype_winky:
            type_name = Fw::getDeviceTypeName(Fw::DeviceType::Winky);
            break;
        default:
            type_name = std::string("Unknown Device Type");
    }

    if (!fixedStringCopy(name, name_size, type_name).has_value()) {
        return fw_error_memory;
    }
    return fw_error_success;
}

CFW_FINDER_API fw_error_t
fw_device_is_standalone(fw_freewili_device_t* device, bool* is_standalone) {
    if (device == nullptr || is_standalone == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    *is_standalone = device->device.standalone;
    return fw_error_success;
}

CFW_FINDER_API fw_error_t fw_device_unique_id(fw_freewili_device_t* device, uint64_t* unique_id) {
    if (device == nullptr || unique_id == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    *unique_id = device->device.uniqueID;
    return fw_error_success;
}

CFW_FINDER_API fw_error_t fw_usb_device_begin(fw_freewili_device_t* device) {
    if (device == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    // Reset the iterator to the beginning of the USB devices
    device->usbDevicesIter = device->device.usbDevices.begin();
    return fw_error_success;
}

CFW_FINDER_API fw_error_t fw_usb_device_next(fw_freewili_device_t* device) {
    if (device == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    if (device->usbDevicesIter == device->device.usbDevices.end()) {
        return fw_error_no_more_devices; // No more USB devices to enumerate
    }

    ++device->usbDevicesIter; // Move to the next USB device
    if (device->usbDevicesIter == device->device.usbDevices.end()) {
        return fw_error_no_more_devices; // No more USB devices to enumerate
    } else {
        return fw_error_success;
    }
}

CFW_FINDER_API fw_error_t fw_usb_device_set(
    fw_freewili_device_t* device,
    fw_usbdevice_iter_set_t iter_set,
    char* const error_message,
    uint32_t* error_message_size
) {
    if (device == nullptr || error_message == nullptr || error_message_size == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    auto usb_device = std::expected<USBDevice, std::string> {};

    if (iter_set == fw_usbdevice_iter_main) {
        usb_device = device->device.getMainUSBDevice();
    } else if (iter_set == fw_usbdevice_iter_display) {
        usb_device = device->device.getDisplayUSBDevice();
    } else if (iter_set == fw_usbdevice_iter_fpga) {
        usb_device = device->device.getFPGAUSBDevice();
    } else if (iter_set == fw_usbdevice_iter_hub) {
        usb_device = device->device.getHubUSBDevice();
    } else {
        usb_device = std::unexpected("Invalid USB device iterator set");
    }

    if (usb_device.has_value()) {
        device->usbDevicesIter = std::find_if(
            device->device.usbDevices.begin(),
            device->device.usbDevices.end(),
            [&](USBDevice& d) { return d == usb_device.value(); }
        );
        if (device->usbDevicesIter == device->device.usbDevices.end()) {
            // Device not found
            if (!fixedStringCopy(
                     error_message,
                     error_message_size,
                     "fw_usb_device_set() internal error"
                )
                     .has_value())
            {
                return fw_error_memory;
            }
            return fw_error_internal_error;
        }
        return fw_error_success;
    } else {
        // Unable to get Main USB device
        if (!fixedStringCopy(error_message, error_message_size, usb_device.error()).has_value()) {
            return fw_error_memory;
        }
        return fw_error_no_more_devices;
    }
}

CFW_FINDER_API fw_error_t fw_usb_device_count(fw_freewili_device_t* device, uint32_t* count) {
    if (device == nullptr || count == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    *count = static_cast<uint32_t>(device->device.usbDevices.size());
    return fw_error_success;
}

CFW_FINDER_API fw_error_t
fw_usb_device_get_type(fw_freewili_device_t* device, fw_usbdevicetype_t* usb_device_type) {
    if (device == nullptr || usb_device_type == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    if (device->usbDevicesIter == device->device.usbDevices.end()) {
        return fw_error_no_more_devices; // No more USB devices to enumerate
    }

    const auto& usbDevice = *device->usbDevicesIter;
    *usb_device_type = static_cast<fw_usbdevicetype_t>(usbDevice.kind);
    return fw_error_success;
}

CFW_FINDER_API fw_error_t fw_usb_device_get_type_name(
    fw_usbdevicetype_t usb_device_type,
    char* const name,
    uint32_t* name_size
) {
    if (name == nullptr || name_size == nullptr) {
        return fw_error_invalid_parameter;
    }

    std::string type_name;
    switch (usb_device_type) {
        case fw_usbdevicetype_hub:
            type_name = getUSBDeviceTypeName(USBDeviceType::Hub);
            break;
        case fw_usbdevicetype_serial:
            type_name = getUSBDeviceTypeName(USBDeviceType::Serial);
            break;
        case fw_usbdevicetype_serialmain:
            type_name = getUSBDeviceTypeName(USBDeviceType::SerialMain);
            break;
        case fw_usbdevicetype_serialdisplay:
            type_name = getUSBDeviceTypeName(USBDeviceType::SerialDisplay);
            break;
        case fw_usbdevicetype_massstorage:
            type_name = getUSBDeviceTypeName(USBDeviceType::MassStorage);
            break;
        case fw_usbdevicetype_esp32:
            type_name = getUSBDeviceTypeName(USBDeviceType::ESP32);
            break;
        case fw_usbdevicetype_ftdi:
            type_name = getUSBDeviceTypeName(USBDeviceType::FTDI);
            break;
        case fw_usbdevicetype_other:
            type_name = getUSBDeviceTypeName(USBDeviceType::Other);
            break;
        default:
            type_name = std::string("Unknown USB Device Type");
    }

    if (!fixedStringCopy(name, name_size, type_name).has_value()) {
        return fw_error_memory;
    }
    return fw_error_success;
}

CFW_FINDER_API fw_error_t fw_usb_device_get_str(
    fw_freewili_device_t* device,
    fw_stringtype_t str_type,
    char* const value,
    uint32_t value_size
) {
    if (device == nullptr || value == nullptr || value_size == 0) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    if (device->usbDevicesIter == device->device.usbDevices.end()) {
        return fw_error_no_more_devices; // No more USB devices to enumerate
    }

    const auto& usbDevice = *device->usbDevicesIter;

    auto copy_value = [&value, &value_size](const std::string& str) {
        uint32_t size = value_size;
        if (fixedStringCopy(value, &size, str).has_value()) {
            return fw_error_success;
        } else {
            return fw_error_memory;
        }
    };

    switch (str_type) {
        case fw_stringtype_name:
            return copy_value(usbDevice.name);
            break;
        case fw_stringtype_serial:
            return copy_value(usbDevice.serial);
            break;
        case fw_stringtype_path: {
            if (!usbDevice.paths.has_value() || usbDevice.paths.value().empty()) {
                return fw_error_none; // No path available for this USB device
            }
            return copy_value(usbDevice.paths.value().front());
            break;
        }
        case fw_stringtype_port:
            if (!usbDevice.port.has_value()) {
                return fw_error_none; // No port available for this USB device
            }
            return copy_value(usbDevice.port.value());
            break;
        case fw_stringtype_raw:
            return copy_value(usbDevice._raw);
            break;
        case fw_stringtype_type:
            return copy_value(getUSBDeviceTypeName(usbDevice.kind));
            break;
    }
    return fw_error_internal_error; // Should not reach here
}

CFW_FINDER_API fw_error_t
fw_usb_device_get_int(fw_freewili_device_t* device, fw_inttype_t int_type, uint32_t* value) {
    if (device == nullptr || value == nullptr) {
        return fw_error_invalid_parameter;
    }

    if (!fw_device_is_valid(device)) {
        return fw_error_invalid_device;
    }

    if (device->usbDevicesIter == device->device.usbDevices.end()) {
        return fw_error_no_more_devices; // No more USB devices to enumerate
    }

    const auto& usbDevice = *device->usbDevicesIter;

    switch (int_type) {
        case fw_inttype_vid:
            *value = usbDevice.vid;
            break;
        case fw_inttype_pid:
            *value = usbDevice.pid;
            break;
        case fw_inttype_location:
            *value = usbDevice.location;
            break;
    }
    return fw_error_success;
}
