#include <gtest/gtest.h>

#include <fwfinder.hpp>
#include <usbdef.hpp>

#include <print>

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
        Fw::getUSBDeviceTypeFrom(
            Fw::USB_VID_FW_RPI,
            Fw::USB_PID_FW_RPI_CDC_PID
        ),
        Fw::USBDeviceType::Serial
    );

    ASSERT_EQ(
        Fw::getUSBDeviceTypeFrom(
            Fw::USB_VID_FW_RPI,
            Fw::USB_PID_FW_RPI_UF2_PID
        ),
        Fw::USBDeviceType::MassStorage
    );

    ASSERT_EQ(Fw::getUSBDeviceTypeFrom(0, 0), Fw::USBDeviceType::Other);
}

TEST(FwFinder, BasicAssertions) {
    //auto finder = Fw::Finder();
    //Fw::list_all();

    if (auto fwDevicesResult = Fw::find_all(); fwDevicesResult.has_value()) {
        std::println(
            "Found {} Free-Wili(s)...",
            fwDevicesResult.value().size()
        );
        int i = 0;
        for (auto& fwDevice : fwDevicesResult.value()) {
            ++i;
            for (auto& usbDevice : fwDevice) {
                std::println(
                    "{})\t{} {} (VID: {:04X}, PID: {:04X})",
                    i,
                    usbDevice.name,
                    usbDevice.serial,
                    usbDevice.vid,
                    usbDevice.pid
                );
            }
        }
    } else {
        std::println("Error: {}", fwDevicesResult.error());
    }
}
