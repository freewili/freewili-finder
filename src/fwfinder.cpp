#include <fwfinder.hpp>
#include <usbdef.hpp>

#include <expected>
#include <string>
#include <algorithm>
#include <sstream>

bool Fw::isStandAloneDevice(uint16_t vid, uint16_t pid) {
    // Check if the VID and PID match any known standalone devices
    return (
        vid == Fw::USB_VID_FW_ICS
        && (pid == Fw::USB_PID_FW_WINKY || pid == Fw::USB_PID_FW_DEFCON_2024
            || pid == Fw::USB_PID_FW_DEFCON_BADGE_2025)
    );
}

auto Fw::getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> Fw::USBDeviceType {
    static std::map<std::tuple<uint16_t, uint16_t>, Fw::USBDeviceType> usbDeviceTypeMap = {
        { { USB_VID_FW_HUB, USB_PID_FW_HUB }, Fw::USBDeviceType::Hub },
        { { USB_VID_FW_FTDI, USB_PID_FW_FTDI }, Fw::USBDeviceType::FTDI },
        { { USB_VID_FW_RPI, USB_PID_FW_RPI_CDC_PID }, Fw::USBDeviceType::Serial },
        { { USB_VID_FW_RPI, USB_PID_FW_RPI_UF2_PID }, Fw::USBDeviceType::MassStorage },
        { { USB_VID_FW_ICS, USB_PID_FW_MAIN_CDC_PID }, Fw::USBDeviceType::SerialMain },
        { { USB_VID_FW_ICS, USB_PID_FW_DISPLAY_CDC_PID }, Fw::USBDeviceType::SerialDisplay },
        { { USB_VID_FW_ICS, USB_PID_FW_WINKY }, Fw::USBDeviceType::SerialMain },
        { { USB_VID_FW_ICS, USB_PID_FW_DEFCON_2024 }, Fw::USBDeviceType::SerialMain },
        { { USB_VID_FW_ICS, USB_PID_FW_DEFCON_BADGE_2025 }, Fw::USBDeviceType::SerialMain },
    };

    if (auto it = usbDeviceTypeMap.find({ vid, pid }); it != usbDeviceTypeMap.end()) {
        it->second;
        return it->second;
    }
    return Fw::USBDeviceType::Other;
}

auto Fw::getUSBDeviceTypeName(Fw::USBDeviceType type) -> std::string {
    switch (type) {
        case Fw::USBDeviceType::Hub:
            return "Hub";
        case Fw::USBDeviceType::FTDI:
            return "FTDI";
        case Fw::USBDeviceType::Serial:
            return "Serial";
        case Fw::USBDeviceType::SerialMain:
            return "Serial Main";
        case Fw::USBDeviceType::SerialDisplay:
            return "Serial Display";
        case Fw::USBDeviceType::MassStorage:
            return "Mass Storage";
        case Fw::USBDeviceType::ESP32:
            return "ESP32";
        case Fw::USBDeviceType::Other:
            return "Other";
        case Fw::USBDeviceType::_MaxValue:
            return "_MaxValue";
    };
    return "Error";
}

auto Fw::FreeWiliDevice::getUSBDevices(Fw::USBDeviceType usbDeviceType) const noexcept
    -> Fw::USBDevices {
    Fw::USBDevices foundDevices;
    std::for_each(usbDevices.begin(), usbDevices.end(), [&](const USBDevice& usb_dev) {
        if (usb_dev.kind == usbDeviceType) {
            foundDevices.push_back(usb_dev);
        }
    });
    return foundDevices;
}

auto Fw::FreeWiliDevice::fromUSBDevices(const Fw::USBDevices& usbDevices)
    -> std::expected<Fw::FreeWiliDevice, std::string> {
    std::string name;
    std::string serial;
    Fw::DeviceType deviceType = Fw::DeviceType::FreeWili;

    // Check if this is a standalone device (e.g., Winky)
    bool isStandaloneDevice = false;
    const USBDevice* standaloneDevice = nullptr;

    for (const auto& device: usbDevices) {
        if (device.vid == Fw::USB_VID_FW_ICS && device.pid == Fw::USB_PID_FW_WINKY) {
            isStandaloneDevice = true;
            standaloneDevice = &device;
            deviceType = Fw::DeviceType::Winky;
            name = device.name;
            serial = device.serial;
            break;
        }
        // Check for other standalone devices like DefCon badges
        if (device.vid == Fw::USB_VID_FW_ICS
            && (device.pid == Fw::USB_PID_FW_DEFCON_2024
                || device.pid == Fw::USB_PID_FW_DEFCON_BADGE_2025))
        {
            isStandaloneDevice = true;
            standaloneDevice = &device;
            deviceType = (device.pid == Fw::USB_PID_FW_DEFCON_2024)
                ? Fw::DeviceType::DefCon2024Badge
                : Fw::DeviceType::DefCon2025FwBadge;
            name = device.name;
            serial = device.serial;
            break;
        }
    }

    if (isStandaloneDevice && standaloneDevice) {
        // For standalone devices, create a simple device structure
        Fw::USBDevices standaloneDevices = { *standaloneDevice };

        // Create a dummy hub device for compatibility
        Fw::USBDevice dummyHub = { .kind = Fw::USBDeviceType::Hub,
                                   .vid = standaloneDevice->vid,
                                   .pid = standaloneDevice->pid,
                                   .name = standaloneDevice->name,
                                   .serial = standaloneDevice->serial,
                                   .location = 0,
                                   .paths = std::nullopt,
                                   .port = std::nullopt,
                                   ._raw = standaloneDevice->_raw };

        return Fw::FreeWiliDevice {
            .deviceType = deviceType,
            .name = name,
            .serial = serial,
            .usbHub = dummyHub,
            .usbDevices = standaloneDevices,
        };
    }

    // Original hub-based device logic for FreeWili devices
    // Find the name and serial from the FTDI chip
    if (auto it = std::find_if(
            usbDevices.begin(),
            usbDevices.end(),
            [&](const USBDevice& usb_dev) { return usb_dev.kind == Fw::USBDeviceType::FTDI; }
        );
        it == usbDevices.end())
    {
        return std::unexpected("Failed to get serial number of FreeWiliDevice. Missing FTDI chip.");
    } else {
        name = it->name;
        serial = it->serial;
    }

    // Copy the USBDevices so we can sort it
    Fw::USBDevices sortedUsbDevices = usbDevices;

    // Lets seperate the USB Hub
    Fw::USBDevice usbHubDevice;
    if (auto it = std::find_if(
            sortedUsbDevices.begin(),
            sortedUsbDevices.end(),
            [&](const USBDevice& usb_dev) { return usb_dev.kind == Fw::USBDeviceType::Hub; }
        );
        it == sortedUsbDevices.end())
    {
        return std::unexpected("Failed to get the hub of the FreeWiliDevice.");
    } else {
        usbHubDevice = *it;
        sortedUsbDevices.erase(it);
    }

    // Sort the USB devices
    std::sort(
        sortedUsbDevices.begin(),
        sortedUsbDevices.end(),
        [](const Fw::USBDevice& lhs, const Fw::USBDevice& rhs) {
            // Always put the hub at the bottom
            if (rhs.kind == Fw::USBDeviceType::Hub) {
                return false;
            }
            // order smallest to largest
            return lhs.location < rhs.location;
        }
    );

    return Fw::FreeWiliDevice {
        .deviceType = Fw::DeviceType::FreeWili,
        .name = name,
        .serial = serial,
        .usbHub = usbHubDevice,
        .usbDevices = sortedUsbDevices,
    };
}
