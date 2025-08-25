#include <gtest/gtest.h>

#include <fwfinder.hpp>
#include <fwbuilder.hpp>
#include <usbdef.hpp>

#include <cstdio>

TEST(FwFinder, getUSBDeviceTypeFrom) {
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_HUB, Fw::USB_PID_FW_HUB),
        Fw::USBDeviceType::Hub
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_FTDI, Fw::USB_PID_FW_FTDI),
        Fw::USBDeviceType::FTDI
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_CDC_PID),
        Fw::USBDeviceType::Serial
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_2040_UF2_PID),
        Fw::USBDeviceType::MassStorage
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_2350_UF2_PID),
        Fw::USBDeviceType::MassStorage
    );

    ASSERT_EQ(Fw::getUSBDeviceTypeFrom(0, 0), Fw::USBDeviceType::Other);
}

TEST(FwFinder, getUSBDeviceTypeName) {
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::Hub).c_str(), "Hub");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::FTDI).c_str(), "FTDI");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::Serial).c_str(), "Serial");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::MassStorage).c_str(), "Mass Storage");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::Other).c_str(), "Other");
}

TEST(FwFinder, getDeviceTypeName) {
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::Unknown).c_str(), "Unknown");
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::FreeWili).c_str(), "Free-WiLi");
    ASSERT_STREQ(
        Fw::getDeviceTypeName(Fw::DeviceType::DEFCON2024Badge).c_str(),
        "DEFCON 2024 Badge"
    );
    ASSERT_STREQ(
        Fw::getDeviceTypeName(Fw::DeviceType::DEFCON2025FwBadge).c_str(),
        "DEFCON 2025 Badge"
    );
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::UF2).c_str(), "UF2");
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::Winky).c_str(), "Winky");
}

TEST(FwFinder, BasicAssertions) {
    if (auto fwDevicesResult = Fw::find_all(); fwDevicesResult.has_value()) {
        printf("Found %zu Free-Wili(s)...\n", fwDevicesResult.value().size());
        int i = 0;
        for (auto& fwDevice: fwDevicesResult.value()) {
            ++i;
            printf("%u) %s %s\n", i, fwDevice.name.c_str(), fwDevice.serial.c_str());
            // Serial Port or Mount Points
            for (auto& usbDevice: fwDevice.usbDevices) {
                printf(
                    "\t%s %s %s\n",
                    Fw::getUSBDeviceTypeName(usbDevice.kind).c_str(),
                    usbDevice.name.c_str(),
                    usbDevice.serial.c_str()
                );
                printf(
                    "\t\tVID: 0x%04X PID 0x%04X Location %u\n",
                    usbDevice.vid,
                    usbDevice.pid,
                    usbDevice.location
                );
                if (usbDevice.port.has_value()) {
                    printf("\t\tPort: %s\n", usbDevice.port.value().c_str());
                }
                if (usbDevice.paths.has_value()) {
                    printf("\t\tPath: ");
                    for (auto& path: usbDevice.paths.value()) {
                        printf("%s ", path.c_str());
                    }
                    printf("\n");
                }
                printf("\t\t%s\n", usbDevice._raw.c_str());
            }
        }
    } else {
        ASSERT_TRUE(fwDevicesResult.has_value()) << "Error: " << fwDevicesResult.error().c_str();
        //printf("Error: %s\n", fwDevicesResult.error().c_str());
    }
}

TEST(FwFinder, isStandAloneDevice) {
    ASSERT_TRUE(Fw::isStandAloneDevice(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_2350_UF2_PID));
    ASSERT_TRUE(Fw::isStandAloneDevice(Fw::USB_VID_FW_ICS, Fw::USB_PID_FW_WINKY));
    ASSERT_TRUE(Fw::isStandAloneDevice(Fw::USB_VID_FW_ICS, Fw::USB_PID_FW_DEFCON_2024));
    ASSERT_TRUE(Fw::isStandAloneDevice(Fw::USB_VID_FW_ICS, Fw::USB_PID_FW_DEFCON_BADGE_2025));

    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_2040_UF2_PID));
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_CDC_PID));
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW_HUB, Fw::USB_PID_FW_HUB));
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW_FTDI, Fw::USB_PID_FW_FTDI));
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW_ICS, Fw::USB_PID_FW_MAIN_CDC_PID));
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW_ICS, Fw::USB_PID_FW_DISPLAY_CDC_PID));

    ASSERT_FALSE(Fw::isStandAloneDevice(0, 0));
}

