cmake_minimum_required(VERSION 3.14)
project(sci C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
    config
)

option(BUILD_TESTS "Build the sci_tests GoogleTest suite" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(Test)
endif()
