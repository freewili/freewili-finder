#ifdef _WIN32

    #include <gtest/gtest.h>
    #include <windows.h>
    #include <tchar.h>

    #include <expected>
    #include <string>

    #include <fwfinder_windows.hpp>

TEST(win32, stringFromTCHARRaw) {
    const TCHAR* const buffer = TEXT("Hello World!");
    auto converted = stringFromTCHARRaw(buffer);
    ASSERT_TRUE(converted.has_value());
    ASSERT_EQ(converted.value().length(), 13);
    ASSERT_STREQ(converted.value().c_str(), "Hello World!");
}

TEST(win32, getLastErrorString) {
    auto errorStringResult = getLastErrorString(0);
    ASSERT_TRUE(errorStringResult.has_value()) << errorStringResult.error();
    ASSERT_STREQ(
        errorStringResult.value().c_str(),
        "The operation completed successfully.\r\n"
    );
}

TEST(win32, stringFromTCHAR) {
    const TCHAR* const buffer = TEXT("Hello World!");
    auto converted = stringFromTCHARRaw(buffer);
    ASSERT_TRUE(converted.has_value());
    ASSERT_EQ(converted.value().length(), 13);
    ASSERT_STREQ(converted.value().c_str(), "Hello World!");
}

TEST(win32, getUSBInstanceID) {
    // FTDI: USB\VID_0403&PID_6014\FW4607
    auto value = getUSBInstanceID("USB\\VID_0403&PID_6014\\FW4607");
    ASSERT_TRUE(value.has_value()) << value.error();
    ASSERT_STREQ(value.value().serial.c_str(), "FW4607");
    ASSERT_EQ(value.value().vid, 0x0403);
    ASSERT_EQ(value.value().pid, 0x6014);

    // RP2040 Serial: USB\VID_2E8A&PID_000A\E463A8574B3F3935
    value = getUSBInstanceID("USB\\VID_2E8A&PID_000A\\E463A8574B3F3935");
    ASSERT_TRUE(value.has_value()) << value.error();
    ASSERT_STREQ(value.value().serial.c_str(), "E463A8574B3F3935");
    ASSERT_EQ(value.value().vid, 0x2E8A);
    ASSERT_EQ(value.value().pid, 0x000A);

    // RP2040 UF2 Mass storage: USB\VID_2E8A&PID_0003&MI_00\A&22CF742D&1&0000
    value = getUSBInstanceID("USB\\VID_2E8A&PID_0003&MI_00\\A&22CF742D&1&0000");
    ASSERT_TRUE(value.has_value()) << value.error();
    ASSERT_STREQ(value.value().serial.c_str(), "A&22CF742D&1&0000");
    ASSERT_EQ(value.value().vid, 0x2E8A);
    ASSERT_EQ(value.value().pid, 0x0003);

    // RP2040 UF2: USB\VID_2E8A&PID_0003\E0C9125B0D9B
    value = getUSBInstanceID("USB\\VID_2E8A&PID_0003\\E0C9125B0D9B");
    ASSERT_TRUE(value.has_value()) << value.error();
    ASSERT_STREQ(value.value().serial.c_str(), "E0C9125B0D9B");
    ASSERT_EQ(value.value().vid, 0x2E8A);
    ASSERT_EQ(value.value().pid, 0x0003);

    // Microchip hub: USB\VID_0424&PID_2513\8&36C81A88&0&1
    value = getUSBInstanceID("USB\\VID_0424&PID_2513\\8&36C81A88&0&1");
    ASSERT_TRUE(value.has_value()) << value.error();
    ASSERT_STREQ(value.value().serial.c_str(), "8&36C81A88&0&1");
    ASSERT_EQ(value.value().vid, 0x0424);
    ASSERT_EQ(value.value().pid, 0x2513);

    // Negative test: ACPI\PNP0501\0
    value = getUSBInstanceID("ACPI\\PNP0501\\0");
    ASSERT_FALSE(value.has_value());
}
#endif // _WIN32
