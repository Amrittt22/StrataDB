cmake_minimum_required(VERSION 3.16)

project(StrataDB VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

# Main executable
add_executable(stratadb
    engine/src/main.cpp
    engine/src/SkipList.cpp
    engine/src/MemTable.cpp
    engine/src/Stats.cpp
    engine/src/Logger.cpp
)
target_include_directories(stratadb PRIVATE
    engine/include
)

# Unit tests
enable_testing()


add_executable(stratadb_tests
    engine/tests/test_main.cpp
    engine/tests/skiplist_test.cpp
    engine/tests/memtable_test.cpp
    engine/tests/stats_test.cpp
    engine/tests/logger_test.cpp
    engine/src/SkipList.cpp
    engine/src/MemTable.cpp
    engine/src/Stats.cpp
    engine/src/Logger.cpp
)

target_include_directories(stratadb_tests PRIVATE
    engine/include
)

target_link_libraries(stratadb_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(stratadb_tests)

if(MSVC)
    target_compile_options(stratadb PRIVATE /W4)
else()
    target_compile_options(stratadb PRIVATE -Wall -Wextra -Wpedantic)
endif()


