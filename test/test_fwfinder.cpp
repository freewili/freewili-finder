#include <gtest/gtest.h>

#include <fwfinder.hpp>
#include <fwbuilder.hpp>
#include <usbdef.hpp>

#include <cstdio>

TEST(FwFinder, getUSBDeviceTypeFrom) {
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_HUB, Fw::USB_PID_FW_HUB, 0),
        Fw::USBDeviceType::Hub
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_FTDI, Fw::USB_PID_FW_FTDI, 0),
        Fw::USBDeviceType::FTDI
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_CDC_PID, 0),
        Fw::USBDeviceType::Serial
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(
            Fw::USB_VID_FW_RPI,
            Fw::USB_PID_FW_RPI_CDC_PID,
            static_cast<uint32_t>(Fw::USBHubPortLocation::Main)
        ),
        Fw::USBDeviceType::SerialMain
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(
            Fw::USB_VID_FW_RPI,
            Fw::USB_PID_FW_RPI_CDC_PID,
            static_cast<uint32_t>(Fw::USBHubPortLocation::Display)
        ),
        Fw::USBDeviceType::SerialDisplay
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_2040_UF2_PID, 0),
        Fw::USBDeviceType::MassStorage
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW_RPI, Fw::USB_PID_FW_RPI_2350_UF2_PID, 0),
        Fw::USBDeviceType::MassStorage
    );

    // FREE-WILi2 type mappings
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_HUB, Fw::USB_PID_FW2_HUB, 0),
        Fw::USBDeviceType::Hub
    );
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_MAIN, Fw::USB_PID_FW2_MAIN, 0),
        Fw::USBDeviceType::SerialMain
    );
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_DISPLAY, Fw::USB_PID_FW2_DISPLAY, 0),
        Fw::USBDeviceType::SerialDisplay
    );
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_FTDI, Fw::USB_PID_FW2_FTDI, 0),
        Fw::USBDeviceType::FTDI
    );
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_DEBUG_PROBE, Fw::USB_PID_FW2_DEBUG_PROBE, 0),
        Fw::USBDeviceType::DebugProbe
    );
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_ESP32, Fw::USB_PID_FW2_ESP32_JTAG, 0),
        Fw::USBDeviceType::ESP32
    );
    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(Fw::USB_VID_FW2_MASS_STORAGE, Fw::USB_PID_FW2_MASS_STORAGE, 0),
        Fw::USBDeviceType::MassStorage
    );

    ASSERT_EQ(Fw::getUSBDeviceTypeFrom(0, 0, 0), Fw::USBDeviceType::Other);
}

TEST(FwFinder, getUSBDeviceTypeName) {
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::Hub).c_str(), "Hub");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::FTDI).c_str(), "FTDI");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::Serial).c_str(), "Serial");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::MassStorage).c_str(), "Mass Storage");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::Other).c_str(), "Other");
    ASSERT_STREQ(Fw::getUSBDeviceTypeName(Fw::USBDeviceType::DebugProbe).c_str(), "Debug Probe");
}

TEST(FwFinder, getDeviceTypeName) {
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::Unknown).c_str(), "Unknown");
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::FreeWili).c_str(), "FREE-WILi");
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
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::FreeWili2).c_str(), "FREE-WILi2");
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

    // FREE-WILi2 hub is not standalone
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW2_HUB, Fw::USB_PID_FW2_HUB));
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW2_MAIN, Fw::USB_PID_FW2_MAIN));

    ASSERT_FALSE(Fw::isStandAloneDevice(0, 0));
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
                               .name = "FREE-WILi Hub",
                               .serial = "HUB001",
                               .location = 3, // Hub is at the root level, not on a specific port
                               .portChain = { 1, 2, 3 },
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
                               .name = "FREE-WILi FTDI",
                               .serial = "FTDI001",
                               .location = 3,
                               .portChain = { 1, 2, 3 },
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
                               .name = "FREE-WILi Main Serial",
                               .serial = "MAIN001",
                               .location = 1,
                               .portChain = { 1, 2, 1 },
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
                               .name = "FREE-WILi Display Serial",
                               .serial = "DISP001",
                               .location = 2,
                               .portChain = { 1, 2, 2 },
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
                               .name = "FREE-WILi Main Storage",
                               .serial = "MASS001",
                               .location = 1,
                               .portChain = { 1, 2, 1 },
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
                               .name = "FREE-WILi Display Storage",
                               .serial = "MASS002",
                               .location = 2,
                               .portChain = { 1, 2, 2 },
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
            .setUniqueID(1)
            .setStandalone(false)
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
            .setUniqueID(2)
            .setStandalone(false)
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
            .setUniqueID(3)
            .setStandalone(false)
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
            .setUniqueID(4)
            .setStandalone(false)
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
            .setUniqueID(5)
            .setStandalone(false)
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

