#pragma once

#include "row.hpp"
#include "column.hpp"
#include "value.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>
#include <optional>
#include <functional>
#include <stdexcept>
#include <shared_mutex>

namespace scalerdb {

/**
 * @brief Represents a database table with schema and data storage
 * 
 * This class provides efficient CRUD operations with primary key indexing.
 * Rows are stored in a vector for sequential access, and a hash map provides
 * fast primary key lookups. Thread-safe with per-table read-write locking.
 */
class Table {
private:
    std::string name_;
    std::vector<Column> schema_;
    std::vector<Row> rows_;
    std::unordered_map<std::string, size_t> primary_key_index_; // PK value -> row index
    size_t primary_key_column_; // Index of the primary key column
    size_t next_row_id_; // For auto-incrementing IDs if needed
    mutable std::shared_mutex table_mutex_; // Per-table read-write lock

    /**
     * @brief Get the primary key value from a row as a string
     * @param row The row to extract PK from
     * @return String representation of the primary key
     */
    std::string getPrimaryKeyValue(const Row& row) const {
        if (primary_key_column_ >= row.size()) {
            throw std::runtime_error("Primary key column index out of range");
        }
        return row.getValue(primary_key_column_).toString();
    }

    /**
     * @brief Validate that all unique constraints are satisfied
     * @param row Row to validate
     * @param exclude_index Row index to exclude from uniqueness check (for updates)
     * @return true if all unique constraints are satisfied
     */
    bool validateUniqueConstraints(const Row& row, std::optional<size_t> exclude_index = std::nullopt) const {
        for (size_t col_idx = 0; col_idx < schema_.size(); ++col_idx) {
            if (schema_[col_idx].isUnique()) {
                const Value& value = row.getValue(col_idx);
                
                // Check against all other rows
                for (size_t row_idx = 0; row_idx < rows_.size(); ++row_idx) {
                    if (exclude_index && row_idx == *exclude_index) {
                        continue; // Skip the row being updated
                    }
                    
                    if (rows_[row_idx].getValue(col_idx) == value) {
                        return false; // Duplicate found
                    }
                }
            }
        }
        return true;
    }

public:
    /**
     * @brief Construct a new Table
     * @param name Table name
     * @param schema Vector of columns defining the table structure
     * @param primary_key_column_name Name of the primary key column
     */
    Table(std::string name, std::vector<Column> schema, const std::string& primary_key_column_name)
        : name_(std::move(name)), schema_(std::move(schema)), next_row_id_(1) {
        initializeTable(primary_key_column_name);
    }
    
    /**
     * @brief Construct a new Table with capacity hint for performance
     * @param name Table name
     * @param schema Vector of columns defining the table structure
     * @param primary_key_column_name Name of the primary key column
     * @param expected_rows Expected number of rows (for memory pre-allocation)
     */
    Table(std::string name, std::vector<Column> schema, const std::string& primary_key_column_name, size_t expected_rows)
        : name_(std::move(name)), schema_(std::move(schema)), next_row_id_(1) {
        // Pre-allocate memory for better performance
        rows_.reserve(expected_rows);
        primary_key_index_.reserve(expected_rows);
        initializeTable(primary_key_column_name);
    }

private:
    void initializeTable(const std::string& primary_key_column_name) {
        
        if (schema_.empty()) {
            throw std::invalid_argument("Table must have at least one column");
        }
        
        // Find the primary key column
        primary_key_column_ = schema_.size(); // Invalid index initially
        for (size_t i = 0; i < schema_.size(); ++i) {
            if (schema_[i].getName() == primary_key_column_name) {
                primary_key_column_ = i;
                break;
            }
        }
        
        if (primary_key_column_ >= schema_.size()) {
            throw std::invalid_argument("Primary key column '" + primary_key_column_name + "' not found in schema");
        }
        
        // Primary key column should be unique and non-nullable
        if (!schema_[primary_key_column_].isUnique()) {
            throw std::invalid_argument("Primary key column must be unique");
        }
        if (schema_[primary_key_column_].isNullable()) {
            throw std::invalid_argument("Primary key column cannot be nullable");
        }
    }

public:
    // Disable copy and move constructors and assignment operators due to shared_mutex
    Table(const Table&) = delete;
    Table& operator=(const Table&) = delete;
    Table(Table&&) = delete;
    Table& operator=(Table&&) = delete;