TEST(FwFinder, generateUniqueID) {
    // Test basic functionality - parent location in upper 32 bits, device location in lower 32 bits
    uint64_t uniqueId = Fw::generateUniqueID(0x12345678, 0x9ABCDEF0);
    uint64_t expected = (static_cast<uint64_t>(0x12345678) << 32) | 0x9ABCDEF0;
    ASSERT_EQ(uniqueId, expected);
    ASSERT_EQ(uniqueId, 0x123456789ABCDEF0ULL);

    // Test with zero values
    ASSERT_EQ(Fw::generateUniqueID(0, 0), 0ULL);

    // Test with zero parent location
    ASSERT_EQ(Fw::generateUniqueID(0, 5), 5ULL);

    // Test with zero device location
    ASSERT_EQ(Fw::generateUniqueID(3, 0), 0x0000000300000000ULL);

    // Test with small values
    ASSERT_EQ(Fw::generateUniqueID(1, 2), 0x0000000100000002ULL);
    ASSERT_EQ(Fw::generateUniqueID(2, 5), 0x0000000200000005ULL);

    // Test with maximum 32-bit values
    ASSERT_EQ(Fw::generateUniqueID(0xFFFFFFFF, 0xFFFFFFFF), 0xFFFFFFFFFFFFFFFFULL);

    // Test that parent and device locations are properly separated
    uint64_t id1 = Fw::generateUniqueID(1, 0);
    uint64_t id2 = Fw::generateUniqueID(0, 1);
    ASSERT_NE(id1, id2);
    ASSERT_EQ(id1, 0x0000000100000000ULL);
    ASSERT_EQ(id2, 0x0000000000000001ULL);

    // Test ordering - verify that higher parent locations result in higher unique IDs
    ASSERT_LT(Fw::generateUniqueID(1, 5), Fw::generateUniqueID(2, 1));
    ASSERT_LT(Fw::generateUniqueID(5, 10), Fw::generateUniqueID(6, 1));

    // Test that device location affects ordering within same parent
    ASSERT_LT(Fw::generateUniqueID(1, 1), Fw::generateUniqueID(1, 2));
    ASSERT_LT(Fw::generateUniqueID(10, 5), Fw::generateUniqueID(10, 6));

    // Test bit extraction - verify we can extract parent and device locations
    uint32_t parentLocation = 0x12345678;
    uint32_t deviceLocation = 0x9ABCDEF0;
    uint64_t combinedId = Fw::generateUniqueID(parentLocation, deviceLocation);

    uint32_t extractedParent = static_cast<uint32_t>(combinedId >> 32);
    uint32_t extractedDevice = static_cast<uint32_t>(combinedId & 0xFFFFFFFF);

    ASSERT_EQ(extractedParent, parentLocation);
    ASSERT_EQ(extractedDevice, deviceLocation);
}

/**
 * @brief Test fixture class for creating various FreeWiliDevice configurations
 *
 * This class provides factory methods to create FreeWiliDevice instances with
 * different USB device configurations for comprehensive testing.
 */
