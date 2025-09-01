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
    #include <vector>
    #include <algorithm>

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

// Extract USB port number from macOS locationID following cyme project approach
auto getUSBPortFromLocationID(uint32_t locationID) noexcept -> uint32_t {
    // macOS locationID format: 0xbbdddddd where:
    // - bb = bus number in hex
    // - dddddd = up to six levels for the tree, each digit represents position at that level
    //
    // Following the cyme project approach:
    // Extract the tree positions and return the last (rightmost) non-zero port number
    // which represents the port on the immediate parent hub/controller

    // Extract the tree position digits (6 digits after bus)
    std::vector<uint32_t> treePositions;
    uint32_t locationDigits = locationID & 0x00FFFFFF; // Remove bus number

    // Extract each hex digit from right to left, but we want left to right order
    for (int i = 20; i >= 0; i -= 4) { // 6 digits * 4 bits each = 24 bits
        uint32_t digit = (locationDigits >> i) & 0x0F;
        if (digit != 0) {
            treePositions.push_back(digit);
        }
    }

    // The port number is the last (deepest) position in the tree
    // This represents the actual port on the immediate parent hub
    if (!treePositions.empty()) {
        return treePositions.back();
    }

    // Fallback: if we can't extract meaningful tree positions,
    // this might be a root hub or special case
    return 1;
}

// Extract USB port chain from macOS locationID following cyme project approach
auto getUSBPortChainFromLocationID(uint32_t locationID) noexcept -> std::vector<uint32_t> {
    // macOS locationID format: 0xbbdddddd where:
    // - bb = bus number in hex
    // - dddddd = up to six levels for the tree, each digit represents position at that level
    //
    // Following the cyme project approach for building the full port chain:
    // Extract each tree position digit from left to right to build the complete path

    std::vector<uint32_t> portChain;

    // Extract the tree position digits (6 digits after bus number)
    uint32_t locationDigits = locationID & 0x00FFFFFF; // Remove bus number (top 8 bits)

    // Extract each hex digit from left to right (most significant to least significant)
    // Each digit represents the port number at that level in the USB tree
    for (int i = 20; i >= 0; i -= 4) { // 6 digits * 4 bits each = 24 bits, starting from left
        uint32_t digit = (locationDigits >> i) & 0x0F;
        if (digit != 0) {
            portChain.push_back(digit);
        }
    }

    // If we couldn't extract any meaningful port chain, create a minimal one
    // This can happen for root hubs or devices directly connected to controller
    if (portChain.empty()) {
        // Use the bus number as a fallback to ensure each device has a unique path
        uint32_t busNumber = (locationID >> 24) & 0xFF;
        if (busNumber > 0) {
            portChain.push_back(busNumber);
        } else {
            portChain.push_back(1); // Default fallback
        }
    }

    return portChain;
}

template<typename T>
    requires std::is_integral_v<T>
