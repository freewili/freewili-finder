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

    #include <cstdio>
    #include <cstring>
    #include <expected>
    #include <sstream>
    #include <string>
    #include <print>
    #include <algorithm>
    #include <iterator>
    #include <regex>
    #include <map>
    #include <vector>
    #include <fwfinder.hpp>
    #include <fwfinder_windows.hpp>
    #include <usbdef.hpp>

//#define INITGUID

Fw::Finder::Finder() = default;

auto stringFromTCHARRaw(const TCHAR* const msg) -> std::expected<std::string, DWORD> {
    #if defined(UNICODE)
    if (size_t const width = WideCharToMultiByte(CP_UTF8, 0, msg, -1, nullptr, 0, nullptr, nullptr); width != 0) {
        std::string new_msg(width, '\0');
        if (auto success = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)msg, -1, new_msg.data(), width, nullptr, nullptr);
            success == 0) {
            return std::unexpected(GetLastError());
        }
        return new_msg;
    } else {
        return std::unexpected(GetLastError());
    }
    #else
    return std::string(msg);
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

auto stringFromTCHAR(const TCHAR* const szValue) -> std::expected<std::string, std::string> {
    if (auto result = stringFromTCHARRaw(szValue); result.has_value()) {
        return result.value();
    } else {
        std::stringstream ss;
        ss << "Error code: #" << result.error() << " (" << getLastErrorString(result.error()).value_or(std::string())
           << ")";
        return std::unexpected(ss.str());
    };
};

