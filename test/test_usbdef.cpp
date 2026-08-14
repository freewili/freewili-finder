#include <gtest/gtest.h>

#include <usbdef.hpp>

TEST(USBVidPidMatch, BasicAssertions) {
    ASSERT_EQ(Fw::USB_VID_FW_HUB, 0x0424);
    ASSERT_EQ(Fw::USB_PID_FW_HUB, 0x2513);
    ASSERT_EQ(Fw::USB_VID_FW_FTDI, 0x0403);
    ASSERT_EQ(Fw::USB_PID_FW_FTDI, 0x6014);
    ASSERT_EQ(Fw::USB_VID_FW_RPI, 0x2E8A);
    ASSERT_EQ(Fw::USB_PID_FW_RPI_CDC_PID, 0x000A);
    ASSERT_EQ(Fw::USB_PID_FW_RPI_2040_UF2_PID, 0x0003);
    ASSERT_EQ(Fw::USB_PID_FW_RPI_2350_UF2_PID, 0x000F);
    ASSERT_EQ(Fw::USB_VID_FW_HUB, 0x0424);
    ASSERT_EQ(Fw::USB_VID_FW_ESP32, 0x303A);
    ASSERT_EQ(Fw::USB_PID_FW_ESP32_JTAG, 0x1001);
    ASSERT_EQ(Fw::USB_VID_FW_ESP32_SERIAL, 0x10C4);
    ASSERT_EQ(Fw::USB_PID_FW_ESP32_SERIAL, 0xEA60);
}

TEST(USBVidPidMatch, FW2Constants) {
    ASSERT_EQ(Fw::USB_VID_FW2_HUB, 0x093C);
    ASSERT_EQ(Fw::USB_PID_FW2_HUB, 0x2059);
    ASSERT_EQ(Fw::USB_VID_FW2_MAIN, 0x093C);
    ASSERT_EQ(Fw::USB_PID_FW2_MAIN, 0x205A);
    ASSERT_EQ(Fw::USB_VID_FW2_DISPLAY, 0x093C);
    ASSERT_EQ(Fw::USB_PID_FW2_DISPLAY, 0x2060);
    ASSERT_EQ(Fw::USB_VID_FW2_FTDI, 0x0403);
    ASSERT_EQ(Fw::USB_PID_FW2_FTDI, 0x6014);
    ASSERT_EQ(Fw::USB_VID_FW2_DEBUG_PROBE, 0x2E8A);
    ASSERT_EQ(Fw::USB_PID_FW2_DEBUG_PROBE, 0x000C);
    ASSERT_EQ(Fw::USB_VID_FW2_MASS_STORAGE, 0x093C);
    ASSERT_EQ(Fw::USB_PID_FW2_MASS_STORAGE, 0x205F);
}

TEST(USBVidPidMatch, FW2Whitelisted) {
    ASSERT_TRUE(Fw::is_vid_pid_whitelisted(Fw::USB_VID_FW2_HUB, Fw::USB_PID_FW2_HUB));
    ASSERT_TRUE(Fw::is_vid_pid_whitelisted(Fw::USB_VID_FW2_MAIN, Fw::USB_PID_FW2_MAIN));
    ASSERT_TRUE(Fw::is_vid_pid_whitelisted(Fw::USB_VID_FW2_DISPLAY, Fw::USB_PID_FW2_DISPLAY));
    ASSERT_TRUE(Fw::is_vid_pid_whitelisted(Fw::USB_VID_FW2_FTDI, Fw::USB_PID_FW2_FTDI));
    ASSERT_TRUE(Fw::is_vid_pid_whitelisted(Fw::USB_VID_FW2_DEBUG_PROBE, Fw::USB_PID_FW2_DEBUG_PROBE)
    );
    ASSERT_TRUE(
        Fw::is_vid_pid_whitelisted(Fw::USB_VID_FW2_MASS_STORAGE, Fw::USB_PID_FW2_MASS_STORAGE)
    );
}

TEST(USBVidPidMatch, IsFreeWiliHub) {
    // Both generations are built around an internal hub
    ASSERT_TRUE(Fw::is_freewili_hub(Fw::USB_VID_FW_HUB, Fw::USB_PID_FW_HUB));
    ASSERT_TRUE(Fw::is_freewili_hub(Fw::USB_VID_FW2_HUB, Fw::USB_PID_FW2_HUB));

    ASSERT_FALSE(Fw::is_freewili_hub(Fw::USB_VID_FW2_MAIN, Fw::USB_PID_FW2_MAIN));
    ASSERT_FALSE(Fw::is_freewili_hub(Fw::USB_VID_FW_FTDI, Fw::USB_PID_FW_FTDI));
    // Cross-matched VID/PID between the two generations should not match
    ASSERT_FALSE(Fw::is_freewili_hub(Fw::USB_VID_FW_HUB, Fw::USB_PID_FW2_HUB));
    ASSERT_FALSE(Fw::is_freewili_hub(Fw::USB_VID_FW2_HUB, Fw::USB_PID_FW_HUB));
    ASSERT_FALSE(Fw::is_freewili_hub(0, 0));
}
