include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
# Keep gtest's runtime library selection consistent with this project on MSVC;
# harmless on MinGW/GCC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(sci_tests
    TestCallbacks.c
    VariablesAndCommands.c
    gtest/SlaveTests.cpp
    gtest/MasterTestCallbacks.c
)

target_include_directories(sci_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sci_tests PRIVATE sci gtest_main)

include(GoogleTest)
gtest_discover_tests(sci_tests)