class FreeWiliDeviceTestSetup {
public:
    /**
     * @brief Creates a USB Hub device
     */
    static Fw::USBDevice createHubDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::Hub,
                               .vid = Fw::USB_VID_FW_HUB,
                               .pid = Fw::USB_PID_FW_HUB,
                               .name = "Free-WiLi Hub",
                               .serial = "HUB001",
                               .location = 0, // Hub is at the root level, not on a specific port
                               .paths = std::nullopt,
                               .port = std::nullopt,
                               ._raw = "/sys/devices/hub" };
    }

    /**
     * @brief Creates an FTDI device (FPGA)
     */
    static Fw::USBDevice createFTDIDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::FTDI,
                               .vid = Fw::USB_VID_FW_FTDI,
                               .pid = Fw::USB_PID_FW_FTDI,
                               .name = "Free-WiLi FTDI",
                               .serial = "FTDI001",
                               .location = static_cast<uint32_t>(Fw::USBHubPortLocation::FPGA),
                               .paths = std::nullopt,
                               .port = std::nullopt,
                               ._raw = "/sys/devices/ftdi" };
    }

    /**
     * @brief Creates a Main Serial device
     */
    static Fw::USBDevice createMainSerialDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::SerialMain,
                               .vid = Fw::USB_VID_FW_ICS,
                               .pid = Fw::USB_PID_FW_MAIN_CDC_PID,
                               .name = "Free-WiLi Main Serial",
                               .serial = "MAIN001",
                               .location = static_cast<uint32_t>(Fw::USBHubPortLocation::Main),
                               .paths = std::nullopt,
                               .port = std::string("/dev/ttyACM0"),
                               ._raw = "/sys/devices/main_serial" };
    }

    /**
     * @brief Creates a Display Serial device
     */
    static Fw::USBDevice createDisplaySerialDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::SerialDisplay,
                               .vid = Fw::USB_VID_FW_ICS,
                               .pid = Fw::USB_PID_FW_DISPLAY_CDC_PID,
                               .name = "Free-WiLi Display Serial",
                               .serial = "DISP001",
                               .location = static_cast<uint32_t>(Fw::USBHubPortLocation::Display),
                               .paths = std::nullopt,
                               .port = std::string("/dev/ttyACM1"),
                               ._raw = "/sys/devices/display_serial" };
    }

    /**
     * @brief Creates a Main Mass Storage device
     */
    static Fw::USBDevice createMainMassStorageDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::MassStorage,
                               .vid = Fw::USB_VID_FW_RPI,
                               .pid = Fw::USB_PID_FW_RPI_2040_UF2_PID,
                               .name = "Free-WiLi Main Storage",
                               .serial = "MASS001",
                               .location = static_cast<uint32_t>(Fw::USBHubPortLocation::Main),
                               .paths = std::vector<std::string> { "/mnt/freewili_main" },
                               .port = std::nullopt,
                               ._raw = "/sys/devices/main_storage" };
    }

    /**
     * @brief Creates a Display Mass Storage device
     */
    static Fw::USBDevice createDisplayMassStorageDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::MassStorage,
                               .vid = Fw::USB_VID_FW_RPI,
                               .pid = Fw::USB_PID_FW_RPI_2040_UF2_PID,
                               .name = "Free-WiLi Display Storage",
                               .serial = "MASS002",
                               .location = static_cast<uint32_t>(Fw::USBHubPortLocation::Display),
                               .paths = std::vector<std::string> { "/mnt/freewili_display" },
                               .port = std::nullopt,
                               ._raw = "/sys/devices/display_storage" };
    }

    /**
     * @brief Creates a FreeWiliDevice with Hub, FTDI, and Serial devices
     */
    static std::expected<Fw::FreeWiliDevice, std::string> createDeviceWithSerials() {
        Fw::USBDevices usbDevices = { createHubDevice(),
                                      createFTDIDevice(),
                                      createMainSerialDevice(),
                                      createDisplaySerialDevice() };

        return Fw::FreeWiliDevice::builder()
            .setDeviceType(Fw::DeviceType::FreeWili)
            .setName("Test FreeWili with Serials")
            .setSerial("FTDI001") // Use FTDI serial as device serial
            .setUniqueID(Fw::generateUniqueID(1, 1))
            .setUSBDevices(std::move(usbDevices))
            .build();
    }

    /**
     * @brief Creates a FreeWiliDevice with Hub, FTDI, and Mass Storage devices
     */
    static std::expected<Fw::FreeWiliDevice, std::string> createDeviceWithMassStorage() {
        Fw::USBDevices usbDevices = { createHubDevice(),
                                      createFTDIDevice(),
                                      createMainMassStorageDevice(),
                                      createDisplayMassStorageDevice() };

        return Fw::FreeWiliDevice::builder()
            .setDeviceType(Fw::DeviceType::FreeWili)
            .setName("Test FreeWili with Mass Storage")
            .setSerial("FTDI001")
            .setUniqueID(Fw::generateUniqueID(1, 2))
            .setUSBDevices(std::move(usbDevices))
            .build();
    }

    /**
     * @brief Creates a FreeWiliDevice missing FTDI with Mass Storage devices
     */
    static std::expected<Fw::FreeWiliDevice, std::string> createDeviceWithoutFTDIMassStorage() {
        Fw::USBDevices usbDevices = { createHubDevice(),
                                      createMainMassStorageDevice(),
                                      createDisplayMassStorageDevice() };

        return Fw::FreeWiliDevice::builder()
            .setDeviceType(Fw::DeviceType::FreeWili)
            .setName("Test FreeWili without FTDI - Mass Storage")
            .setSerial("HUB001") // Use Hub serial since no FTDI
            .setUniqueID(Fw::generateUniqueID(1, 3))
            .setUSBDevices(std::move(usbDevices))
            .build();
    }

    /**
     * @brief Creates a FreeWiliDevice missing FTDI with Serial devices
     */
    static std::expected<Fw::FreeWiliDevice, std::string> createDeviceWithoutFTDISerials() {
        Fw::USBDevices usbDevices = { createHubDevice(),
                                      createMainSerialDevice(),
                                      createDisplaySerialDevice() };

        return Fw::FreeWiliDevice::builder()
            .setDeviceType(Fw::DeviceType::FreeWili)
            .setName("Test FreeWili without FTDI - Serials")
            .setSerial("HUB001") // Use Hub serial since no FTDI
            .setUniqueID(Fw::generateUniqueID(1, 4))
            .setUSBDevices(std::move(usbDevices))
            .build();
    }

    /**
     * @brief Creates a minimal FreeWiliDevice with only Hub and FTDI
     */
    static std::expected<Fw::FreeWiliDevice, std::string> createMinimalDevice() {
        Fw::USBDevices usbDevices = { createHubDevice(), createFTDIDevice() };

        return Fw::FreeWiliDevice::builder()
            .setDeviceType(Fw::DeviceType::FreeWili)
            .setName("Test Minimal FreeWili")
            .setSerial("FTDI001")
            .setUniqueID(Fw::generateUniqueID(1, 5))
            .setUSBDevices(std::move(usbDevices))
            .build();
    }
};

