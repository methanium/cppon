cmake_minimum_required(VERSION 3.16)

# Option : utiliser un GTest système si disponible
option(CPPON_TESTS_USE_SYSTEM_GTEST "Use system-installed GoogleTest" OFF)

if(CPPON_TESTS_USE_SYSTEM_GTEST)
    find_package(GTest REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

enable_testing()

file(GLOB CPPON_TEST_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

if(NOT CPPON_TEST_SOURCES)
    message(WARNING "[cppon] No test sources found in tests/")
endif()

add_executable(cppon_tests ${CPPON_TEST_SOURCES})
target_link_libraries(cppon_tests PRIVATE
    cppon::cppon
    GTest::gtest
    GTest::gtest_main)

target_compile_features(cppon_tests PRIVATE cxx_std_17)

if (MSVC)
    target_compile_options(cppon_tests PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(cppon_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(GoogleTest)
gtest_discover_tests(cppon_tests)