    // Getters
    const std::string& getName() const { return name_; }
    const std::vector<Column>& getSchema() const { return schema_; }
    size_t getRowCount() const { 
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        return rows_.size(); 
    }
    bool isEmpty() const { 
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        return rows_.empty(); 
    }
    
    const std::string& getPrimaryKeyColumnName() const { 
        return schema_[primary_key_column_].getName(); 
    }

    /**
     * @brief Insert a new row into the table (thread-safe with exclusive lock)
     * @param row Row to insert
     * @return true if insertion was successful
     */
    bool insertRow(Row row) {
        std::unique_lock<std::shared_mutex> lock(table_mutex_);
        
        // Set schema for the row if not already set
        row.setSchema(&schema_);
        
        // Validate the row
        if (!row.validate()) {
            throw std::invalid_argument("Row validation failed");
        }
        
        // Check if row has correct number of columns
        if (row.size() != schema_.size()) {
            throw std::invalid_argument("Row size doesn't match schema");
        }
        
        // Check unique constraints
        if (!validateUniqueConstraints(row)) {
            throw std::invalid_argument("Unique constraint violation");
        }
        
        // Get primary key value
        std::string pk_value = getPrimaryKeyValue(row);
        if (primary_key_index_.find(pk_value) != primary_key_index_.end()) {
            throw std::invalid_argument("Primary key '" + pk_value + "' already exists");
        }
        
        // Insert the row
        size_t new_index = rows_.size();
        rows_.push_back(std::move(row));
        primary_key_index_[pk_value] = new_index;
        
        return true;
    }

    /**
     * @brief Insert a row using individual values (thread-safe)
     * @param values Vector of values for the new row
     * @return true if insertion was successful
     */
    bool insertRow(std::vector<Value> values) {
        Row row(&schema_, std::move(values));
        return insertRow(std::move(row));
    }

    /**
     * @brief Find a row by its primary key (thread-safe with shared lock)
     * @param primary_key Primary key value to search for
     * @return Pointer to the row if found, nullptr otherwise
     */
    const Row* findRowByPK(const Value& primary_key) const {
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        
        std::string pk_str = primary_key.toString();
        auto it = primary_key_index_.find(pk_str);
        if (it != primary_key_index_.end()) {
            return &rows_[it->second];
        }
        return nullptr;
    }

    /**
     * @brief Find a row by its primary key (mutable version, thread-safe with shared lock)
     * @param primary_key Primary key value to search for
     * @return Pointer to the row if found, nullptr otherwise
     */
    Row* findRowByPK(const Value& primary_key) {
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        
        std::string pk_str = primary_key.toString();
        auto it = primary_key_index_.find(pk_str);
        if (it != primary_key_index_.end()) {
            return &rows_[it->second];
        }
        return nullptr;
    }

    /**
     * @brief Update a row identified by primary key
     * @param primary_key Primary key of the row to update
     * @param new_values New values for the row
     * @return true if update was successful
     */
    bool updateRow(const Value& primary_key, std::vector<Value> new_values) {
        std::string pk_str = primary_key.toString();
        auto it = primary_key_index_.find(pk_str);
        if (it == primary_key_index_.end()) {
            return false; // Row not found
        }
        
        size_t row_index = it->second;
        Row new_row(&schema_, std::move(new_values));
        
        // Validate the new row
        if (!new_row.validate()) {
            throw std::invalid_argument("New row validation failed");
        }
        
        // Check unique constraints (excluding the current row)
        if (!validateUniqueConstraints(new_row, row_index)) {
            throw std::invalid_argument("Unique constraint violation");
        }
        
        // Check if primary key changed
        std::string new_pk_str = getPrimaryKeyValue(new_row);
        if (new_pk_str != pk_str) {
            // Primary key changed, update the index
            primary_key_index_.erase(it);
            
            // Check if new primary key already exists
            if (primary_key_index_.find(new_pk_str) != primary_key_index_.end()) {
                // Restore the old entry
                primary_key_index_[pk_str] = row_index;
                throw std::invalid_argument("New primary key '" + new_pk_str + "' already exists");
            }
            
            primary_key_index_[new_pk_str] = row_index;
        }
        
        // Update the row
        rows_[row_index] = std::move(new_row);
        return true;
    }