/**
 * @brief Test fixture for FreeWiliDevice method testing
 */
class FreeWiliDeviceMethodTest: public ::testing::Test {
protected:
    void SetUp() override {
        // Create all test devices
        auto deviceWithSerials = FreeWiliDeviceTestSetup::createDeviceWithSerials();
        ASSERT_TRUE(deviceWithSerials.has_value())
            << "Failed to create device with serials: " << deviceWithSerials.error();
        deviceWithSerials_ =
            std::make_unique<Fw::FreeWiliDevice>(std::move(deviceWithSerials.value()));

        auto deviceWithMassStorage = FreeWiliDeviceTestSetup::createDeviceWithMassStorage();
        ASSERT_TRUE(deviceWithMassStorage.has_value())
            << "Failed to create device with mass storage: " << deviceWithMassStorage.error();
        deviceWithMassStorage_ =
            std::make_unique<Fw::FreeWiliDevice>(std::move(deviceWithMassStorage.value()));

        auto deviceWithoutFTDIMassStorage =
            FreeWiliDeviceTestSetup::createDeviceWithoutFTDIMassStorage();
        ASSERT_TRUE(deviceWithoutFTDIMassStorage.has_value())
            << "Failed to create device without FTDI with mass storage: "
            << deviceWithoutFTDIMassStorage.error();
        deviceWithoutFTDIMassStorage_ =
            std::make_unique<Fw::FreeWiliDevice>(std::move(deviceWithoutFTDIMassStorage.value()));

        auto deviceWithoutFTDISerials = FreeWiliDeviceTestSetup::createDeviceWithoutFTDISerials();
        ASSERT_TRUE(deviceWithoutFTDISerials.has_value())
            << "Failed to create device without FTDI with serials: "
            << deviceWithoutFTDISerials.error();
        deviceWithoutFTDISerials_ =
            std::make_unique<Fw::FreeWiliDevice>(std::move(deviceWithoutFTDISerials.value()));

        auto minimalDevice = FreeWiliDeviceTestSetup::createMinimalDevice();
        ASSERT_TRUE(minimalDevice.has_value())
            << "Failed to create minimal device: " << minimalDevice.error();
        minimalDevice_ = std::make_unique<Fw::FreeWiliDevice>(std::move(minimalDevice.value()));
    }

    std::unique_ptr<Fw::FreeWiliDevice> deviceWithSerials_;
    std::unique_ptr<Fw::FreeWiliDevice> deviceWithMassStorage_;
    std::unique_ptr<Fw::FreeWiliDevice> deviceWithoutFTDIMassStorage_;
    std::unique_ptr<Fw::FreeWiliDevice> deviceWithoutFTDISerials_;
    std::unique_ptr<Fw::FreeWiliDevice> minimalDevice_;
};

