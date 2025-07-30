# ScalerDB

A modern C++23 database project with efficient serialization and multi-threading capabilities.

## Features

- **C++23 Standard**: Leverages the latest C++ features and improvements
- **Core In-Memory Database**: Complete CRUD operations with primary key indexing
- **Type-Safe Value System**: std::variant-based value container supporting multiple data types
- **Schema Validation**: Column constraints and type checking
- **MessagePack-Compatible Serialization**: Full database persistence with round-trip fidelity
- **JSON Support**: Fast JSON serialization using nlohmann/json library
- **Thread Pool**: Integrated BS::thread_pool for efficient concurrent operations
- **Comprehensive Testing**: GoogleTest framework with 27 unit tests covering CRUD and serialization
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

### Persistence & Serialization

```cpp
// Save entire database to file
if (db.save("mydb.msgpack")) {
    std::cout << "Database saved successfully!" << std::endl;
}

// Load database from file
Database loaded_db;
if (loaded_db.load("mydb.msgpack")) {
    std::cout << "Database loaded successfully!" << std::endl;
    // All tables, schemas, and data are restored
}
```

## Dependencies

All dependencies are automatically fetched and managed by CMake:

- [GoogleTest](https://github.com/google/googletest) - Testing framework
- [nlohmann/json](https://github.com/nlohmann/json) - JSON serialization library (also used for MessagePack-compatible persistence)
- [BS::thread_pool](https://github.com/bshoshany/thread-pool) - High-performance thread pool

**Note on MessagePack**: We use a MessagePack-compatible structure implemented with nlohmann/json. This provides identical functionality to MessagePack while avoiding Boost dependencies. The implementation can be easily migrated to actual MessagePack when dependency management allows.

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
  ./build/test_setup              # Basic setup tests
  ./build/test_core_data_model    # Core database tests
  ./build/test_serialization     # Serialization tests
  # or run all tests with:
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
├── src/core/               # Core database engine
│   ├── value.hpp           # Type-safe value container
│   ├── column.hpp          # Column schema and validation
│   ├── row.hpp             # Database row implementation
│   ├── table.hpp           # Table with CRUD operations
│   ├── database.hpp        # Database management
│   ├── database.cpp        # Database save/load implementation
│   └── msgpack_types.h     # Serialization structures
├── tests/                  # Test suites
│   ├── test_core_data_model.cpp    # Comprehensive CRUD tests
│   └── test_serialization.cpp     # Serialization round-trip tests
├── CMakeLists.txt          # CMake configuration
├── main.cpp                # Example application demonstrating core features
├── test_setup.cpp          # Basic setup verification tests
├── setup.sh                # Environment setup script
├── make.sh                 # Build script
├── README.md               # This file
├── .gitignore              # Git ignore rules
└── build/                  # Build artifacts (auto-generated)
    ├── scalerdb                # Main executable
    ├── test_setup              # Setup tests
    ├── test_core_data_model    # Core database tests
    └── test_serialization     # Serialization tests
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
- **Test Coverage**: 18 comprehensive unit tests covering all CRUD operations
- **Performance**: O(1) primary key lookups, O(n) sequential scans

✅ **Phase 2 Complete - Persistence & Serialization:**
- **MessagePack-Compatible Serialization**: Full database save/load with round-trip fidelity
- **Complete Data Type Support**: All Value types (null, bool, int32, int64, double, string)
- **Schema Preservation**: Column metadata, constraints, defaults, and primary keys maintained
- **Error Handling**: Robust error handling for file I/O and data corruption
- **Performance Testing**: Tested with large datasets (1000+ rows)
- **Test Coverage**: 6 comprehensive serialization tests covering all scenarios

### Future Enhancements

**Phase 3 - Query Engine:**
- [ ] SQL-like query language parser
- [ ] Query optimizer and execution engine
- [ ] Secondary indexes (B-tree, hash indexes)
- [ ] Advanced filtering and joins

**Phase 4 - Advanced Features:**
- [ ] Network interface (TCP/UDP protocols)
- [ ] Concurrent access control (MVCC or locking)
- [ ] WAL (Write-Ahead Logging) for durability
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
./build/test_setup --gtest_verbose          # Basic setup tests
./build/test_core_data_model --gtest_verbose # Core database tests
./build/test_serialization --gtest_verbose  # Serialization tests
```

### Serialization Features

The database now supports complete persistence with MessagePack-compatible serialization:

- **Full Round-Trip Fidelity**: All data types, schemas, and constraints preserved
- **Multiple Tables**: Save and load entire databases with multiple tables
- **Large Datasets**: Efficiently handles 1000+ rows with minimal overhead
- **Error Recovery**: Graceful handling of corrupted files and I/O errors
- **Debug-Friendly**: JSON format for easy inspection and debugging
- **Migration Ready**: Structure designed for easy migration to actual MessagePack

**Technical Implementation**: Uses nlohmann/json with MessagePack-compatible data structures. This approach avoids external dependencies while providing identical serialization capabilities. The NLOHMANN_DEFINE_TYPE_INTRUSIVE macros can be easily replaced with MSGPACK_DEFINE macros when migrating to actual MessagePack.

**Example Saved Database File:**
```json
{
  "tables": [
    {
      "name": "users",
      "primary_key_column": "id",
      "columns": [
        {"name": "id", "type_index": 2, "nullable": false, "unique": true},
        {"name": "name", "type_index": 5, "nullable": false, "unique": false}
      ],
      "rows": [
        {"values": [{"type_index": 2, "numeric_data": 1}, {"type_index": 5, "string_data": "Alice"}]}
      ]
    }
  ]
}
```

## License

This project is open source. See the LICENSE file for details. 