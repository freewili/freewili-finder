#ifdef __APPLE__
// NOLINTBEGIN
    #include <fwfinder.hpp>
    #include <usbdef.hpp>

    #include <IOKit/IOKitLib.h>
    #include <IOKit/usb/IOUSBLib.h>
    #include <IOKit/usb/USBSpec.h>
    #include <CoreFoundation/CoreFoundation.h>

    #include <expected>
    #include <string>
    #include <iostream>
    #include <cassert>
    #include <sstream>
    #include <map>

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

auto findSerialPort(io_object_t entry, int level) noexcept -> std::expected<std::string, std::string> {
    io_iterator_t children;
    if (IORegistryEntryGetChildIterator(entry, kIOServicePlane, &children) != KERN_SUCCESS) {
        return std::unexpected("IORegistryEntryGetChildIterator() failed");
    }
    io_object_t child;
    std::string serialPort;
    while ((child = IOIteratorNext(children)) && serialPort.empty()) {
        io_name_t className;
        if (auto result = IOObjectGetClass(child, className); result != KERN_SUCCESS) {
            continue;
        }

        // Uncomment if you want to print the tree
        // for (int i = 0; i < level; i++) printf("  ");
        // printf("Class: %s\n", className);

        if (strcmp(className, "IOSerialBSDClient") != 0) {
            if (auto result = findSerialPort(child, level + 1); result.has_value()) {
                IOObjectRelease(child);
                return result.value();
            }
            IOObjectRelease(child);
            continue;
        }
        CFTypeRef calloutPath =
            IORegistryEntryCreateCFProperty(child, CFSTR("IOCalloutDevice"), kCFAllocatorDefault, 0);
        if (calloutPath && CFGetTypeID(calloutPath) == CFStringGetTypeID()) {
            char path[256] {};
            CFStringGetCString((CFStringRef)calloutPath, path, sizeof(path), kCFStringEncodingUTF8);
            printf("✅ Found serial device: %s\n", path);
            serialPort = path;
        }
        if (calloutPath) {
            CFRelease(calloutPath);
        }

        IOObjectRelease(child);
    }
    IOObjectRelease(children);
    if (serialPort.empty()) {
        return std::unexpected("Serial Port no found");
    } else {
        return serialPort;
    }
}

