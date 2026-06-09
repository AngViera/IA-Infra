cmake_minimum_required(VERSION 3.16)
project(mancala_motor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── OpenMP ────────────────────────────────────────────────────────────────────
find_package(OpenMP REQUIRED)

# ── OpenSSL ───────────────────────────────────────────────────────────────────
find_package(OpenSSL REQUIRED)

# ── Optimisation flags ────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# ── FetchContent: cpp-httplib ─────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)

FetchContent_MakeAvailable(httplib)

# ── FetchContent: nlohmann/json ───────────────────────────────────────────────
FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR ${CMAKE_BINARY_DIR}/nlohmann
)

FetchContent_Populate(nlohmann_json)

# ── FetchContent: doctest ─────────────────────────────────────────────────────
FetchContent_Declare(
    doctest
    URL https://github.com/doctest/doctest/releases/download/v2.4.11/doctest.h
    DOWNLOAD_NO_EXTRACT TRUE
    SOURCE_DIR ${CMAKE_BINARY_DIR}/doctest
)

FetchContent_Populate(doctest)

# ── Headers available to targets ──────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${httplib_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/nlohmann
    ${CMAKE_BINARY_DIR}/doctest
)

# ── Server binary ─────────────────────────────────────────────────────────────
add_executable(mancala_server src/server.cpp)

target_link_libraries(
    mancala_server
    PRIVATE
    OpenMP::OpenMP_CXX
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_compile_options(
    mancala_server
    PRIVATE
    -Wall
    -Wextra
)

# ── Benchmark binary ──────────────────────────────────────────────────────────
add_executable(mancala_bench bench/bench.cpp)

target_link_libraries(
    mancala_bench
    PRIVATE
    OpenMP::OpenMP_CXX
)

target_compile_options(
    mancala_bench
    PRIVATE
    -Wall
    -Wextra
)

# ── Test binary ───────────────────────────────────────────────────────────────
add_executable(test_kalah tests/test_kalah.cpp)

target_link_libraries(
    test_kalah
    PRIVATE
    OpenMP::OpenMP_CXX
)

target_compile_options(
    test_kalah
    PRIVATE
    -Wall
    -Wextra
)

# ── CTest integration ─────────────────────────────────────────────────────────
enable_testing()

add_test(
    NAME kalah_unit_tests
    COMMAND test_kalah
)