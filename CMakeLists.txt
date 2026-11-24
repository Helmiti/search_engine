cmake_minimum_required(VERSION 3.16)
project(search_engine VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Threads REQUIRED)

add_library(search_core
    src/MiniJSON.cpp
    src/ConverterJSON.cpp
    src/InvertedIndex.cpp
    src/SearchServer.cpp
)
target_include_directories(search_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(search_core PUBLIC Threads::Threads)

add_executable(search_engine src/main.cpp)
target_link_libraries(search_engine PRIVATE search_core)

set_target_properties(search_engine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

include(CTest)
if(BUILD_TESTING)
    add_executable(search_tests tests/test_offline.cpp)
    target_link_libraries(search_tests PRIVATE search_core)
    add_test(NAME offline_tests COMMAND search_tests)
    set_tests_properties(offline_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif()

option(ENABLE_GOOGLE_TEST "Enable official Google Test suite from local source or installation" OFF)
set(GOOGLE_TEST_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest" CACHE PATH "Google Test source directory")
if(ENABLE_GOOGLE_TEST)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    if(EXISTS "${GOOGLE_TEST_SOURCE_DIR}/CMakeLists.txt")
        add_subdirectory("${GOOGLE_TEST_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/googletest_build" EXCLUDE_FROM_ALL)
        message(STATUS "Using local Google Test sources: ${GOOGLE_TEST_SOURCE_DIR}")
    else()
        find_package(GTest CONFIG QUIET)
        if(NOT GTest_FOUND)
            message(FATAL_ERROR "Google Test not found. Extract Google Test source into third_party/googletest, or set GOOGLE_TEST_SOURCE_DIR to its folder (containing CMakeLists.txt), or set ENABLE_GOOGLE_TEST=OFF. No network download is attempted.")
        endif()
    endif()
    add_executable(search_gtests tests/test_gtest.cpp)
    target_link_libraries(search_gtests PRIVATE search_core GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(search_gtests)
endif()
