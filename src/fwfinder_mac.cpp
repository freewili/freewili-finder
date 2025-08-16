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
    #include <set>

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

// Extract USB port number from macOS locationID
auto getUSBPortFromLocationID(uint32_t locationID) noexcept -> uint32_t {
    // macOS locationID format is topology-dependent:
    // Direct connection:    Hub=0x100000, children=0x110000, 0x120000, 0x130000
    // External hub:         Hub=0x1110000, children=0x1111000, 0x1112000, 0x1113000
    // 
    // For external hub children, the pattern is 0x11XY000 where:
    // - Bits 16-19 (X) = hub identifier (always 1 for external hub)
    // - Bits 12-15 (Y) = actual port number (1, 2, 3)
    //
    // For direct connection children, the pattern is 0x1X0000 where:
    // - Bits 16-19 (X) = port number (1, 2, 3)
    
    uint32_t portCandidate1 = (locationID >> 16) & 0x0F;  // Bits 16-19
    uint32_t portCandidate2 = (locationID >> 12) & 0x0F;  // Bits 12-15
    
    // Check if this looks like an external hub child device (0x11XY000 pattern)
    if ((locationID & 0xFF0F000) != 0 && portCandidate1 == 1 && portCandidate2 != 0) {
        // This is likely an external hub child - use bits 12-15 for port number
        return portCandidate2;
    }
    
    // For direct connections or hub devices, use bits 16-19
    if (portCandidate1 != 0) {
        return portCandidate1;
    }
    
    // For hub devices or when we can't extract port info, 
    // use controller info to distinguish different physical USB ports
    uint32_t controller = (locationID >> 24) & 0xFF;
    uint32_t altController = (locationID >> 20) & 0x0F;
    
    // Return a reasonable identifier based on what we can extract
    if (controller > 0) {
        return controller + 1;
    }
    return altController + 1;
}

auto getPropertyAsInt(io_service_t& usbDevice, CFStringRef propertyName)
    -> std::optional<uint32_t> {
    uint32_t value = 0;
    if (auto res = (CFNumberRef
        )IORegistryEntryCreateCFProperty(usbDevice, propertyName, kCFAllocatorDefault, 0);
        !res)
    {
        return std::nullopt;
    } else if (!CFNumberGetValue(res, kCFNumberSInt32Type, &value)) {
        CFRelease(res);
        return std::nullopt;
    } else {
        CFRelease(res);
    }
    return value;
}

