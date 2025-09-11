option(CPPON_BUILD_EXAMPLES "Build C++ON examples" ON)
option(CPPON_BUILD_TESTS "Build C++ON tests" ON)

if(CPPON_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

if(CPPON_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()