// ============================================================================
// FREE-WILi2 Test Setup and Tests
// ============================================================================

class FW2DeviceTestSetup {
public:
    static Fw::USBDevice createFW2HubDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::Hub,
                               .vid = Fw::USB_VID_FW2_HUB,
                               .pid = Fw::USB_PID_FW2_HUB,
                               .name = "FREE-WILi2 Hub",
                               .serial = "FWTST1",
                               .location = 1,
                               .portChain = { 1 },
                               .paths = std::nullopt,
                               .port = std::nullopt,
                               ._raw = "USB\\VID_093C&PID_2059\\FWTST1" };
    }

    static Fw::USBDevice createFW2MainDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::SerialMain,
                               .vid = Fw::USB_VID_FW2_MAIN,
                               .pid = Fw::USB_PID_FW2_MAIN,
                               .name = "FW2 v07",
                               .serial = "FWTST1",
                               .location = 1,
                               .portChain = { 1, 1 },
                               .paths = std::nullopt,
                               .port = std::string("COM3"),
                               ._raw = "USB\\VID_093C&PID_205A\\FWTST1" };
    }

    /// The Display is an optional component that isn't populated on most units.
    static Fw::USBDevice createFW2DisplayDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::SerialDisplay,
                               .vid = Fw::USB_VID_FW2_DISPLAY,
                               .pid = Fw::USB_PID_FW2_DISPLAY,
                               .name = "FW2 Display CDC",
                               .serial = "C6ACB61A8BD41507",
                               .location = 2,
                               .portChain = { 1, 2 },
                               .paths = std::nullopt,
                               .port = std::string("COM4"),
                               ._raw = "USB\\VID_093C&PID_2060\\C6ACB61A8BD41507" };
    }

    static Fw::USBDevice createFW2FTDIDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::FTDI,
                               .vid = Fw::USB_VID_FW2_FTDI,
                               .pid = Fw::USB_PID_FW2_FTDI,
                               .name = "FREE-WILi FW2",
                               .serial = "FWTST1",
                               .location = 3,
                               .portChain = { 1, 3 },
                               .paths = std::nullopt,
                               .port = std::nullopt,
                               ._raw = "USB\\VID_0403&PID_6014" };
    }

    static Fw::USBDevice createFW2DebugProbeDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::DebugProbe,
                               .vid = Fw::USB_VID_FW2_DEBUG_PROBE,
                               .pid = Fw::USB_PID_FW2_DEBUG_PROBE,
                               .name = "FreeWili Debug Probe (CMSIS-DAP)",
                               .serial = "E66568714F28A828",
                               .location = 4,
                               .portChain = { 1, 4 },
                               .paths = std::nullopt,
                               .port = std::string("COM5"),
                               ._raw = "USB\\VID_2E8A&PID_000C\\E66568714F28A828" };
    }

    static Fw::USBDevice createFW2ESP32Device() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::ESP32,
                               .vid = Fw::USB_VID_FW2_ESP32,
                               .pid = Fw::USB_PID_FW2_ESP32_JTAG,
                               .name = "Espressif USB JTAG/serial debug unit",
                               .serial = "3C:DC:75:9A:BB:40",
                               .location = 5,
                               .portChain = { 1, 5 },
                               .paths = std::nullopt,
                               .port = std::string("COM6"),
                               ._raw = "USB\\VID_303A&PID_1001\\3CDC759ABB40" };
    }

    static Fw::USBDevice createFW2MassStorageDevice() {
        return Fw::USBDevice { .kind = Fw::USBDeviceType::MassStorage,
                               .vid = Fw::USB_VID_FW2_MASS_STORAGE,
                               .pid = Fw::USB_PID_FW2_MASS_STORAGE,
                               .name = "FW Ultra Fast Media",
                               .serial = "0000395D5D4D",
                               .location = 6,
                               .portChain = { 1, 6 },
                               .paths = std::vector<std::string> { "E:\\" },
                               .port = std::nullopt,
                               ._raw = "USB\\VID_093C&PID_205F\\0000395D5D4D" };
    }

    /// A FREE-WILi2 as it ships: no Display processor populated.
    static std::expected<Fw::FreeWiliDevice, std::string> createFullFW2Device() {
        Fw::USBDevices usbDevices = { createFW2HubDevice(),   createFW2MainDevice(),
                                      createFW2FTDIDevice(),  createFW2DebugProbeDevice(),
                                      createFW2ESP32Device(), createFW2MassStorageDevice() };
        return Fw::FreeWiliDevice::fromUSBDevices(usbDevices);
    }

    /// A FREE-WILi2 with the optional Display processor populated.
    static std::expected<Fw::FreeWiliDevice, std::string> createFW2DeviceWithDisplay() {
        Fw::USBDevices usbDevices = { createFW2HubDevice(),        createFW2MainDevice(),
                                      createFW2DisplayDevice(),    createFW2FTDIDevice(),
                                      createFW2DebugProbeDevice(), createFW2MassStorageDevice() };
        return Fw::FreeWiliDevice::fromUSBDevices(usbDevices);
    }
};