auto getPropertyAsInt(io_service_t& usbDevice, CFStringRef propertyName) -> std::optional<T> {
    uint64_t value = 0;
    if (auto res = (CFNumberRef
        )IORegistryEntryCreateCFProperty(usbDevice, propertyName, kCFAllocatorDefault, 0);
        !res)
    {
        return std::nullopt;
    } else if (!CFNumberGetValue(res, kCFNumberSInt64Type, &value)) {
        CFRelease(res);
        return std::nullopt;
    } else {
        CFRelease(res);
    }
    return static_cast<T>(value);
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
                    // TODO: The RP2350 UF2 bootloader can hang diskutil.
                    // Seems to only happen when its been plugged in for a long time.
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
    if (IORegistryEntryGetChildIterator(parentDevice, kIOServicePlane, &childIterator)
        != KERN_SUCCESS)
    {
        return children;
    }

    io_service_t child;
    while ((child = IOIteratorNext(childIterator))) {
        // Check if this child is a USB device
        io_name_t className;
        if (IOObjectGetClass(child, className) == KERN_SUCCESS) {
            if (strcmp(className, "IOUSBDevice") == 0 || strcmp(className, "IOUSBHostDevice") == 0)
            {
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

static auto _find_all_fw_classic() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
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
        if (auto result = getPropertyAsInt<uint16_t>(usbDevice, CFSTR("idVendor"));
            result.has_value())
        {
            vid = result.value();
        }
        uint16_t pid = 0;
        if (auto result = getPropertyAsInt<uint16_t>(usbDevice, CFSTR("idProduct"));
            result.has_value())
        {
            pid = result.value();
        }

        // Only process FreeWili hubs
        if (vid != Fw::USB_VID_FW_HUB || pid != Fw::USB_PID_FW_HUB) {
            IOObjectRelease(usbDevice);
            continue;
        }

        uint32_t addr = 0;
        if (auto result = getPropertyAsInt<uint32_t>(usbDevice, CFSTR("locationID"));
            result.has_value())
        {
            addr = result.value();
        }

        std::string manuName;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Vendor Name")); result.has_value())
        {
            manuName = result.value();
        }
        std::string productName;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Product Name"));
            result.has_value())
        {
            productName = result.value();
        }
        std::string serial;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Serial Number"));
            result.has_value())
        {
            serial = result.value();
        }
        // Create the hub device
        Fw::USBDevice hubDevice { .kind = Fw::USBDeviceType::Hub,
                                  .vid = vid,
                                  .pid = pid,
                                  .name = manuName + " " + productName,
                                  .serial = serial,
                                  .location = getUSBPortFromLocationID(addr),
                                  .portChain = getUSBPortChainFromLocationID(addr),
                                  .paths = std::nullopt,
                                  .port = std::nullopt,
                                  ._raw = "" };

        std::vector<Fw::USBDevice> hubGroup;
        hubGroup.push_back(hubDevice);

        // Find all children of this hub using IOKit registry
        auto children = findUSBChildren(usbDevice);
        for (auto childDevice: children) {
            uint16_t childVid = 0;
            if (auto result = getPropertyAsInt<uint16_t>(childDevice, CFSTR("idVendor"));
                result.has_value())
            {
                childVid = result.value();
            }
            uint16_t childPid = 0;
            if (auto result = getPropertyAsInt<uint16_t>(childDevice, CFSTR("idProduct"));
                result.has_value())
            {
                childPid = result.value();
            }

            // Only include whitelisted, non-standalone children
            if (Fw::is_vid_pid_whitelisted(childVid, childPid)
                && !Fw::isStandAloneDevice(childVid, childPid))
            {
                uint32_t childAddr = 0;
                if (auto result = getPropertyAsInt<uint32_t>(childDevice, CFSTR("locationID"));
                    result.has_value())
                {
                    childAddr = result.value();
                }

                std::string childManuName;
                if (auto result = getPropertyAsStr(childDevice, CFSTR("USB Vendor Name"));
                    result.has_value())
                {
                    childManuName = result.value();
                }
                std::string childProductName;
                if (auto result = getPropertyAsStr(childDevice, CFSTR("USB Product Name"));
                    result.has_value())
                {
                    childProductName = result.value();
                }
                std::string childSerial;
                if (auto result = getPropertyAsStr(childDevice, CFSTR("USB Serial Number"));
                    result.has_value())
                {
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
                    .kind = Fw::getUSBDeviceTypeFrom(childVid, childPid, childAddr),
                    .vid = childVid,
                    .pid = childPid,
                    .name = childManuName + " " + childProductName,
                    .serial = childSerial,
                    .location = getUSBPortFromLocationID(childAddr),
                    .portChain = getUSBPortChainFromLocationID(childAddr),
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
    for (auto&& devices: hubGroups) {
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
        if (auto result = getPropertyAsInt<uint16_t>(usbDevice, CFSTR("idVendor"));
            result.has_value())
        {
            vid = result.value();
        }
        uint16_t pid = 0;
        if (auto result = getPropertyAsInt<uint16_t>(usbDevice, CFSTR("idProduct"));
            result.has_value())
        {
            pid = result.value();
        }

        // Only process standalone devices - simple and direct check
        if (!Fw::is_vid_pid_whitelisted(vid, pid) || !Fw::isStandAloneDevice(vid, pid)) {
            IOObjectRelease(usbDevice);
            continue;
        }

        uint32_t addr = 0;
        if (auto result = getPropertyAsInt<uint32_t>(usbDevice, CFSTR("locationID"));
            result.has_value())
        {
            addr = result.value();
        }
        std::string containerId;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("kUSBContainerID")); result.has_value())
        {
            containerId = result.value();
        }
        std::string manuName;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Vendor Name")); result.has_value())
        {
            manuName = result.value();
        }
        std::string productName;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Product Name"));
            result.has_value())
        {
            productName = result.value();
        }
        std::string serial;
        if (auto result = getPropertyAsStr(usbDevice, CFSTR("USB Serial Number"));
            result.has_value())
        {
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
        std::string deviceKey =
            containerId + "_" + serial + "_" + std::to_string(vid) + "_" + std::to_string(pid);

        standaloneDevices[deviceKey].push_back(
            Fw::USBDevice { .kind = Fw::getUSBDeviceTypeFrom(vid, pid, getUSBPortFromLocationID(addr)),
                            .vid = vid,
                            .pid = pid,
                            .name = manuName + " " + productName,
                            .serial = serial,
                            .location = getUSBPortFromLocationID(addr),
                            .portChain = getUSBPortChainFromLocationID(addr),
                            .paths = storagePaths.empty() ? std::nullopt
                                                          : std::make_optional(storagePaths),
                            .port = serialPort,
                            ._raw = "" }
        );

        IOObjectRelease(usbDevice);
    }

    IOObjectRelease(iter);

    // Create FreeWiliDevice instances for each standalone device found
    Fw::FreeWiliDevices fwDevices;
    for (auto&& [deviceKey, devices]: standaloneDevices) {
        if (auto result = Fw::FreeWiliDevice::fromUSBDevices(devices); result.has_value()) {
            fwDevices.push_back(result.value());
        }
    }

    return fwDevices;
}

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    Fw::FreeWiliDevices devices;

    // First, find all classic FreeWili devices (hub-based)
    if (auto classicResult = _find_all_fw_classic(); classicResult.has_value()) {
        for (auto&& device: classicResult.value()) {
            devices.push_back(device);
        }
    }

    // Then, find all standalone devices (badges)
    if (auto standaloneResult = _find_all_standalone(); standaloneResult.has_value()) {
        for (auto&& device: standaloneResult.value()) {
            devices.push_back(device);
        }
    }

    // Sort the devices by unique ID
    std::sort(
        devices.begin(),
        devices.end(),
        [](const Fw::FreeWiliDevice& lhs, const Fw::FreeWiliDevice& rhs) {
            // order smallest to largest
            return lhs.uniqueID < rhs.uniqueID;
        }
    );
    return devices;
}

// NOLINTEND
#endif // __APPLE__
