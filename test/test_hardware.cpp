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
            ASSERT_EQ(device.getUSBDevices(Fw::USBDeviceType::FTDI).size(), 1)
                << "Expected one USB FTDI device";
            auto serialMain = device.getUSBDevices(Fw::USBDeviceType::SerialMain);
            auto serialDisplay = device.getUSBDevices(Fw::USBDeviceType::SerialDisplay);
            auto massStorage = device.getUSBDevices(Fw::USBDeviceType::MassStorage);
            if (!massStorage.size()) {
                // We don't have any in UF2 Bootloader so we should be good here.
                ASSERT_EQ(serialMain.size(), 1) << "Expected one USB SerialMain device";
                ASSERT_EQ(serialDisplay.size(), 1) << "Expected one USB SerialDisplay device";
            } else if (massStorage.size() == 1) {
                // We are in UF2 mode, so we should only have one serial device
                if (serialMain.size() == 1) {
                    ASSERT_EQ(serialDisplay.size(), 0)
                        << "Expected no USB SerialDisplay device in UF2 mode";
                    ASSERT_EQ(serialMain[0].location, 1)
                        << "Expected USB SerialMain device to be at location 1 in UF2 mode";
                } else if (serialDisplay.size() == 1) {
                    ASSERT_EQ(serialMain.size(), 0)
                        << "Expected no USB SerialMain device in UF2 mode";
                } else {
                    FAIL() << "Unexpected number of Serial devices in UF2 mode: "
                           << serialMain.size() << " SerialMain, " << serialDisplay.size()
                           << " SerialDisplay";
                }
            } else if (massStorage.size() == 2) {
                // We are in UF2 mode with both Main and Display CPUs
                ASSERT_EQ(serialMain.size(), 0) << "Expected no USB SerialMain device in UF2 mode.";
                ASSERT_EQ(serialDisplay.size(), 0)
                    << "Expected no USB SerialDisplay device in UF2 mode.";
            } else {
                FAIL() << "Unexpected number of Mass Storage devices: " << massStorage.size();
            }
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
