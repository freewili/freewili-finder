// NOLINTBEGIN
#ifdef _WIN32
    #include <windows.h>
    #include <stringapiset.h>
    #include <winnls.h>
    #include <errhandlingapi.h>
    #include <tchar.h>
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

    #include <cstdio>
    #include <cstring>
    #include <expected>
    #include <sstream>
    #include <string>
    #include <print>
    #include <fwfinder.hpp>

    #define INITGUID

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

// Convert TCHAR to std::string, return string representation of GetLastError() otherwise.
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

    // #include "c:\WinDDK\7600.16385.1\inc\api\devpkey.h"

    // include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpropdef.h
    #ifdef DEFINE_DEVPROPKEY
        #undef DEFINE_DEVPROPKEY
    #endif
    #ifdef INITGUID
        #define DEFINE_DEVPROPKEY( \
            name, \
            l, \
            w1, \
            w2, \
            b1, \
            b2, \
            b3, \
            b4, \
            b5, \
            b6, \
            b7, \
            b8, \
            pid \
        ) \
            EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY \
                name = {{l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}, pid}
    #else
        #define DEFINE_DEVPROPKEY( \
            name, \
            l, \
            w1, \
            w2, \
            b1, \
            b2, \
            b3, \
            b4, \
            b5, \
            b6, \
            b7, \
            b8, \
            pid \
        ) \
            EXTERN_C const DEVPROPKEY name
    #endif // INITGUID

// include DEVPKEY_Device_BusReportedDeviceDesc from WinDDK\7600.16385.1\inc\api\devpkey.h
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_BusReportedDeviceDesc,
    0x540b947e,
    0x8b40,
    0x45bc,
    0xa8,
    0xa2,
    0x6a,
    0x0b,
    0x89,
    0x4c,
    0xbd,
    0xa2,
    4
); // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_ContainerId,
    0x8c7ed206,
    0x3f8a,
    0x4827,
    0xb3,
    0xab,
    0xae,
    0x9e,
    0x1f,
    0xae,
    0xfc,
    0x6c,
    2
); // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_FriendlyName,
    0xa45c254e,
    0xdf1c,
    0x4efd,
    0x80,
    0x20,
    0x67,
    0xd1,
    0x46,
    0xa8,
    0x50,
    0xe0,
    14
); // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(
    DEVPKEY_DeviceDisplay_Category,
    0x78c34fc8,
    0x104a,
    0x4aca,
    0x9e,
    0xa4,
    0x52,
    0x4d,
    0x52,
    0x99,
    0x6e,
    0x57,
    0x5a
); // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_LocationInfo,
    0xa45c254e,
    0xdf1c,
    0x4efd,
    0x80,
    0x20,
    0x67,
    0xd1,
    0x46,
    0xa8,
    0x50,
    0xe0,
    15
); // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_Manufacturer,
    0xa45c254e,
    0xdf1c,
    0x4efd,
    0x80,
    0x20,
    0x67,
    0xd1,
    0x46,
    0xa8,
    0x50,
    0xe0,
    13
); // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(
    DEVPKEY_Device_SecuritySDS,
    0xa45c254e,
    0xdf1c,
    0x4efd,
    0x80,
    0x20,
    0x67,
    0xd1,
    0x46,
    0xa8,
    0x50,
    0xe0,
    26
); // DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING

auto Fw::find_all() noexcept
    -> std::expected<Fw::FreeWiliDevices, std::string> {
    HDEVINFO hDevInfo = nullptr;
    /*
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
    */
    // List all connected USB devices - GUID_DEVCLASS_USB
    if (auto hDevInfo = SetupDiGetClassDevs(
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
        if (auto result = stringFromTCHAR(instanceID); !result.has_value()) {
            return std::unexpected(result.error());
        }

        // name = Bus Reported Device Description + Device Description
    }

    // List all connected Ports - GUID_DEVCLASS_PORTS

    // List all connected Disk Drives - GUID_DEVCLASS_DISKDRIVE

    return std::unexpected("TODO");
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
            == 0) {
            continue;
        }
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

    // printf (TEXT("\n"));
    // printf (TEXT("----------------------------------------------------------------\n"));
    // printf (TEXT("- devices with disk drives -\n"));
    // printf (TEXT("----------------------------------------------------------------\n"));
    // ListDevices(&GUID_DEVCLASS_DISKDRIVE, NULL);
}

#endif // _WIN32
