#include <fwfinder.hpp>
#include <usbdef.hpp>

#include <expected>
#include <string>
#include <algorithm>
#include <sstream>

auto Fw::getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> Fw::USBDeviceType {
    if (vid == USB_VID_FW_HUB) {
        if (pid == USB_PID_FW_HUB) {
            return Fw::USBDeviceType::Hub;
        }
    } else if (vid == USB_VID_FW_FTDI) {
        if (pid == USB_PID_FW_FTDI) {
            return Fw::USBDeviceType::FTDI;
        }
    } else if (vid == USB_VID_FW_RPI) {
        if (pid == USB_PID_FW_RPI_CDC_PID) {
            return Fw::USBDeviceType::Serial;
        } else if (pid == USB_PID_FW_RPI_UF2_PID) {
            return Fw::USBDeviceType::MassStorage;
        }
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
        case Fw::USBDeviceType::MassStorage:
            return "Mass Storage";
        case Fw::USBDeviceType::ESP32:
            return "ESP32";
        case Fw::USBDeviceType::Other:
            return "Other";
    };
    return "Error";
}

auto Fw::FreeWiliDevice::getUSBDevices(Fw::USBDeviceType usbDeviceType) const noexcept -> Fw::USBDevices {
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

    // Find the name and serial from the FTDI chip
    if (auto it = std::find_if(
            usbDevices.begin(),
            usbDevices.end(),
            [&](const USBDevice& usb_dev) { return usb_dev.kind == Fw::USBDeviceType::FTDI; }
        );
        it == usbDevices.end()) {
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
        it == sortedUsbDevices.end()) {
        return std::unexpected("Failed to get the hub of the FreeWiliDevice.");
    } else {
        usbHubDevice = *it;
        sortedUsbDevices.erase(it);
    }

    // Sort the USB devices
    std::sort(sortedUsbDevices.begin(), sortedUsbDevices.end(), [](const Fw::USBDevice& lhs, const Fw::USBDevice& rhs) {
        // Always put the hub at the bottom
        if (rhs.kind == Fw::USBDeviceType::Hub) {
            return false;
        }
        // order smallest to largest
        return lhs.location < rhs.location;
    });

    return Fw::FreeWiliDevice {
        .name = name,
        .serial = serial,
        .usbHub = usbHubDevice,
        .usbDevices = sortedUsbDevices,
    };
}
