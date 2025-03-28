#ifdef _WIN32

    #include <gtest/gtest.h>
    #include <windows.h>
    #include <tchar.h>

    #include <expected>
    #include <string>

extern auto stringFromTCHAR(const TCHAR* const msg)
    -> std::expected<std::string, DWORD>;
extern auto getLastErrorString(DWORD errorCode)
    -> std::expected<std::string, DWORD>;

TEST(win32, stringFromTCHAR) {
    const TCHAR* const buffer = TEXT("Hello World!");
    auto converted = stringFromTCHAR(buffer);
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

#endif // _WIN32
