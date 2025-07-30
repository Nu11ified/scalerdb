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

# Note: msgpack-c requires Boost, using nlohmann/json with msgpack-compatible structure
# This implementation can be easily migrated to actual msgpack once dependencies are resolved

# nlohmann/json for JSON support alongside msgpack
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# Threading support for our custom ThreadPool implementation
find_package(Threads REQUIRED)

# Enable testing
enable_testing()

# Create main executable
add_executable(scalerdb 
    main.cpp
    src/core/database.cpp
)

# Link libraries and include directories
target_link_libraries(scalerdb 
    PRIVATE 
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_include_directories(scalerdb 
    PRIVATE 
    src
)

# Add core data model tests
add_executable(
    test_core_data_model
    tests/test_core_data_model.cpp
    src/core/database.cpp
)

target_link_libraries(
    test_core_data_model
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_include_directories(test_core_data_model 
    PRIVATE 
    src
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
    Threads::Threads
)

target_include_directories(test_setup 
    PRIVATE 
    src
)

# Add serialization tests
add_executable(
    test_serialization
    tests/test_serialization.cpp
    src/core/database.cpp
)

target_link_libraries(
    test_serialization
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_include_directories(test_serialization 
    PRIVATE 
    src
)

# Add threading tests
add_executable(
    test_threading
    tests/test_threading.cpp
    src/core/database.cpp
)

target_link_libraries(
    test_threading
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_include_directories(test_threading 
    PRIVATE 
    src
)

# Add persistence demo
add_executable(
    demo_persistence
    demo_persistence.cpp
    src/core/database.cpp
)

target_link_libraries(
    demo_persistence
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_include_directories(demo_persistence 
    PRIVATE 
    src
)

# Add performance tests
add_executable(
    test_performance
    tests/test_performance.cpp
    src/core/database.cpp
)

target_link_libraries(
    test_performance
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    Threads::Threads
)

target_include_directories(test_performance 
    PRIVATE 
    src
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(test_core_data_model)
gtest_discover_tests(test_setup)
gtest_discover_tests(test_serialization)
gtest_discover_tests(test_threading)
gtest_discover_tests(test_performance) 