auto findStoragePah(io_object_t entry, int level) noexcept -> std::expected<std::string, std::string> {
    io_iterator_t children;
    if (IORegistryEntryGetChildIterator(entry, kIOServicePlane, &children) != KERN_SUCCESS) {
        return std::unexpected("IORegistryEntryGetChildIterator() failed");
    }
    io_object_t child;
    std::string storagePath;
    while ((child = IOIteratorNext(children)) && storagePath.empty()) {
        io_name_t className;
        if (auto result = IOObjectGetClass(child, className); result != KERN_SUCCESS) {
            continue;
        }

        // Uncomment if you want to print the tree
        // for (int i = 0; i < level; i++) printf("  ");
        // printf("Class: %s\n", className);

        if (strcmp(className, "IOMedia") != 0) {
            if (auto result = findStoragePah(child, level + 1); result.has_value()) {
                IOObjectRelease(child);
                return result.value();
            }
            IOObjectRelease(child);
            continue;
        }
        CFTypeRef whole = IORegistryEntryCreateCFProperty(child, CFSTR("Whole"), kCFAllocatorDefault, 0);
        if (whole && CFGetTypeID(whole) == CFBooleanGetTypeID() && CFBooleanGetValue((CFBooleanRef)whole)) {
            CFTypeRef bsdName = IORegistryEntryCreateCFProperty(child, CFSTR("BSD Name"), kCFAllocatorDefault, 0);
            if (bsdName && CFGetTypeID(bsdName) == CFStringGetTypeID()) {
                char path[128];
                CFStringGetCString((CFStringRef)bsdName, path, sizeof(path), kCFStringEncodingUTF8);
                // printf("✅ Found mass storage: /dev/%s\n", path);
                storagePath = "/dev/" + std::string(path);

                // Shell out to diskutil to get the mount point
                std::stringstream ss;
                ss << "diskutil info " << storagePath << "s1 | awk -F': *' '/Mount Point/ { print $2 }'";
                FILE* fp = popen(ss.str().c_str(), "r");
                if (fp) {
                    char mountPath[256] {};
                    if (fgets(mountPath, sizeof(mountPath), fp)) {
                        mountPath[strcspn(mountPath, "\n")] = 0; // Trim newline
                        if (strlen(mountPath) > 0) {
                            // printf("   📂 Mount Point: %s\n", mountPath);
                            storagePath = mountPath;
                        }
                    }
                    pclose(fp);
                }
            }
            if (bsdName) {
                CFRelease(bsdName);
            }
        }
        IOObjectRelease(child);
    }
    IOObjectRelease(children);
    if (storagePath.empty()) {
        return std::unexpected("Storage path no found");
    } else {
        return storagePath;
    }
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
    io_service_t usbDevice;
    std::map<std::string, std::vector<USBDevice>> containerDevices;
    while ((usbDevice = IOIteratorNext(iter))) {
        //io_name_t deviceName;
        std::string deviceName(128, '\0');
        assert(deviceName.capacity() >= 128);
        if (auto res = IORegistryEntryGetName(usbDevice, &deviceName[0]); res != KERN_SUCCESS) {
            IOObjectRelease(usbDevice);
            continue;
        }
        deviceName = deviceName.c_str();

        // Get the VID/PID and filter it
        uint16_t vid = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("idVendor")); result.has_value()) {
            vid = result.value();
        }
        uint16_t pid = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("idProduct")); result.has_value()) {
            pid = result.value();
        }
        if (!Fw::is_vid_pid_whitelisted(vid, pid)) {
            IOObjectRelease(usbDevice);
            continue;
        }
        uint8_t addr = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("USB Address")); result.has_value()) {
            addr = static_cast<uint8_t>(result.value());
        }

        std::string containerId;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("kUSBContainerID")); result.has_value()) {
            containerId = result.value();
        }
        std::string manuName;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Vendor Name")); result.has_value()) {
            manuName = result.value();
        }
        std::string productName;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Product Name")); result.has_value()) {
            productName = result.value();
        }
        std::string serial;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Serial Number")); result.has_value()) {
            serial = result.value();
        }

        std::optional<std::string> serialPort;
        if (auto result = findSerialPort(usbDevice, 0); result.has_value()) {
            serialPort = result.value();
        }

        std::optional<std::string> storagePath;
        std::vector<std::string> storagePaths;
        if (auto result = findStoragePah(usbDevice, 0); result.has_value()) {
            storagePath = result.value();
            storagePaths.push_back(result.value());
        }

        auto kind = Fw::getUSBDeviceTypeFrom(vid, pid);

        containerDevices[containerId].push_back(
            USBDevice {
                .kind = Fw::getUSBDeviceTypeFrom(vid, pid),
                .vid = vid,
                .pid = pid,
                .name = manuName + " " + productName,
                .serial = serial,
                .location = addr,
                .paths = std::nullopt,
                .port = serialPort,
                ._raw = ""
            }
        );
        if (!storagePaths.empty()) {
            containerDevices[containerId].back().paths = storagePaths;
        }
        IOObjectRelease(usbDevice);
    }

    /* Done, release the iterator */
    IOObjectRelease(iter);

    Fw::FreeWiliDevices fwDevices;
    for (auto&& devices : containerDevices) {
        if (auto result = Fw::FreeWiliDevice::fromUSBDevices(devices.second); result.has_value()) {
            fwDevices.push_back(result.value());
        } else {
            // TODO?
        }
    }
    return fwDevices;
}

// NOLINTEND
#endif // __APPLE__
