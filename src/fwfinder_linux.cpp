#ifdef __linux__

    #include <fwfinder.hpp>
    #include <usbdef.hpp>

    #include <libudev.h>
    #include <mntent.h>

    #include <expected>
    #include <string>
    #include <cstdio>
    #include <iostream>
    #include <iomanip>
    #include <algorithm>
    #include <charconv>

// Helper function to get udev device attribute
std::string get_device_property(struct udev_device* dev, const char* property) {
    const char* value = udev_device_get_sysattr_value(dev, property);
    return value ? value : "";
}

auto _findMountPoints(const std::string& devnode) noexcept -> std::vector<std::string> {
    std::vector<std::string> mountPoints;
    FILE* mtab = setmntent("/proc/mounts", "r");
    if (!mtab) {
        perror("setmntent");
        return mountPoints;
    }

    struct mntent* ent;

    while ((ent = getmntent(mtab)) != nullptr) {
        if (std::string(ent->mnt_fsname).contains(devnode)) {
            mountPoints.push_back(ent->mnt_dir);
            //std::cout << "Device " << devnode << " is mounted at " << ent->mnt_dir << std::endl;
        }
    }

    endmntent(mtab);

    return mountPoints;
}

// Template function to convert string to integer types
template<typename T>
auto string_to_int(const std::string& str, int base = 10) -> std::optional<T>
    requires std::is_integral_v<T>
{
    T value = 0;
    if (std::from_chars(str.data(), str.data() + str.size(), value, base).ec == std::errc {}) {
        return value;
    }
    return std::nullopt;
}

struct DiskInfo {
    /// /sys/devices/pci0000:00/0000:00:01.2/0000:02:00.0/usb1/1-2/1-2.1
    std::string devPath;
    /// /dev/sdX
    std::string diskName;
    /// USB serial descriptor
    std::string serial;
    /// actual file-system mount paths
    std::vector<std::string> mountPoints;
};

