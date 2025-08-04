# ============================================================================
# Examples CMakeLists.txt
# ============================================================================

# C++ Example using the main fwfinder library
add_executable(
    basic_usage_cpp
    basic_usage.cpp
)

target_include_directories(
    basic_usage_cpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(
    basic_usage_cpp
    PRIVATE
        ${PROJECT_NAME}
)

# C Example using the C API
if (FW_BUILD_C_API)
    add_executable(
        basic_usage_c
        c_api_basic_usage.c
    )

    target_include_directories(
        basic_usage_c
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../c_api/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    # Link against shared C API library
    target_link_libraries(
        basic_usage_c
        PRIVATE
            cfwfinder
    )
endif ()
