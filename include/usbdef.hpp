#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace Fw {
/// FreeWili USB Hub Vendor ID.
const uint16_t USB_VID_FW_HUB = 0x0424;
/// FreeWili USB Hub Product ID.
const uint16_t USB_PID_FW_HUB = 0x2513;

/// FreeWili Black FTDI VendorID
const uint16_t USB_VID_FW_FTDI = 0x0403;
/// FreeWili Black FTDI ProductID
const uint16_t USB_PID_FW_FTDI = 0x6014;

/// Raspberry Pi Vendor ID
const uint16_t USB_VID_FW_RPI = 0x2E8A;
const uint16_t USB_VID_FW_ICS = 0x093C;
/// Raspberry Pi Pico SDK CDC UART Product ID
const uint16_t USB_PID_FW_RPI_CDC_PID = 0x000A;
const uint16_t USB_PID_FW_MAIN_CDC_PID = 0x2054;
const uint16_t USB_PID_FW_DISPLAY_CDC_PID = 0x2055;
/// Raspberry Pi Pico SDK UF2 Product ID
const uint16_t USB_PID_FW_RPI_UF2_PID = 0x0003;

/// ESP32-C6 USB
const uint16_t USB_VID_FW_ESP32 = 0x303A;
const uint16_t USB_PID_FW_ESP32_JTAG = 0x1001;
/// ESP32-C6 Silicon Labs CP210x USB to UART Bridge
const uint16_t USB_VID_FW_ESP32_SERIAL = 0x10C4;
const uint16_t USB_PID_FW_ESP32_SERIAL = 0xEA60;

/// FreeWili Winky Product ID
const uint16_t USB_PID_FW_WINKY = 0x2056;
/// DefCon 2024 Badge Product ID
const uint16_t USB_PID_FW_DEFCON_2024 = 0x2057;
/// DefCon 2025 FreeWili Badge Product ID
const uint16_t USB_PID_FW_DEFCON_BADGE_2025 = 0x2058;

static std::map<uint16_t, std::vector<uint16_t>> WhitelistVIDPID = {
    { USB_VID_FW_HUB,
      {
          USB_PID_FW_HUB,
      } },
    { USB_VID_FW_FTDI,
      {
          USB_PID_FW_FTDI,
      } },
    { USB_VID_FW_RPI, { USB_PID_FW_RPI_CDC_PID, USB_PID_FW_RPI_UF2_PID } },
    { USB_VID_FW_ICS,
      { USB_PID_FW_MAIN_CDC_PID,
        USB_PID_FW_DISPLAY_CDC_PID,
        USB_PID_FW_WINKY,
        USB_PID_FW_DEFCON_2024,
        USB_PID_FW_DEFCON_BADGE_2025 } },
};

auto is_vid_pid_whitelisted(uint16_t vid, uint16_t pid) -> bool;

}; // namespace Fw
