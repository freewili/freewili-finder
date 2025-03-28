#include <gtest/gtest.h>

#include <usbdef.hpp>

TEST(USBVidPidMatch, BasicAssertions) {
    ASSERT_EQ(Fw::USB_VID_FW_HUB, 0x0424);
    ASSERT_EQ(Fw::USB_PID_FW_HUB, 0x2513);
    ASSERT_EQ(Fw::USB_VID_FW_FTDI, 0x0403);
    ASSERT_EQ(Fw::USB_PID_FW_FTDI, 0x6014);
    ASSERT_EQ(Fw::USB_VID_FW_RPI, 0x2E8A);
    ASSERT_EQ(Fw::USB_PID_FW_RPI_CDC_PID, 0x000A);
    ASSERT_EQ(Fw::USB_PID_FW_RPI_UF2_PID, 0x0003);
    ASSERT_EQ(Fw::USB_VID_FW_HUB, 0x0424);
}