auto _listDisks() noexcept -> std::vector<DiskInfo> {
    std::vector<DiskInfo> foundDisks;
    struct udev* udev = udev_new();
    if (!udev) {
        return foundDisks;
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block"); // Only block devices
    udev_enumerate_add_match_property(
        enumerate,
        "DEVTYPE",
        "disk"
    ); // only whole disks, not partitions
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
        const char* syspath = udev_list_entry_get_name(entry);
        struct udev_device* usbDisk = udev_device_new_from_syspath(udev, syspath);

        const char* devnode = udev_device_get_devnode(usbDisk); // Should give /dev/sdX
        if (!devnode) {
            udev_device_unref(usbDisk);
            continue;
        }
        // Go up to the USB device
        struct udev_device* parent =
            udev_device_get_parent_with_subsystem_devtype(usbDisk, "usb", "usb_device");
        if (parent) {
            auto mountPoints = _findMountPoints(devnode);
            std::string serial = get_device_property(parent, "serial");
            std::string devPath = udev_device_get_syspath(parent);

            foundDisks.push_back(
                DiskInfo {
                    .devPath = devPath,
                    .diskName = devnode,
                    .serial = serial,
                    .mountPoints = mountPoints,
                }
            );
        }
        udev_device_unref(usbDisk);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return foundDisks;
}

struct SerialInfo {
    /// /sys/devices/pci0000:00/0000:00:01.2/0000:02:00.0/usb1/1-2/1-2.1
    std::string devPath;
    /// /dev/ttyACM0 /dev/ttyUSB0
    std::string ttyName;
    /// USB serial descriptor
    std::string serial;
};

auto usbPortChainFromUdevDevice(udev_device* dev) -> std::vector<uint32_t> {
    if (!dev) {
        return {};
    }

    std::vector<uint32_t> portChain;

    // Start with the current device and walk up the tree
    udev_device* current = dev;

    while (current) {
        std::string sysnum;
        if (const char* value = udev_device_get_sysnum(current); value) {
            sysnum = value;
        }
        if (sysnum.empty()) {
            break;
        }

        try {
            uint32_t num = static_cast<uint32_t>(std::stoul(sysnum));
            portChain.push_back(num);
        } catch (...) {
            break;
        }

        udev_device* parent =
            udev_device_get_parent_with_subsystem_devtype(current, "usb", "usb_device");
        if (!parent) {
            break;
        }

        // Move up to the parent for next iteration
        current = parent;
    }

    // Reverse the chain so it goes from root to leaf
    std::reverse(portChain.begin(), portChain.end());

    return portChain;
}

auto _listSerialPorts() noexcept -> std::vector<SerialInfo> {
    std::vector<SerialInfo> foundSerials;
    struct udev* udev = udev_new();
    if (!udev) {
        return foundSerials;
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;

    udev_list_entry_foreach(entry, devices) {
        const char* syspath = udev_list_entry_get_name(entry);
        struct udev_device* tty = udev_device_new_from_syspath(udev, syspath);

        const char* devNode =
            udev_device_get_devnode(tty); // Should give /dev/ttyUSBx or /dev/ttyACMx
        if (!devNode) {
            udev_device_unref(tty);
            continue;
        }

        // Go up to the USB device
        struct udev_device* parent =
            udev_device_get_parent_with_subsystem_devtype(tty, "usb", "usb_device");
        if (parent) {
            std::string serial = get_device_property(parent, "serial");
            std::string devPath = udev_device_get_syspath(parent);
            foundSerials.push_back(
                SerialInfo {
                    .devPath = devPath,
                    .ttyName = devNode,
                    .serial = serial,
                }
            );
        }
        udev_device_unref(tty);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return foundSerials;
}

auto _createUSBDeviceFromUDevDevice(
    udev_device* dev,
    std::optional<std::reference_wrapper<const DiskInfo>> diskInfo,
    std::optional<std::reference_wrapper<const SerialInfo>> serialInfo
) -> std::optional<Fw::USBDevice> {
    using namespace Fw;

    if (!dev) {
        return std::nullopt;
    }

    // Get device properties
    std::string idVendor = get_device_property(dev, "idVendor");
    std::string idProduct = get_device_property(dev, "idProduct");

    if (idVendor.empty() || idProduct.empty()) {
        return std::nullopt;
    }

    uint16_t vid = string_to_int<uint16_t>(idVendor, 16).value_or(0);
    uint16_t pid = string_to_int<uint16_t>(idProduct, 16).value_or(0);

    if (vid == 0 || pid == 0) {
        return std::nullopt;
    }

    std::string manufacturer = get_device_property(dev, "manufacturer");
    std::string productName = get_device_property(dev, "product");
    std::string serial = get_device_property(dev, "serial");
    const char* _devPath = udev_device_get_syspath(dev);
    std::string devPath = _devPath ? _devPath : "";
    const char* _sysnum = udev_device_get_sysnum(dev);
    std::string sysnum = _sysnum ? _sysnum : "";
    uint32_t location = string_to_int<uint32_t>(sysnum, 10).value_or(0);
    std::vector<uint32_t> portChain = usbPortChainFromUdevDevice(dev);

    // Build device name
    std::string deviceName;
    if (!manufacturer.empty() && !productName.empty()) {
        deviceName = manufacturer + " " + productName;
    } else if (!productName.empty()) {
        deviceName = productName;
    } else if (!manufacturer.empty()) {
        deviceName = manufacturer;
    }

    return Fw::USBDevice {
        .kind = Fw::getUSBDeviceTypeFrom(vid, pid, location),
        .vid = vid,
        .pid = pid,
        .name = deviceName,
        .serial = serial,
        .location = static_cast<uint8_t>(location),
        .portChain = portChain,
        .paths = diskInfo.has_value()
            ? std::optional<std::vector<std::string>>(diskInfo->get().mountPoints)
            : std::nullopt,
        .port = serialInfo.has_value() ? std::optional<std::string>(serialInfo->get().ttyName)
                                       : std::nullopt,
        ._raw = devPath,
    };
}

auto _find_all_standalone(
    const std::vector<DiskInfo>& disks,
    const std::vector<SerialInfo>& serialPorts
) noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    using namespace Fw;
    Fw::FreeWiliDevices fwDevices;

    struct udev* udev = udev_new();
    if (!udev) {
        return std::unexpected("Failed to initialize udev");
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* dev_list_entry;

    udev_list_entry_foreach(dev_list_entry, devices) {
        const char* path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);

        std::string idVendor = get_device_property(dev, "idVendor");
        std::string idProduct = get_device_property(dev, "idProduct");
        uint16_t vid = string_to_int<uint16_t>(idVendor, 16).value_or(0);
        uint16_t pid = string_to_int<uint16_t>(idProduct, 16).value_or(0);

        // Check if this is a standalone device
        if (!Fw::isStandAloneDevice(vid, pid)) {
            udev_device_unref(dev);
            continue;
        }

        // Check if the parent USB controller is NOT a FreeWili Hub
        struct udev_device* parent =
            udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (parent) {
            std::string parentIdVendor = get_device_property(parent, "idVendor");
            std::string parentIdProduct = get_device_property(parent, "idProduct");
            uint16_t parentVid = string_to_int<uint16_t>(parentIdVendor, 16).value_or(0);
            uint16_t parentPid = string_to_int<uint16_t>(parentIdProduct, 16).value_or(0);

            // Skip if parent is a FreeWili Hub
            if (parentVid == USB_VID_FW_HUB && parentPid == USB_PID_FW_HUB) {
                udev_device_unref(dev);
                continue;
            }
        }

        // Get device information
        std::string manufacturer = get_device_property(dev, "manufacturer");
        std::string productName = get_device_property(dev, "product");
        std::string serial = get_device_property(dev, "serial");
        const char* _sysnum = udev_device_get_sysnum(dev);
        std::string sysnum = _sysnum ? _sysnum : "";
        uint32_t deviceLocation = string_to_int<uint32_t>(sysnum, 10).value_or(0);
        std::string devPath = udev_device_get_syspath(dev);
        std::vector<uint32_t> portChain = usbPortChainFromUdevDevice(dev);

        // Find associated disk
        auto diskIter = std::find_if(disks.begin(), disks.end(), [&](const DiskInfo& disk) {
            return disk.devPath == devPath;
        });

        // Find associated serial port
        auto serialIter =
            std::find_if(serialPorts.begin(), serialPorts.end(), [&](const SerialInfo& serialInfo) {
                return serialInfo.devPath == devPath;
            });

        // Create USB devices list
        USBDevices devices;

        // Add the standalone device
        devices.push_back(
            Fw::USBDevice {
                .kind = Fw::getUSBDeviceTypeFrom(vid, pid, deviceLocation),
                .vid = vid,
                .pid = pid,
                .name = manufacturer.empty() ? productName : manufacturer + " " + productName,
                .serial = serial,
                .location = static_cast<uint8_t>(deviceLocation),
                .portChain = portChain,
                .paths = diskIter == disks.end()
                    ? std::nullopt
                    : std::optional<std::vector<std::string>>(diskIter->mountPoints),
                .port = serialIter == serialPorts.end()
                    ? std::nullopt
                    : std::optional<std::string>(serialIter->ttyName),
                ._raw = devPath,
            }
        );

        // Create FreeWili device from USB devices
        if (auto result = Fw::FreeWiliDevice::fromUSBDevices(devices); result.has_value()) {
            auto fwDevice = std::move(result.value());
            fwDevices.push_back(std::move(fwDevice));
        } else {
            std::cerr << "Failed to create FreeWiliDevice: " << result.error() << std::endl;
        }

        udev_device_unref(dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    // Sort the devices by UniqueID
    std::sort(
        fwDevices.begin(),
        fwDevices.end(),
        [](const Fw::FreeWiliDevice& lhs, const Fw::FreeWiliDevice& rhs) {
            return lhs.uniqueID < rhs.uniqueID;
        }
    );

    return fwDevices;
}

auto _find_all_freewili(
    const std::vector<DiskInfo>& disks,
    const std::vector<SerialInfo>& serialPorts
) noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    using namespace Fw;
    // helper function to find all usb devices attached to the hub
    auto findUsbHubChildren = [&](udev* udev,
                                  udev_list_entry* dev_list_entry,
                                  udev_list_entry* devices,
                                  const USBDevice& usbHubDevice) -> USBDevices {
        USBDevices foundUsbDevices;
        udev_list_entry_foreach(dev_list_entry, devices) {
            const char* path = udev_list_entry_get_name(dev_list_entry);
            struct udev_device* dev = udev_device_new_from_syspath(udev, path);

            struct udev_device* parent =
                udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
            if (!parent) {
                udev_device_unref(dev);
                continue;
            }
            std::string parentPath = udev_device_get_syspath(parent);
            if (!parentPath.contains(usbHubDevice._raw)) {
                udev_device_unref(dev);
                continue;
            }
            // Finally we matched a USB child to the parent hub
            std::string idVendor = get_device_property(dev, "idVendor");
            std::string idProduct = get_device_property(dev, "idProduct");
            if (idVendor.empty() || idProduct.empty()) {
                udev_device_unref(dev);
                continue;
            }
            std::string manufacturer = get_device_property(dev, "manufacturer");
            std::string productName = get_device_property(dev, "product");
            std::string serial = get_device_property(dev, "serial");
            const char* _devPath = udev_device_get_syspath(dev);
            std::string devPath = _devPath ? _devPath : "";
            const char* busnum = udev_device_get_sysattr_value(dev, "busnum");
            const char* devnum = udev_device_get_sysattr_value(dev, "devnum");
            const char* _sysnum = udev_device_get_sysnum(dev);
            std::string sysnum = _sysnum ? _sysnum : "";
            const char* _devNode = udev_device_get_devnode(dev);
            uint16_t vid = string_to_int<uint16_t>(idVendor, 16).value_or(0);
            uint16_t pid = string_to_int<uint16_t>(idProduct, 16).value_or(0);
            uint32_t location = string_to_int<uint32_t>(sysnum, 10).value_or(0);
            std::vector<uint32_t> portChain = usbPortChainFromUdevDevice(dev);
            if (vid == 0 || pid == 0) {
                udev_device_unref(dev);
                continue;
            }
            auto diskPathIter = std::find_if(disks.begin(), disks.end(), [&](const DiskInfo& disk) {
                return devPath.contains(disk.devPath);
            });
            auto serialIter =
                std::find_if(serialPorts.begin(), serialPorts.end(), [&](const SerialInfo& serial) {
                    return devPath.contains(serial.devPath);
                });
            foundUsbDevices.push_back(
                USBDevice {
                    .kind = Fw::getUSBDeviceTypeFrom(vid, pid, location),
                    .vid = vid,
                    .pid = pid,
                    .name = manufacturer + " " + productName,
                    .serial = serial,
                    .location = static_cast<uint8_t>(location),
                    .portChain = portChain,
                    .paths = diskPathIter == disks.end()
                        ? std::nullopt
                        : std::optional<std::vector<std::string>>(diskPathIter->mountPoints),
                    .port = serialIter == serialPorts.end()
                        ? std::nullopt
                        : std::optional<std::string>(serialIter->ttyName),
                    ._raw = devPath,
                }
            );
            udev_device_unref(dev);
        }
        return foundUsbDevices;
    };

    Fw::FreeWiliDevices fwDevices;

    struct udev* udev = udev_new();
    if (!udev) {
        return std::unexpected("Failed to initialize udev");
    }

    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "usb");
    udev_enumerate_scan_devices(enumerate);

    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* dev_list_entry;

    // Find the Hubs
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(4) << std::hex << USB_VID_FW_HUB;
    const std::string targetHubVid = ss.str();
    ss = {};
    ss << std::setfill('0') << std::setw(4) << std::hex << USB_PID_FW_HUB;
    const std::string targetHubPid = ss.str();

    USBDevices usbHubDevices;
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char* path = udev_list_entry_get_name(dev_list_entry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);
        std::string idVendor = get_device_property(dev, "idVendor");
        std::string idProduct = get_device_property(dev, "idProduct");
        std::string manufacturer = get_device_property(dev, "manufacturer");
        std::string productName = get_device_property(dev, "product");
        std::string serial = get_device_property(dev, "serial");
        const char* _sysnum = udev_device_get_sysnum(dev);
        std::string sysnum = _sysnum ? _sysnum : "";
        uint16_t vid = string_to_int<uint16_t>(idVendor, 16).value_or(0);
        uint16_t pid = string_to_int<uint16_t>(idProduct, 16).value_or(0);
        uint32_t location = string_to_int<uint32_t>(sysnum, 10).value_or(0);
        std::vector<uint32_t> portChain = usbPortChainFromUdevDevice(dev);
        if (idVendor == targetHubVid && idProduct == targetHubPid) {
            std::string hubSysPath = udev_device_get_syspath(dev);
            auto hubDevice = USBDevice {
                .kind = Fw::getUSBDeviceTypeFrom(vid, pid, location),
                .vid = vid,
                .pid = pid,
                .name = productName,
                .serial = serial,
                .location = static_cast<uint8_t>(location),
                .portChain = portChain,
                .paths = std::nullopt,
                .port = std::nullopt,
                ._raw = hubSysPath,
            };
            auto usbChildren = findUsbHubChildren(udev, dev_list_entry, devices, hubDevice);
            usbChildren.push_back(hubDevice);
            if (auto fwDeviceResult = Fw::FreeWiliDevice::fromUSBDevices(usbChildren);
                fwDeviceResult.has_value())
            {
                fwDevices.push_back(std::move(fwDeviceResult.value()));
            } else {
                std::cerr << fwDeviceResult.error();
            }
        } else {
            // Not a FreeWili Hub, lets see if its a whitelisted device
            if (!Fw::is_vid_pid_whitelisted(vid, pid)) {
                udev_device_unref(dev);
                continue;
            }
            // We have a whitelisted device by itself that isn't behind a hub
            //auto usbChildren = findUsbHubChildren(udev, dev_list_entry, devices, USBDevice);
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return fwDevices;
}

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    const auto disks = _listDisks();
    const auto serialPorts = _listSerialPorts();

    Fw::FreeWiliDevices devices;
    if (auto result = _find_all_standalone(disks, serialPorts); result.has_value()) {
        devices = std::move(result.value());
    }
    if (auto result = _find_all_freewili(disks, serialPorts); result.has_value()) {
        devices.insert(devices.end(), result.value().begin(), result.value().end());
    } else {
        return std::unexpected(result.error());
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

#endif // __linux__