auto splitInstanceID(std::string value) -> std::expected<std::vector<std::string>, std::string> {
    std::size_t current = 0, previous = 0;
    std::vector<std::string> values;
    do {
        current = value.find('\\', previous);
        if (current == std::string::npos) {
            values.push_back(value.substr(previous, value.length() - 1));
            break;
        }
        values.push_back(value.substr(previous, current - previous));
        previous = current + 1;
    } while (current != std::string::npos);
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

auto Fw::find_all() noexcept -> std::expected<Fw::FreeWiliDevices, std::string> {
    HDEVINFO hDevInfo = nullptr;

    // Helper function to get Registry Property
    auto _getDeviceRegistryProp = [&](SP_DEVINFO_DATA& devInfoData,
                                      DWORD prop) -> std::expected<std::string, std::string> {
        DWORD dwPropertyRegDataType = 0;
        TCHAR buffer[1024] {};
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
    auto _getDeviceProperty = [](DEVINST devInst, const DEVPROPKEY key) -> std::expected<std::string, std::string> {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_STRING;
        TCHAR szValue[MAX_DEVICE_ID_LEN + 1] {};
        ULONG size = MAX_DEVICE_ID_LEN;
        if (auto res = CM_Get_DevNode_Property(devInst, &key, &propertyType, (BYTE*)szValue, &size, 0);
            res != CR_SUCCESS) {
            std::stringstream ss;
            ss << "CM_Get_DevNode_Property() failed with CR error: " << res;
            return std::unexpected(ss.str());
        }
        return stringFromTCHAR(szValue);
    };
    // Helper function to get device Property GUID as a String
    auto _getDevicePropertyGUID = [](DEVINST devInst, const DEVPROPKEY key) -> std::expected<std::string, std::string> {
        DEVPROPTYPE propertyType = DEVPROP_TYPE_GUID;
        GUID guid {};
        ULONG size = sizeof(guid);
        if (auto res = CM_Get_DevNode_Property(devInst, &key, &propertyType, (BYTE*)&guid, &size, 0);
            res != CR_SUCCESS) {
            std::stringstream ss;
            ss << "CM_Get_DevNode_Property() failed with CR error: " << res;
            return std::unexpected(ss.str());
        }
        TCHAR szValue[MAX_DEVICE_ID_LEN + 1] {};
        if (StringFromGUID2(guid, szValue, MAX_DEVICE_ID_LEN) == 0) {
            return std::unexpected("StringFromGUID2() failed");
        }
        return stringFromTCHAR(szValue);
    };

    // List all connected USB devices - GUID_DEVCLASS_USB
    if (hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_USB, nullptr, nullptr, DIGCF_PRESENT);
        hDevInfo == INVALID_HANDLE_VALUE) {
        std::stringstream ss;
        ss << "SetupDiGetClassDevs(GUID_DEVCLASS_USB) failed: "
           << getLastErrorString(GetLastError()).value_or(std::string());
        return std::unexpected(ss.str());
    }

    std::map<std::string, USBDevices> containerDevices;
    for (auto i = 0;; i++) {
        // Get Device information
        SP_DEVINFO_DATA devInfoData {};
        devInfoData.cbSize = sizeof(devInfoData);
        if (auto res = SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData); res == FALSE) {
            if (auto err = GetLastError(); err == ERROR_NO_MORE_ITEMS) {
                break;
            } else {
                std::stringstream ss;
                ss << "SetupDiEnumDeviceInfo() failed: " << getLastErrorString(err).value_or(std::string());
                return std::unexpected(ss.str());
            }
        }
        // Get Instance ID
        // FTDI: USB\VID_0403&PID_6014\FW4607
        // RP2040 Serial: USB\VID_2E8A&PID_000A\E463A8574B3F3935
        // RP2040 UF2 Mass storage: USB\VID_2E8A&PID_0003&MI_00\A&22CF742D&1&0000
        // RP2040 UF2: USB\VID_2E8A&PID_0003\E0C9125B0D9B
        // Microchip hub: USB\VID_0424&PID_2513\8&36C81A88&0&1
        TCHAR szInstanceID[MAX_DEVICE_ID_LEN + 1] {};
        if (auto res = CM_Get_Device_ID(devInfoData.DevInst, szInstanceID, MAX_DEVICE_ID_LEN, 0); res != CR_SUCCESS) {
            std::stringstream ss;
            ss << "CM_Get_Device_ID() failed with CR error: " << res;
            return std::unexpected(ss.str());
        }
        auto instanceIdResult = stringFromTCHAR(szInstanceID);
        if (!instanceIdResult.has_value()) {
            return std::unexpected(instanceIdResult.error());
        }
        std::string instanceID = instanceIdResult.value();
        // This grabs all VID/PID devices we care about for now.
        auto usbInstId = getUSBInstanceID(instanceID);
        if (!usbInstId.has_value()) {
            // TODO
            //std::println("\t\tInvalid VID/PID: {} {}", usbInstId.error(), instanceID);
            continue;
        } else if (!Fw::is_vid_pid_whitelisted(usbInstId.value().vid, usbInstId.value().pid)) {
            continue;
        }
        auto kind = Fw::getUSBDeviceTypeFrom(usbInstId.value().vid, usbInstId.value().pid);

        // Lets get all the children
        DEVINST childDevInst = 0;
        auto res = CM_Get_Child(&childDevInst, devInfoData.DevInst, 0);
        if (res != CR_SUCCESS) {
            std::stringstream ss;
            ss << "CM_Get_Child() failed with CR error: " << res;
            return std::unexpected(ss.str());
        }
        DEVINST siblingDevInst = 0;
        DEVINST previousDevInst = childDevInst;
        std::optional<std::string> portName = std::nullopt;
        do {
            bool isPort = false;
            if (auto deviceClassIdResult = _getDeviceProperty(previousDevInst, DEVPKEY_Device_Class);
                deviceClassIdResult.has_value()) {
                isPort = deviceClassIdResult.value().contains("Ports");
            }
            auto deviceNameResult = _getDeviceProperty(previousDevInst, DEVPKEY_NAME);
            // Get the Serial Port Value "USB Serial Device (COM6)"
            if (isPort && deviceNameResult.has_value()) {
                std::regex re(R"(COM\d{1,3})");
                std::cmatch comPortMatch;
                if (!std::regex_search(deviceNameResult.value().c_str(), comPortMatch, re)) {
                    return std::unexpected("Failed to regex_match Hex value of VID");
                }
                portName = comPortMatch[0];
            }

            res = CM_Get_Sibling(&siblingDevInst, previousDevInst, 0);
            if (res == CR_SUCCESS) {
                previousDevInst = siblingDevInst;
            }
            if (res == CR_NO_SUCH_DEVNODE) {
                // We are done.
                break;
            }
        } while (res == CR_SUCCESS);

        // name = Bus Reported Device Description + Device Description
        auto descResult = _getDeviceRegistryProp(devInfoData, SPDRP_DEVICEDESC);
        auto busDescResult = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_BusReportedDeviceDesc);

        std::string name;
        // Append Device Description
        if (descResult.has_value()) {
            name = descResult.value();
        }
        // append Bus Reported Device Description
        if (busDescResult.has_value()) {
            if (!name.empty()) {
                name += " ";
            }
            name = busDescResult.value();
        }
        // Location ID
        auto locationIdResult = _getDeviceProperty(devInfoData.DevInst, DEVPKEY_Device_LocationInfo);
        unsigned int location = 0;
        if (locationIdResult.has_value()) {
            std::regex re(R"(Port_#[0-9]{1,4})");
            std::cmatch match;
            if (std::regex_search(locationIdResult.value().c_str(), match, re)) {
                std::regex re_num(R"([0-9]{1,4}$)");
                std::cmatch num_match;
                if (std::regex_search(match[0].str().c_str(), num_match, re_num)) {
                    std::stringstream ss(num_match[0].str());
                    ss >> location;
                }
            }
        }
        // We get a double hit here, lets ignore it for now.
        // This shows up in device manager under Other devices as RP2 Boot
        if (location == 0 && kind == Fw::USBDeviceType::MassStorage) {
            continue;
        }

        // Container ID
        // https://learn.microsoft.com/en-us/windows-hardware/drivers/install/container-ids
        if (auto containerIdResult = _getDevicePropertyGUID(devInfoData.DevInst, DEVPKEY_Device_ContainerId);
            !containerIdResult.has_value()) {
            return std::unexpected("Failed to get containerId");
        } else {
            // Finally lets add it to the list
            containerDevices[containerIdResult.value()].push_back(Fw::USBDevice {
                .kind = kind,
                .vid = usbInstId.value().vid,
                .pid = usbInstId.value().pid,
                .name = name,
                .serial = usbInstId.value().serial,
                .location = static_cast<uint8_t>(location),
                .paths = std::nullopt, // TODO
                .port = portName,
                ._raw = instanceID,
            });
        }
    }
    // Convert to FreeWiliDevices
    FreeWiliDevices fwDevices;
    for (auto& containerDevice : containerDevices) {
        if (auto fwDeviceResult = Fw::FreeWiliDevice::fromUSBDevices(containerDevice.second);
            fwDeviceResult.has_value()) {
            fwDevices.push_back(fwDeviceResult.value());
        } else {
            // TOOD
        }
    }
    return fwDevices;
}

// NOLINTEND
#endif // _WIN32
