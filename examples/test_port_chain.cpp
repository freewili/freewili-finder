/**
 * @file test_port_chain.cpp
 * @brief Example demonstrating USB port chain analysis after locationID refactoring
 *
 * This example shows how the refactored locationID parsing works on macOS:
 * - Location: represents the actual port number on the immediate parent hub/controller
 * - PortChain: represents the full path from root hub to the device
 * 
 * Following the cyme project's approach for macOS USB location identification.
 */

#include <fwfinder.hpp>
#include <iostream>
#include <iomanip>

int main() {
    std::cout << "Testing locationID refactoring - Port Chain Analysis" << std::endl;
    std::cout << "====================================================" << std::endl << std::endl;

    if (auto fw_devices = Fw::find_all(); fw_devices.has_value()) {
        const auto& devices = fw_devices.value();

        std::cout << "Found " << devices.size() << " FreeWili device(s):" << std::endl << std::endl;

        for (size_t i = 0; i < devices.size(); ++i) {
            const auto& device = devices[i];

            std::cout << "Device " << (i + 1) << ": " << device.name << std::endl;
            std::cout << "  Serial: " << device.serial << std::endl;
            std::cout << "  USB Devices with Port Chain Analysis:" << std::endl;

            for (const auto& usb : device.usbDevices) {
                std::cout << "    - " << usb.name << std::endl;
                std::cout << "      Location (port): " << usb.location << std::endl;
                std::cout << "      Port Chain: [";
                for (size_t j = 0; j < usb.portChain.size(); ++j) {
                    std::cout << usb.portChain[j];
                    if (j < usb.portChain.size() - 1) std::cout << ", ";
                }
                std::cout << "]" << std::endl;
                std::cout << "      Type: " << Fw::getUSBDeviceTypeName(usb.kind) << std::endl;
                std::cout << "      VID:PID: 0x" << std::hex << std::setw(4) << std::setfill('0') 
                          << usb.vid << ":0x" << std::hex << std::setw(4) << std::setfill('0') 
                          << usb.pid << std::dec << std::endl;
                std::cout << std::endl;
            }
        }
    } else {
        std::cerr << "Failed to find FreeWili devices: " << fw_devices.error() << std::endl;
        return 1;
    }

    return 0;
}
