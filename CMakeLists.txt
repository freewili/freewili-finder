cmake_minimum_required(VERSION 3.20)
project(fwfinder)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# ============================================================================
# Options
# ============================================================================
option(FW_BUILD_STATIC "Build static library." ON)
option(FW_BUILD_C_API "Build C API." ON)
option(FW_FINDER_BUILD_TESTS "Build tests" ON)
option(FW_FINDER_ENABLE_BINDINGS_PYTHON "Build Python bindings" OFF)
option(FW_BUILD_EXAMPLES "Build examples" ON)


if (FW_FINDER_ENABLE_BINDINGS_PYTHON)
    set(FW_BUILD_STATIC ON)
    add_subdirectory(bindings)
endif ()



# ============================================================================
# Source files
# ============================================================================
set(SRC_FILES
    src/fwfinder.cpp
    src/fwfinder_linux.cpp
    src/fwfinder_mac.cpp
    src/fwfinder_windows.cpp
    src/usbdef.cpp
)

# Unit test files
set(TEST_SRC_FILES
    test/test_fwfinder.cpp
    test/test_usbdef.cpp
    test/test_win32.cpp
)

# ============================================================================
# Compiler and linker options
# ============================================================================
if(MSVC)
    set_source_files_properties(${SRC_FILES} /W4)
    set_source_files_properties(${TEST_SRC_FILES} /W4)
else()
    set_source_files_properties(${SRC_FILES} -Wall -Werror -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wfloat-equal -Wold-style-cast)
    set_source_files_properties(${TEST_SRC_FILES} -Wall -Werror -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wfloat-equal -Wold-style-cast)
endif()

add_library(
    ${PROJECT_NAME}
    SHARED
    ${SRC_FILES}
)

# Export all symbols on Windows for shared library
set_target_properties(${PROJECT_NAME} PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# ============================================================================
# Project configuration - Includes, Libraries, defines
# ============================================================================
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(LIB_LIST "")
if (WIN32)
    list(APPEND LIB_LIST setupapi Cfgmgr32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -D_UNICODE -DUNICODE -D_WIN32)
elseif(LINUX)
    list(APPEND LIB_LIST udev)
elseif(APPLE)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    list(APPEND LIB_LIST ${IOKIT_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
endif ()
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_LIST})

# ============================================================================
# Build static library if enabled, we use this for python bindings
# ============================================================================
if (FW_BUILD_STATIC)
    add_library(
        ${PROJECT_NAME}-static
        STATIC
        ${SRC_FILES}
    )

    set_target_properties(${PROJECT_NAME}-static PROPERTIES
                            POSITION_INDEPENDENT_CODE ON
                            )

    target_include_directories(
    ${PROJECT_NAME}-static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(${PROJECT_NAME}-static PRIVATE ${LIB_LIST})
endif ()

# ============================================================================
# Build C API
# ============================================================================

if (FW_BUILD_C_API)
    add_subdirectory(c_api)
endif ()

# ============================================================================
# Examples
# ============================================================================
if (FW_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# Testing ====================================================================
# ============================================================================
if (FW_FINDER_BUILD_TESTS AND FW_BUILD_STATIC)
    message(STATUS "Enabling tests...")
    # Include GoogleTest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
    )
if (WIN32)
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    # I couldn't for the life of me get google test to link correctly on windows when BUILD_SHARED_LIBS is ON
    # gtest was placing the gtest.dll inside a subdirectory bin and the executable was getting error code 0xc0000135
    set(OLD_BUILD_SHARED_LIBS_VALUE BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
endif ()

    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(
        "${PROJECT_NAME}_test"
        ${SRC_FILES}
        ${TEST_SRC_FILES}
    )

    target_link_libraries(
        "${PROJECT_NAME}_test"
        GTest::gtest_main ${LIB_LIST} ${PROJECT_NAME}-static ${C_API_STATIC_PROJECT_NAME}
    )

    target_include_directories(
        "${PROJECT_NAME}_test"
        PUBLIC
        include/)

    include(GoogleTest)
    gtest_discover_tests("${PROJECT_NAME}_test")
    # Restore the value
    if (WIN32)
        set(BUILD_SHARED_LIBS OLD_BUILD_SHARED_LIBS_VALUE)
    endif ()
endif()