    /**
     * @brief Delete a row by primary key
     * @param primary_key Primary key of the row to delete
     * @return true if deletion was successful
     */
    bool deleteRow(const Value& primary_key) {
        std::string pk_str = primary_key.toString();
        auto it = primary_key_index_.find(pk_str);
        if (it == primary_key_index_.end()) {
            return false; // Row not found
        }
        
        size_t row_index = it->second;
        
        // Remove from primary key index
        primary_key_index_.erase(it);
        
        // Remove the row (this is O(n) operation)
        rows_.erase(rows_.begin() + row_index);
        
        // Update indices in the primary key map (all indices after the deleted one need to be decremented)
        for (auto& pair : primary_key_index_) {
            if (pair.second > row_index) {
                pair.second--;
            }
        }
        
        return true;
    }

    /**
     * @brief Get all rows in the table (thread-safe with shared lock)
     * @return Reference to the rows vector
     */
    const std::vector<Row>& getAllRows() const {
        std::shared_lock<std::shared_mutex> lock(table_mutex_);
        return rows_;
    }

    /**
     * @brief Find rows that match a predicate function
     * @param predicate Function that takes a Row and returns true if it matches
     * @return Vector of pointers to matching rows
     */
    std::vector<const Row*> findRows(std::function<bool(const Row&)> predicate) const {
        std::vector<const Row*> results;
        for (const auto& row : rows_) {
            if (predicate(row)) {
                results.push_back(&row);
            }
        }
        return results;
    }

    /**
     * @brief Find rows where a specific column matches a value
     * @param column_name Name of the column to check
     * @param value Value to match against
     * @return Vector of pointers to matching rows
     */
    std::vector<const Row*> findRowsByColumn(const std::string& column_name, const Value& value) const {
        return findRows([&column_name, &value](const Row& row) {
            try {
                return row.getValue(column_name) == value;
            } catch (...) {
                return false; // Column not found or other error
            }
        });
    }

    /**
     * @brief Get a specific row by index
     * @param index Row index
     * @return Reference to the row
     */
    const Row& getRow(size_t index) const {
        if (index >= rows_.size()) {
            throw std::out_of_range("Row index out of range");
        }
        return rows_[index];
    }

    /**
     * @brief Get a specific row by index (mutable)
     * @param index Row index
     * @return Reference to the row
     */
    Row& getRow(size_t index) {
        if (index >= rows_.size()) {
            throw std::out_of_range("Row index out of range");
        }
        return rows_[index];
    }

    /**
     * @brief Clear all data from the table
     */
    void clear() {
        rows_.clear();
        primary_key_index_.clear();
        next_row_id_ = 1;
    }

    /**
     * @brief Get table statistics
     */
    struct TableStats {
        size_t row_count;
        size_t column_count;
        std::string primary_key_column;
        size_t memory_usage_estimate; // Rough estimate in bytes
    };

    TableStats getStats() const {
        TableStats stats;
        stats.row_count = rows_.size();
        stats.column_count = schema_.size();
        stats.primary_key_column = schema_[primary_key_column_].getName();
        
        // Rough memory estimate
        stats.memory_usage_estimate = rows_.size() * schema_.size() * sizeof(Value) + 
                                    primary_key_index_.size() * (sizeof(std::string) + sizeof(size_t));
        
        return stats;
    }
};

} // namespace scalerdb 