#pragma once

#include "value.hpp"
#include "column.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <stdexcept>

namespace scalerdb {

/**
 * @brief Represents a database row with values corresponding to columns
 * 
 * This class provides efficient access to values both by column index (for performance)
 * and by column name (for convenience). It maintains the relationship between
 * columns and their values.
 */
class Row {
private:
    std::vector<Value> values_;
    const std::vector<Column>* schema_; // Pointer to the table's schema
    mutable std::unordered_map<std::string, size_t> name_to_index_; // Cached column name to index mapping

    /**
     * @brief Build the name-to-index mapping cache if needed
     */
    void buildNameMapping() const {
        if (name_to_index_.empty() && schema_) {
            for (size_t i = 0; i < schema_->size(); ++i) {
                name_to_index_[(*schema_)[i].getName()] = i;
            }
        }
    }

public:
    /**
     * @brief Construct an empty row
     */
    Row() : schema_(nullptr) {}

    /**
     * @brief Construct a row with a schema (columns will be initialized to defaults/nulls)
     * @param schema Pointer to the table schema
     */
    explicit Row(const std::vector<Column>* schema) : schema_(schema) {
        if (schema_) {
            values_.reserve(schema_->size());
            for (const auto& column : *schema_) {
                values_.push_back(column.getDefaultOrNull());
            }
        }
    }

    /**
     * @brief Construct a row with specific values
     * @param schema Pointer to the table schema
     * @param values Initial values for the row
     */
    Row(const std::vector<Column>* schema, std::vector<Value> values) 
        : values_(std::move(values)), schema_(schema) {
        
        if (schema_ && values_.size() != schema_->size()) {
            throw std::invalid_argument("Number of values doesn't match schema size");
        }
    }

    /**
     * @brief Copy constructor
     */
    Row(const Row& other) 
        : values_(other.values_), schema_(other.schema_) {
        // Don't copy the cache, let it rebuild lazily
    }

    /**
     * @brief Move constructor
     */
    Row(Row&& other) noexcept
        : values_(std::move(other.values_)), schema_(other.schema_), 
          name_to_index_(std::move(other.name_to_index_)) {
        other.schema_ = nullptr;
    }

    /**
     * @brief Copy assignment
     */
    Row& operator=(const Row& other) {
        if (this != &other) {
            values_ = other.values_;
            schema_ = other.schema_;
            name_to_index_.clear(); // Clear cache, will rebuild if needed
        }
        return *this;
    }

    /**
     * @brief Move assignment
     */
    Row& operator=(Row&& other) noexcept {
        if (this != &other) {
            values_ = std::move(other.values_);
            schema_ = other.schema_;
            name_to_index_ = std::move(other.name_to_index_);
            other.schema_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief Get the number of columns in this row
     */
    size_t size() const {
        return values_.size();
    }

    /**
     * @brief Check if the row is empty
     */
    bool empty() const {
        return values_.empty();
    }

    /**
     * @brief Get value by column index (fast access)
     * @param index Column index
     * @return Reference to the value
     */
    const Value& getValue(size_t index) const {
        if (index >= values_.size()) {
            throw std::out_of_range("Column index out of range");
        }
        return values_[index];
    }

    /**
     * @brief Get value by column index (mutable)
     * @param index Column index
     * @return Reference to the value
     */
    Value& getValue(size_t index) {
        if (index >= values_.size()) {
            throw std::out_of_range("Column index out of range");
        }
        return values_[index];
    }

    /**
     * @brief Get value by column name
     * @param column_name Name of the column
     * @return Reference to the value
     */
    const Value& getValue(const std::string& column_name) const {
        buildNameMapping();
        auto it = name_to_index_.find(column_name);
        if (it == name_to_index_.end()) {
            throw std::invalid_argument("Column '" + column_name + "' not found");
        }
        return values_[it->second];
    }

    /**
     * @brief Get value by column name (mutable)
     * @param column_name Name of the column
     * @return Reference to the value
     */
    Value& getValue(const std::string& column_name) {
        buildNameMapping();
        auto it = name_to_index_.find(column_name);
        if (it == name_to_index_.end()) {
            throw std::invalid_argument("Column '" + column_name + "' not found");
        }
        return values_[it->second];
    }

    /**
     * @brief Set value by column index
     * @param index Column index
     * @param value New value
     */
    void setValue(size_t index, Value value) {
        if (index >= values_.size()) {
            throw std::out_of_range("Column index out of range");
        }
        
        // Validate against schema if available
        if (schema_ && index < schema_->size()) {
            if (!(*schema_)[index].validateValue(value)) {
                throw std::invalid_argument("Value doesn't satisfy column constraints");
            }
        }
        
        values_[index] = std::move(value);
    }

    /**
     * @brief Set value by column name
     * @param column_name Name of the column
     * @param value New value
     */
    void setValue(const std::string& column_name, Value value) {
        buildNameMapping();
        auto it = name_to_index_.find(column_name);
        if (it == name_to_index_.end()) {
            throw std::invalid_argument("Column '" + column_name + "' not found");
        }
        setValue(it->second, std::move(value));
    }

    /**
     * @brief Convenient access operators
     */
    const Value& operator[](size_t index) const {
        return getValue(index);
    }

    Value& operator[](size_t index) {
        return getValue(index);
    }

    const Value& operator[](const std::string& column_name) const {
        return getValue(column_name);
    }

    Value& operator[](const std::string& column_name) {
        return getValue(column_name);
    }

    /**
     * @brief Get all values as a vector
     * @return Reference to the internal values vector
     */
    const std::vector<Value>& getValues() const {
        return values_;
    }

    /**
     * @brief Set the schema for this row
     * @param schema Pointer to the schema
     */
    void setSchema(const std::vector<Column>* schema) {
        schema_ = schema;
        name_to_index_.clear(); // Clear cache
        
        // Resize values if needed
        if (schema_ && values_.size() != schema_->size()) {
            values_.resize(schema_->size());
            
            // Fill missing values with defaults
            for (size_t i = 0; i < schema_->size(); ++i) {
                if (i >= values_.size() || values_[i].isNull()) {
                    values_[i] = (*schema_)[i].getDefaultOrNull();
                }
            }
        }
    }

    /**
     * @brief Validate this row against its schema
     * @return true if all values satisfy their column constraints
     */
    bool validate() const {
        if (!schema_) {
            return true; // No schema to validate against
        }
        
        if (values_.size() != schema_->size()) {
            return false;
        }
        
        for (size_t i = 0; i < values_.size(); ++i) {
            if (!(*schema_)[i].validateValue(values_[i])) {
                return false;
            }
        }
        
        return true;
    }

    /**
     * @brief Get the column index for a given column name
     * @param column_name Name of the column
     * @return Column index
     */
    size_t getColumnIndex(const std::string& column_name) const {
        buildNameMapping();
        auto it = name_to_index_.find(column_name);
        if (it == name_to_index_.end()) {
            throw std::invalid_argument("Column '" + column_name + "' not found");
        }
        return it->second;
    }

    /**
     * @brief Comparison operators
     */
    bool operator==(const Row& other) const {
        return values_ == other.values_;
    }

    bool operator!=(const Row& other) const {
        return !(*this == other);
    }
};

} // namespace scalerdb 