TEST(FW2Device, FromUSBDevices_FullStack) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << "Failed: " << result.error();

    const auto& device = result.value();
    EXPECT_EQ(device.deviceType, Fw::DeviceType::FreeWili2);
    EXPECT_EQ(device.name, "FREE-WILi2");
    EXPECT_EQ(device.serial, "FWTST1");
    EXPECT_FALSE(device.standalone);
    EXPECT_EQ(device.usbDevices.size(), 6);
}

/// The hub serial lives in a string descriptor that takes a platform specific workaround to read
/// (_getUSBSerialFromDescriptor() on Windows, getStringDescriptor() on macOS). When it can't be
/// read, fall back to the children that carry the FREE-WILi2 serial instead of dropping the
/// device entirely.
TEST(FW2Device, SerialFallsBackToFTDIWhenHubSerialMissing) {
    auto hub = FW2DeviceTestSetup::createFW2HubDevice();
    hub.serial = "";
    Fw::USBDevices usbDevices = { hub,
                                  FW2DeviceTestSetup::createFW2MainDevice(),
                                  FW2DeviceTestSetup::createFW2FTDIDevice(),
                                  FW2DeviceTestSetup::createFW2DebugProbeDevice(),
                                  FW2DeviceTestSetup::createFW2ESP32Device(),
                                  FW2DeviceTestSetup::createFW2MassStorageDevice() };

    auto result = Fw::FreeWiliDevice::fromUSBDevices(usbDevices);
    ASSERT_TRUE(result.has_value()) << "Failed: " << result.error();
    EXPECT_EQ(result->deviceType, Fw::DeviceType::FreeWili2);
    EXPECT_EQ(result->serial, "FWTST1");
}

