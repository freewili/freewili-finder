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
//#include <devguid.h>

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

auto stringFromTCHARRaw(const TCHAR* const msg)
    -> std::expected<std::string, DWORD> {
    #if defined(UNICODE)
    if (size_t const width = WideCharToMultiByte(
            CP_UTF8,
            0,
            msg,
            -1,
            nullptr,
            0,
            nullptr,
            nullptr
        );
        width != 0) {
        std::string new_msg(width, '\0');
        if (auto success = WideCharToMultiByte(
                CP_UTF8,
                0,
                (LPWSTR)msg,
                -1,
                new_msg.data(),
                width,
                nullptr,
                nullptr
            );
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
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
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

auto stringFromTCHAR(const TCHAR* const szValue)
    -> std::expected<std::string, std::string> {
    if (auto result = stringFromTCHARRaw(szValue); result.has_value()) {
        return result.value();
    } else {
        std::stringstream ss;
        ss << "Error code: #" << result.error() << " ("
           << getLastErrorString(result.error()).value_or(std::string()) << ")";
        return std::unexpected(ss.str());
    };
};

auto splitInstanceID(std::string value)
    -> std::expected<std::vector<std::string>, std::string> {
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

auto getUSBInstanceID(std::string value)
    -> std::expected<USBInstanceID, std::string> {
    if (auto values = splitInstanceID(value); !values.has_value()) {
        return std::unexpected(values.error());
    } else {
        auto split = values.value();
        // seperate the VID/PID values
        std::regex re(R"((VID_[0-9a-fA-F]+)|(PID_[0-9a-fA-F]+))");
        auto match_begin =
            std::sregex_iterator(split[1].begin(), split[1].end(), re);
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

auto getNodesFromInstanceId(TCHAR* instanceID)
    -> std::expected<std::string, std::string> {
    DEVINST parentDevInst = 0;
    auto res =
        CM_Locate_DevNode(&parentDevInst, instanceID, CM_LOCATE_DEVNODE_NORMAL);
    if (res != CR_SUCCESS) {
        std::stringstream ss;
        ss << "CM_Locate_DevNode() failed with CR error: " << res;
        return std::unexpected(ss.str());
    }

    DEVINST childDevInst = 0;
    res = CM_Get_Child(&childDevInst, parentDevInst, 0);
    if (res != CR_SUCCESS) {
        std::stringstream ss;
        ss << "CM_Get_Child() failed with CR error: " << res;
        return std::unexpected(ss.str());
    }
    DEVINST siblingDevInst = 0;
    DEVINST previousDevInst = childDevInst;
    do {
        TCHAR childInstanceID[MAX_DEVICE_ID_LEN + 1] {};
        if (auto res = CM_Get_Device_ID(
                previousDevInst,
                childInstanceID,
                MAX_DEVICE_ID_LEN + 1,
                0
            );
            res != CR_SUCCESS) {
            std::stringstream ss;
            ss << "CM_Get_Device_ID() failed with CR error: " << res;
            return std::unexpected(ss.str());
        }

        DEVPROPTYPE propertyType = 0;
        TCHAR childFriendlyName[MAX_DEVICE_ID_LEN + 1] {};
        ULONG size = MAX_DEVICE_ID_LEN;
        res = CM_Get_DevNode_Property(
            previousDevInst,
            &DEVPKEY_Device_DeviceDesc,
            &propertyType,
            (BYTE*)childFriendlyName,
            &size,
            0
        );

        propertyType = 0;
        TCHAR childClassID[MAX_DEVICE_ID_LEN + 1] {};
        size = MAX_DEVICE_ID_LEN;
        res = CM_Get_DevNode_Property(
            previousDevInst,
            &DEVPKEY_Device_Class,
            &propertyType,
            (BYTE*)childClassID,
            &size,
            0
        );

        DEVPROPTYPE type = DEVPROP_TYPE_STRING;
        TCHAR childName[1024] {};
        ULONG childNameSize = 1024;
        res = CM_Locate_DevNode(&previousDevInst, childInstanceID, 0);
        res = CM_Get_DevNode_Property(
            previousDevInst,
            &DEVPKEY_NAME,
            &type,
            (BYTE*)childName,
            &childNameSize,
            0
        );

        type = DEVPROP_TYPE_STRING;
        TCHAR childPDOName[1024] {};
        ULONG childPDONameSize = 1024;
        res = CM_Locate_DevNode(&previousDevInst, childInstanceID, 0);
        res = CM_Get_DevNode_Property(
            previousDevInst,
            &DEVPKEY_Device_PDOName,
            &type,
            (BYTE*)childPDOName,
            &childPDONameSize,
            0
        );

        // TODO: This doesn't work yet.
        CONFIGRET cres;
        ULONG ifaceListSize = 0;
        wchar_t* ifaceList = nullptr;

        // Get the size of the device interface list
        res = CM_Get_Device_Interface_List_Size(
            &ifaceListSize,
            (LPGUID)&GUID_DEVINTERFACE_VOLUME,
            childInstanceID,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        );

        ifaceList = new TCHAR[ifaceListSize * 2] {};

        res = CM_Get_Device_Interface_List(
            (LPGUID)&GUID_DEVINTERFACE_VOLUME,
            childInstanceID, //L"USB\\VID_2109&PID_0813\\8&216C1825&0&4\\0",
            ifaceList,
            ifaceListSize,
            CM_GET_DEVICE_INTERFACE_LIST_PRESENT
        );

        // Iterate over the list of device paths
        TCHAR* sz = ifaceList;
        while (*sz) {
            HANDLE hFile = CreateFile(
                sz,
                FILE_GENERIC_READ,
                FILE_SHARE_READ,
                0,
                OPEN_EXISTING,
                0,
                0
            );
            if (hFile != INVALID_HANDLE_VALUE) {
                WCHAR volumeName[MAX_PATH + 1] = {0};
                DWORD serialNumber = 0;
                DWORD maxComponentLen = 0;
                DWORD fileSystemFlags = 0;
                WCHAR fileSystemName[MAX_PATH + 1] = {0};

                if (GetVolumeInformationByHandleW(
                        hFile,
                        volumeName,
                        ARRAYSIZE(volumeName),
                        &serialNumber,
                        &maxComponentLen,
                        &fileSystemFlags,
                        fileSystemName,
                        ARRAYSIZE(fileSystemName)
                    )) {
                    wprintf(L"Volume Name: %s\n", volumeName);
                    wprintf(L"Serial Number: %lu\n", serialNumber);
                    wprintf(L"Max Component Length: %lu\n", maxComponentLen);
                    wprintf(L"File System Flags: %lu\n", fileSystemFlags);
                    wprintf(L"File System Name: %s\n", fileSystemName);
                }
                // Do something with the handle
                CloseHandle(hFile);
            }
            sz += 1 + wcslen(sz);
        }

        // Free the allocated memory
        delete[] ifaceList;

        /*
            CONFIGRET cres;
    ULONG ifaceListSize = 0;
    wchar_t* ifaceList = nullptr;

    // Get the size of the device interface list
    cres = CM_Get_Device_Interface_List_Size(&ifaceListSize, &GUID_DEVINTERFACE_VOLUME, L"USB\\VID_2109&PID_0813\\8&216C1825&0&4\\0", CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cres != CR_SUCCESS) {
        return -1;
    }

    // Allocate memory for the list
    ifaceList = new wchar_t[ifaceListSize * 2];

    // Populate the list
    cres = CM_Get_Device_Interface_List(&GUID_DEVINTERFACE_VOLUME, L"USB\\VID_2109&PID_0813\\8&216C1825&0&4\\0", ifaceList, ifaceListSize, CM_GET_DEVICE_INTERFACE_LIST_PRESENT);
    if (cres != CR_SUCCESS) {
        delete[] ifaceList;
        return -2;
    }

    // Iterate over the list of device paths
    wchar_t* sz = ifaceList;
    while (*sz) {
        HANDLE hFile = CreateFile(sz, FILE_GENERIC_READ, FILE_SHARE_VALID_FLAGS, 0, OPEN_EXISTING, 0, 0);
        if (hFile != INVALID_HANDLE_VALUE) {
            // Do something with the handle
            CloseHandle(hFile);
        }
        sz += 1 + wcslen(sz);
    }

    // Free the allocated memory
    delete[] ifaceList;
        */

        res = CM_Get_Sibling(&siblingDevInst, previousDevInst, 0);
        if (res == CR_SUCCESS) {
            previousDevInst = siblingDevInst;
        }
        if (res == CR_NO_SUCH_DEVNODE) {
            // We are done.
            break;
        }
    } while (res == CR_SUCCESS);

    return std::unexpected("TODO");
}

auto Fw::find_all() noexcept
    -> std::expected<Fw::FreeWiliDevices, std::string> {
    HDEVINFO hDevInfo = nullptr;

    // Helper function to get Registry Property
    auto _getDeviceRegistryProp = [&](SP_DEVINFO_DATA& devInfoData, DWORD prop
                                  ) -> std::expected<std::string, std::string> {
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
    auto _getDeviceProperty = [&](SP_DEVINFO_DATA& devInfoData,
                                  const DEVPROPKEY key,
                                  bool fromGUID = false
                              ) -> std::expected<std::string, std::string> {
        DEVPROPTYPE propertyType = 0;
        TCHAR buffer[1024] {};
        if (SetupDiGetDeviceProperty(
                hDevInfo,
                &devInfoData,
                &key,
                &propertyType,
                reinterpret_cast<BYTE*>(buffer),
                sizeof(buffer),
                nullptr,
                0
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
        if (fromGUID) {
            TCHAR convertedBuffer[4096] {};
            StringFromGUID2((REFGUID)buffer, convertedBuffer, 4096);
            return stringFromTCHAR(convertedBuffer);
        }
        return stringFromTCHAR(buffer);
    };
    // List all connected USB devices - GUID_DEVCLASS_USB
    if (hDevInfo = SetupDiGetClassDevs(
            &GUID_DEVCLASS_USB,
            nullptr,
            nullptr,
            DIGCF_PRESENT
        );
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
        if (auto res = SetupDiEnumDeviceInfo(hDevInfo, i, &devInfoData);
            res == FALSE) {
            if (auto err = GetLastError(); err == ERROR_NO_MORE_ITEMS) {
                break;
            } else {
                std::stringstream ss;
                ss << "SetupDiEnumDeviceInfo() failed: "
                   << getLastErrorString(err).value_or(std::string());
                return std::unexpected(ss.str());
            }
        }
        // Get Instance ID
        // FTDI: USB\VID_0403&PID_6014\FW4607
        // RP2040 Serial: USB\VID_2E8A&PID_000A\E463A8574B3F3935
        // RP2040 UF2 Mass storage: USB\VID_2E8A&PID_0003&MI_00\A&22CF742D&1&0000
        // RP2040 UF2: USB\VID_2E8A&PID_0003\E0C9125B0D9B
        // Microchip hub: USB\VID_0424&PID_2513\8&36C81A88&0&1
        TCHAR instanceID[MAX_DEVICE_ID_LEN + 1] {};
        if (auto res = CM_Get_Device_ID(
                devInfoData.DevInst,
                instanceID,
                MAX_DEVICE_ID_LEN + 1,
                0
            );
            res != CR_SUCCESS) {
            std::stringstream ss;
            ss << "CM_Get_Device_ID() failed with CR error: " << res;
            return std::unexpected(ss.str());
        }
        auto result = stringFromTCHAR(instanceID);
        if (!result.has_value()) {
            return std::unexpected(result.error());
        }
        // This grabs all VID/PID devices we care about for now.
        auto usbInstId = getUSBInstanceID(result.value());
        if (!usbInstId.has_value()) {
            //return std::unexpected(usb_inst_id.error());
            std::println(
                "\t\tInvalid VID/PID: {} {}",
                usbInstId.error(),
                result.value()
            );
            continue;
        } else if (!Fw::is_vid_pid_whitelisted(
                       usbInstId.value().vid,
                       usbInstId.value().pid
                   )) {
            continue;
        }
        std::println(
            "VID: {:X} PID: {:X} Serial: {}",
            usbInstId.value().vid,
            usbInstId.value().pid,
            usbInstId.value().serial
        );

        auto kind = Fw::getUSBDeviceTypeFrom(
            usbInstId.value().vid,
            usbInstId.value().pid
        );

        // name = Bus Reported Device Description + Device Description
        auto descResult = _getDeviceRegistryProp(devInfoData, SPDRP_DEVICEDESC);
        auto busDescResult = _getDeviceProperty(
            devInfoData,
            DEVPKEY_Device_BusReportedDeviceDesc
        );

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
        std::println("\tname: {}", name);

        // Container ID
        // https://learn.microsoft.com/en-us/windows-hardware/drivers/install/container-ids
        auto containerIdResult =
            _getDeviceProperty(devInfoData, DEVPKEY_Device_ContainerId, true);
        if (containerIdResult.has_value()) {
            std::println("\tContainer ID: {}", containerIdResult.value());
        }

        // Location ID
        auto locationIdResult =
            _getDeviceProperty(devInfoData, DEVPKEY_Device_LocationInfo);
        if (locationIdResult.has_value()) {
            std::println("\tLocation ID: {}", locationIdResult.value());
        }

        getNodesFromInstanceId(instanceID);

        // Finally lets add it to the list
        containerDevices[containerIdResult.value()].push_back(Fw::USBDevice {
            .kind = kind,
            .vid = usbInstId.value().vid,
            .pid = usbInstId.value().pid,
            .name = name,
            .serial = usbInstId.value().serial,
            .location = 0,
            .path_or_port = std::nullopt,
        });
    }
    // Convert to FreeWiliDevices
    FreeWiliDevices fwDevices;
    for (auto& containerDevice : containerDevices) {
        fwDevices.push_back(containerDevice.second);
    }
    // List all connected Ports - GUID_DEVCLASS_PORTS

    // List all connected Disk Drives - GUID_DEVCLASS_DISKDRIVE

    return fwDevices;
    //return std::unexpected("TODO");
}

    #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

// #pragma comment(lib, "setupapi.lib")

// using FN_SetupDiGetDevicePropertyW = BOOL (*)(int, PSP_DEVINFO_DATA, const DEVPROPKEY *, DEVPROPTYPE *, PBYTE, DWORD,
//                                               PDWORD, DWORD); __attribute__((stdcall)))(__in HDEVINFO DeviceInfoSet,
//                                               __in PSP_DEVINFO_DATA DeviceInfoData,
//                                                    __in const DEVPROPKEY *PropertyKey, __out DEVPROPTYPE
//                                                    *PropertyType,
//                                                    __out_opt PBYTE PropertyBuffer, __in DWORD PropertyBufferSize,
//                                                    __out_opt PDWORD RequiredSize, __in DWORD Flags);

// List all USB devices with some additional information
static void ListDevices(CONST GUID* pClassGuid, LPCTSTR pszEnumerator) {
    unsigned i = 0;
    unsigned j = 0;
    DWORD dwSize = 0;
    DWORD dwPropertyRegDataType = 0;
    DEVPROPTYPE ulPropertyType = 0;
    CONFIGRET status = 0;
    HDEVINFO hDevInfo = nullptr;
    SP_DEVINFO_DATA DeviceInfoData {};
    const static LPCTSTR arPrefix[3] =
        {TEXT("VID_"), TEXT("PID_"), TEXT("MI_")};
    TCHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN];
    TCHAR szDesc[1024];
    TCHAR szHardwareIDs[4096];
    WCHAR szBuffer[4096];
    LPTSTR pszToken = nullptr;
    LPTSTR pszNextToken = nullptr;
    TCHAR szVid[MAX_DEVICE_ID_LEN];
    TCHAR szPid[MAX_DEVICE_ID_LEN];
    TCHAR szMi[MAX_DEVICE_ID_LEN];

    // Convert TCHAR to std::string, place the error inside the string if
    // we failed to convert
    auto _toString = [](const TCHAR* szValue) -> std::string {
        if (auto result = stringFromTCHARRaw(szValue); result.has_value()) {
            return result.value();
        } else {
            std::stringstream ss;
            ss << "Error code: #" << result.error() << " ("
               << getLastErrorString(result.error()).value_or(std::string())
               << ")";
            return ss.str();
        };
    };

    // List all connected USB devices
    hDevInfo = SetupDiGetClassDevs(
        pClassGuid,
        pszEnumerator,
        nullptr,
        pClassGuid != nullptr ? DIGCF_PRESENT : DIGCF_ALLCLASSES | DIGCF_PRESENT
    );
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        return;
    }

    // Find the ones that are driverless
    for (i = 0;; i++) {
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);
        if (SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData) == 0) {
            break;
        }

        status = CM_Get_Device_ID(
            DeviceInfoData.DevInst,
            szDeviceInstanceID,
            MAX_PATH,
            0
        );
        if (status != CR_SUCCESS) {
            continue;
        }
        // Display device instance ID
        std::println("Device Instance ID: {}", _toString(szDeviceInstanceID));

        if (SetupDiGetDeviceRegistryProperty(
                hDevInfo,
                &DeviceInfoData,
                SPDRP_DEVICEDESC,
                &dwPropertyRegDataType,
                (BYTE*)szDesc,
                sizeof(szDesc), // The size, in bytes
                &dwSize
            )) {
            std::println("    Device Description: \"{}\"", _toString(szDesc));
        }

        if (SetupDiGetDeviceRegistryProperty(
                hDevInfo,
                &DeviceInfoData,
                SPDRP_HARDWAREID,
                &dwPropertyRegDataType,
                (BYTE*)szHardwareIDs,
                sizeof(szHardwareIDs), // The size, in bytes
                &dwSize
            )) {
            LPCTSTR pszId = nullptr;
            std::println("    Hardware IDs:");
            for (pszId = szHardwareIDs; *pszId != TEXT('\0')
                 && pszId + dwSize / sizeof(TCHAR)
                     <= szHardwareIDs + ARRAYSIZE(szHardwareIDs);
                 pszId += lstrlen(pszId) + 1) {
                std::println("        \"{}\"", _toString(pszId));
            }
        }

        // Retreive the device description as reported by the device itself
        // On Vista and earlier, we can use only SPDRP_DEVICEDESC
        // On Windows 7, the information we want ("Bus reported device description") is
        // accessed through DEVPKEY_Device_BusReportedDeviceDesc
        // if (SetupDiGetDevicePropertyW(
        //         hDevInfo,
        //         &DeviceInfoData,
        //         &DEVPKEY_Device_BusReportedDeviceDesc,
        //         &ulPropertyType,
        //         (BYTE*)szBuffer,
        //         sizeof(szBuffer),
        //         &dwSize,
        //         0
        //     )
        //     == 0) {
        //     continue;
        // }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_BusReportedDeviceDesc,
                &ulPropertyType,
                (BYTE*)szBuffer,
                sizeof(szBuffer),
                &dwSize,
                0
            )
            != 0) {
            std::println(
                "    Bus Reported Device Description: \"{}\"",
                _toString(szBuffer)
            );
        }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_Manufacturer,
                &ulPropertyType,
                (BYTE*)szBuffer,
                sizeof(szBuffer),
                &dwSize,
                0
            )
            != 0) {
            std::println(
                "    Device Manufacturer: \"{}\"",
                _toString(szBuffer)
            );
        }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_FriendlyName,
                &ulPropertyType,
                (BYTE*)szBuffer,
                sizeof(szBuffer),
                &dwSize,
                0
            )
            != 0) {
            std::println(
                "    Device Friendly Name: \"{}\"",
                _toString(szBuffer)
            );
        }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_LocationInfo,
                &ulPropertyType,
                (BYTE*)szBuffer,
                sizeof(szBuffer),
                &dwSize,
                0
            )
            != 0) {
            std::println(
                "    Device Location Info: \"{}\"",
                _toString(szBuffer)
            );
        }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_SecuritySDS,
                &ulPropertyType,
                (BYTE*)szBuffer,
                sizeof(szBuffer),
                &dwSize,
                0
            )
            != 0) {
            // See Security Descriptor Definition Language on MSDN
            // (http://msdn.microsoft.com/en-us/library/windows/desktop/aa379567(v=vs.85).aspx)
            std::println(
                "    Device Security Descriptor String: \"{}\"",
                _toString(szBuffer)
            );
        }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_Device_ContainerId,
                &ulPropertyType,
                (BYTE*)szDesc,
                sizeof(szDesc),
                &dwSize,
                0
            )
            != 0) {
            StringFromGUID2((REFGUID)szDesc, szBuffer, ARRAY_SIZE(szBuffer));
            std::println("    ContainerId: \"{}\"", _toString(szBuffer));
        }
        if (SetupDiGetDevicePropertyW(
                hDevInfo,
                &DeviceInfoData,
                &DEVPKEY_DeviceDisplay_Category,
                &ulPropertyType,
                (BYTE*)szBuffer,
                sizeof(szBuffer),
                &dwSize,
                0
            )
            != 0) {
            std::println(
                "    Device Display Category: \"{}\"",
                _toString(szBuffer)
            );
        }

        pszToken = _tcstok_s(szDeviceInstanceID, TEXT("\\#&"), &pszNextToken);
        while (pszToken != nullptr) {
            szVid[0] = TEXT('\0');
            szPid[0] = TEXT('\0');
            szMi[0] = TEXT('\0');
            for (j = 0; j < 3; j++) {
                if (_tcsncmp(pszToken, arPrefix[j], lstrlen(arPrefix[j]))
                    != 0) {
                    continue;
                }
                switch (j) {
                    case 0:
                        _tcscpy_s(szVid, ARRAY_SIZE(szVid), pszToken);
                        break;
                    case 1:
                        _tcscpy_s(szPid, ARRAY_SIZE(szPid), pszToken);
                        break;
                    case 2:
                        _tcscpy_s(szMi, ARRAY_SIZE(szMi), pszToken);
                        break;
                    default:
                        break;
                }
            }
            if (szVid[0] != TEXT('\0')) {
                std::println("    vid: \"{}\"", _toString(szVid));
            }
            if (szPid[0] != TEXT('\0')) {
                std::println("    pid: \"{}\"", _toString(szPid));
            }
            if (szMi[0] != TEXT('\0')) {
                std::println("    mi: \"{}\"", _toString(szMi));
            }
            pszToken = _tcstok_s(nullptr, TEXT("\\#&"), &pszNextToken);
        }
    }
}

