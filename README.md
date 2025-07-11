# FreeWili Finder

A cross-platform C/C++ library for discovering and interfacing with FreeWili devices connected to a host system.

## Overview

The FreeWili Finder library provides both modern C++ and C APIs to:
- **Discover** all connected FreeWili devices automatically
- **Enumerate** USB devices associated with each FreeWili
- **Identify** device types (Serial ports, Mass storage, FTDI/FPGA, etc.)
- **Access** device information (VID/PID, serial numbers, port paths)

## Features

- **Dual API Support**: Modern C++23 API with `std::expected` error handling, plus C API for maximum compatibility
- **Cross-Platform**: Supports Windows, macOS, and Linux
- **Type Safety**: Strongly-typed USB device classification system
- **Error Handling**: Comprehensive error reporting and handling
- **Memory Safe**: RAII in C++, explicit memory management in C API
- **Testing**: Full test suite with GoogleTest integration

## Quick Start

### C++ API Example

```cpp
#include <fwfinder.hpp>
#include <iostream>

int main() {
    if (auto devices = Fw::find_all(); devices.has_value()) {
        for (const auto& device : devices.value()) {
            std::cout << "Found: " << device.name
                     << " (Serial: " << device.serial << ")\n";

            // Get serial ports
            auto serials = device.getUSBDevices(Fw::USBDeviceType::Serial);
            for (const auto& serial : serials) {
                std::cout << "  Port: " << serial.port.value_or("Unknown") << "\n";
            }
        }
    } else {
        std::cerr << "Error: " << devices.error() << "\n";
    }
    return 0;
}
```

### C API Example

```c
#include <cfwfinder.h>
#include <stdio.h>

int main(void) {
    fw_freewili_device_t* devices[16];
    uint32_t device_count = 16;
    char error_msg[256];
    uint32_t error_size = sizeof(error_msg);

    if (fw_device_find_all(devices, &device_count, error_msg, &error_size) == fw_error_success) {
        printf("Found %u device(s)\n", device_count);

        for (uint32_t i = 0; i < device_count; ++i) {
            char name[64];
            fw_device_get_str(devices[i], fw_stringtype_name, name, sizeof(name));
            printf("Device: %s\n", name);
        }

        fw_device_free(devices, device_count);
    }
    return 0;
}
```


## Building

### Prerequisites

- **CMake** 3.20 or later
- **C++23** compatible compiler (GCC 11+, Clang 14+, MSVC 2022+)
- **Platform dependencies**:
  - Linux: `libudev-dev`
  - macOS: Xcode command line tools
  - Windows: Windows SDK (SetupAPI, Cfgmgr32)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/your-org/freewili-finder.git
cd freewili-finder

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .

# Run tests
ctest --output-on-failure
```

### CMake Options

- `FW_FINDER_BUILD_TESTS=ON/OFF` - Build test suite (default: ON)
- `FW_BUILD_C_API=ON/OFF` - Build C API library (default: ON)
- `FW_BUILD_STATIC=ON/OFF` - Build static libraries (default: ON)
- `FW_BUILD_EXAMPLES=ON/OFF` - Build example applications (default: ON)

## API Reference

### C++ API (`fwfinder.hpp`)

#### Core Functions

```cpp
namespace Fw {
    // Find all connected FreeWili devices
    auto find_all() noexcept -> std::expected<FreeWiliDevices, std::string>;

    // USB device type detection
    auto getUSBDeviceTypeFrom(uint16_t vid, uint16_t pid) -> USBDeviceType;
    auto getUSBDeviceTypeName(USBDeviceType type) -> std::string;
}
```

#### Device Types

```cpp
enum class USBDeviceType {
    Hub,              // USB Hub (parent device)
    Serial,           // Serial Port (general)
    SerialMain,       // MainCPU Serial Port
    SerialDisplay,    // DisplayCPU Serial Port
    MassStorage,      // Mass Storage Device
    ESP32,            // ESP32 USB (JTAG/RTT)
    FTDI,             // FTDI/FPGA Device
    Other             // Other USB device
};
```

#### Data Structures

```cpp
struct USBDevice {
    USBDeviceType kind;
    uint16_t vid, pid;
    std::string name, serial;
    uint32_t location;
    std::optional<std::string> port;           // For serial devices
    std::optional<std::vector<std::string>> paths; // For mass storage
};

struct FreeWiliDevice {
    std::string name, serial;
    USBDevice usbHub;
    std::vector<USBDevice> usbDevices;

    auto getUSBDevices(USBDeviceType type) const -> std::vector<USBDevice>;
};
```

### C API (`cfwfinder.h`)

#### Core Functions

```c
// Find all devices
fw_error_t fw_device_find_all(fw_freewili_device_t** devices, uint32_t* count,
                              char* error_msg, uint32_t* error_size);

// Device validation and info
bool fw_device_is_valid(fw_freewili_device_t* device);
fw_error_t fw_device_get_str(fw_freewili_device_t* device, fw_stringtype_t type,
                             char* buffer, uint32_t buffer_size);

// USB device enumeration
fw_error_t fw_usb_device_begin(fw_freewili_device_t* device);
fw_error_t fw_usb_device_count(fw_freewili_device_t* device, uint32_t* count);
fw_error_t fw_usb_device_next(fw_freewili_device_t* device);
fw_error_t fw_usb_device_get_str(fw_freewili_device_t* device, fw_stringtype_t type,
                                 char* buffer, uint32_t buffer_size);
fw_error_t fw_usb_device_get_int(fw_freewili_device_t* device, fw_inttype_t type,
                                 uint32_t* value);

// Memory management
fw_error_t fw_device_free(fw_freewili_device_t** devices, uint32_t count);
```

## Examples

Complete example applications are provided in the `examples/` directory:

- **`basic_usage.cpp`** - Modern C++ API demonstration
- **`c_api_basic_usage.cpp`** - C API demonstration (compiled as C++)
- **`c_api_basic_usage_pure_c.c`** - Pure C implementation

Build and run examples:

```bash
cd build
make basic_usage_cpp basic_usage_c
./examples/basic_usage_cpp
./examples/basic_usage_c
```

## Project Structure

```
freewili-finder/
├── include/
│   ├── fwfinder.hpp          # Main C++ API header
│   └── usbdef.hpp            # USB device definitions
├── src/
│   ├── fwfinder.cpp          # Core implementation
│   ├── fwfinder_linux.cpp    # Linux-specific code
│   ├── fwfinder_mac.cpp      # macOS-specific code
│   ├── fwfinder_windows.cpp  # Windows-specific code
│   └── usbdef.cpp            # USB device type mappings
├── c_api/
│   ├── include/cfwfinder.h   # C API header
│   ├── src/cfwfinder.cpp     # C API implementation
│   └── test/                 # C API tests
├── test/                     # C++ API tests
├── examples/                 # Example applications
└── bindings/                 # Language bindings (Python, etc.)
```

## Testing

The library includes comprehensive tests using GoogleTest:

```bash
cd build
ctest --output-on-failure
```

Tests cover:
- Device discovery functionality
- USB device type detection
- C API functionality
- Cross-platform compatibility
- Error handling scenarios

## Platform Support

| Platform | Status | Requirements |
|----------|--------|--------------|
| Linux | ✅ Supported | `libudev` |
| macOS | ✅ Supported | IOKit framework |
| Windows | ✅ Supported | Windows SDK (SetupAPI, Cfgmgr32) |

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

Please ensure all tests pass and add tests for new functionality.

## License

Copyright Free-Wili

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
