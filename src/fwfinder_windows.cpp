// NOLINTBEGIN
#ifdef _WIN32
    #include <windows.h>
    #include <stringapiset.h>
    #include <winnls.h>
    #include <errhandlingapi.h>
    #include <tchar.h>
    #include <winioctl.h>
    #include <initguid.h>
    #include <Ntddstor.h>
    #include <cfgmgr32.h> // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
    #include <combaseapi.h>
    #include <devguid.h> // for GUID_DEVCLASS_CDROM etc
    #include <handleapi.h>
    #include <libloaderapi.h>
    #include <minwindef.h>
    #include <setupapi.h>
    #include <specstrings_strict.h>
    #include <winbase.h>
    #include <winnt.h>
    #include <devpkey.h>
    #include <fileapi.h>
    #include <usbiodef.h>
    #include <usbioctl.h>

    #include <cstdio>
    #include <cstring>
    #include <expected>
    #include <sstream>
    #include <string>
    #include <print>
    #include <algorithm>
    #include <iterator>
    #include <regex>
    #include <unordered_map>
    #include <map>
    #include <vector>
    #include <tuple>
    #include <cctype>
    #include <fwfinder.hpp>
    #include <fwfinder_windows.hpp>
    #include <usbdef.hpp>

//#define INITGUID

auto stringFromTCHARRaw(const TCHAR* const msg) -> std::expected<std::string, DWORD> {
    #if defined(UNICODE)
    if (size_t const width = WideCharToMultiByte(CP_UTF8, 0, msg, -1, nullptr, 0, nullptr, nullptr); width != 0) {
        std::string new_msg(width, '\0');
        if (auto success = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)msg, -1, new_msg.data(), width, nullptr, nullptr);
            success == 0) {
            return std::unexpected(GetLastError());
        }
        return new_msg.c_str();
    } else {
        return std::unexpected(GetLastError());
    }
    #else
    return std::string(msg);
    #endif
}

auto TCHARFromString(std::string value) noexcept -> std::unique_ptr<TCHAR[]> {
    #if defined(UNICODE)
    // Get the length required
    int len = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, NULL, 0);
    if (!len) {
        return nullptr;
    }
    std::unique_ptr<TCHAR[]> new_value(new TCHAR[len] {});
    if (auto success = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, new_value.get(), len); success == 0) {
        return nullptr;
    }
    return new_value;
    #else
    return value.c_str();
    #endif
}

auto getLastErrorString(DWORD errorCode) -> std::expected<std::string, DWORD> {
    TCHAR* buffer = nullptr;
    if (auto len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&buffer,
            0,
            nullptr
        );
        len != 0) {
        LocalFree(buffer);
        return stringFromTCHARRaw(buffer);
    } else {
        LocalFree(buffer);
        // Error state, FormatMessage failed.
        return std::unexpected(GetLastError());
    }
}

/// @brief Convert TCHAR to std::string
/// @param szValue string to be converted
/// @return std::string - error is put into the string if there was an error converting
auto stringFromTCHAR(const TCHAR* const szValue) -> std::string {
    if (auto result = stringFromTCHARRaw(szValue); result.has_value()) {
        return result.value();
    } else {
        std::stringstream ss;
        ss << "Error code: #" << result.error() << " (" << getLastErrorString(result.error()).value_or(std::string())
           << ")";
        return ss.str();
    };
};

/// @brief Helper function to split a string by a key
/// @param value string to split
/// @param key key to split string by
/// @return vector of strings
auto splitByKey(std::string value, char key) noexcept -> std::vector<std::string> {
    std::size_t current = 0, previous = 0;
    std::vector<std::string> values;
    do {
        current = value.find(key, previous);
        if (current == std::string::npos) {
            values.push_back(value.substr(previous, value.length() - 1));
            break;
        }
        values.push_back(value.substr(previous, current - previous));
        previous = current + 1;
    } while (current != std::string::npos);
    return values;
}

