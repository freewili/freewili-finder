cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "Building pyfwfinder python bindings...")

option(PYFWFINDER_STUB_GEN_ENABLE "Enable python stub generation" ON)

# find_package(Python COMPONENTS Interpreter Development)
find_package(Python
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule)

include(FetchContent)
FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind.git
    GIT_TAG v2.6.1
)
FetchContent_MakeAvailable(nanobind)

nanobind_add_module(
    pyfwfinder
    NB_STATIC STABLE_ABI LTO
    src/pyfwfinder.cpp
)

target_link_libraries(pyfwfinder PRIVATE fwfinder-static)

# Add the following lines below. They configure CMake to perform an optimized release build by default
# unless another build type is specified. Without this addition, binding code may run slowly and
# produce large binaries.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

install(TARGETS pyfwfinder DESTINATION pyfwfinder)

# ============================================================================
# Stub Generation
# ============================================================================
if (PYFWFINDER_STUB_GEN_ENABLE)
    message(STATUS "Building pyfwfinder python stubs...")

    set(PYFWFINDER_STUB_FILE "${CMAKE_CURRENT_BINARY_DIR}/pyfwfinder.pyi")

    nanobind_add_stub(
        pyfwfinder_stub
        MODULE pyfwfinder
        OUTPUT "${PYFWFINDER_STUB_FILE}"
        PYTHON_PATH $<TARGET_FILE_DIR:pyfwfinder>
        DEPENDS pyfwfinder
        INSTALL_TIME
        MARKER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/src/pyfwfinder/py.typed"
    )


    install(FILES src/pyfwfinder/__init__.py DESTINATION pyfwfinder)
    install(FILES src/pyfwfinder/py.typed "${PYFWFINDER_STUB_FILE}" DESTINATION pyfwfinder)
endif ()
