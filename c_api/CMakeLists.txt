# ============================================================================
# C API Library CMakeLists.txt
# ============================================================================

set(C_API_PROJECT_NAME c${PROJECT_NAME})
set(C_API_STATIC_PROJECT_NAME c${PROJECT_NAME}-static)

# Shared C API library
add_library(
    ${C_API_PROJECT_NAME}
    SHARED
    src/cfwfinder.cpp
)

target_include_directories(
    ${C_API_PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link against the main fwfinder library
target_link_libraries(${C_API_PROJECT_NAME} PRIVATE ${PROJECT_NAME}-static)

set_target_properties(${C_API_PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${C_API_PROJECT_NAME}
)

target_compile_definitions(${C_API_PROJECT_NAME} PRIVATE CFW_FINDER_BUILD_DYNAMIC)
target_compile_definitions(${C_API_PROJECT_NAME}
    PRIVATE
        CFW_FINDER_EXPORTS
)

# Static C API library (if enabled)
if (FW_BUILD_STATIC)
    add_library(
        ${C_API_STATIC_PROJECT_NAME}
        STATIC
        src/cfwfinder.cpp
    )

    set_target_properties(${C_API_STATIC_PROJECT_NAME} PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        OUTPUT_NAME ${C_API_STATIC_PROJECT_NAME}
    )

    target_include_directories(
        ${C_API_STATIC_PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
            $<INSTALL_INTERFACE:>
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    # Link against the main fwfinder static library
    target_link_libraries(${C_API_STATIC_PROJECT_NAME} PRIVATE ${PROJECT_NAME}-static)
    target_compile_definitions(${C_API_STATIC_PROJECT_NAME} PRIVATE CFW_FINDER_BUILD_STATIC)
endif ()

# ============================================================================
# C API Tests
# ============================================================================

if (FW_FINDER_BUILD_TESTS AND FW_BUILD_STATIC)
    # Ensure testing is enabled
    enable_testing()

    # C API test executable
    add_executable(
        "${C_API_PROJECT_NAME}_test"
        test/test_cfwfinder.cpp
    )

    target_include_directories(
        "${C_API_PROJECT_NAME}_test"
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
    )

    target_link_libraries(
        "${C_API_PROJECT_NAME}_test"
        GTest::gtest_main
        ${C_API_STATIC_PROJECT_NAME}
    )

    # Define that we're using static linking for the test
    target_compile_definitions(
        "${C_API_PROJECT_NAME}_test"
        PRIVATE
            CFW_FINDER_BUILD_STATIC
    )

    # Register the test with CTest using add_test
    add_test(
        NAME "cfwfinder_test_all"
        COMMAND "${C_API_PROJECT_NAME}_test"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endif ()