auto splitInstanceID(std::string value) -> std::expected<std::vector<std::string>, std::string> {
    auto values = splitByKey(value, '\\');
    // We should always have 3 values here.
    if (values.size() != 3) {
        std::stringstream ss;
        ss << "Size of values is wrong, expected 3 got " << values.size();
        return std::unexpected(ss.str());
    }
    return values;
}

auto getUSBInstanceID(std::string value) -> std::expected<USBInstanceID, std::string> {
    if (auto values = splitInstanceID(value); !values.has_value()) {
        return std::unexpected(values.error());
    } else {
        auto split = values.value();
        // seperate the VID/PID values
        std::regex re(R"((VID_[0-9a-fA-F]+)|(PID_[0-9a-fA-F]+))");
        auto match_begin = std::sregex_iterator(split[1].begin(), split[1].end(), re);
        std::vector<std::string> matches;
        for (auto iter = match_begin; iter != std::sregex_iterator(); ++iter) {
            matches.push_back(iter->str());
        }
        if (matches.size() != 2) {
            std::stringstream ss;
            ss << "Size of matches is wrong, expected 2 got " << matches.size();
            return std::unexpected(ss.str());
        }
        // Extract the VID hex values
        std::regex hex_re(R"([0-9a-fA-F]+$)");
        std::cmatch vid_hex_match;
        if (!std::regex_search(matches[0].c_str(), vid_hex_match, hex_re)) {
            return std::unexpected("Failed to regex_match Hex value of VID");
        }
        // Extract the PID hex values
        std::cmatch pid_hex_match;
        if (!std::regex_search(matches[1].c_str(), pid_hex_match, hex_re)) {
            return std::unexpected("Failed to regex_match Hex value of PID");
        }
        // Convert hex string to integer
        uint16_t vid = 0, pid = 0;
        std::stringstream ss;
        // VID
        ss << std::hex << vid_hex_match[0].str();
        ss >> vid;
        ss.clear();
        // PID
        ss << std::hex << pid_hex_match[0].str();
        ss >> pid;
        // Finally construct the struct and return it.
        auto usb_instance_id = USBInstanceID {
            .vid = vid,
            .pid = pid,
            .serial = split[2],
        };
        return usb_instance_id;
    }
}

// Helper function to get Registry Property
auto _getDeviceRegistryProp(HDEVINFO hDevInfo, SP_DEVINFO_DATA& devInfoData, DWORD prop) noexcept
    -> std::expected<std::string, std::string> {
    DWORD dwPropertyRegDataType = 0;
    TCHAR buffer[2048] {};
    if (SetupDiGetDeviceRegistryProperty(
            hDevInfo,
            &devInfoData,
            prop,
            &dwPropertyRegDataType,
            reinterpret_cast<BYTE*>(buffer),
            sizeof(buffer),
            nullptr
        )
        == FALSE) {
        auto errorCode = GetLastError();
        if (auto res = getLastErrorString(errorCode); res.has_value()) {
            return std::unexpected(res.value());
        } else {
            std::stringstream ss;
            ss << "Error code: #" << errorCode;
            return std::unexpected(ss.str());
        }
    }
    return stringFromTCHAR(buffer);
};

// Helper function to get device Property
auto _getDeviceProperty(DEVINST devInst, const DEVPROPKEY key) noexcept -> std::expected<std::string, std::string> {
    DEVPROPTYPE propertyType = DEVPROP_TYPE_STRING;
    TCHAR szValue[4096 + 1] {};
    ULONG size = 4096;
    if (auto res = CM_Get_DevNode_Property(devInst, &key, &propertyType, (BYTE*)szValue, &size, 0); res != CR_SUCCESS) {
        std::stringstream ss;
        ss << "CM_Get_DevNode_Property() failed with CR error: " << res;
        return std::unexpected(ss.str());
    }
    return stringFromTCHAR(szValue);
};

