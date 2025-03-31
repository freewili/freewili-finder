#pragma once

#include <cstdint>

namespace Fw {
//! FreeWili USB Hub Vendor ID.
const uint32_t USB_VID_FW_HUB = 0x0424;
//! FreeWili USB Hub Product ID.
const uint32_t USB_PID_FW_HUB = 0x2513;

//! FreeWili Black FTDI VendorID
const uint32_t USB_VID_FW_FTDI = 0x0403;
//! FreeWili Black FTDI ProductID
const uint32_t USB_PID_FW_FTDI = 0x6014;

//! Raspberry Pi Vendor ID
const uint32_t USB_VID_FW_RPI = 0x2E8A;
//! Raspberry Pi Pico SDK CDC UART Product ID
const uint32_t USB_PID_FW_RPI_CDC_PID = 0x000A;
//! Raspberry Pi Pico SDK UF2 Product ID
const uint32_t USB_PID_FW_RPI_UF2_PID = 0x0003;
}; // namespace Fw
