#pragma once
#ifdef _WIN32

    #include <windows.h>
    #include <tchar.h>

    #include <string>
    #include <expected>
    #include <vector>

struct USBInstanceID {
    uint16_t vid;
    uint16_t pid;
    std::string serial;
};

/// @brief Convert TCHAR to std::string. Use stringFromTCHAR is usually preferred.
/// @param msg TCHAR to convert
/// @return std::string on success, DWORD on failure.
auto stringFromTCHARRaw(const TCHAR* const msg)
    -> std::expected<std::string, DWORD>;

/// @brief Convert GetLastError() int a string
/// @param errorCode value from GetLastError()
/// @return std::string on success, DWORD on failure.
auto getLastErrorString(DWORD errorCode) -> std::expected<std::string, DWORD>;

// Convert TCHAR to std::string, return string representation of GetLastError() otherwise.
auto stringFromTCHAR(const TCHAR* const msg)
    -> std::expected<std::string, std::string>;

/// @brief Split the Device instance ID by backspaces.
/// @param value Instance ID (ie. "USB\VID_0403&PID_6014\FW4607")
/// @return std::vector<std::string> on success, std::string on error.
auto splitInstanceID(std::string value)
    -> std::expected<std::vector<std::string>, std::string>;

/// @brief  Construct the USBInstanceID struct from the raw Device InstanceID string
/// @param value Instance ID (ie. "USB\VID_0403&PID_6014\FW4607")
/// @return USBInstanceID on success, std::string on error.
auto getUSBInstanceID(std::string value)
    -> std::expected<USBInstanceID, std::string>;

#endif // _WIN32
