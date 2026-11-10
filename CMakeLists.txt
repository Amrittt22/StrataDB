
cmake_minimum_required(VERSION 3.16)

project(StrataDB VERSION 1.0.0 LANGUAGES CXX)
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.3
)

FetchContent_MakeAvailable(httplib)

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

# -----------------------------------------
# StrataDB Engine Library
# -----------------------------------------

add_library(stratadb_engine
    engine/src/SkipList.cpp
    engine/src/MemTable.cpp
    engine/src/Stats.cpp
    engine/src/Logger.cpp
    engine/src/WAL.cpp
    engine/src/BloomFilter.cpp
    engine/src/SSTable.cpp
    engine/src/SSTableManager.cpp
    engine/src/Database.cpp
    engine/src/Manifest.cpp
)

target_include_directories(stratadb_engine
    PUBLIC
        engine/include
)

if(MSVC)
    target_compile_options(stratadb_engine PRIVATE /W4)
else()
    target_compile_options(stratadb_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

# -----------------------------------------
# Main Executable
# -----------------------------------------

add_executable(stratadb
    engine/src/main.cpp
)

find_package(Threads REQUIRED)

add_executable(stratadb_server
    engine/src/server.cpp
)



target_link_libraries(stratadb_server
    PRIVATE
    stratadb_engine
    httplib::httplib
    Threads::Threads
)

target_link_libraries(stratadb
    PRIVATE stratadb_engine
)

# -----------------------------------------
# Unit Tests
# -----------------------------------------

enable_testing()

add_executable(stratadb_tests
    engine/tests/test_main.cpp
    engine/tests/skiplist_test.cpp
    engine/tests/memtable_test.cpp
    engine/tests/stats_test.cpp
    engine/tests/logger_test.cpp
    engine/tests/wal_test.cpp
    engine/tests/bloom_filter_test.cpp
    engine/tests/sstable_test.cpp
    engine/tests/sstable_manager_test.cpp
    engine/tests/database_test.cpp
)

target_link_libraries(stratadb_tests
    PRIVATE
        stratadb_engine
        GTest::gtest_main
)

# -----------------------------------------
# Manual Automatic Flush Test
# -----------------------------------------

add_executable(auto_flush_manual_test
    engine/tests/auto_flush_manual_test.cpp
)

target_link_libraries(auto_flush_manual_test
    PRIVATE stratadb_engine
)

# -----------------------------------------
# Automatic Flush Recovery Test
# -----------------------------------------

add_executable(auto_flush_recovery_test
    engine/tests/auto_flush_recovery_test.cpp
)

target_link_libraries(auto_flush_recovery_test
    PRIVATE stratadb_engine
)

include(GoogleTest)
gtest_discover_tests(stratadb_tests)
