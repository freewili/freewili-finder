#include <gtest/gtest.h>
#include <cfwfinder_internal.hpp>

class FixedStringCopyTest: public ::testing::Test {
protected:
    void SetUp() override {
        // Clear buffer before each test
        memset(buffer, 0xAA, sizeof(buffer)); // Fill with known pattern
    }

    static constexpr size_t BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];
};

// Test basic functionality with normal strings
TEST_F(FixedStringCopyTest, BasicCopy) {
    std::string source = "Hello World";
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), source.length());
    EXPECT_STREQ(buffer, "Hello World");
    EXPECT_EQ(buffer_size, source.length() + 1); // +1 for null terminator
    EXPECT_EQ(buffer[source.length()], '\0'); // Verify null termination
}

// Test with empty string
TEST_F(FixedStringCopyTest, EmptyString) {
    std::string source = "";
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(buffer_size, 1); // Just the null terminator
    EXPECT_EQ(buffer[0], '\0');
}

// Test with string that exactly fits buffer
TEST_F(FixedStringCopyTest, ExactFit) {
    std::string source(BUFFER_SIZE - 1, 'X'); // Fill buffer exactly (minus null terminator)
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BUFFER_SIZE - 1);
    EXPECT_EQ(strlen(buffer), BUFFER_SIZE - 1);
    EXPECT_EQ(buffer_size, BUFFER_SIZE); // size + null terminator
    EXPECT_EQ(buffer[BUFFER_SIZE - 1], '\0');
}

// Test with string longer than buffer (truncation)
TEST_F(FixedStringCopyTest, StringTooLong) {
    std::string source(BUFFER_SIZE + 10, 'Y'); // Longer than buffer
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), BUFFER_SIZE - 1); // Truncated length
    EXPECT_EQ(strlen(buffer), BUFFER_SIZE - 1);
    EXPECT_EQ(buffer_size, BUFFER_SIZE); // size + null terminator
    EXPECT_EQ(buffer[BUFFER_SIZE - 1], '\0');

    // Verify all characters before null terminator are 'Y'
    for (size_t i = 0; i < BUFFER_SIZE - 1; ++i) {
        EXPECT_EQ(buffer[i], 'Y');
    }
}

// Test with very small buffer
TEST_F(FixedStringCopyTest, SmallBuffer) {
    std::string source = "Hello";
    uint32_t buffer_size = 3; // Only room for 2 chars + null terminator

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 2);
    EXPECT_EQ(strlen(buffer), 2);
    EXPECT_EQ(buffer_size, 3);
    EXPECT_STREQ(buffer, "He");
    EXPECT_EQ(buffer[2], '\0');
}

// Test with buffer size of 1 (only null terminator)
TEST_F(FixedStringCopyTest, BufferSizeOne) {
    std::string source = "Hello";
    uint32_t buffer_size = 1;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0);
    EXPECT_EQ(strlen(buffer), 0);
    EXPECT_EQ(buffer_size, 1);
    EXPECT_STREQ(buffer, "");
    EXPECT_EQ(buffer[0], '\0');
}

// Test null pointer parameters
TEST_F(FixedStringCopyTest, NullDestPointer) {
    std::string source = "Hello";
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy<uint32_t>(nullptr, &buffer_size, source);

    EXPECT_FALSE(result.has_value());
}

TEST_F(FixedStringCopyTest, NullSizePointer) {
    std::string source = "Hello";

    auto result = fixedStringCopy<uint32_t>(buffer, nullptr, source);

    EXPECT_FALSE(result.has_value());
}

TEST_F(FixedStringCopyTest, BothNullPointers) {
    std::string source = "Hello";

    auto result = fixedStringCopy<uint32_t>(nullptr, nullptr, source);

    EXPECT_FALSE(result.has_value());
}

// Test with different unsigned integer types
TEST_F(FixedStringCopyTest, DifferentSizeTypes) {
    std::string source = "Test";

    // Test with uint8_t
    {
        uint8_t size8 = 10;
        auto result = fixedStringCopy(buffer, &size8, source);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), 4);
        EXPECT_EQ(size8, 5);
    }

    // Test with uint16_t
    {
        uint16_t size16 = 10;
        auto result = fixedStringCopy(buffer, &size16, source);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), 4);
        EXPECT_EQ(size16, 5);
    }

    // Test with uint64_t
    {
        uint64_t size64 = 10;
        auto result = fixedStringCopy(buffer, &size64, source);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result.value(), 4);
        EXPECT_EQ(size64, 5);
    }
}

// Test buffer clearing behavior
TEST_F(FixedStringCopyTest, BufferClearing) {
    // Fill buffer with known pattern
    memset(buffer, 0xFF, BUFFER_SIZE);

    std::string source = "Hi";
    uint32_t buffer_size = 10;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_STREQ(buffer, "Hi");

    // Verify that bytes after the string are cleared (not 0xFF anymore)
    for (size_t i = 3; i < 10; ++i) {
        EXPECT_EQ(buffer[i], 0) << "Buffer not properly cleared at index " << i;
    }
}

// Test with string containing null characters
TEST_F(FixedStringCopyTest, StringWithNullChars) {
    std::string source = std::string("Hello\0World", 11); // String with embedded null
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 11);
    EXPECT_EQ(buffer_size, 12);

    // Verify the embedded null is preserved
    EXPECT_EQ(buffer[5], '\0');
    EXPECT_EQ(buffer[11], '\0'); // Final null terminator

    // Verify the content manually since STREQ stops at first null
    EXPECT_EQ(memcmp(buffer, "Hello\0World\0", 12), 0);
}

// Test with Unicode/UTF-8 strings
TEST_F(FixedStringCopyTest, UTF8String) {
    std::string source = "Hello 世界"; // UTF-8 string
    uint32_t buffer_size = BUFFER_SIZE;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), source.length());
    EXPECT_EQ(buffer_size, source.length() + 1);
    EXPECT_EQ(memcmp(buffer, source.c_str(), source.length()), 0);
    EXPECT_EQ(buffer[source.length()], '\0');
}

// Test edge case: buffer size 0 (should return nullopt safely)
TEST_F(FixedStringCopyTest, BufferSizeZero) {
    std::string source = "Hello";
    uint32_t buffer_size = 0;

    auto result = fixedStringCopy(buffer, &buffer_size, source);

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(buffer_size, 0); // Should remain unchanged
}
