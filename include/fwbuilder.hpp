#pragma once

#include <fwfinder.hpp>
#include <optional>
#include <expected>
#include <string>

namespace Fw {

/**
 * @brief Builder class for constructing FreeWiliDevice objects with validation.
 * 
 * The FreeWiliDeviceBuilder provides a fluent interface for creating FreeWiliDevice
 * objects with proper validation of required fields. All parameters are optional
 * during construction, but validation occurs during the build() call to ensure
 * all required fields are set.
 * 
 * @code{.cpp}
 * auto device = Fw::FreeWiliDevice::builder()
 *     .setDeviceType(Fw::DeviceType::FreeWili)
 *     .setName("My Device")
 *     .setSerial("ABC123")
 *     .setUniqueID(Fw::generateUniqueID(0, 0))
 *     .setUSBDevices(std::move(usbDevices))
 *     .build();
 * 
 * if (device.has_value()) {
 *     // Use device.value()
 * } else {
 *     // Handle error: device.error()
 * }
 * @endcode
 */
class FreeWiliDeviceBuilder {
private:
    std::optional<Fw::DeviceType> deviceType_;
    std::optional<std::string> name_;
    std::optional<std::string> serial_;
    std::optional<uint64_t> uniqueID_;
    std::optional<Fw::USBDevices> usbDevices_;

public:
    /**
     * @brief Default constructor for FreeWiliDeviceBuilder.
     */
    FreeWiliDeviceBuilder() = default;

    /**
     * @brief Sets the device type for the FreeWiliDevice being built.
     * 
     * @param type The type of device (FreeWili, DefCon badge, etc.)
     * @return Reference to this builder for method chaining
     */
    FreeWiliDeviceBuilder& setDeviceType(Fw::DeviceType type);

    /**
     * @brief Sets the name for the FreeWiliDevice being built.
     * 
     * @param name The human-readable name of the device
     * @return Reference to this builder for method chaining
     */
    FreeWiliDeviceBuilder& setName(const std::string& name);

    /**
     * @brief Sets the serial number for the FreeWiliDevice being built.
     * 
     * @param serial The unique serial number of the device
     * @return Reference to this builder for method chaining
     */
    FreeWiliDeviceBuilder& setSerial(const std::string& serial);

    /**
     * @brief Sets the unique ID for the FreeWiliDevice being built.
     * 
     * @param id The unique 64-bit identifier for the device
     * @return Reference to this builder for method chaining
     */
    FreeWiliDeviceBuilder& setUniqueID(uint64_t id);

    /**
     * @brief Sets the USB devices list for the FreeWiliDevice being built (copy).
     * 
     * @param devices Vector of USB devices associated with this FreeWili device
     * @return Reference to this builder for method chaining
     */
    FreeWiliDeviceBuilder& setUSBDevices(const Fw::USBDevices& devices);

    /**
     * @brief Sets the USB devices list for the FreeWiliDevice being built (move).
     * 
     * @param devices Vector of USB devices associated with this FreeWili device
     * @return Reference to this builder for method chaining
     */
    FreeWiliDeviceBuilder& setUSBDevices(Fw::USBDevices&& devices);

    /**
     * @brief Builds and validates the FreeWiliDevice.
     * 
     * Validates that all required fields have been set and constructs a
     * FreeWiliDevice object. If any required field is missing, returns
     * an error describing which field is missing.
     * 
     * @return std::expected containing either a valid FreeWiliDevice or an error message
     */
    std::expected<FreeWiliDevice, std::string> build();

private:
    /**
     * @brief Validates that all required fields have been set.
     * 
     * @return std::optional containing an error message if validation fails, 
     *         or std::nullopt if all fields are valid
     */
    std::optional<std::string> validate() const;
};

} // namespace Fw
