cmake_minimum_required(VERSION 3.20)
project(scalerdb VERSION 1.0.0 LANGUAGES CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags for better debugging and optimization
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# FetchContent for external dependencies
include(FetchContent)

# GoogleTest
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Use nlohmann/json for now (simpler than msgpack with no dependencies)
# We can switch to msgpack later if needed
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Thread pool library (using a simple header-only one)
FetchContent_Declare(
  thread-pool
  GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
  GIT_TAG master
)
FetchContent_MakeAvailable(thread-pool)

# Enable testing
enable_testing()

# Create main executable
add_executable(scalerdb main.cpp)

# Link libraries and include directories
target_link_libraries(scalerdb 
    PRIVATE 
    nlohmann_json::nlohmann_json
)

target_include_directories(scalerdb 
    PRIVATE 
    ${thread-pool_SOURCE_DIR}/include
)

# Add example test executable to verify everything works
add_executable(
    test_setup
    test_setup.cpp
)

target_link_libraries(
    test_setup
    GTest::gtest_main
    nlohmann_json::nlohmann_json
)

target_include_directories(test_setup 
    PRIVATE 
    ${thread-pool_SOURCE_DIR}/include
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(test_setup) 