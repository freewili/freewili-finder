#include <fwfinder.hpp>
#include <usbdef.hpp>

auto Fw::getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> Fw::USBDeviceType {
    if (vid == USB_VID_FW_HUB) {
        if (pid == USB_PID_FW_HUB) {
            return Fw::USBDeviceType::Hub;
        }
    } else if (vid == USB_VID_FW_FTDI) {
        if (pid == USB_PID_FW_FTDI) {
            return Fw::USBDeviceType::FTDI;
        }
    } else if (vid == USB_VID_FW_RPI) {
        if (pid == USB_PID_FW_RPI_CDC_PID) {
            return Fw::USBDeviceType::Serial;
        } else if (pid == USB_PID_FW_RPI_UF2_PID) {
            return Fw::USBDeviceType::MassStorage;
        }
    }
    return Fw::USBDeviceType::Other;
}