// Helper function to get device Property GUID
auto _getDevicePropertyGUID(DEVINST devInst, const DEVPROPKEY key) noexcept -> std::expected<GUID, std::string> {
    DEVPROPTYPE propertyType = DEVPROP_TYPE_GUID;
    GUID guid {};
    ULONG size = sizeof(guid);
    if (auto res = CM_Get_DevNode_Property(devInst, &key, &propertyType, (BYTE*)&guid, &size, 0); res != CR_SUCCESS) {
        std::stringstream ss;
        ss << "CM_Get_DevNode_Property() failed with CR error: " << res;
        return std::unexpected(ss.str());
    }
    return guid;
};

// Helper function to get device Property GUID as a String
auto _getDevicePropertyGUIDString(DEVINST devInst, const DEVPROPKEY key) noexcept
    -> std::expected<std::string, std::string> {
    if (auto guidResult = _getDevicePropertyGUID(devInst, key); guidResult.has_value()) {
        TCHAR szValue[4096 + 1] {};
        if (StringFromGUID2(guidResult.value(), szValue, 4096) == 0) {
            return std::unexpected("StringFromGUID2() failed");
        }
        return stringFromTCHAR(szValue);
    } else {
        return std::unexpected(guidResult.error());
    }
};

typedef std::unordered_map<std::string, std::string> VolumesMap;

/// @brief  Helper function to get all volumes attached to the host
/// @return VolumesMap on success, std::string on failure
auto getVolumes() noexcept -> std::expected<VolumesMap, std::string> {
    VolumesMap volumeLookupByDevice;
    {
        // example: "\\?\Volume{b6f17eca-5e7a-4475-9da7-7497fdd869c6}\"
        TCHAR szVolumeName[MAX_PATH + 1] {};
        DWORD szVolumeNameSize = ARRAYSIZE(szVolumeName) - 1;
        HANDLE handle = INVALID_HANDLE_VALUE;
        // Get the first volume
        if (handle = FindFirstVolume(&szVolumeName[0], szVolumeNameSize); handle == INVALID_HANDLE_VALUE) {
            auto lastError = GetLastError();
            if (auto errorRes = getLastErrorString(lastError); errorRes.has_value()) {
                return std::unexpected(errorRes.value());
            } else {
                std::stringstream ss;
                ss << "FindFirstVolume() failed with error code '" << lastError << "'";
                return std::unexpected(ss.str());
            }
        }
        // Loop through the rest of the volumes
        do {
            // Get the volume name
            TCHAR szDeviceName[MAX_PATH + 1] {};
            // Query with "\\?\" removed from the front and the last \\ removed
            szVolumeName[_tcslen(szVolumeName) - 1] = TEXT('\0');
            if (auto res = QueryDosDevice(&szVolumeName[4], szDeviceName, ARRAYSIZE(szDeviceName)); res == 0) {
                auto lastError = GetLastError();
                if (auto errorRes = getLastErrorString(lastError); errorRes.has_value()) {
                    return std::unexpected(errorRes.value());
                } else {
                    std::stringstream ss;
                    ss << "QueryDosDevice() failed with error code '" << lastError << "'";
                    return std::unexpected(ss.str());
                }
            }
            // Convert the TCHAR to string and store it
            auto deviceName = stringFromTCHAR(szDeviceName);
            auto volumeName = stringFromTCHAR(szVolumeName);
            // Append a backslash to the end of the volume name
            volumeName += R"(\)";
            volumeLookupByDevice[deviceName] = volumeName;
            // Find the next volume
            if (auto res = FindNextVolume(handle, szVolumeName, szVolumeNameSize); res != 0) {
                continue;
            } else {
                BOOL _ = FindVolumeClose(handle);
                auto lastError = GetLastError();
                if (lastError == ERROR_NO_MORE_FILES) {
                    // We are done
                    break;
                }
                // Something bad happened
                if (auto errorRes = getLastErrorString(lastError); errorRes.has_value()) {
                    return std::unexpected(errorRes.value());
                } else {
                    std::stringstream ss;
                    ss << "FindNextVolume() failed with error code '" << lastError << "'";
                    return std::unexpected(ss.str());
                }
            }
            std::fill(szVolumeName, szVolumeName + ARRAYSIZE(szVolumeName) - 1, TEXT('\0'));
        } while (handle != INVALID_HANDLE_VALUE);
    }
    return volumeLookupByDevice;
}

