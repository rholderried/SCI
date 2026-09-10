cmake_minimum_required(VERSION 3.14)
project(sci C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build the sci_tests GoogleTest suite" ON)

# SCIconfig.h is intentionally NOT bundled as a fixed part of this library.
# Every file under Common/, Master/, and Slave/ does `#include "SCIconfig.h"`
# unqualified, and expects the integrating project to supply that header on
# the include path (buffer sizes, var-struct size, EEPROM layout, etc. are
# all target/application specific). See config/SCIconfig_Template.h for a
# documented starting point to copy into your own project.
#
# SCI_CONFIG_DIR must point at a directory containing your SCIconfig.h. When
# BUILD_TESTS is ON and SCI_CONFIG_DIR is not set, it defaults to the
# GoogleTest suite's own config (Test/config), so `cmake --build` works out
# of the box for running this repo's own tests.
set(SCI_CONFIG_DIR "" CACHE PATH "Directory containing your project's SCIconfig.h")

if(NOT SCI_CONFIG_DIR)
    if(BUILD_TESTS)
        set(SCI_CONFIG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Test/config")
    else()
        message(FATAL_ERROR
            "SCI_CONFIG_DIR is not set. The sci library requires an "
            "application-supplied SCIconfig.h (see config/SCIconfig_Template.h). "
            "Configure with -DSCI_CONFIG_DIR=<dir containing your SCIconfig.h>, "
            "or enable -DBUILD_TESTS=ON to use the bundled test config.")
    endif()
endif()

add_library(sci STATIC
    Common/Src/Buffer.c
    Common/Src/Helpers.c
    Common/Src/SCIDatalink.c
    Master/Src/SCIMaster.c
    Master/Src/SCIMasterDataframe.c
    Master/Src/SCIMasterTransfer.c
    Slave/Src/SCISlave.c
    Slave/Src/SCISlaveDataframe.c
    Slave/Src/SCISlaveTransfer.c
    Slave/Src/VarAccess.c
)

target_include_directories(sci PUBLIC
    Common/Inc
    Master/Inc
    Slave/Inc
    ${SCI_CONFIG_DIR}
)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Test)
endif()
