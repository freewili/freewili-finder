#pragma once

#include <cstdint>
#include <map>
#include <vector>

namespace Fw {
/*
● 2-0   USB Root Hub (USB 3.0) PCI\VEN_1022&DEV_15C0&SUBSYS_50D917AA&REV_00\4&f0530c4&0&0343  -
└──○    1   1 0x093c 0x2059 FREE-WILi2 FWTST1
   ├──•  1 󰅕 󰚥   2 mA
   │  ├──◦ 0-1:1.0  0x00 Hub   0x00 0x01 -
   │  └──◦ 0-1:1.0  0x01 Hub   0x00 0x02 -
   ├──○    1   2 0x093c 0x2060 FW2 v01    C6ACB61A8BD41507
   │  └──•  1 󱇰   250 mA
   │     ├──◦ 0-1.1:1.0  0x00 CdcCommunications   0x02 0x00 Board CDC
   │     ├──◦ 0-1.1:1.1  0x00 CdcData             0x00 0x00 -
   │     └──◦ 0-1.1:1.2 ☶ 0x00 VendorSpecificClass 0x00 0x01 Reset
   ├──○    3   3 0x0403 0x6014 Single RS232-HS -
   │  └──•  1 󱇰   500 mA
   │     └──◦ 0-1.3:1.0 ☶ 0x00 VendorSpecificClass 0xff 0xff Single RS232-HS
   ├──○    5   4 0x303a 0x1001 USB JTAG/serial debug unit 3C:DC:75:84:FA:BC
   │  └──•  1 󰚥   500 mA
   │     ├──◦ 0-1.5:1.0  0x00 CdcCommunications   0x02 0x00 -
   │     ├──◦ 0-1.5:1.1  0x00 CdcData             0x02 0x00 -
   │     └──◦ 0-1.5:1.2 ☶ 0x00 VendorSpecificClass 0xff 0x01 -
   └──○    6   5 0x0424 0x2240 Ultra Fast Media  000000225001
      └──•  1 󱇰    96 mA
         └──◦ 0-1.6:1.0  0x00 MassStorage 0x06 0x50 -
*/
/// FREE-WILi2 USB Hub Vendor ID.
const uint16_t USB_VID_FW2_HUB = 0x093C;
/// FREE-WILi2 USB Hub Product ID.
const uint16_t USB_PID_FW2_HUB = 0x2059;
/// FREE-WILi2 Main Vendor ID.
const uint16_t USB_VID_FW2_MAIN = 0x093C;
/// FREE-WILi2 Main Product ID.
const uint16_t USB_PID_FW2_MAIN = 0x2060; // 0x205A;
/// FREE-WILi2 FTDI Vendor ID.
const uint16_t USB_VID_FW2_FTDI = 0x0403;
/// FREE-WILi2 FTDI Product ID.
const uint16_t USB_PID_FW2_FTDI = 0x6014;
/// FREE-WILi2 ESP32 Vendor ID.
const uint16_t USB_VID_FW2_ESP32 = 0x303A;
/// FREE-WILi2 ESP32 JTAG Product ID.
const uint16_t USB_PID_FW2_ESP32_JTAG = 0x1001;
/// FREE-WILi2 ESP32 Serial Product ID.
const uint16_t USB_PID_FW2_ESP32_SERIAL = 0xEA60;
/// FREE-WILi2 SD Card Reader Vendor ID.
const uint16_t USB_VID_FW2_SDCARD = 0x0424;
/// FREE-WILi2 SD Card Reader Product ID.
const uint16_t USB_PID_FW2_SDCARD = 0x2240;

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
      { USB_PID_FW_RPI_CDC_PID, USB_PID_FW_RPI_2040_UF2_PID, USB_PID_FW_RPI_2350_UF2_PID } },
    { USB_VID_FW_ICS,
      { USB_PID_FW_MAIN_CDC_PID,
        USB_PID_FW_DISPLAY_CDC_PID,
        USB_PID_FW_WINKY,
        USB_PID_FW_DEFCON_2024,
        USB_PID_FW_DEFCON_BADGE_2025,
        USB_PID_FW2_HUB,
        USB_PID_FW2_MAIN } },
};

auto is_vid_pid_whitelisted(uint16_t vid, uint16_t pid) -> bool;

}; // namespace Fw
