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

# Copy DLL to the examples directory on Windows
if(WIN32)
    add_custom_command(TARGET basic_usage_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:basic_usage_cpp>
        COMMENT "Copying fwfinder.dll to examples directory"
    )
endif()

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

    # Copy DLLs to the examples directory on Windows
    if(WIN32)
        add_custom_command(TARGET basic_usage_c POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:cfwfinder>
            $<TARGET_FILE_DIR:basic_usage_c>
            COMMENT "Copying cfwfinder.dll to examples directory"
        )
        add_custom_command(TARGET basic_usage_c POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:basic_usage_c>
            COMMENT "Copying fwfinder.dll to examples directory"
        )
    endif()
endif ()
