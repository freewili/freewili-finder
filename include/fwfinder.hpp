#pragma once

#include <string>
#include <expected>
#include <cstdint>
#include <optional>
#include <vector>

namespace Fw {

// Forward declaration
class FreeWiliDeviceBuilder;

/// Location of the USB device on the FreeWili Classic hub
enum class USBHubPortLocation : uint32_t {
    Main = 1,
    Display = 2,
    FPGA = 3,
};

enum class DeviceType : uint32_t {
    Unknown = 0,
    FreeWili = 1,
    DefCon2024Badge = 2,
    DefCon2025FwBadge = 3,
    UF2 = 4,
    Winky = 5,
};

/// Type of USB Device
enum class USBDeviceType : uint32_t {
    /// USB Hub, every FreeWili has one as the parent
    Hub,
    /// Serial Port (COMx) on windows
    Serial,
    /// Serial Port MainCPU
    SerialMain,
    /// Serial Port DisplayCPU
    SerialDisplay,
    /// Mass Storage Device
    MassStorage,
    /// ESP32 USB (JTAG/RTT)
    ESP32,
    /// FTDI / FPGA
    FTDI,
    /// Some other USB device attached to the same hub
    Other,

    // Keep this at the end
    _MaxValue,
};

bool isStandAloneDevice(uint16_t vid, uint16_t pid);
auto getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> USBDeviceType;
auto getUSBDeviceTypeName(USBDeviceType type) -> std::string;
auto getDeviceTypeName(DeviceType type) -> std::string;

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
    /// USB physical location, 1 = first port
    uint32_t location;

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
    DeviceType deviceType;

    std::string name;
    std::string serial;

    // This is a unique ID based on location so we
    // can identify devices when the configuration
    // changes.
    uint64_t uniqueID;

    USBDevices usbDevices;

    // Copy constructor
    FreeWiliDevice(const FreeWiliDevice& other) = default;
    
    // Move constructor
    FreeWiliDevice(FreeWiliDevice&& other) noexcept;
    
    // Copy assignment operator
    FreeWiliDevice& operator=(const FreeWiliDevice& other) = default;

    // Get all USB devices attached to the USB Hub
    // On standalone devices like the badge this will return Main only.
    // specifying an empty vector will return all.
    auto getUSBDevices(std::vector<USBDeviceType> usbDeviceTypes) const noexcept -> USBDevices;
    auto getUSBDevices(USBDeviceType usbDeviceType) const noexcept -> USBDevices;
    auto getUSBDevices() const noexcept -> USBDevices;

    // Get the Main Processor as a USBDevice
    auto getMainUSBDevice() const noexcept -> std::expected<USBDevice, std::string>;
    // Get the Display Processor as a USBDevice
    auto getDisplayUSBDevice() const noexcept -> std::expected<USBDevice, std::string>;
    // Get the FPGA Processor as a USBDevice
    auto getFPGAUSBDevice() const noexcept -> std::expected<USBDevice, std::string>;
    // Get the Hub as a USBDevice
    auto getHubUSBDevice() const noexcept -> std::expected<USBDevice, std::string>;

    /// Helper function to create a FreeWiliDevice from USBDevices
    static auto fromUSBDevices(const USBDevices& usbDevices)
        -> std::expected<FreeWiliDevice, std::string>;

    /**
     * @brief Creates a new FreeWiliDeviceBuilder for constructing FreeWiliDevice objects.
     * 
     * @return A new FreeWiliDeviceBuilder instance for fluent construction
     */
    static FreeWiliDeviceBuilder builder();

    bool operator==(const FreeWiliDevice& other) const noexcept {
        return uniqueID == other.uniqueID;
    }

private:
    // Private constructor for builder pattern - only accessible by FreeWiliDeviceBuilder
    friend class FreeWiliDeviceBuilder;
    
    FreeWiliDevice(Fw::DeviceType type, const std::string& name, const std::string& serial, 
                uint64_t id, Fw::USBDevices&& devices) 
    : deviceType(type), name(name), serial(serial), uniqueID(id), usbDevices(std::move(devices)) {}

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

/**
 * @brief Generates a unique 64-bit ID from parent and device USB locations.
 *
 * Creates a hierarchical unique identifier by combining the parent USB controller
 * location in the upper 32 bits and the device's own USB location in the lower
 * 32 bits. This allows for consistent device identification across USB topology
 * changes while maintaining hierarchical relationships.
 *
 * @param parentLocation The USB location of the parent device (upper 32 bits)
 * @param deviceLocation The USB location of the device itself (lower 32 bits)
 * @return A 64-bit unique identifier combining both locations
 *
 * @code{.cpp}
 * // Example: Parent at location 2, device at location 5
 * uint64_t uniqueId = Fw::generateUniqueID(2, 5);
 * // Result: 0x0000000200000005
 * @endcode
 */
auto generateUniqueID(uint32_t parentLocation, uint32_t deviceLocation) -> uint64_t;

}; // namespace Fw
