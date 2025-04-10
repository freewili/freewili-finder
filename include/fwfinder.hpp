#pragma once

#include <string>
#include <expected>
#include <memory>
#include <optional>
#include <vector>

namespace Fw {

/// Type of USB Device
enum class USBDeviceType : uint32_t {
    /// USB Hub, every FreeWili has one as the parent
    Hub,
    /// Serial Port (COMx) on windows
    Serial,
    /// Mass Storage Device
    MassStorage,
    /// ESP32 USB (JTAG/RTT)
    ESP32,
    /// FTDI / FPGA
    FTDI,
    /// Some other USB device attached to the same hub
    Other,
};

auto getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> USBDeviceType;
auto getUSBDeviceTypeName(USBDeviceType type) -> std::string;

/// Common USB device information
struct USBDevice {
    /// USB Device Type
    USBDeviceType kind;

    /// Vendor ID
    uint16_t vid;
    /// Product ID
    uint16_t pid;
    /// Name of the device
    std::string name;
    /// Serial of the device
    std::string serial;
    /// USB physical location, 0 = first port
    uint8_t location;

    /// USB Mass storage Path - This is only valid when kind is MassStorage
    std::optional<std::vector<std::string>> paths;
    /// Serial Port Path - This is only valid when kind is Serial
    std::optional<std::string> port;

    /// Internal system path of the USBDevice
    std::string _raw;
};

/// Container of all USB Devices.
typedef std::vector<USBDevice> USBDevices;

struct FreeWiliDevice {
    std::string name;
    std::string serial;

    USBDevice usbHub;
    USBDevices usbDevices;

    auto getUSBDevices(USBDeviceType usbDeviceType) const noexcept -> USBDevices;

    /// Helper function to create a FreeWiliDevice from USBDevices
    static auto fromUSBDevices(const USBDevices& usbDevices) -> std::expected<FreeWiliDevice, std::string>;
};

/// Free-Wili Devices
typedef std::vector<FreeWiliDevice> FreeWiliDevices;

/**
   * @brief Finds all Free-Wili devices attached to a host.
   *
   * @return USBDevices on success, std::string on failure.
   *
   * @code{.cpp}
   *
   * #include <fwfinder.hpp>
   *
   * // Find and display all Devices connected
   * if (auto fw_devices = Fw::find_all(); fw_devices.has_value()) {
   *    for (auto& device : fw_devices.value()) {
   *      std::println("VID: {}, PID: {}, Name: {}, Serial: {}, Kind: {}",
   *        device.vid, device.pid, device.name, device.serial, device.kind);
   *    }
   * } else {
   *    // We ran into problems, lets display the error.
   *    std::println("Failed to find devices: {}", fw_devices.error());
   * }
   */
auto find_all() noexcept -> std::expected<FreeWiliDevices, std::string>;

class Finder {
  public:
    Finder();
};

void list_all();

}; // namespace Fw

// USB
//  - VID
//  - VID
//  - Location ID
//  - Name
//  - Serial

// Serial Port
//  - VID
//  - PID
//  - Serial
//  - Location ID
//  - ComPort

// USB Mass Storage (UF2 bootloader)
//  - VID
//  - PID
//  - Location ID
//  - Path

// ESP32 USB
//  - VID
//  - PID
//  - Location ID
//  - Serial?

// Do we care about the HUB?
