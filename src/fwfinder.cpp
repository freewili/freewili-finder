#include <fwfinder.hpp>
#include <fwbuilder.hpp>
#include <usbdef.hpp>

#include <expected>
#include <string>
#include <algorithm>
#include <sstream>

bool Fw::isStandAloneDevice(uint16_t vid, uint16_t pid) {
    // Check if the VID and PID match any known standalone devices
    return (vid == Fw::USB_VID_FW_RPI && pid == Fw::USB_PID_FW_RPI_2350_UF2_PID)
        || (vid == Fw::USB_VID_FW_ICS
            && (pid == Fw::USB_PID_FW_WINKY || pid == Fw::USB_PID_FW_DEFCON_2024
                || pid == Fw::USB_PID_FW_DEFCON_BADGE_2025));
}

auto Fw::getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> Fw::USBDeviceType {
    static std::map<std::tuple<uint16_t, uint16_t>, Fw::USBDeviceType> usbDeviceTypeMap = {
        { { USB_VID_FW_HUB, USB_PID_FW_HUB }, Fw::USBDeviceType::Hub },
        { { USB_VID_FW_FTDI, USB_PID_FW_FTDI }, Fw::USBDeviceType::FTDI },
        { { USB_VID_FW_RPI, USB_PID_FW_RPI_CDC_PID }, Fw::USBDeviceType::Serial },
        { { USB_VID_FW_RPI, USB_PID_FW_RPI_2040_UF2_PID }, Fw::USBDeviceType::MassStorage },
        { { USB_VID_FW_RPI, USB_PID_FW_RPI_2350_UF2_PID }, Fw::USBDeviceType::MassStorage },
        { { USB_VID_FW_ICS, USB_PID_FW_MAIN_CDC_PID }, Fw::USBDeviceType::SerialMain },
        { { USB_VID_FW_ICS, USB_PID_FW_DISPLAY_CDC_PID }, Fw::USBDeviceType::SerialDisplay },
        { { USB_VID_FW_ICS, USB_PID_FW_WINKY }, Fw::USBDeviceType::SerialMain },
        { { USB_VID_FW_ICS, USB_PID_FW_DEFCON_2024 }, Fw::USBDeviceType::SerialMain },
        { { USB_VID_FW_ICS, USB_PID_FW_DEFCON_BADGE_2025 }, Fw::USBDeviceType::SerialMain },
    };

    if (auto it = usbDeviceTypeMap.find({ vid, pid }); it != usbDeviceTypeMap.end()) {
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

auto Fw::getDeviceTypeName(Fw::DeviceType type) -> std::string {
    switch (type) {
        case Fw::DeviceType::FreeWili:
            return "Free-WiLi";
        case Fw::DeviceType::DefCon2024Badge:
            return "DefCon 2024 Badge";
        case Fw::DeviceType::DefCon2025FwBadge:
            return "DefCon 2025 Badge";
        case Fw::DeviceType::UF2:
            return "UF2";
        case Fw::DeviceType::Winky:
            return "Winky";
        default:
            return "Unknown";
    }
}

Fw::FreeWiliDevice::FreeWiliDevice(FreeWiliDevice&& other) noexcept:
    deviceType(other.deviceType),
    name(std::move(other.name)),
    serial(std::move(other.serial)),
    uniqueID(other.uniqueID),
    usbDevices(std::move(other.usbDevices)) {
    other.deviceType = Fw::DeviceType::Unknown;
    other.uniqueID = std::numeric_limits<uint64_t>::max();
}

auto Fw::FreeWiliDevice::getUSBDevices(std::vector<Fw::USBDeviceType> usbDeviceTypes) const noexcept
    -> Fw::USBDevices {
    // Helper function to see if a vector contains the DeviceType
    auto contains = [&](const Fw::USBDeviceType other_type) {
        return std::find_if(
                   usbDeviceTypes.begin(),
                   usbDeviceTypes.end(),
                   [&](const Fw::USBDeviceType& device_type) { return device_type == other_type; }
               )
            != usbDeviceTypes.end();
    };

    Fw::USBDevices foundDevices;
    std::for_each(usbDevices.begin(), usbDevices.end(), [&](const USBDevice& usb_dev) {
        if (usbDeviceTypes.empty() || contains(usb_dev.kind)) {
            foundDevices.push_back(usb_dev);
        }
    });
    return foundDevices;
}

auto Fw::FreeWiliDevice::getUSBDevices(Fw::USBDeviceType usbDeviceType) const noexcept
    -> Fw::USBDevices {
    std::vector<Fw::USBDeviceType> types = { usbDeviceType };
    return getUSBDevices(types);
}

auto Fw::FreeWiliDevice::getUSBDevices() const noexcept -> Fw::USBDevices {
    return usbDevices;
}

auto Fw::FreeWiliDevice::fromUSBDevices(const Fw::USBDevices& usbDevices)
    -> std::expected<Fw::FreeWiliDevice, std::string> {
    std::string name;
    std::string serial;
    uint64_t uniqueID = std::numeric_limits<uint64_t>::max();
    Fw::DeviceType deviceType = Fw::DeviceType::Unknown;

    // Check if this is a standalone device (e.g., Winky)
    bool isStandaloneDevice = false;
    for (const auto& device: usbDevices) {
        if (Fw::isStandAloneDevice(device.vid, device.pid)) {
            isStandaloneDevice = true;
            if (device.pid == Fw::USB_PID_FW_DEFCON_2024) {
                deviceType = Fw::DeviceType::DefCon2024Badge;
            } else if (device.pid == Fw::USB_PID_FW_DEFCON_BADGE_2025) {
                deviceType = Fw::DeviceType::DefCon2025FwBadge;
            } else if (device.pid == Fw::USB_PID_FW_WINKY) {
                deviceType = Fw::DeviceType::Winky;
            } else if (device.pid == Fw::USB_PID_FW_RPI_2040_UF2_PID
                       || device.pid == Fw::USB_PID_FW_RPI_2350_UF2_PID)
            {
                deviceType = Fw::DeviceType::UF2;
            } else {
                deviceType = Fw::DeviceType::Unknown;
            }
            name = device.name;
            serial = device.serial;
            // TODO: Parent should be the parent USB controller location.
            uniqueID = Fw::generateUniqueID(std::numeric_limits<uint32_t>::max(), device.location);
            break;
        }
    }

    // Copy the USBDevices so we can sort it
    Fw::USBDevices sortedUsbDevices = usbDevices;

    // Original hub-based device logic for FreeWili devices
    // Find the name and serial from the FTDI chip
    if (!isStandaloneDevice) {
        name = Fw::getDeviceTypeName(Fw::DeviceType::FreeWili);
        deviceType = Fw::DeviceType::FreeWili; // Default to FreeWili if not standalone
        if (auto it = std::find_if(
                usbDevices.begin(),
                usbDevices.end(),
                [&](const USBDevice& usb_dev) { return usb_dev.kind == Fw::USBDeviceType::FTDI; }
            );
            it != usbDevices.end())
        {
            serial = it->serial;
        } else {
            serial = "Unknown";
        }

        // Get the location ID of the hub
        if (auto it = std::find_if(
                sortedUsbDevices.begin(),
                sortedUsbDevices.end(),
                [&](const USBDevice& usb_dev) { return usb_dev.kind == Fw::USBDeviceType::Hub; }
            );
            it != sortedUsbDevices.end())
        {
            uniqueID = Fw::generateUniqueID(std::numeric_limits<uint32_t>::max(), it->location);
        }

        // Sort the USB devices
        std::sort(
            sortedUsbDevices.begin(),
            sortedUsbDevices.end(),
            [](const Fw::USBDevice& lhs, const Fw::USBDevice& rhs) {
                // Always put the hub at the bottom
                if (lhs.kind == Fw::USBDeviceType::Hub || rhs.kind == Fw::USBDeviceType::Hub) {
                    return false;
                }
                // order smallest to largest
                return lhs.location < rhs.location;
            }
        );
    }

    return Fw::FreeWiliDevice::builder()
        .setDeviceType(deviceType)
        .setName(name)
        .setSerial(serial)
        .setUniqueID(uniqueID)
        .setUSBDevices(std::move(sortedUsbDevices))
        .build();
}

auto Fw::generateUniqueID(uint32_t parentLocation, uint32_t deviceLocation) -> uint64_t {
    // Generate UniqueID: upper bytes = parent location, lower bytes = device location
    uint64_t uniqueID = (static_cast<uint64_t>(parentLocation) << 32) | deviceLocation;
    return uniqueID;
}

Fw::FreeWiliDeviceBuilder Fw::FreeWiliDevice::builder() {
    return Fw::FreeWiliDeviceBuilder();
}

auto Fw::FreeWiliDevice::getMainUSBDevice() const noexcept
    -> std::expected<USBDevice, std::string> {
    if (auto it = std::find_if(
            usbDevices.begin(),
            usbDevices.end(),
            [&](const USBDevice& usb_dev) {
                return usb_dev.location == static_cast<uint32_t>(Fw::USBHubPortLocation::Main)
                    && usb_dev.kind != Fw::USBDeviceType::Hub
                    && usb_dev.kind != Fw::USBDeviceType::Other;
            }
        );
        it != usbDevices.end())
    {
        return *it;
    }
    return std::unexpected("Main USB device not found");
}

auto Fw::FreeWiliDevice::getDisplayUSBDevice() const noexcept
    -> std::expected<USBDevice, std::string> {
    if (auto it = std::find_if(
            usbDevices.begin(),
            usbDevices.end(),
            [&](const USBDevice& usb_dev) {
                return usb_dev.location == static_cast<uint32_t>(Fw::USBHubPortLocation::Display)
                    && usb_dev.kind != Fw::USBDeviceType::Hub
                    && usb_dev.kind != Fw::USBDeviceType::Other;
            }
        );
        it != usbDevices.end())
    {
        return *it;
    }
    return std::unexpected("Display USB device not found");
}

auto Fw::FreeWiliDevice::getFPGAUSBDevice() const noexcept
    -> std::expected<USBDevice, std::string> {
    if (auto it = std::find_if(
            usbDevices.begin(),
            usbDevices.end(),
            [&](const USBDevice& usb_dev) {
                return usb_dev.location == static_cast<uint32_t>(Fw::USBHubPortLocation::FPGA)
                    && usb_dev.kind != Fw::USBDeviceType::Hub
                    && usb_dev.kind != Fw::USBDeviceType::Other;
            }
        );
        it != usbDevices.end())
    {
        return *it;
    }
    return std::unexpected("FPGA USB device not found");
}

auto Fw::FreeWiliDevice::getHubUSBDevice() const noexcept -> std::expected<USBDevice, std::string> {
    if (auto it = std::find_if(
            usbDevices.begin(),
            usbDevices.end(),
            [&](const USBDevice& usb_dev) { return usb_dev.kind == Fw::USBDeviceType::Hub; }
        );
        it != usbDevices.end())
    {
        return *it;
    }
    return std::unexpected("Hub USB device not found");
}