struct EnumeratedDevice {
    SP_DEVINFO_DATA deviceInfo;
    std::string description;
    std::string busDescription;
    std::string instanceId;
    std::string containerId;
    std::string pdo;
    GUID classGuid;
    std::vector<std::string> children;
    std::vector<std::string> removableRelations;
    std::string driveLetter;
    uint8_t location;
    uint16_t vid;
    uint16_t pid;
    std::string serial;
};

typedef std::unordered_map<std::string, std::shared_ptr<EnumeratedDevice>> EnumeratedDevices;

/// @brief Get All Devices sorted by DriverKey and Instance ID
/// @return std::tuple<devicesByDriverKey, devicesByInstanceId> on success, std::string on error
auto getEnumeratedDevices() noexcept -> std::expected<std::tuple<EnumeratedDevices, EnumeratedDevices>, std::string> {
    EnumeratedDevices devicesByDriverKey, devicesByInstanceId;

    HDEVINFO hDevInfo = nullptr;
    SP_DEVINFO_DATA devInfoData {};
    devInfoData.cbSize = sizeof(devInfoData);
    if (hDevInfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
        hDevInfo == INVALID_HANDLE_VALUE) {
        std::stringstream ss;
        ss << "SetupDiGetClassDevs(GUID_DEVCLASS_USB) failed: "
           << getLastErrorString(GetLastError()).value_or(std::string());
        return std::unexpected(ss.str());
    }

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); ++i) {
        auto enumeratedDevice = std::make_shared<EnumeratedDevice>();
        enumeratedDevice->deviceInfo = devInfoData;

        // Instance ID
        if (auto instanceIdResult = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_InstanceId);
            instanceIdResult.has_value()) {
            enumeratedDevice->instanceId = instanceIdResult.value();
            devicesByInstanceId[instanceIdResult.value()] = enumeratedDevice;
        } else {
            return std::unexpected("unable to get device instance id");
        }
        // Device Description
        if (auto result = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_DeviceDesc); result.has_value()) {
            enumeratedDevice->description = result.value();
        }
        // Bus Reported Device Description
        if (auto result = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_BusReportedDeviceDesc);
            result.has_value()) {
            enumeratedDevice->busDescription = result.value();
        }
        // Device PDO Name
        if (auto result = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_PDOName); result.has_value()) {
            enumeratedDevice->pdo = result.value();
        }
        // Device Class GUID
        if (auto result = _getDevicePropertyGUID(devInfoData.DevInst, DEVPKEY_Device_ClassGuid); result.has_value()) {
            enumeratedDevice->classGuid = result.value();
        }
        // Driver Key
        if (auto result = _getDeviceRegistryProp(hDevInfo, devInfoData, SPDRP_DRIVER); result.has_value()) {
            devicesByDriverKey[result.value()] = enumeratedDevice;
        } else {
            // This can fail, not everything has a driver key
        }
        // Get Children
        if (auto result = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_Children); result.has_value()) {
            auto children = splitByKey(result.value(), '\0');
            // convert all the values to upper case
            for (auto&& child : children) {
                std::transform(child.begin(), child.end(), child.begin(), [](char c) { return std::toupper(c); });
            }
            enumeratedDevice->children = children;
        }
        // Get RemovalRelations
        if (auto result = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_RemovalRelations);
            result.has_value()) {
            auto values = splitByKey(result.value(), '\0');
            // convert all the values to upper case
            for (auto&& value : values) {
                std::transform(value.begin(), value.end(), value.begin(), [](char c) { return std::toupper(c); });
            }
            enumeratedDevice->removableRelations = values;
        }
        // Container ID
        // https://learn.microsoft.com/en-us/windows-hardware/drivers/install/container-ids
        if (auto result = _getDevicePropertyGUIDString(devInfoData.DevInst, DEVPKEY_Device_ContainerId);
            result.has_value()) {
            enumeratedDevice->containerId = result.value();
        }
    }
    return std::make_tuple(devicesByDriverKey, devicesByInstanceId);
}

