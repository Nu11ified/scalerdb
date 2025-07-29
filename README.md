# ScalerDB

A modern C++23 database project with efficient serialization and multi-threading capabilities.

## Features

- **C++23 Standard**: Leverages the latest C++ features and improvements
- **Core In-Memory Database**: Complete CRUD operations with primary key indexing
- **Type-Safe Value System**: std::variant-based value container supporting multiple data types
- **Schema Validation**: Column constraints and type checking
- **JSON Serialization**: Uses nlohmann/json for fast and reliable data serialization
- **Thread Pool**: Integrated BS::thread_pool for efficient concurrent operations
- **Comprehensive Testing**: GoogleTest framework with 21 unit tests covering all CRUD operations
- **CMake Build System**: Modern CMake setup with automatic dependency management

## Core Data Model

### Architecture Overview

ScalerDB implements a layered architecture:

1. **Value**: Type-safe container using `std::variant` for database values
2. **Column**: Schema definition with metadata and validation constraints  
3. **Row**: Collection of values with efficient access by index and name
4. **Table**: In-memory storage with CRUD operations and primary key indexing
5. **Database**: Top-level container managing multiple tables

### Key Classes

- `scalerdb::Value` - Supports null, bool, int32, int64, double, and string types
- `scalerdb::Column` - Column metadata with validation constraints
- `scalerdb::Row` - Database row with schema-aware value access
- `scalerdb::Table` - Table with primary key indexing and CRUD operations
- `scalerdb::Database` - Database container with table management

### CRUD Operations

```cpp
// Create database and table
Database db("my_db");
auto* table = db.createTable("users", schema, "id");

// INSERT
table->insertRow({Value(1), Value("Alice"), Value(28)});

// SELECT
const Row* row = table->findRowByPK(Value(1));

// UPDATE  
table->updateRow(Value(1), {Value(1), Value("Alice Smith"), Value(29)});

// DELETE
table->deleteRow(Value(1));
```

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
├── src/core/           # Core database engine
│   ├── value.hpp       # Type-safe value container
│   ├── column.hpp      # Column schema and validation
│   ├── row.hpp         # Database row implementation
│   ├── table.hpp       # Table with CRUD operations
│   └── database.hpp    # Database management
├── tests/              # Test suites
│   └── test_core_data_model.cpp # Comprehensive CRUD tests
├── CMakeLists.txt      # CMake configuration
├── main.cpp            # Example application demonstrating core features
├── test_setup.cpp      # Basic setup verification tests
├── setup.sh            # Environment setup script
├── make.sh             # Build script
├── README.md           # This file
├── .gitignore          # Git ignore rules
└── build/              # Build artifacts (auto-generated)
    ├── scalerdb            # Main executable
    ├── test_setup          # Setup tests
    └── test_core_data_model # Core database tests
```

## Development

### Adding New Features

1. Add source files to the project
2. Update `CMakeLists.txt` if needed
3. Add corresponding tests in the test files
4. Build and test: `./make.sh test`

### Dependencies

To add new dependencies, modify the `FetchContent_Declare` sections in `CMakeLists.txt`. The build system will automatically download and configure them.

## Development Status

✅ **Phase 1 Complete - Core In-Memory Data Model:**
- **Value System**: Type-safe container using `std::variant` supporting null, bool, int32, int64, double, string
- **Schema Definition**: Column class with metadata, constraints, and validation
- **Row Management**: Efficient data access by index and column name
- **Table Operations**: Complete CRUD with primary key indexing and unique constraints
- **Database Management**: Multi-table container with statistics and querying
- **Test Coverage**: 21 comprehensive unit tests covering all CRUD operations
- **Performance**: O(1) primary key lookups, O(n) sequential scans

### Future Enhancements

**Phase 2 - Persistence & Serialization:**
- [ ] Add MessagePack support for efficient binary serialization
- [ ] Implement WAL (Write-Ahead Logging) for durability
- [ ] File-based storage backend with crash recovery

**Phase 3 - Query Engine:**
- [ ] SQL-like query language parser
- [ ] Query optimizer and execution engine
- [ ] Secondary indexes (B-tree, hash indexes)

**Phase 4 - Advanced Features:**
- [ ] Network interface (TCP/UDP protocols)
- [ ] Concurrent access control (MVCC or locking)
- [ ] Performance benchmarking suite
- [ ] Clustering and replication

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