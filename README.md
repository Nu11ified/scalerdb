# ScalerDB

A modern C++23 database project with efficient serialization and multi-threading capabilities.

## Features

- **C++23 Standard**: Leverages the latest C++ features and improvements
- **JSON Serialization**: Uses nlohmann/json for fast and reliable data serialization (msgpack can be added later)
- **Thread Pool**: Integrated BS::thread_pool for efficient concurrent operations
- **Testing**: GoogleTest framework for comprehensive unit testing
- **CMake Build System**: Modern CMake setup with automatic dependency management

## Dependencies

All dependencies are automatically fetched and managed by CMake:

- [GoogleTest](https://github.com/google/googletest) - Testing framework
- [nlohmann/json](https://github.com/nlohmann/json) - JSON serialization library
- [BS::thread_pool](https://github.com/bshoshany/thread-pool) - High-performance thread pool

## Quick Start

### Prerequisites

- CMake 3.20 or higher
- C++23 compatible compiler (GCC 11+, Clang 13+, or MSVC 2022+)
- Git

### Setup

1. **Initialize the environment:**
   ```bash
   ./setup.sh
   ```
   This script will:
   - Check for required tools (CMake, C++23 compiler)
   - Create the build directory
   - Initialize git repository with appropriate .gitignore

2. **Build the project:**
   ```bash
   ./make.sh
   ```

3. **Build and run tests:**
   ```bash
   ./make.sh test
   ```

4. **Build in debug mode:**
   ```bash
   ./make.sh debug
   ```

5. **Clean build:**
   ```bash
   ./make.sh clean
   ```

### Running

- **Main executable:**
  ```bash
  ./build/scalerdb
  ```

- **Run tests:**
  ```bash
  ./build/test_setup
  # or
  cd build && ctest
  ```

## Build Options

The `make.sh` script supports several options:

- `debug` - Build in Debug mode with debugging symbols
- `release` - Build in Release mode (default, optimized)
- `test` - Run tests after building
- `clean` - Clean the build directory before building
- `help` - Show help message

Examples:
```bash
./make.sh debug test    # Debug build + run tests
./make.sh clean release # Clean build + release mode
```

## Project Structure

```
scalerdb/
├── CMakeLists.txt      # CMake configuration
├── main.cpp            # Main application entry point
├── test_setup.cpp      # Test suite for verifying setup
├── setup.sh            # Environment setup script
├── make.sh             # Build script
├── README.md           # This file
├── .gitignore          # Git ignore rules
└── build/              # Build artifacts (auto-generated)
    ├── scalerdb        # Main executable
    └── test_setup      # Test executable
```

## Development

### Adding New Features

1. Add source files to the project
2. Update `CMakeLists.txt` if needed
3. Add corresponding tests in the test files
4. Build and test: `./make.sh test`

### Dependencies

To add new dependencies, modify the `FetchContent_Declare` sections in `CMakeLists.txt`. The build system will automatically download and configure them.

### Future Enhancements

- [ ] Add MessagePack support for more efficient serialization
- [ ] Implement database storage backend
- [ ] Add network interface
- [ ] Performance benchmarking suite
- [ ] Documentation generation

## Troubleshooting

### Build Issues

1. **CMake version too old:**
   - Ensure CMake 3.20+ is installed
   - On macOS: `brew install cmake`
   - On Ubuntu: `sudo apt install cmake`

2. **C++23 not supported:**
   - Update your compiler to a recent version
   - GCC 11+, Clang 13+, or MSVC 2022+

3. **Clean build if issues persist:**
   ```bash
   ./make.sh clean
   ```

### Testing Issues

If tests fail, run them individually:
```bash
./build/test_setup --gtest_verbose
```

## License

This project is open source. See the LICENSE file for details. 