/// The Main CPU carries the same serial and takes over when there is no FTDI.
TEST(FW2Device, SerialFallsBackToMainWhenHubAndFTDIMissing) {
    auto hub = FW2DeviceTestSetup::createFW2HubDevice();
    hub.serial = "";
    Fw::USBDevices usbDevices = { hub,
                                  FW2DeviceTestSetup::createFW2MainDevice(),
                                  FW2DeviceTestSetup::createFW2MassStorageDevice() };

    auto result = Fw::FreeWiliDevice::fromUSBDevices(usbDevices);
    ASSERT_TRUE(result.has_value()) << "Failed: " << result.error();
    EXPECT_EQ(result->serial, "FWTST1");
}

/// The debug probe, ESP32 and mass storage each report their own module serial. Using one of
/// those would label the FREE-WILi2 with a serial that isn't its own, so they are never used as a
/// fallback - reporting no device is better than reporting a wrong one.
TEST(FW2Device, SerialFallbackIgnoresModuleSerials) {
    auto hub = FW2DeviceTestSetup::createFW2HubDevice();
    hub.serial = "";
    Fw::USBDevices usbDevices = { hub,
                                  FW2DeviceTestSetup::createFW2DebugProbeDevice(),
                                  FW2DeviceTestSetup::createFW2ESP32Device(),
                                  FW2DeviceTestSetup::createFW2MassStorageDevice() };

    auto result = Fw::FreeWiliDevice::fromUSBDevices(usbDevices);
    EXPECT_FALSE(result.has_value())
        << "Expected no device rather than one labelled with a module serial, got: "
        << (result.has_value() ? result->serial : std::string());
}

TEST(FW2Device, GetMainUSBDevice_ReturnsFW2Main) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto mainResult = result->getMainUSBDevice();
    ASSERT_TRUE(mainResult.has_value()) << mainResult.error();

    const auto& main = mainResult.value();
    EXPECT_EQ(main.kind, Fw::USBDeviceType::SerialMain);
    EXPECT_EQ(main.vid, Fw::USB_VID_FW2_MAIN);
    EXPECT_EQ(main.pid, Fw::USB_PID_FW2_MAIN);
    EXPECT_EQ(main.location, static_cast<uint32_t>(Fw::FW2HubPortLocation::Main));
    EXPECT_TRUE(main.port.has_value());
    EXPECT_EQ(main.port.value(), "COM3");
}

TEST(FW2Device, GetFPGAUSBDevice_ReturnsFTDI) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto fpgaResult = result->getFPGAUSBDevice();
    ASSERT_TRUE(fpgaResult.has_value()) << fpgaResult.error();

    const auto& fpga = fpgaResult.value();
    EXPECT_EQ(fpga.kind, Fw::USBDeviceType::FTDI);
    EXPECT_EQ(fpga.location, static_cast<uint32_t>(Fw::FW2HubPortLocation::FPGA));
}

TEST(FW2Device, GetHubUSBDevice_ReturnsFW2Hub) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto hubResult = result->getHubUSBDevice();
    ASSERT_TRUE(hubResult.has_value()) << hubResult.error();

    const auto& hub = hubResult.value();
    EXPECT_EQ(hub.kind, Fw::USBDeviceType::Hub);
    EXPECT_EQ(hub.vid, Fw::USB_VID_FW2_HUB);
    EXPECT_EQ(hub.pid, Fw::USB_PID_FW2_HUB);
    EXPECT_EQ(hub.serial, "FWTST1");
}

TEST(FW2Device, GetDisplayUSBDevice_ReturnsErrorWhenNotPopulated) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto displayResult = result->getDisplayUSBDevice();
    EXPECT_FALSE(displayResult.has_value());
}

TEST(FW2Device, GetDisplayUSBDevice_ReturnsDisplayWhenPopulated) {
    auto result = FW2DeviceTestSetup::createFW2DeviceWithDisplay();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto displayResult = result->getDisplayUSBDevice();
    ASSERT_TRUE(displayResult.has_value()) << displayResult.error();

    const auto& display = displayResult.value();
    EXPECT_EQ(display.kind, Fw::USBDeviceType::SerialDisplay);
    EXPECT_EQ(display.vid, Fw::USB_VID_FW2_DISPLAY);
    EXPECT_EQ(display.pid, Fw::USB_PID_FW2_DISPLAY);
    EXPECT_EQ(display.port.value(), "COM4");
}

