cmake_minimum_required(VERSION 3.24)

project(scalerdb VERSION 0.1.0 LANGUAGES CXX)

# Enable modern C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# -------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------

# msgpack-c (C++ interface)
FetchContent_Declare(
    msgpack
    GIT_REPOSITORY https://github.com/msgpack/msgpack-c.git
    GIT_TAG cpp-4.0      # Latest stable C++ branch/tag
)

# BS_thread_pool ‑ lightweight header-only thread pool
FetchContent_Declare(
    bs_thread_pool
    GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
    GIT_TAG v4.1.0
)

# GoogleTest (needed only for tests)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.14.0
)

# Make the above repos available
FetchContent_MakeAvailable(msgpack bs_thread_pool googletest)

# -------------------------------------------------------------------
# Main target
# -------------------------------------------------------------------
add_executable(scalerdb src/main.cpp)

# Link dependencies (msgpackc-cxx provided by msgpack build; BS::thread_pool by thread-pool repo)
# Both are header-only, but keep linkage explicit for include dirs / transitive compile options.
target_link_libraries(scalerdb PRIVATE msgpackc-cxx BS::thread_pool)

target_include_directories(scalerdb PRIVATE ${msgpack_SOURCE_DIR}/include)

# -------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------
include(CTest)
enable_testing()

add_subdirectory(tests) 