#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace Fw {
/*
FREE-WILi2 topology as enumerated from production hardware (serial FX0025):

/: Bus 003.Port 004: Dev 003, Class=Hub, Driver=hub/6p, 480M
    |__ Port 001: Dev 060, 0x093c 0x2059 FREE-WILi2 FX0025 (internal hub, 7 ports)
        |__ Port 001: Dev 061, 0x093c 0x205a FW2 v07 FX0025
        |      |__ If 0, Class=Communications, Driver=cdc_acm
        |      |__ If 1, Class=CDC Data, Driver=cdc_acm
        |__ Port 003: Dev 062, 0x0403 0x6014 FREE-WILi FW2 FX0025
        |      |__ If 0, Class=Vendor Specific Class, Driver=ftdi_sio
        |__ Port 004: Dev 067, 0x2e8a 0x000c FreeWili Debug Probe (CMSIS-DAP) E66568714F28A828
        |      |__ If 0-3, Class=Vendor Specific Class (CMSIS-DAP)
        |      |__ If 4-7, Class=Communications/CDC Data, Driver=cdc_acm (2 ports)
        |__ Port 006: Dev 066, 0x093c 0x205f FW Ultra Fast Media 0000395D5D4D
               |__ If 0, Class=Mass Storage, Driver=usb-storage

The Display processor (0x093c 0x2060) is an optional component that isn't
populated on most units, so it is usually absent from the tree above.
*/
/// FREE-WILi2 USB Hub Vendor ID.
const uint16_t USB_VID_FW2_HUB = 0x093C;
/// FREE-WILi2 USB Hub Product ID.
const uint16_t USB_PID_FW2_HUB = 0x2059;
/// FREE-WILi2 Main Vendor ID.
const uint16_t USB_VID_FW2_MAIN = 0x093C;
/// FREE-WILi2 Main Product ID.
const uint16_t USB_PID_FW2_MAIN = 0x205A;
/// FREE-WILi2 Display Vendor ID.
const uint16_t USB_VID_FW2_DISPLAY = 0x093C;
/// FREE-WILi2 Display Product ID. Optional component, usually not populated.
const uint16_t USB_PID_FW2_DISPLAY = 0x2060;
/// FREE-WILi2 FTDI Vendor ID.
const uint16_t USB_VID_FW2_FTDI = 0x0403;
/// FREE-WILi2 FTDI Product ID.
const uint16_t USB_PID_FW2_FTDI = 0x6014;
/// FREE-WILi2 Debug Probe (CMSIS-DAP) Vendor ID.
const uint16_t USB_VID_FW2_DEBUG_PROBE = 0x2E8A;
/// FREE-WILi2 Debug Probe (CMSIS-DAP) Product ID.
const uint16_t USB_PID_FW2_DEBUG_PROBE = 0x000C;
/// FREE-WILi2 Mass Storage ("FW Ultra Fast Media") Vendor ID.
const uint16_t USB_VID_FW2_MASS_STORAGE = 0x093C;
/// FREE-WILi2 Mass Storage ("FW Ultra Fast Media") Product ID.
const uint16_t USB_PID_FW2_MASS_STORAGE = 0x205F;

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
/// Raspberry Pi RP2040 UF2 Product ID
const uint16_t USB_PID_FW_RPI_2040_UF2_PID = 0x0003;
/// Raspberry Pi RP2350 UF2 Product ID
const uint16_t USB_PID_FW_RPI_2350_UF2_PID = 0x000F;

/// ESP32-C6 USB
const uint16_t USB_VID_FW_ESP32 = 0x303A;
const uint16_t USB_PID_FW_ESP32_JTAG = 0x1001;
/// ESP32-C6 Silicon Labs CP210x USB to UART Bridge
const uint16_t USB_VID_FW_ESP32_SERIAL = 0x10C4;
const uint16_t USB_PID_FW_ESP32_SERIAL = 0xEA60;

/// FreeWili Winky Product ID
const uint16_t USB_PID_FW_WINKY = 0x2056;
/// DEFCON 2024 Badge Product ID
const uint16_t USB_PID_FW_DEFCON_2024 = 0x2057;
/// DEFCON 2025 FreeWili Badge Product ID
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
    { USB_VID_FW_RPI,
      { USB_PID_FW_RPI_CDC_PID,
        USB_PID_FW_RPI_2040_UF2_PID,
        USB_PID_FW_RPI_2350_UF2_PID,
        USB_PID_FW2_DEBUG_PROBE } },
    { USB_VID_FW_ICS,
      { USB_PID_FW_MAIN_CDC_PID,
        USB_PID_FW_DISPLAY_CDC_PID,
        USB_PID_FW_WINKY,
        USB_PID_FW_DEFCON_2024,
        USB_PID_FW_DEFCON_BADGE_2025,
        USB_PID_FW2_HUB,
        USB_PID_FW2_MAIN,
        USB_PID_FW2_DISPLAY,
        USB_PID_FW2_MASS_STORAGE } },
};

auto is_vid_pid_whitelisted(uint16_t vid, uint16_t pid) -> bool;

/// @brief Check if a VID/PID pair identifies a USB hub that a FreeWili is built around.
///
/// Both the FREE-WILi (0x0424/0x2513) and the FREE-WILi2 (0x093C/0x2059) present
/// themselves as an internal hub with the individual processors hanging off of it.
///
/// @param vid USB Vendor ID
/// @param pid USB Product ID
/// @return true if the VID/PID belongs to a known FreeWili hub.
auto is_freewili_hub(uint16_t vid, uint16_t pid) -> bool;

}; // namespace Fw
