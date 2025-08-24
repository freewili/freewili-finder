#include <fwbuilder.hpp>
#include <limits>

namespace Fw {

FreeWiliDeviceBuilder& FreeWiliDeviceBuilder::setDeviceType(Fw::DeviceType type) {
    deviceType_ = type;
    return *this;
}

FreeWiliDeviceBuilder& FreeWiliDeviceBuilder::setName(const std::string& name) {
    name_ = name;
    return *this;
}

FreeWiliDeviceBuilder& FreeWiliDeviceBuilder::setSerial(const std::string& serial) {
    serial_ = serial;
    return *this;
}

FreeWiliDeviceBuilder& FreeWiliDeviceBuilder::setUniqueID(uint64_t id) {
    uniqueID_ = id;
    return *this;
}

FreeWiliDeviceBuilder& FreeWiliDeviceBuilder::setUSBDevices(const Fw::USBDevices& devices) {
    usbDevices_ = devices;
    return *this;
}

FreeWiliDeviceBuilder& FreeWiliDeviceBuilder::setUSBDevices(Fw::USBDevices&& devices) {
    usbDevices_ = std::move(devices);
    return *this;
}

std::expected<FreeWiliDevice, std::string> FreeWiliDeviceBuilder::build() {
    if (auto error = validate(); error.has_value()) {
        return std::unexpected(error.value());
    }

    // All required fields are present, construct the device
    return FreeWiliDevice(
        deviceType_.value(),
        name_.value(),
        serial_.value(),
        uniqueID_.value(),
        std::move(usbDevices_.value())
    );
}

std::optional<std::string> FreeWiliDeviceBuilder::validate() const {
    if (!deviceType_.has_value()) {
        return "Device type is required but not set";
    }

    if (!name_.has_value()) {
        return "Device name is required but not set";
    }

    if (!serial_.has_value()) {
        return "Device serial is required but not set";
    }

    if (!uniqueID_.has_value()) {
        return "Device unique ID is required but not set";
    }

    if (!usbDevices_.has_value()) {
        return "USB devices list is required but not set";
    }

    // Additional validation checks
    if (name_.value().empty()) {
        return "Device name cannot be empty";
    }

    if (serial_.value().empty()) {
        return "Device serial cannot be empty";
    }

    if (deviceType_.value() == DeviceType::Unknown) {
        return "Device type cannot be Unknown";
    }

    return std::nullopt; // No validation errors
}

} // namespace Fw
