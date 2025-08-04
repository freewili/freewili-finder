#include <gtest/gtest.h>

#include <fwfinder.hpp>
#include <usbdef.hpp>

TEST(FwFinder, ExpectedHardware) {
    auto results = Fw::find_all();
    if (!results.has_value()) {
        FAIL() << "Failed to find hardware: " << results.error();
        return;
    }

    if (results.value().empty()) {
        GTEST_SKIP() << "No FreeWili hardware connected - skipping hardware test";
        return;
    }

    // If we get here, we have hardware connected - add assertions as needed
    ASSERT_FALSE(results.value().empty()) << "Expected to find at least one FreeWili device";

    for (const auto& device: results.value()) {
        if (device.deviceType == Fw::DeviceType::FreeWili) {
            // Verify we found a FreeWili device successfully.
            ASSERT_EQ(device.usbDevices.size(), 4)
                << "Expected exactly four USB devices for FreeWili device";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::Hub).size(), 1)
                << "Expected one USB Hub device";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::SerialMain).size(), 1)
                << "Expected one USB SerialMain device";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::SerialDisplay).size(), 1)
                << "Expected one USB SerialDisplay device";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::FTDI).size(), 1)
                << "Expected one USB FTDI device";
        } else if (device.deviceType == Fw::DeviceType::DefCon2024Badge
                   || device.deviceType == Fw::DeviceType::DefCon2025FwBadge
                   || device.deviceType == Fw::DeviceType::Winky)
        {
            // Verify we found a DefCon badge device successfully.
            ASSERT_EQ(device.usbDevices.size(), 1) << "Expected exactly one USB device for badge";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::SerialMain).size(), 1)
                << "Expected one USB SerialMain device for badge";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::Hub).size(), 0)
                << "Expected no USB Hub device for badge";
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::SerialDisplay).size(), 0)
                << "Expected no USB SerialDisplay device for badge";
        } else if (device.deviceType == Fw::DeviceType::Unknown) {
            FAIL() << "Found device with unknown type: " << device.name
                   << " - expected FreeWili or DefCon badge type";
        } else {
            // For other types, we can just log the type
            std::cout << "Found device of type: " << Fw::getDeviceTypeName(device.deviceType)
                      << std::endl;
        }
    }
}