// Tests for getMainUSBDevice
TEST_F(FreeWiliDeviceMethodTest, GetMainUSBDevice_WithSerials_ReturnsMainSerial) {
    auto result = deviceWithSerials_->getMainUSBDevice();
    ASSERT_TRUE(result.has_value()) << "Expected main USB device, got error: " << result.error();

    const auto& device = result.value();
    EXPECT_EQ(device.kind, Fw::USBDeviceType::SerialMain);
    EXPECT_EQ(device.location, static_cast<uint32_t>(Fw::USBHubPortLocation::Main));
    EXPECT_EQ(device.serial, "MAIN001");
    EXPECT_TRUE(device.port.has_value());
    EXPECT_EQ(device.port.value(), "/dev/ttyACM0");
}

TEST_F(FreeWiliDeviceMethodTest, GetMainUSBDevice_WithMassStorage_ReturnsMainMassStorage) {
    auto result = deviceWithMassStorage_->getMainUSBDevice();
    ASSERT_TRUE(result.has_value()) << "Expected main USB device, got error: " << result.error();

    const auto& device = result.value();
    EXPECT_EQ(device.kind, Fw::USBDeviceType::MassStorage);
    EXPECT_EQ(device.location, static_cast<uint32_t>(Fw::USBHubPortLocation::Main));
    EXPECT_EQ(device.serial, "MASS001");
    EXPECT_TRUE(device.paths.has_value());
    EXPECT_EQ(device.paths.value().size(), 1);
    EXPECT_EQ(device.paths.value()[0], "/mnt/freewili_main");
}

TEST_F(FreeWiliDeviceMethodTest, GetMainUSBDevice_MinimalDevice_ReturnsError) {
    auto result = minimalDevice_->getMainUSBDevice();
    EXPECT_FALSE(result.has_value());
    // Error message may vary - just check that it failed
}

// Tests for getDisplayUSBDevice
TEST_F(FreeWiliDeviceMethodTest, GetDisplayUSBDevice_WithSerials_ReturnsDisplaySerial) {
    auto result = deviceWithSerials_->getDisplayUSBDevice();
    ASSERT_TRUE(result.has_value()) << "Expected display USB device, got error: " << result.error();

    const auto& device = result.value();
    EXPECT_EQ(device.kind, Fw::USBDeviceType::SerialDisplay);
    EXPECT_EQ(device.location, static_cast<uint32_t>(Fw::USBHubPortLocation::Display));
    EXPECT_EQ(device.serial, "DISP001");
    EXPECT_TRUE(device.port.has_value());
    EXPECT_EQ(device.port.value(), "/dev/ttyACM1");
}

TEST_F(FreeWiliDeviceMethodTest, GetDisplayUSBDevice_WithMassStorage_ReturnsDisplayMassStorage) {
    auto result = deviceWithMassStorage_->getDisplayUSBDevice();
    ASSERT_TRUE(result.has_value()) << "Expected display USB device, got error: " << result.error();

    const auto& device = result.value();
    EXPECT_EQ(device.kind, Fw::USBDeviceType::MassStorage);
    EXPECT_EQ(device.location, static_cast<uint32_t>(Fw::USBHubPortLocation::Display));
    EXPECT_EQ(device.serial, "MASS002");
    EXPECT_TRUE(device.paths.has_value());
    EXPECT_EQ(device.paths.value().size(), 1);
    EXPECT_EQ(device.paths.value()[0], "/mnt/freewili_display");
}

TEST_F(FreeWiliDeviceMethodTest, GetDisplayUSBDevice_MinimalDevice_ReturnsError) {
    auto result = minimalDevice_->getDisplayUSBDevice();
    EXPECT_FALSE(result.has_value());
    // Error message may vary - just check that it failed
}

