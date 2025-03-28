// NOLINTBEGIN
#ifdef _WIN32
    #include <windows.h>
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
    #include <fwfinder.hpp>

    #define INITGUID

Fw::Finder::Finder() {}

auto stringFromTCHAR(const TCHAR* const msg)
    -> std::expected<std::string, DWORD> {
    #if defined(UNICODE)
    if (size_t width =
            WideCharToMultiByte(CP_UTF8, 0, msg, -1, 0, 0, NULL, NULL);
        width != 0) {
        std::string new_msg(width, '\0');
        if (auto success = WideCharToMultiByte(
                CP_UTF8,
                0,
                (LPWSTR)msg,
                -1,
                new_msg.data(),
                width,
                NULL,
                NULL
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
            NULL,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&buffer,
            0,
            NULL
        );
        len != 0) {
        LocalFree(buffer);
        return stringFromTCHAR(buffer);
    } else {
        LocalFree(buffer);
        // Error state, FormatMessage failed.
        return std::unexpected(GetLastError());
    }
}

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
        if (auto result = stringFromTCHAR(szValue); result.has_value()) {
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
        printf("%s\n", _toString(szDeviceInstanceID).c_str());

        if (SetupDiGetDeviceRegistryProperty(
                hDevInfo,
                &DeviceInfoData,
                SPDRP_DEVICEDESC,
                &dwPropertyRegDataType,
                (BYTE*)szDesc,
                sizeof(szDesc), // The size, in bytes
                &dwSize
            )) {
            printf(
                "    Device Description: \"%s\"\n",
                _toString(szDesc).c_str()
            );
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
            printf("    Hardware IDs:\n");
            for (pszId = szHardwareIDs; *pszId != TEXT('\0')
                 && pszId + dwSize / sizeof(TCHAR)
                     <= szHardwareIDs + ARRAYSIZE(szHardwareIDs);
                 pszId += lstrlen(pszId) + 1) {
                printf("        \"%s\"\n", _toString(pszId).c_str());
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
            )) {
            if (SetupDiGetDevicePropertyW(
                    hDevInfo,
                    &DeviceInfoData,
                    &DEVPKEY_Device_BusReportedDeviceDesc,
                    &ulPropertyType,
                    (BYTE*)szBuffer,
                    sizeof(szBuffer),
                    &dwSize,
                    0
                )) {
                printf(
                    "    Bus Reported Device Description: \"%s\"\n",
                    _toString(szBuffer).c_str()
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
                )) {
                printf(
                    "    Device Manufacturer: \"%s\"\n",
                    _toString(szBuffer).c_str()
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
                )) {
                printf(
                    "    Device Friendly Name: \"%s\"\n",
                    _toString(szBuffer).c_str()
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
                )) {
                printf(
                    "    Device Location Info: \"%s\"\n",
                    _toString(szBuffer).c_str()
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
                )) {
                // See Security Descriptor Definition Language on MSDN
                // (http://msdn.microsoft.com/en-us/library/windows/desktop/aa379567(v=vs.85).aspx)
                printf(
                    "    Device Security Descriptor String: \"%s\"\n",
                    _toString(szBuffer).c_str()
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
                )) {
                StringFromGUID2(
                    (REFGUID)szDesc,
                    szBuffer,
                    ARRAY_SIZE(szBuffer)
                );
                printf(
                    "    ContainerId: \"%s\"\n",
                    _toString(szBuffer).c_str()
                );
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
                )) {
                printf(
                    "    Device Display Category: \"%s\"\n",
                    _toString(szBuffer).c_str()
                );
            }
        }

        pszToken = _tcstok_s(szDeviceInstanceID, TEXT("\\#&"), &pszNextToken);
        while (pszToken != nullptr) {
            szVid[0] = TEXT('\0');
            szPid[0] = TEXT('\0');
            szMi[0] = TEXT('\0');
            for (j = 0; j < 3; j++) {
                if (_tcsncmp(pszToken, arPrefix[j], lstrlen(arPrefix[j]))
                    == 0) {
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
            }
            if (szVid[0] != TEXT('\0')) {
                printf("    vid: \"%s\"\n", _toString(szVid).c_str());
            }
            if (szPid[0] != TEXT('\0')) {
                printf("    pid: \"%s\"\n", _toString(szPid).c_str());
            }
            if (szMi[0] != TEXT('\0')) {
                printf("    mi: \"%s\"\n", _toString(szMi).c_str());
            }
            pszToken = _tcstok_s(nullptr, TEXT("\\#&"), &pszNextToken);
        }
    }
}

//void Fw::list_all() {}

void Fw::list_all() {
    // // List all connected USB devices
    // printf (TEXT("---------------------------------------------------\n"));
    // printf (TEXT("- USB devices -\n"));
    // printf (TEXT("---------------------------------------------------\n"));
    // ListDevices(NULL, TEXT("USB"));

    printf("\n");
    printf("-------------------------------------------------------\n");
    printf("- USBSTOR devices -\n");
    printf("-------------------------------------------------------\n");
    ListDevices(nullptr, TEXT("USBSTOR"));

    // printf (TEXT("\n"));
    // printf (TEXT("--------------------------------------------------\n"));
    // printf (TEXT("- SD devices -\n"));
    // printf (TEXT("--------------------------------------------------\n"));
    // ListDevices(NULL, TEXT("SD"));

    printf("\n");
    printf("--------------------------------------------------\n");
    printf("USB\n");
    printf("--------------------------------------------------\n");
    ListDevices(&GUID_DEVCLASS_USB, nullptr);
    printf("\n");

    printf("\n");
    printf("--------------------------------------------------\n");
    printf("Ports\n");
    printf("--------------------------------------------------\n");
    ListDevices(&GUID_DEVCLASS_PORTS, nullptr);
    printf("\n");

    printf("\n");
    printf("-----------\n");
    printf("- Disk Drives -\n");
    printf("-----------\n");
    // ListDevices(NULL, TEXT("STORAGE\\VOLUME"));
    //printf (TEXT("\n"));
    ListDevices(&GUID_DEVCLASS_DISKDRIVE, nullptr);

    // printf (TEXT("\n"));
    // printf (TEXT("----------------------------------------------------------------\n"));
    // printf (TEXT("- devices with disk drives -\n"));
    // printf (TEXT("----------------------------------------------------------------\n"));
    // ListDevices(&GUID_DEVCLASS_DISKDRIVE, NULL);
}

#endif // _WIN32 \
    // NOLINTEND