TEST(FW2Device, GetDebugProbeUSBDevice) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto probeResult = result->getDebugProbeUSBDevice();
    ASSERT_TRUE(probeResult.has_value()) << probeResult.error();

    const auto& probe = probeResult.value();
    EXPECT_EQ(probe.kind, Fw::USBDeviceType::DebugProbe);
    EXPECT_EQ(probe.vid, Fw::USB_VID_FW2_DEBUG_PROBE);
    EXPECT_EQ(probe.pid, Fw::USB_PID_FW2_DEBUG_PROBE);
    EXPECT_EQ(probe.location, static_cast<uint32_t>(Fw::FW2HubPortLocation::DebugProbe));
}

TEST(FW2Device, GetDebugProbeViaFilter) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto probes = result->getUSBDevices(Fw::USBDeviceType::DebugProbe);
    ASSERT_EQ(probes.size(), 1);
    EXPECT_EQ(probes[0].vid, Fw::USB_VID_FW2_DEBUG_PROBE);
    EXPECT_EQ(probes[0].pid, Fw::USB_PID_FW2_DEBUG_PROBE);
    EXPECT_TRUE(probes[0].port.has_value());
}

TEST(FW2Device, GetESP32USBDevice) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto esp32Result = result->getESP32USBDevice();
    ASSERT_TRUE(esp32Result.has_value()) << esp32Result.error();

    const auto& esp32 = esp32Result.value();
    EXPECT_EQ(esp32.kind, Fw::USBDeviceType::ESP32);
    EXPECT_EQ(esp32.vid, Fw::USB_VID_FW2_ESP32);
    EXPECT_EQ(esp32.pid, Fw::USB_PID_FW2_ESP32_JTAG);
    EXPECT_EQ(esp32.location, static_cast<uint32_t>(Fw::FW2HubPortLocation::ESP32));
}

TEST(FW2Device, GetESP32ViaFilter) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto esp32Devices = result->getUSBDevices(Fw::USBDeviceType::ESP32);
    ASSERT_EQ(esp32Devices.size(), 1);
    EXPECT_EQ(esp32Devices[0].vid, Fw::USB_VID_FW2_ESP32);
    EXPECT_EQ(esp32Devices[0].pid, Fw::USB_PID_FW2_ESP32_JTAG);
    EXPECT_TRUE(esp32Devices[0].port.has_value());
}

/// The ESP32 must only ever be attributed to a FreeWili when it sits behind the
/// hub. A bare ESP32 elsewhere on the bus is not a standalone FreeWili device.
TEST(FW2Device, ESP32IsNotStandalone) {
    ASSERT_FALSE(Fw::isStandAloneDevice(Fw::USB_VID_FW2_ESP32, Fw::USB_PID_FW2_ESP32_JTAG));
}

TEST(FW2Device, GetMassStorageViaFilter) {
    auto result = FW2DeviceTestSetup::createFullFW2Device();
    ASSERT_TRUE(result.has_value()) << result.error();

    auto massStorageDevices = result->getUSBDevices(Fw::USBDeviceType::MassStorage);
    ASSERT_EQ(massStorageDevices.size(), 1);
    EXPECT_EQ(massStorageDevices[0].vid, Fw::USB_VID_FW2_MASS_STORAGE);
    EXPECT_EQ(massStorageDevices[0].pid, Fw::USB_PID_FW2_MASS_STORAGE);
    EXPECT_EQ(
        massStorageDevices[0].location,
        static_cast<uint32_t>(Fw::FW2HubPortLocation::SDCard)
    );
    EXPECT_TRUE(massStorageDevices[0].paths.has_value());
}
