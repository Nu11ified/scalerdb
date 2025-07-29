#pragma once

#include "table.hpp"
#include "column.hpp"
#include "value.hpp"
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <stdexcept>

namespace scalerdb {

/**
 * @brief Represents a database containing multiple tables
 * 
 * This class provides the top-level interface for database operations,
 * managing multiple tables and providing table creation/management functionality.
 */
class Database {
private:
    std::string name_;
    std::unordered_map<std::string, std::unique_ptr<Table>> tables_;

public:
    /**
     * @brief Construct a new Database
     * @param name Database name
     */
    explicit Database(std::string name) : name_(std::move(name)) {}

    // Disable copy constructor and assignment (due to unique_ptr)
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Enable move constructor and assignment
    Database(Database&&) = default;
    Database& operator=(Database&&) = default;

    /**
     * @brief Get the database name
     * @return Database name
     */
    const std::string& getName() const {
        return name_;
    }

    /**
     * @brief Create a new table in the database
     * @param table_name Name of the table to create
     * @param schema Vector of columns defining the table structure
     * @param primary_key_column_name Name of the primary key column
     * @return Pointer to the created table
     */
    Table* createTable(const std::string& table_name, 
                      std::vector<Column> schema, 
                      const std::string& primary_key_column_name) {
        
        if (tables_.find(table_name) != tables_.end()) {
            throw std::invalid_argument("Table '" + table_name + "' already exists");
        }
        
        auto table = std::make_unique<Table>(table_name, std::move(schema), primary_key_column_name);
        Table* table_ptr = table.get();
        tables_[table_name] = std::move(table);
        
        return table_ptr;
    }

    /**
     * @brief Get a table by name
     * @param table_name Name of the table
     * @return Pointer to the table if found, nullptr otherwise
     */
    Table* getTable(const std::string& table_name) {
        auto it = tables_.find(table_name);
        return (it != tables_.end()) ? it->second.get() : nullptr;
    }

    /**
     * @brief Get a table by name (const version)
     * @param table_name Name of the table
     * @return Pointer to the table if found, nullptr otherwise
     */
    const Table* getTable(const std::string& table_name) const {
        auto it = tables_.find(table_name);
        return (it != tables_.end()) ? it->second.get() : nullptr;
    }

    /**
     * @brief Check if a table exists
     * @param table_name Name of the table
     * @return true if table exists, false otherwise
     */
    bool hasTable(const std::string& table_name) const {
        return tables_.find(table_name) != tables_.end();
    }

    /**
     * @brief Drop (delete) a table from the database
     * @param table_name Name of the table to drop
     * @return true if table was dropped, false if table didn't exist
     */
    bool dropTable(const std::string& table_name) {
        auto it = tables_.find(table_name);
        if (it != tables_.end()) {
            tables_.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Get the names of all tables in the database
     * @return Vector of table names
     */
    std::vector<std::string> getTableNames() const {
        std::vector<std::string> names;
        names.reserve(tables_.size());
        
        for (const auto& pair : tables_) {
            names.push_back(pair.first);
        }
        
        return names;
    }

    /**
     * @brief Get the number of tables in the database
     * @return Number of tables
     */
    size_t getTableCount() const {
        return tables_.size();
    }

    /**
     * @brief Check if the database is empty (no tables)
     * @return true if database has no tables
     */
    bool isEmpty() const {
        return tables_.empty();
    }

    /**
     * @brief Clear all tables from the database
     */
    void clear() {
        tables_.clear();
    }

    /**
     * @brief Get database statistics
     */
    struct DatabaseStats {
        std::string name;
        size_t table_count;
        size_t total_row_count;
        size_t total_memory_estimate;
        std::vector<std::pair<std::string, size_t>> table_row_counts; // table name -> row count
    };

    DatabaseStats getStats() const {
        DatabaseStats stats;
        stats.name = name_;
        stats.table_count = tables_.size();
        stats.total_row_count = 0;
        stats.total_memory_estimate = 0;
        
        for (const auto& pair : tables_) {
            const auto& table = pair.second;
            size_t row_count = table->getRowCount();
            auto table_stats = table->getStats();
            
            stats.total_row_count += row_count;
            stats.total_memory_estimate += table_stats.memory_usage_estimate;
            stats.table_row_counts.emplace_back(pair.first, row_count);
        }
        
        return stats;
    }

    /**
     * @brief Helper function to create a simple table with basic column types
     * @param table_name Name of the table
     * @param column_specs Vector of (name, type, nullable) tuples for columns
     * @param primary_key_column_name Name of the primary key column
     * @return Pointer to the created table
     */
    Table* createSimpleTable(const std::string& table_name,
                            const std::vector<std::tuple<std::string, ValueType, bool>>& column_specs,
                            const std::string& primary_key_column_name) {
        
        std::vector<Column> schema;
        schema.reserve(column_specs.size());
        
        for (const auto& spec : column_specs) {
            const std::string& col_name = std::get<0>(spec);
            ValueType col_type = std::get<1>(spec);
            bool nullable = std::get<2>(spec);
            
            // Make primary key column unique and non-nullable
            bool unique = (col_name == primary_key_column_name);
            bool actual_nullable = (col_name == primary_key_column_name) ? false : nullable;
            
            schema.emplace_back(col_name, col_type, actual_nullable, unique);
        }
        
        return createTable(table_name, std::move(schema), primary_key_column_name);
    }

    /**
     * @brief Execute a simple query across all tables (for debugging/testing)
     * @param predicate Function that takes table name and table, returns true to include in results
     * @return Vector of table names that match the predicate
     */
    std::vector<std::string> queryTables(std::function<bool(const std::string&, const Table&)> predicate) const {
        std::vector<std::string> results;
        
        for (const auto& pair : tables_) {
            if (predicate(pair.first, *pair.second)) {
                results.push_back(pair.first);
            }
        }
        
        return results;
    }

    /**
     * @brief Iterator support for range-based loops
     */
    auto begin() { return tables_.begin(); }
    auto end() { return tables_.end(); }
    auto begin() const { return tables_.begin(); }
    auto end() const { return tables_.end(); }
    auto cbegin() const { return tables_.cbegin(); }
    auto cend() const { return tables_.cend(); }
};

} // namespace scalerdb 