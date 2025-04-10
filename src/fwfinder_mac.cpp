#ifdef __APPLE__
// NOLINTBEGIN
    #include <fwfinder.hpp>

    #include <IOKit/IOKitLib.h>
    #include <IOKit/usb/IOUSBLib.h>
    #include <IOKit/usb/USBSpec.h>
    #include <CoreFoundation/CoreFoundation.h>

    #include <expected>
    #include <string>
    #include <iostream>
    #include <cassert>
    #include <sstream>

auto cfstrAsString(CFStringRef string_ref) noexcept -> std::string {
    if (string_ref == nullptr) {
        return std::string();
    }

    CFIndex length = CFStringGetLength(string_ref);
    CFRange range = CFRangeMake(0, length);
    CFIndex buffer_size = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
    char* buffer = new char[buffer_size];

    if (CFStringGetCString(string_ref, buffer, buffer_size, kCFStringEncodingUTF8)) {
        std::string result(buffer);
        delete[] buffer;
        return result;
    } else {
        delete[] buffer;
        return std::string();
    }
}

// Helper to convert char strings to std::string
auto _toString(const char* value) noexcept -> std::string {
    std::string strValue = value ? value : "";
    return strValue;
}

auto getPropertyAsInt(io_service_t& usbDevice, CFStringRef propertyName) -> std::optional<uint32_t> {
    uint32_t value = 0;
    if (auto res = (CFNumberRef)IORegistryEntryCreateCFProperty(usbDevice, propertyName, kCFAllocatorDefault, 0);
        !res) {
        return std::nullopt;
    } else if (!CFNumberGetValue(res, kCFNumberSInt32Type, &value)) {
        CFRelease(res);
        return std::nullopt;
    } else {
        CFRelease(res);
    }
    return value;
}

auto getPropertyAsStr(io_service_t& usbDevice, CFStringRef propertyName) -> std::optional<std::string> {
    std::string value(1024, '\0');
    assert(value.capacity() >= 1024);
    if (CFStringRef res = (CFStringRef)IORegistryEntryCreateCFProperty(usbDevice, propertyName, kCFAllocatorDefault, 0);
        !res) {
        return std::nullopt;
    } else if (!CFStringGetCString(res, &value[0], 1024, kCFStringEncodingUTF8)) {
        CFRelease(res);
        return std::nullopt;
    } else {
        CFRelease(res);
        value = value.c_str();
    }
    return value;
}

auto getUUIDPropertyAsStr(io_service_t& usbDevice, CFStringRef propertyName) -> std::optional<std::string> {
    std::string value(1024, '\0');
    assert(value.capacity() >= 1024);
    // CFUUIDRef res;
    // if (res = (CFUUIDRef)IORegistryEntryCreateCFProperty(usbDevice, propertyName, kCFAllocatorDefault, 0); !res) {
    //     return std::nullopt;
    // }
    // if (auto strRes = CFUUIDCreateString(kCFAllocatorDefault, res); !strRes) {
    //     CFRelease(res);
    //     return std::nullopt;
    // } else {
    //     if (!CFStringGetCString(resStr, &value[0], 1024, kCFStringEncodingUTF8))
    //         CFRelease(res);
    //     value = value.c_str();
    // }
    return value;
}

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    // Initialization to loop USB devices
    CFMutableDictionaryRef matchingDict;
    if (matchingDict = IOServiceMatching(kIOUSBDeviceClassName); !matchingDict) {
        return std::unexpected("IOServiceMatching() Failure");
    }
    io_iterator_t iter;
    if (auto res = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iter); res != KERN_SUCCESS) {
        std::stringstream ss;
        ss << "IOServiceGetMatchingServices() Failure: " << res;
        return std::unexpected(ss.str());
    }
    // Iterate over all the USB devices
    Fw::FreeWiliDevices fwDevices;
    io_service_t usbDevice;
    while ((usbDevice = IOIteratorNext(iter))) {
        //io_name_t deviceName;
        std::string deviceName(128, '\0');
        assert(deviceName.capacity() >= 128);
        if (auto res = IORegistryEntryGetName(usbDevice, &deviceName[0]); res != KERN_SUCCESS) {
            IOObjectRelease(usbDevice);
            continue;
        }
        deviceName = deviceName.c_str();
        std::cout << deviceName << "\n";

        auto printOptional = [](auto& opt) {
            if (opt.has_value()) {
                std::cout << "\t" << opt.value() << "\n";
            } else {
                std::cout << "\tNone.\n";
            }
        };
        auto vid = getPropertyAsInt(usbDevice, CFSTR("idVendor"));
        auto pid = getPropertyAsInt(usbDevice, CFSTR("idProduct"));
        auto loc = getPropertyAsInt(usbDevice, CFSTR("locationID"));

        auto manuName = getPropertyAsStr(usbDevice, CFSTR("USB Vendor Name"));
        auto productName = getPropertyAsStr(usbDevice, CFSTR("USB Product Name"));
        auto serial = getPropertyAsStr(usbDevice, CFSTR("USB Serial Number"));
        auto containerID = getPropertyAsStr(usbDevice, CFSTR("kUSBContainerID"));

        printOptional(vid);
        printOptional(pid);
        printOptional(loc);
        printOptional(manuName);
        printOptional(productName);
        printOptional(serial);
        printOptional(containerID);

        uint32_t locationID = 0;
        if (auto res =
                (CFNumberRef)IORegistryEntryCreateCFProperty(usbDevice, CFSTR("locationID"), kCFAllocatorDefault, 0);
            !res) {
            IOObjectRelease(usbDevice);
            continue;
        } else if (!CFNumberGetValue(res, kCFNumberSInt32Type, &locationID)) {
            IOObjectRelease(usbDevice);
            continue;
        }
        std::cout << "\t" << locationID << "\n";
        IOObjectRelease(usbDevice);
    }

    /* Done, release the iterator */
    IOObjectRelease(iter);

    return std::unexpected("TODO");
}

// NOLINTEND
#endif // __APPLE__