auto getPropertyAsStr(io_service_t& usbDevice, CFStringRef propertyName)
    -> std::optional<std::string> {
    std::string value(1024, '\0');
    assert(value.capacity() >= 1024);
    if (CFStringRef res = (CFStringRef
        )IORegistryEntryCreateCFProperty(usbDevice, propertyName, kCFAllocatorDefault, 0);
        !res)
    {
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

auto findSerialPort(io_object_t entry, int level) noexcept
    -> std::expected<std::string, std::string> {
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
        CFTypeRef calloutPath = IORegistryEntryCreateCFProperty(
            child,
            CFSTR("IOCalloutDevice"),
            kCFAllocatorDefault,
            0
        );
        if (calloutPath && CFGetTypeID(calloutPath) == CFStringGetTypeID()) {
            char path[256] {};
            CFStringGetCString((CFStringRef)calloutPath, path, sizeof(path), kCFStringEncodingUTF8);
            // printf("✅ Found serial device: %s\n", path);
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

auto findStoragePah(io_object_t entry, int level) noexcept
    -> std::expected<std::string, std::string> {
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
        CFTypeRef whole =
            IORegistryEntryCreateCFProperty(child, CFSTR("Whole"), kCFAllocatorDefault, 0);
        if (whole && CFGetTypeID(whole) == CFBooleanGetTypeID()
            && CFBooleanGetValue((CFBooleanRef)whole))
        {
            CFTypeRef bsdName =
                IORegistryEntryCreateCFProperty(child, CFSTR("BSD Name"), kCFAllocatorDefault, 0);
            if (bsdName && CFGetTypeID(bsdName) == CFStringGetTypeID()) {
                char path[128];
                CFStringGetCString((CFStringRef)bsdName, path, sizeof(path), kCFStringEncodingUTF8);
                // printf("✅ Found mass storage: /dev/%s\n", path);
                storagePath = "/dev/" + std::string(path);

                // Shell out to diskutil to get the mount point
                std::stringstream ss;
                ss << "diskutil info " << storagePath
                   << "s1 | awk -F': *' '/Mount Point/ { print $2 }'";
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

// Helper function to find all USB device children of a given device using IOKit registry
static auto findUSBChildren(io_service_t parentDevice) noexcept -> std::vector<io_service_t> {
    std::vector<io_service_t> children;
    
    io_iterator_t childIterator;
    if (IORegistryEntryGetChildIterator(parentDevice, kIOServicePlane, &childIterator) != KERN_SUCCESS) {
        return children;
    }
    
    io_service_t child;
    while ((child = IOIteratorNext(childIterator))) {
        // Check if this child is a USB device
        io_name_t className;
        if (IOObjectGetClass(child, className) == KERN_SUCCESS) {
            if (strcmp(className, "IOUSBDevice") == 0 || strcmp(className, "IOUSBHostDevice") == 0) {
                children.push_back(child);
                // Don't release here - caller will release
            } else {
                // Recursively check grandchildren
                auto grandchildren = findUSBChildren(child);
                children.insert(children.end(), grandchildren.begin(), grandchildren.end());
                IOObjectRelease(child);
            }
        } else {
            IOObjectRelease(child);
        }
    }
    
    IOObjectRelease(childIterator);
    return children;
}

static auto _find_all_fw_classic() noexcept
    -> std::expected<Fw::FreeWiliDevices, std::string> {
    CFMutableDictionaryRef matchingDict;
    if (matchingDict = IOServiceMatching(kIOUSBDeviceClassName); !matchingDict) {
        return std::unexpected("IOServiceMatching() Failure");
    }
    
    io_iterator_t iter;
    if (auto res = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iter);
        res != KERN_SUCCESS)
    {
        std::stringstream ss;
        ss << "IOServiceGetMatchingServices() Failure: " << res;
        return std::unexpected(ss.str());
    }

    std::vector<std::vector<Fw::USBDevice>> hubGroups;
    io_service_t usbDevice;
    
    // Find all FreeWili hubs first
    while ((usbDevice = IOIteratorNext(iter))) {
        uint16_t vid = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("idVendor")); result.has_value()) {
            vid = result.value();
        }
        uint16_t pid = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("idProduct")); result.has_value()) {
            pid = result.value();
        }
        
        // Only process FreeWili hubs
        if (vid != Fw::USB_VID_FW_HUB || pid != Fw::USB_PID_FW_HUB) {
            IOObjectRelease(usbDevice);
            continue;
        }
        
        uint32_t addr = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("locationID")); result.has_value()) {
            addr = result.value();
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
        // Create the hub device
        Fw::USBDevice hubDevice {
            .kind = Fw::USBDeviceType::Hub,
            .vid = vid,
            .pid = pid,
            .name = manuName + " " + productName,
            .serial = serial,
            .location = getUSBPortFromLocationID(addr),
            .paths = std::nullopt,
            .port = std::nullopt,
            ._raw = ""
        };

        std::vector<Fw::USBDevice> hubGroup;
        hubGroup.push_back(hubDevice);

        // Find all children of this hub using IOKit registry
        auto children = findUSBChildren(usbDevice);
        for (auto childDevice : children) {
            uint16_t childVid = 0;
            if (auto result = getPropertyAsInt(childDevice, CFSTR("idVendor")); result.has_value()) {
                childVid = result.value();
            }
            uint16_t childPid = 0;
            if (auto result = getPropertyAsInt(childDevice, CFSTR("idProduct")); result.has_value()) {
                childPid = result.value();
            }
            
            // Only include whitelisted, non-standalone children
            if (Fw::is_vid_pid_whitelisted(childVid, childPid) && !Fw::isStandAloneDevice(childVid, childPid)) {
                uint32_t childAddr = 0;
                if (auto result = getPropertyAsInt(childDevice, CFSTR("locationID")); result.has_value()) {
                    childAddr = result.value();
                }

                std::string childManuName;
                if (auto result = getPropertyAsStr(childDevice, CFSTR("USB Vendor Name")); result.has_value()) {
                    childManuName = result.value();
                }
                std::string childProductName;
                if (auto result = getPropertyAsStr(childDevice, CFSTR("USB Product Name")); result.has_value()) {
                    childProductName = result.value();
                }
                std::string childSerial;
                if (auto result = getPropertyAsStr(childDevice, CFSTR("USB Serial Number")); result.has_value()) {
                    childSerial = result.value();
                }

                std::optional<std::string> serialPort;
                if (auto result = findSerialPort(childDevice, 0); result.has_value()) {
                    serialPort = result.value();
                }

                std::optional<std::string> storagePath;
                std::vector<std::string> storagePaths;
                if (auto result = findStoragePah(childDevice, 0); result.has_value()) {
                    storagePath = result.value();
                    storagePaths.push_back(result.value());
                }

                Fw::USBDevice childUSBDevice {
                    .kind = Fw::getUSBDeviceTypeFrom(childVid, childPid),
                    .vid = childVid,
                    .pid = childPid,
                    .name = childManuName + " " + childProductName,
                    .serial = childSerial,
                    .location = getUSBPortFromLocationID(childAddr),
                    .paths = storagePaths.empty() ? std::nullopt : std::make_optional(storagePaths),
                    .port = serialPort,
                    ._raw = ""
                };

                hubGroup.push_back(childUSBDevice);
            }
            IOObjectRelease(childDevice);
        }

        if (hubGroup.size() > 1) { // Hub + at least one child
            hubGroups.push_back(hubGroup);
        }

        IOObjectRelease(usbDevice);
    }

    IOObjectRelease(iter);

    // Convert hub groups to FreeWiliDevices
    Fw::FreeWiliDevices fwDevices;
    for (auto&& devices : hubGroups) {
        if (auto result = Fw::FreeWiliDevice::fromUSBDevices(devices); result.has_value()) {
            fwDevices.push_back(result.value());
        }
    }

    return fwDevices;
}

auto _find_all_standalone() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    CFMutableDictionaryRef matchingDict;
    if (matchingDict = IOServiceMatching(kIOUSBDeviceClassName); !matchingDict) {
        return std::unexpected("IOServiceMatching() Failure");
    }
    
    io_iterator_t iter;
    if (auto res = IOServiceGetMatchingServices(kIOMainPortDefault, matchingDict, &iter);
        res != KERN_SUCCESS)
    {
        std::stringstream ss;
        ss << "IOServiceGetMatchingServices() Failure: " << res;
        return std::unexpected(ss.str());
    }

    std::map<std::string, std::vector<Fw::USBDevice>> standaloneDevices;
    io_service_t usbDevice;
    
    while ((usbDevice = IOIteratorNext(iter))) {
        uint16_t vid = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("idVendor")); result.has_value()) {
            vid = result.value();
        }
        uint16_t pid = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("idProduct")); result.has_value()) {
            pid = result.value();
        }

        // Only process standalone devices - simple and direct check
        if (!Fw::is_vid_pid_whitelisted(vid, pid) || !Fw::isStandAloneDevice(vid, pid)) {
            IOObjectRelease(usbDevice);
            continue;
        }

        uint32_t addr = 0;
        if (auto result = getPropertyAsInt(usbDevice, CFSTR("locationID")); result.has_value()) {
            addr = result.value();
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

        // For standalone devices, use a unique key that includes the serial number
        // to ensure each device gets its own entry instead of being grouped together
        std::string deviceKey = containerId + "_" + serial + "_" + std::to_string(vid) + "_" + std::to_string(pid);

        standaloneDevices[deviceKey].push_back(Fw::USBDevice { 
            .kind = Fw::getUSBDeviceTypeFrom(vid, pid),
            .vid = vid,
            .pid = pid,
            .name = manuName + " " + productName,
            .serial = serial,
            .location = getUSBPortFromLocationID(addr),
            .paths = storagePaths.empty() ? std::nullopt : std::make_optional(storagePaths),
            .port = serialPort,
            ._raw = "" 
        });

        IOObjectRelease(usbDevice);
    }

    IOObjectRelease(iter);

    // Create FreeWiliDevice instances for each standalone device found
    Fw::FreeWiliDevices fwDevices;
    for (auto&& [deviceKey, devices] : standaloneDevices) {
        if (auto result = Fw::FreeWiliDevice::fromUSBDevices(devices); result.has_value()) {
            fwDevices.push_back(result.value());
        }
    }

    return fwDevices;
}

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    Fw::FreeWiliDevices allDevices;

    // First, find all classic FreeWili devices (hub-based)
    if (auto classicResult = _find_all_fw_classic(); classicResult.has_value()) {
        for (auto&& device : classicResult.value()) {
            allDevices.push_back(device);
        }
    }

    // Then, find all standalone devices (badges)
    if (auto standaloneResult = _find_all_standalone(); standaloneResult.has_value()) {
        for (auto&& device : standaloneResult.value()) {
            allDevices.push_back(device);
        }
    }

    return allDevices;
}

// NOLINTEND
#endif // __APPLE__