// Tests for getHubUSBDevice
TEST_F(FreeWiliDeviceMethodTest, GetHubUSBDevice_AllDevices_ReturnsHub) {
    // Test all device configurations should have a hub
    std::vector<std::unique_ptr<Fw::FreeWiliDevice>*> devices = { &deviceWithSerials_,
                                                                  &deviceWithMassStorage_,
                                                                  &deviceWithoutFTDIMassStorage_,
                                                                  &deviceWithoutFTDISerials_,
                                                                  &minimalDevice_ };

    for (const auto& devicePtr: devices) {
        auto result = (*devicePtr)->getHubUSBDevice();
        ASSERT_TRUE(result.has_value()) << "Expected hub USB device, got error: " << result.error();

        const auto& hubDevice = result.value();
        EXPECT_EQ(hubDevice.kind, Fw::USBDeviceType::Hub);
        EXPECT_EQ(hubDevice.vid, Fw::USB_VID_FW_HUB);
        EXPECT_EQ(hubDevice.pid, Fw::USB_PID_FW_HUB);
        EXPECT_EQ(hubDevice.serial, "HUB001");
    }
}

// Tests for getFPGAUSBDevice (FTDI)
TEST_F(FreeWiliDeviceMethodTest, GetFPGAUSBDevice_WithFTDI_ReturnsFTDI) {
    std::vector<std::unique_ptr<Fw::FreeWiliDevice>*> devicesWithFTDI = { &deviceWithSerials_,
                                                                          &deviceWithMassStorage_,
                                                                          &minimalDevice_ };

    for (const auto& devicePtr: devicesWithFTDI) {
        auto result = (*devicePtr)->getFPGAUSBDevice();
        ASSERT_TRUE(result.has_value())
            << "Expected FPGA USB device, got error: " << result.error();

        const auto& fpgaDevice = result.value();
        EXPECT_EQ(fpgaDevice.kind, Fw::USBDeviceType::FTDI);
        EXPECT_EQ(fpgaDevice.location, static_cast<uint32_t>(Fw::USBHubPortLocation::FPGA));
        EXPECT_EQ(fpgaDevice.vid, Fw::USB_VID_FW_FTDI);
        EXPECT_EQ(fpgaDevice.pid, Fw::USB_PID_FW_FTDI);
        EXPECT_EQ(fpgaDevice.serial, "FTDI001");
    }
}

TEST_F(FreeWiliDeviceMethodTest, GetFPGAUSBDevice_WithoutFTDI_ReturnsError) {
    std::vector<std::unique_ptr<Fw::FreeWiliDevice>*> devicesWithoutFTDI = {
        &deviceWithoutFTDIMassStorage_,
        &deviceWithoutFTDISerials_
    };

    for (const auto& devicePtr: devicesWithoutFTDI) {
        auto result = (*devicePtr)->getFPGAUSBDevice();
        EXPECT_FALSE(result.has_value());
        // The exact error message depends on the implementation
    }
}

// Integration tests for device creation
TEST(FreeWiliDeviceTestSetupTest, CreateAllDeviceTypes_Success) {
    // Test that all device factory methods work
    auto deviceWithSerials = FreeWiliDeviceTestSetup::createDeviceWithSerials();
    EXPECT_TRUE(deviceWithSerials.has_value()) << deviceWithSerials.error();

    auto deviceWithMassStorage = FreeWiliDeviceTestSetup::createDeviceWithMassStorage();
    EXPECT_TRUE(deviceWithMassStorage.has_value()) << deviceWithMassStorage.error();

    auto deviceWithoutFTDIMassStorage =
        FreeWiliDeviceTestSetup::createDeviceWithoutFTDIMassStorage();
    EXPECT_TRUE(deviceWithoutFTDIMassStorage.has_value()) << deviceWithoutFTDIMassStorage.error();

    auto deviceWithoutFTDISerials = FreeWiliDeviceTestSetup::createDeviceWithoutFTDISerials();
    EXPECT_TRUE(deviceWithoutFTDISerials.has_value()) << deviceWithoutFTDISerials.error();

    auto minimalDevice = FreeWiliDeviceTestSetup::createMinimalDevice();
    EXPECT_TRUE(minimalDevice.has_value()) << minimalDevice.error();
}

TEST(FreeWiliDeviceTestSetupTest, DeviceTypes_AreCorrect) {
    auto device = FreeWiliDeviceTestSetup::createDeviceWithSerials();
    ASSERT_TRUE(device.has_value());

    EXPECT_EQ(device->deviceType, Fw::DeviceType::FreeWili);
    EXPECT_FALSE(device->name.empty());
    EXPECT_FALSE(device->serial.empty());
    EXPECT_NE(device->uniqueID, std::numeric_limits<uint64_t>::max());
    EXPECT_FALSE(device->usbDevices.empty());
}