// <Hub, HubChildren>
typedef std::unordered_map<std::shared_ptr<EnumeratedDevice>, std::vector<std::shared_ptr<EnumeratedDevice>>>
    EnumeratedHubDevices;

/// @brief Get All USB HubDevices filtered by VendorID and ProductID
/// @param devicesByInstanceId List of devices from getEnumeratedDevices()
/// @param devicesByDriverKey List of devices from getEnumeratedDevices()
/// @param vid VendorID to filter by. std::nullopt if no filtering.
/// @param pid ProductID to filter by. std::nullopt if no filtering.
/// @return std::tuple<devicesByDriveKey, devicesByInstanceId> on success, std::string on error
auto getHubEnumeratedDevices(
    const EnumeratedDevices& devicesByDriverKey,
    const EnumeratedDevices& devicesByInstanceId,
    std::optional<uint16_t> vid,
    std::optional<uint16_t> pid
) noexcept -> std::expected<EnumeratedHubDevices, std::string> {
    EnumeratedHubDevices hubs;

    HDEVINFO hDevInfo = nullptr;
    SP_DEVINFO_DATA devInfoData {};
    devInfoData.cbSize = sizeof(devInfoData);
    if (hDevInfo =
            SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_HUB, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
        hDevInfo == INVALID_HANDLE_VALUE) {
        std::stringstream ss;
        ss << "SetupDiGetClassDevs(GUID_DEVINTERFACE_USB_HUB) failed: "
           << getLastErrorString(GetLastError()).value_or(std::string());
        return std::unexpected(ss.str());
    }
    // Iterate through all the Hubs
    for (DWORD i = 0;; ++i) {
        if (auto res = SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); !res) {
            DWORD errorCode = GetLastError();
            if (errorCode == ERROR_NO_MORE_ITEMS) {
                break;
            }
            // Something bad happened
            std::stringstream ss;
            ss << "SetupDiEnumDeviceInfo() " << i
               << " failed: " << getLastErrorString(GetLastError()).value_or(std::string());
            return std::unexpected(ss.str());
        }
        // Get USB Hub instanceID
        std::string instanceId;
        if (auto result = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_InstanceId); result.has_value()) {
            instanceId = result.value();
        } else {
            // We can't do anything without the InstanceId of the HUB
            continue;
        }
        // This grabs all VID/PID devices we care about for now.
        auto usbInstId = getUSBInstanceID(instanceId);
        if (!usbInstId.has_value()) {
            // TODO
            //std::println("\t\tInvalid VID/PID: {} {}", usbInstId.error(), instanceID);
            continue;
        } else if (!Fw::is_vid_pid_whitelisted(usbInstId.value().vid, usbInstId.value().pid)) {
            continue;
        }
        // Find the actual Hub in our previous list
        std::shared_ptr<EnumeratedDevice> enumeratedHubDevice;
        if (auto iter = devicesByInstanceId.find(instanceId); iter == devicesByInstanceId.end() && iter->second) {
            // Nothing we can do if we don't have anything
            continue;
        } else {
            enumeratedHubDevice = iter->second;
            enumeratedHubDevice->vid = usbInstId->vid;
            enumeratedHubDevice->pid = usbInstId->pid;
            enumeratedHubDevice->serial = usbInstId->serial;
            hubs[enumeratedHubDevice] = {};
        }
        // Open the Hub so we can poll some information
        SP_DEVICE_INTERFACE_DATA deviceInterfaceData {};
        deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
        for (DWORD interfaceIndex = 0;; ++interfaceIndex) {
            if (auto result = SetupDiEnumDeviceInterfaces(
                    hDevInfo,
                    &devInfoData,
                    &GUID_DEVINTERFACE_USB_HUB,
                    interfaceIndex,
                    &deviceInterfaceData
                );
                !result) {
                if (auto err = GetLastError(); err == ERROR_NO_MORE_ITEMS) {
                    break;
                } else {
                    std::stringstream ss;
                    ss << "SetupDiEnumDeviceInterfaces() failed: " << getLastErrorString(err).value_or(std::string());
                    return std::unexpected(ss.str());
                }
            }
            TCHAR buffer[1024] {};
            DWORD bufferSize = sizeof(buffer);
            SP_DEVICE_INTERFACE_DETAIL_DATA* devInterfaceDetailData =
                reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer);
            devInterfaceDetailData->cbSize = sizeof(*devInterfaceDetailData);
            // Get the device path to open
            if (auto result = SetupDiGetDeviceInterfaceDetail(
                    hDevInfo,
                    &deviceInterfaceData,
                    devInterfaceDetailData,
                    bufferSize,
                    &bufferSize,
                    nullptr
                );
                !result) {
                auto errorCode = GetLastError();
                if (auto res = getLastErrorString(errorCode); res.has_value()) {
                    return std::unexpected("SetupDiGetDeviceInterfaceDetail() Error " + res.value());
                } else {
                    std::stringstream ss;
                    ss << "SetupDiGetDeviceInterfaceDetail() Error code: #" << errorCode;
                    return std::unexpected(ss.str());
                }
            }
            // Open the Hub for IO Ctrl
            HANDLE handle = INVALID_HANDLE_VALUE;
            if (handle = CreateFile(
                    devInterfaceDetailData->DevicePath,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    nullptr,
                    OPEN_EXISTING,
                    0,
                    nullptr
                );
                handle == INVALID_HANDLE_VALUE) {
                return std::unexpected("CreateFile() failed for DevicePath");
            }
            // Get the Hub info
            USB_NODE_INFORMATION usbNodeInfo {};
            if (auto result = DeviceIoControl(
                    handle,
                    IOCTL_USB_GET_NODE_INFORMATION,
                    &usbNodeInfo,
                    sizeof(usbNodeInfo),
                    &usbNodeInfo,
                    sizeof(usbNodeInfo),
                    nullptr,
                    nullptr
                );
                !result) {
                CloseHandle(handle);
                auto errorCode = GetLastError();
                if (auto res = getLastErrorString(errorCode); res.has_value()) {
                    return std::unexpected("DeviceIoControl(IOCTL_USB_GET_NODE_INFORMATION) Error " + res.value());
                } else {
                    std::stringstream ss;
                    ss << "DeviceIoControl(IOCTL_USB_GET_NODE_INFORMATION) Error code: #" << errorCode;
                    return std::unexpected(ss.str());
                }
            }
            // Find all the children of the Hub
            const uint8_t numberOfPorts =
                static_cast<uint8_t>(usbNodeInfo.u.HubInformation.HubDescriptor.bNumberOfPorts);
            for (uint8_t i = 0; i <= numberOfPorts; ++i) {
                uint8_t buffer[512] {};
                DWORD bufferSize = static_cast<DWORD>(sizeof(buffer));
                USB_NODE_CONNECTION_DRIVERKEY_NAME* driverKeyName =
                    reinterpret_cast<USB_NODE_CONNECTION_DRIVERKEY_NAME*>(buffer);
                driverKeyName->ConnectionIndex = i;

                if (auto res = DeviceIoControl(
                        handle,
                        IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                        driverKeyName,
                        bufferSize,
                        driverKeyName,
                        bufferSize,
                        nullptr,
                        nullptr
                    );
                    !res) {
                    // If there is nothing on the port this will fail
                    continue;
                }
                // Find the actual enumeratedDevice by driver key and append it to the list
                auto name = stringFromTCHAR(driverKeyName->DriverKeyName);
                if (auto iter = devicesByDriverKey.find(name); iter == devicesByDriverKey.end()) {
                    // Something bad happened here.
                    std::stringstream ss;
                    ss << "Failed to find '" << name << "' by driver key.";
                    return std::unexpected(ss.str());
                } else {
                    auto enumeratedDevice = iter->second;
                    if (!enumeratedDevice) {
                        return std::unexpected("Failed to get enumeratedDevice by driver key. Value is null");
                    }
                    // This grabs all VID/PID devices we care about for now.
                    auto usbInstId = getUSBInstanceID(enumeratedDevice->instanceId);
                    if (usbInstId.has_value()) {
                        enumeratedDevice->vid = usbInstId->vid;
                        enumeratedDevice->pid = usbInstId->pid;
                        enumeratedDevice->serial = usbInstId->serial;
                    }
                    enumeratedDevice->location = i;
                    // Get the serial port if we have it
                    // TODO
                    // Get the drive letters if we have it
                    // TODO
                    hubs[enumeratedHubDevice].push_back(enumeratedDevice);
                }
            }
        }
    }

    return hubs;
}

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    // Get all USB Devices connected to the host
    EnumeratedDevices devicesByDriverKey, devicesByInstanceId;
    if (auto result = getEnumeratedDevices(); result.has_value()) {
        std::tie(devicesByDriverKey, devicesByInstanceId) = result.value();
    } else {
        std::stringstream ss;
        ss << "getEnumeratedDevices() failed: " << result.error();
        return std::unexpected(ss.str());
    }
    // Get all Hubs connected to the host
    EnumeratedHubDevices hubs;
    if (auto result = getHubEnumeratedDevices(devicesByDriverKey, devicesByInstanceId, std::nullopt, std::nullopt);
        result.has_value()) {
        hubs = result.value();
    } else {
        std::stringstream ss;
        ss << "getHubEnumeratedDevices() failed: " << result.error();
        return std::unexpected(ss.str());
    }
    // Get all Volumes connected to the host
    VolumesMap volumes;
    if (auto result = getVolumes(); result.has_value()) {
        volumes = result.value();
    } else {
        // TODO, do we care if this fails?
    }

    // auto getPaths = [&](std::shared_ptr<EnumeratedDevice> device) noexcept -> std::optional<std::vector<std::string>> {
    //     if (!IsEqualGUID(device->classGuid, GUID_DEVCLASS_VOLUME)) {
    //         return std::nullopt;
    //     }
    //     auto volumeName = volumes.find(device->pdo);
    //     std::vector<std::string> driveLetters;
    //     if (volumeName != volumes.end()) {
    //         TCHAR szVolumePathNames[1024] {};
    //         DWORD szVolumePathNamesSize = sizeof(szVolumePathNames);
    //         DWORD length = 0;
    //         auto szVolumeName = TCHARFromString(volumeName->second);
    //         if (szVolumeName) {
    //             auto success = GetVolumePathNamesForVolumeName(
    //                 szVolumeName.get(),
    //                 szVolumePathNames,
    //                 szVolumePathNamesSize,
    //                 &length
    //             );
    //             if (success) {
    //                 driveLetters.push_back(stringFromTCHAR(szVolumePathNames));
    //             }
    //         }
    //     }
    //     return driveLetters;
    // };

    FreeWiliDevices fwDevices;
    for (auto&& hub : hubs) {
        USBDevices devices;

        devices.push_back(Fw::USBDevice {
            .kind = Fw::getUSBDeviceTypeFrom(hub.first->vid, hub.first->pid),
            .vid = hub.first->vid,
            .pid = hub.first->pid,
            .name = hub.first->busDescription + " (" + hub.first->description + ")",
            .serial = hub.first->serial,
            .location = hub.first->location,
            .paths = std::nullopt, // TODO
            .port = "", // TODO
            ._raw = hub.first->instanceId,
        });
        for (auto&& child : hub.second) {
            devices.push_back(Fw::USBDevice {
                .kind = Fw::getUSBDeviceTypeFrom(child->vid, child->pid),
                .vid = child->vid,
                .pid = child->pid,
                .name = child->busDescription + " (" + child->description + ")",
                .serial = child->serial,
                .location = child->location,
                .paths = std::nullopt, // TODO
                .port = "", // TODO
                ._raw = child->instanceId,
            });
        }
        if (auto result = Fw::FreeWiliDevice::fromUSBDevices(devices); result.has_value()) {
            fwDevices.push_back(result.value());
        } else {
            return std::unexpected(result.error());
            // TOOD
        }
    }
    return fwDevices;
}

// NOLINTEND
#endif // _WIN32
