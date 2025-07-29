#pragma once

#include "value.hpp"
#include <string>
#include <functional>
#include <optional>
#include <stdexcept>

namespace scalerdb {

/**
 * @brief Column constraint validator function type
 * Returns true if the value satisfies the constraint, false otherwise
 */
using ConstraintValidator = std::function<bool(const Value&)>;

/**
 * @brief Represents a database column with metadata and constraints
 */
class Column {
private:
    std::string name_;
    ValueType type_;
    bool nullable_;
    bool unique_;
    std::optional<Value> default_value_;
    std::vector<ConstraintValidator> constraints_;

public:
    /**
     * @brief Construct a new Column
     * 
     * @param name Column name
     * @param type Expected value type
     * @param nullable Whether the column accepts NULL values
     * @param unique Whether the column requires unique values
     * @param default_val Optional default value
     */
    Column(std::string name, ValueType type, bool nullable = true, 
           bool unique = false, std::optional<Value> default_val = std::nullopt)
        : name_(std::move(name)), type_(type), nullable_(nullable), 
          unique_(unique), default_value_(std::move(default_val)) {
        
        // Validate default value type if provided
        if (default_value_ && !default_value_->isNull() && default_value_->getType() != type_) {
            throw std::invalid_argument("Default value type doesn't match column type");
        }
    }

    // Getters
    const std::string& getName() const { return name_; }
    ValueType getType() const { return type_; }
    bool isNullable() const { return nullable_; }
    bool isUnique() const { return unique_; }
    const std::optional<Value>& getDefaultValue() const { return default_value_; }

    /**
     * @brief Add a constraint validator to this column
     * @param validator Function that returns true if value is valid
     */
    void addConstraint(ConstraintValidator validator) {
        constraints_.push_back(std::move(validator));
    }

    /**
     * @brief Validate a value against this column's constraints
     * @param value Value to validate
     * @return true if value is valid for this column
     */
    bool validateValue(const Value& value) const {
        // Check null constraint
        if (value.isNull()) {
            return nullable_;
        }

        // Check type constraint
        if (value.getType() != type_) {
            return false;
        }

        // Check custom constraints
        for (const auto& constraint : constraints_) {
            if (!constraint(value)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Get the default value for this column
     * @return Default value or NULL if no default is set
     */
    Value getDefaultOrNull() const {
        return default_value_.value_or(Value{});
    }

    /**
     * @brief Create a range constraint for numeric columns
     * @param min_val Minimum allowed value (inclusive)
     * @param max_val Maximum allowed value (inclusive)
     */
    template<typename T>
    static ConstraintValidator createRangeConstraint(T min_val, T max_val) {
        return [min_val, max_val](const Value& value) -> bool {
            if (value.isNull()) return true; // Let nullable constraint handle nulls
            
            try {
                T val = value.get<T>();
                return val >= min_val && val <= max_val;
            } catch (...) {
                return false; // Type mismatch
            }
        };
    }

    /**
     * @brief Create a string length constraint
     * @param min_len Minimum string length
     * @param max_len Maximum string length  
     */
    static ConstraintValidator createLengthConstraint(size_t min_len, size_t max_len) {
        return [min_len, max_len](const Value& value) -> bool {
            if (value.isNull()) return true;
            
            try {
                std::string str = value.get<std::string>();
                return str.length() >= min_len && str.length() <= max_len;
            } catch (...) {
                return false;
            }
        };
    }

    /**
     * @brief Create a constraint that checks if value is in a set of allowed values
     * @param allowed_values Set of allowed values
     */
    template<typename T>
    static ConstraintValidator createInSetConstraint(const std::vector<T>& allowed_values) {
        return [allowed_values](const Value& value) -> bool {
            if (value.isNull()) return true;
            
            try {
                T val = value.get<T>();
                return std::find(allowed_values.begin(), allowed_values.end(), val) != allowed_values.end();
            } catch (...) {
                return false;
            }
        };
    }

    // Comparison operators for column ordering
    bool operator==(const Column& other) const {
        return name_ == other.name_;
    }

    bool operator<(const Column& other) const {
        return name_ < other.name_;
    }
};

} // namespace scalerdb 