// NOLINTEND

//void Fw::list_all() {}

void Fw::list_all() {
    // // List all connected USB devices
    // printf (TEXT("---------------------------------------------------\n"));
    // printf (TEXT("- USB devices -\n"));
    // printf (TEXT("---------------------------------------------------\n"));
    // ListDevices(NULL, TEXT("USB"));

    std::println("");
    std::println("-------------------------------------------------------");
    std::println("- USBSTOR devices -");
    std::println("-------------------------------------------------------");
    ListDevices(nullptr, TEXT("USBSTOR"));

    // printf (TEXT("\n"));
    // printf (TEXT("--------------------------------------------------\n"));
    // printf (TEXT("- SD devices -\n"));
    // printf (TEXT("--------------------------------------------------\n"));
    // ListDevices(NULL, TEXT("SD"));

    std::println("");
    std::println("--------------------------------------------------");
    std::println("USB");
    std::println("--------------------------------------------------");
    ListDevices(&GUID_DEVCLASS_USB, nullptr);
    std::println("");

    std::println("");
    std::println("--------------------------------------------------");
    std::println("Ports");
    std::println("--------------------------------------------------");
    ListDevices(&GUID_DEVCLASS_PORTS, nullptr);
    std::println("");

    std::println("");
    std::println("-----------");
    std::println("- Disk Drives -");
    std::println("-----------");
    // ListDevices(NULL, TEXT("STORAGE\\VOLUME"));
    //printf (TEXT("\n"));
    ListDevices(&GUID_DEVCLASS_DISKDRIVE, nullptr);

    std::println("");
    std::println(
        "----------------------------------------------------------------"
    );
    std::println("- devices with disk drives -");
    std::println(
        "----------------------------------------------------------------"
    );
    ListDevices(&GUID_DEVCLASS_DISKDRIVE, NULL);
}

// https://learn.microsoft.com/en-us/answers/questions/699300/how-to-get-drive-letter-from-usb-vid-and-pid
#endif // _WIN32
