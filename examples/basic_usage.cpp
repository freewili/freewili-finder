/**
 * @file basic_usage.cpp
 * @brief Basic example demonstrating how to use the fwfinder library
 *
 * This example shows how to:
 * - Find all connected FreeWili devices
 * - Display device information
 * - Enumerate USB devices for each FreeWili
 * - Handle errors properly
 */

#include <fwfinder.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

int main() {
    std::cout << "FreeWili Device Finder Example" << std::endl;
    std::cout << "==============================" << std::endl << std::endl;

    // Find all FreeWili devices
    if (auto fw_devices = Fw::find_all(); fw_devices.has_value()) {
        const auto& devices = fw_devices.value();

        std::cout << "Found " << devices.size() << " FreeWili device(s):" << std::endl << std::endl;

        // Iterate through each FreeWili device
        for (size_t i = 0; i < devices.size(); ++i) {
            const auto& device = devices[i];

            std::cout << "Device " << (i + 1) << ": " << device.name << std::endl;
            std::cout << "  Serial: " << device.serial << std::endl;
            std::cout << "  USB Hub: " << device.usbHub.name << " (VID: 0x" << std::hex
                      << std::setw(4) << std::setfill('0') << device.usbHub.vid << ", PID: 0x"
                      << std::hex << std::setw(4) << std::setfill('0') << device.usbHub.pid << ")"
                      << std::dec << std::endl;
            std::cout << "  Total USB Devices: " << device.usbDevices.size() << std::endl;

            // Show different types of USB devices
            std::cout << "  USB Devices:" << std::endl;

            // Get serial devices
            auto serialDevices = device.getUSBDevices(Fw::USBDeviceType::Serial);
            if (!serialDevices.empty()) {
                std::cout << "    Serial Ports:" << std::endl;
                for (const auto& serial: serialDevices) {
                    std::cout << "      - " << serial.name << " ("
                              << (serial.port.has_value() ? serial.port.value() : "No port") << ")"
                              << std::endl;
                }
            }

            // Get MainCPU serial devices
            auto mainCpuDevices = device.getUSBDevices(Fw::USBDeviceType::SerialMain);
            if (!mainCpuDevices.empty()) {
                std::cout << "    MainCPU Serial Ports:" << std::endl;
                for (const auto& mainCpu: mainCpuDevices) {
                    std::cout << "      - " << mainCpu.name << " (Serial: " << mainCpu.serial << ")"
                              << std::endl;
                }
            }

            // Get DisplayCPU serial devices
            auto displayCpuDevices = device.getUSBDevices(Fw::USBDeviceType::SerialDisplay);
            if (!displayCpuDevices.empty()) {
                std::cout << "    DisplayCPU Serial Ports:" << std::endl;
                for (const auto& displayCpu: displayCpuDevices) {
                    std::cout << "      - " << displayCpu.name << " (Serial: " << displayCpu.serial
                              << ")" << std::endl;
                }
            }

            // Get mass storage devices
            auto massStorageDevices = device.getUSBDevices(Fw::USBDeviceType::MassStorage);
            if (!massStorageDevices.empty()) {
                std::cout << "    Mass Storage:" << std::endl;
                for (const auto& storage: massStorageDevices) {
                    std::cout << "      - " << storage.name << std::endl;
                    if (storage.paths.has_value()) {
                        for (const auto& path: storage.paths.value()) {
                            std::cout << "        Path: " << path << std::endl;
                        }
                    }
                }
            }

            // Get FTDI devices
            auto ftdiDevices = device.getUSBDevices(Fw::USBDeviceType::FTDI);
            if (!ftdiDevices.empty()) {
                std::cout << "    FTDI/FPGA Devices:" << std::endl;
                for (const auto& ftdi: ftdiDevices) {
                    std::cout << "      - " << ftdi.name << " (Serial: " << ftdi.serial << ")"
                              << std::endl;
                }
            }

            // Show all USB devices with detailed info
            std::cout << "    All USB Devices (detailed):" << std::endl;
            for (const auto& usb: device.usbDevices) {
                std::cout << "      - " << usb.name << " (VID: 0x" << std::hex << std::setw(4)
                          << std::setfill('0') << usb.vid << ", PID: 0x" << std::hex << std::setw(4)
                          << std::setfill('0') << usb.pid << ", Location: " << std::dec
                          << usb.location << ")" << std::endl;
                std::cout << "        Serial: " << usb.serial << std::endl;
                std::cout << "        Type: " << Fw::getUSBDeviceTypeName(usb.kind) << std::endl;
            }

            std::cout << std::endl;
        }

        // Demonstrate USB device type detection
        std::cout << "USB Device Type Examples:" << std::endl;
        std::cout << "  VID: 0x093C, PID: 0x2054 -> "
                  << Fw::getUSBDeviceTypeName(Fw::getUSBDeviceTypeFrom(0x093C, 0x2054))
                  << std::endl;
        std::cout << "  VID: 0x093C, PID: 0x2055 -> "
                  << Fw::getUSBDeviceTypeName(Fw::getUSBDeviceTypeFrom(0x093C, 0x2055))
                  << std::endl;
        std::cout << "  VID: 0x0403, PID: 0x6014 -> "
                  << Fw::getUSBDeviceTypeName(Fw::getUSBDeviceTypeFrom(0x0403, 0x6014))
                  << std::endl;

    } else {
        // Handle errors
        std::cerr << "Failed to find FreeWili devices: " << fw_devices.error() << std::endl;
        return 1;
    }

    return 0;
}
