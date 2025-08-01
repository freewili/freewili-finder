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
