# FreeWili Finder Examples

This directory contains example code demonstrating how to use the fwfinder library.

## Examples

### 1. C++ Example (`basic_usage.cpp`)

Shows how to use the modern C++ API (`fwfinder.hpp`) to:
- Find all connected FreeWili devices
- Display device information (name, serial, USB hub info)
- Enumerate different types of USB devices (Serial, MainCPU, DisplayCPU, Mass Storage, FTDI)
- Handle errors using `std::expected`
- Demonstrate USB device type detection

**Key Features:**
- Uses modern C++23 features
- Type-safe enum classes
- `std::expected` for error handling
- RAII for automatic resource management

### 2. C API Examples

#### C API Example (Mixed C/C++ - `c_api_basic_usage.cpp`)

Shows how to use the C API (`cfwfinder.h`) to:
- Find all connected FreeWili devices using the C API
- Display device information
- Enumerate USB devices using the iterator pattern
- Handle errors using error codes
- Properly manage memory

**Key Features:**
- Uses C API but compiled as C++ for easier integration
- Error code based error handling
- Manual memory management
- Compatible with modern C++ projects

#### Pure C Example (`c_api_basic_usage_pure_c.c`)

Pure C implementation showing the same functionality:
- 100% C code (no C++ features)
- C89/C99 compatible
- Traditional C-style variable declarations
- C-style comments

**Key Features:**
- Pure C code
- Error code based error handling
- Manual memory management
- Compatible with C99 and later

## Building Examples

The examples are automatically built when you build the main project:

```bash
cd build
cmake ..
cmake --build .
```

This will create the following executables:
- `examples/basic_usage_cpp` - C++ example
- `examples/basic_usage_c` - C API example (compiled as C++ for easier integration)

### Running Examples

After building, you can run the examples from the build directory:

```bash
# Run C++ example
./examples/basic_usage_cpp

# Run C API example
./examples/basic_usage_c
```

Both examples will detect and display information about connected FreeWili devices.

## Example Output

When a FreeWili device is connected, you should see output similar to:

```
FreeWili Device Finder Example
==============================

Found 1 FreeWili device(s):

Device 1: Intrepid FreeWili
  Serial: FW5419
  USB Hub:  (VID: 0x0424, PID: 0x2513)
  Total USB Devices: 3
  USB Devices:
    MainCPU Serial Ports:
      - FreeWili MainCPU v49 (Serial: E463A8574B3E3539)
    DisplayCPU Serial Ports:
      - FreeWili DisplayCPU v47 (Serial: E463A8574B463639)
    FTDI/FPGA Devices:
      - Intrepid FreeWili (Serial: FW5419)
```

## Running Examples

### C++ Example
```bash
./examples/basic_usage_cpp
```

### C Example
```bash
./examples/basic_usage_c
```

## Sample Output

When a FreeWili device is connected, you might see output like:

```
FreeWili Device Finder Example
==============================

Found 1 FreeWili device(s):

Device 1: Intrepid FreeWili
  Serial: FW5419
  USB Hub: Intrepid FreeWili (VID: 0x0403, PID: 0x6014)
  Total USB Devices: 3
  USB Devices:
    MainCPU Serial Ports:
      - FreeWili MainCPU v49 (Serial: E463A8574B3E3539)
    DisplayCPU Serial Ports:
      - FreeWili DisplayCPU v47 (Serial: E463A8574B463639)
    FTDI/FPGA Devices:
      - Intrepid FreeWili (Serial: FW5419)
    All USB Devices (detailed):
      - FreeWili MainCPU v49 (VID: 0x093C, PID: 0x2054, Location: 1)
        Serial: E463A8574B3E3539
        Type: SerialMain
      - FreeWili DisplayCPU v47 (VID: 0x093C, PID: 0x2055, Location: 2)
        Serial: E463A8574B463639
        Type: SerialDisplay
      - Intrepid FreeWili (VID: 0x0403, PID: 0x6014, Location: 3)
        Serial: FW5419
        Type: FTDI

USB Device Type Examples:
  VID: 0x093C, PID: 0x2054 -> SerialMain
  VID: 0x093C, PID: 0x2055 -> SerialDisplay
  VID: 0x0403, PID: 0x6014 -> FTDI
```

## Error Handling

Both examples demonstrate proper error handling:

- **C++ Example**: Uses `std::expected<T, std::string>` for type-safe error handling
- **C Example**: Uses error codes (`fw_error_t`) and error messages

## API Documentation

- **C++ API**: See `include/fwfinder.hpp` for complete API documentation
- **C API**: See `c_api/include/cfwfinder.h` for complete C API documentation
