cmake_minimum_required(VERSION 3.20)
project(fwfinder)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(FW_FINDER_BUILD_TESTS "Build tests" ON)
option(FW_FINDER_ENABLE_BINDINGS_PYTHON "Build Python bindings" OFF)



# Library files
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

# Compiler and linker options
if(MSVC)
    set_source_files_properties(${SRC_FILES} /W4)
    set_source_files_properties(${TEST_SRC_FILES} /W4)
else()
    set_source_files_properties(${SRC_FILES} -Wall -Werror -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wfloat-equal -Wold-style-cast)
    set_source_files_properties(${TEST_SRC_FILES} -Wall -Werror -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wfloat-equal -Wold-style-cast)
endif()

add_library(
    ${CMAKE_PROJECT_NAME}
    ${SRC_FILES}
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
                        POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
                        )

add_subdirectory(bindings)

target_include_directories(
    ${CMAKE_PROJECT_NAME}
    PUBLIC
    include/)

set(LIB_LIST "")
if (WIN32)
    list(APPEND LIB_LIST setupapi Cfgmgr32)
    add_definitions(-DWIN32_LEAN_AND_MEAN -D_UNICODE -DUNICODE)
elseif(LINUX)
    list(APPEND LIB_LIST udev)
elseif(APPLE)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    list(APPEND LIB_LIST ${IOKIT_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK})
endif ()
target_link_libraries(${CMAKE_PROJECT_NAME} PUBLIC ${LIB_LIST})

if (FW_FINDER_BUILD_TESTS)
    message(STATUS "Enabling tests...")
    # Include GoogleTest
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(
        "${CMAKE_PROJECT_NAME}_test"
        ${SRC_FILES}
        ${TEST_SRC_FILES}
    )

    target_link_libraries(
        "${CMAKE_PROJECT_NAME}_test"
        GTest::gtest_main ${LIB_LIST}
    )

    # if (WIN32)
    #     target_link_libraries(${CMAKE_PROJECT_NAME}_test PRIVATE setupapi)
    # endif ()

    target_include_directories(
        "${CMAKE_PROJECT_NAME}_test"
        PUBLIC
        include/)

    include(GoogleTest)
    gtest_discover_tests("${CMAKE_PROJECT_NAME}_test")
endif()
