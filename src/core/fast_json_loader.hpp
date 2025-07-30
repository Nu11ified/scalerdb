#pragma once

#include "database.hpp"
#include "table.hpp"
#include "msgpack_types.h"
#include <string>
#include <memory>

// Forward declaration for simdjson
namespace simdjson {
    namespace ondemand {
        class parser;
        class document;
        class object;
        class array;
        class value;
    }
}

namespace scalerdb {

/**
 * @brief High-performance JSON loader using simdjson
 * 
 * This class provides significantly faster JSON parsing compared to nlohmann/json
 * by using SIMD instructions. Benchmarks show 4-25x speed improvements.
 */
class FastJsonLoader {
private:
    std::unique_ptr<simdjson::ondemand::parser> parser_;
    
public:
    FastJsonLoader();
    ~FastJsonLoader();
    
    /**
     * @brief Load database from JSON file using simdjson
     * @param database Target database to load into
     * @param filename JSON file path
     * @return true if successful, false otherwise
     */
    bool loadDatabase(Database& database, const std::string& filename);
    
    /**
     * @brief Parse SerializableDatabase from JSON document
     * @param doc simdjson document
     * @return SerializableDatabase object
     */
    SerializableDatabase parseDatabase(simdjson::ondemand::document& doc);
    
private:
    SerializableTable parseTable(simdjson::ondemand::object& obj);
    SerializableRow parseRow(simdjson::ondemand::object& obj);
    Column parseColumn(simdjson::ondemand::object& obj);
    Value parseValue(simdjson::ondemand::value& val, ValueType expected_type);
    ValueType parseValueType(std::string_view type_str);
};

/**
 * @brief High-performance JSON saver with streaming output
 */
class FastJsonSaver {
public:
    /**
     * @brief Save database to JSON file with optimized serialization
     * @param database Source database
     * @param filename Output JSON file path
     * @return true if successful, false otherwise
     */
    static bool saveDatabase(const Database& database, const std::string& filename);
    
private:
    static void writeTable(std::ofstream& out, const Table& table, const std::string& name);
    static void writeRows(std::ofstream& out, const Table& table);
    static void writeValue(std::ofstream& out, const Value& value);
};

} // namespace scalerdb 