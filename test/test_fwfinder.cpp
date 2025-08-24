#include <gtest/gtest.h>

#include <fwfinder.hpp>
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
    ASSERT_STREQ(Fw::getDeviceTypeName(Fw::DeviceType::DefCon2024Badge).c_str(), "DefCon 2024 Badge");
    ASSERT_STREQ(
        Fw::getDeviceTypeName(Fw::DeviceType::DefCon2025FwBadge).c_str(),
        "DefCon 2025 Badge"
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
    ASSERT_EQ(
        Fw::generateUniqueID(0xFFFFFFFF, 0xFFFFFFFF), 
        0xFFFFFFFFFFFFFFFFULL
    );

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
