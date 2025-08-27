#pragma once

#include <string>
#include <optional>
#include <concepts>
#include <cstring>

/**
 * @brief Template function for safely copying strings to fixed-size buffers
 *
 * This function copies a string to a destination buffer with size tracking,
 * ensuring null termination and preventing buffer overflows.
 *
 * @tparam size_type An unsigned integral type for size parameters
 * @param dest Destination buffer (must not be nullptr)
 * @param dest_size Pointer to buffer size (input: max size, output: actual size including null terminator)
 * @param src Source string to copy
 * @return Optional containing the number of characters copied (excluding null terminator), or nullopt on error
 */
template<typename size_type>
    requires std::unsigned_integral<size_type>
auto fixedStringCopy(char* const dest, size_type* dest_size, std::string src)
    -> std::optional<size_type> {
    if (dest == nullptr || dest_size == nullptr || *dest_size == 0) {
        return std::nullopt;
    }

    // Clear the destination buffer for safety
    memset(dest, 0, *dest_size);
    size_type size = src.copy(dest, *dest_size - 1);
    dest[size] = '\0'; // Null-terminate the string
    *dest_size = size + 1; // Update the size to include the null terminator
    return size;
}
