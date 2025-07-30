#pragma once

// Using nlohmann/json with msgpack-compatible structure for now
// This can be easily migrated to actual msgpack once dependency issues are resolved
#include <nlohmann/json.hpp>
#include "value.hpp"
#include "column.hpp"
#include "row.hpp"
#include "table.hpp"
#include "database.hpp"
#include <vector>
#include <string>
#include <unordered_map>

namespace scalerdb {

/**
 * @brief Serializable representation of a Value for msgpack
 * 
 * Since std::variant is not directly supported by msgpack, we use a struct
 * with explicit type and data fields for serialization.
 */
struct SerializableValue {
    int type_index;
    std::string string_data;
    double numeric_data;
    bool bool_data;
    
    SerializableValue() = default;
    
    // Convert from Value to SerializableValue
    explicit SerializableValue(const Value& value) {
        type_index = static_cast<int>(value.getType());
        
        switch (value.getType()) {
            case ValueType::Null:
                break;
            case ValueType::Boolean:
                bool_data = value.get<bool>();
                break;
            case ValueType::Integer32:
                numeric_data = static_cast<double>(value.get<int32_t>());
                break;
            case ValueType::Integer64:
                numeric_data = static_cast<double>(value.get<int64_t>());
                break;
            case ValueType::Double:
                numeric_data = value.get<double>();
                break;
            case ValueType::String:
                string_data = value.get<std::string>();
                break;
        }
    }
    
    // Convert back to Value
    Value toValue() const {
        switch (static_cast<ValueType>(type_index)) {
            case ValueType::Null:
                return Value();
            case ValueType::Boolean:
                return Value(bool_data);
            case ValueType::Integer32:
                return Value(static_cast<int32_t>(numeric_data));
            case ValueType::Integer64:
                return Value(static_cast<int64_t>(numeric_data));
            case ValueType::Double:
                return Value(numeric_data);
            case ValueType::String:
                return Value(string_data);
            default:
                throw std::runtime_error("Invalid value type during deserialization");
        }
    }
    
    // JSON serialization (msgpack-compatible structure)
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializableValue, type_index, string_data, numeric_data, bool_data);
};

/**
 * @brief Serializable representation of a Column
 * 
 * Note: ConstraintValidator functions cannot be serialized, so they will be
 * lost during serialization. Only basic column metadata is preserved.
 */
struct SerializableColumn {
    std::string name;
    int type_index;
    bool nullable;
    bool unique;
    SerializableValue default_value;
    bool has_default;
    
    SerializableColumn() = default;
    
    // Convert from Column to SerializableColumn
    explicit SerializableColumn(const Column& column) 
        : name(column.getName())
        , type_index(static_cast<int>(column.getType()))
        , nullable(column.isNullable())
        , unique(column.isUnique())
        , has_default(column.getDefaultValue().has_value()) {
        
        if (has_default) {
            default_value = SerializableValue(column.getDefaultValue().value());
        }
        // Note: Constraints are not serialized as std::function cannot be serialized
    }
    
    // Convert back to Column
    Column toColumn() const {
        std::optional<Value> default_val;
        if (has_default) {
            default_val = default_value.toValue();
        }
        
        return Column(name, static_cast<ValueType>(type_index), nullable, unique, default_val);
        // Note: Constraints will need to be re-added after deserialization if needed
    }
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializableColumn, name, type_index, nullable, unique, default_value, has_default);
};

/**
 * @brief Serializable representation of a Row
 */
struct SerializableRow {
    std::vector<SerializableValue> values;
    
    SerializableRow() = default;
    
    // Convert from Row to SerializableRow
    explicit SerializableRow(const Row& row) {
        values.reserve(row.size());
        for (size_t i = 0; i < row.size(); ++i) {
            values.emplace_back(row.getValue(i));
        }
    }
    
    // Convert back to Row (requires column schema)
    Row toRow(const std::vector<Column>& columns) const {
        Row row(&columns);
        for (size_t i = 0; i < values.size() && i < columns.size(); ++i) {
            row.setValue(i, values[i].toValue());
        }
        return row;
    }
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializableRow, values);
};

/**
 * @brief Serializable representation of a Table
 */
struct SerializableTable {
    std::string name;
    std::vector<SerializableColumn> columns;
    std::vector<SerializableRow> rows;
    std::string primary_key_column;
    
    SerializableTable() = default;
    
    // Convert from Table to SerializableTable
    explicit SerializableTable(const Table& table) 
        : name(table.getName())
        , primary_key_column(table.getPrimaryKeyColumnName()) {
        
        // Serialize columns
        const auto& table_columns = table.getSchema();
        columns.reserve(table_columns.size());
        for (const auto& column : table_columns) {
            columns.emplace_back(column);
        }
        
        // Serialize rows
        size_t row_count = table.getRowCount();
        rows.reserve(row_count);
        for (size_t i = 0; i < row_count; ++i) {
            rows.emplace_back(table.getRow(i));
        }
    }
    
    // Convert back to Table
    Table toTable() const {
        // Reconstruct column schema
        std::vector<Column> table_columns;
        table_columns.reserve(columns.size());
        for (const auto& col : columns) {
            table_columns.push_back(col.toColumn());
        }
        
        // Create table with schema
        Table table(name, std::move(table_columns), primary_key_column);
        
        // Add all rows
        for (const auto& row : rows) {
            Row reconstructed_row = row.toRow(table.getSchema());
            table.insertRow(std::move(reconstructed_row));
        }
        
        return table;
    }
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializableTable, name, columns, rows, primary_key_column);
};

/**
 * @brief Serializable representation of a Database
 */
struct SerializableDatabase {
    std::vector<SerializableTable> tables;
    
    SerializableDatabase() = default;
    
    // Convert from Database to SerializableDatabase
    explicit SerializableDatabase(const Database& database) {
        const auto& table_names = database.getTableNames();
        tables.reserve(table_names.size());
        
        for (const auto& table_name : table_names) {
            const Table* table = database.getTable(table_name);
            if (table) {
                tables.emplace_back(*table);
            }
        }
    }
    
    // Convert back to Database  
    Database toDatabase() const {
        Database database;
        
        for (const auto& serializable_table : tables) {
            Table table = serializable_table.toTable();
            // Use the Table move constructor by moving into database
            std::string table_name = table.getName();
            std::vector<Column> schema = table.getSchema();
            std::string pk_column = table.getPrimaryKeyColumnName();
            
            // Create new table in database
            auto* new_table = database.createTable(table_name, std::move(schema), pk_column);
            
            // Copy rows from the deserialized table
            size_t row_count = table.getRowCount();
            for (size_t i = 0; i < row_count; ++i) {
                Row row = table.getRow(i);
                new_table->insertRow(std::move(row));
            }
        }
        
        return database;
    }
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SerializableDatabase, tables);
};

} // namespace scalerdb