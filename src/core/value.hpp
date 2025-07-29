#pragma once

#include <variant>
#include <string>
#include <optional>
#include <iostream>
#include <type_traits>
#include <concepts>

namespace scalerdb {

// Forward declarations
class Value;

// Type alias for our supported data types
using ValueVariant = std::variant<
    std::nullopt_t,
    bool,
    int32_t,
    int64_t,
    double,
    std::string
>;

// Enum for value types
enum class ValueType {
    Null,
    Boolean,
    Integer32,
    Integer64,
    Double,
    String
};

/**
 * @brief A type-safe value container that can hold different data types
 * 
 * This class uses std::variant to provide type-safe storage for database values.
 * It supports null values, integers, doubles, booleans, and strings.
 */
class Value {
private:
    ValueVariant data_;

public:
    // Constructors
    Value() : data_(std::nullopt) {}
    Value(std::nullopt_t) : data_(std::nullopt) {}
    Value(bool val) : data_(val) {}
    Value(int32_t val) : data_(val) {}
    Value(int64_t val) : data_(val) {}
    Value(double val) : data_(val) {}
    Value(float val) : data_(static_cast<double>(val)) {}
    Value(const std::string& val) : data_(val) {}
    Value(std::string&& val) : data_(std::move(val)) {}
    Value(const char* val) : data_(std::string(val)) {}
    
    // Handle the case where int might be different from int32_t
    template<typename T>
    Value(T val) requires (std::is_integral_v<T> && !std::is_same_v<T, bool> && 
                          !std::is_same_v<T, int32_t> && !std::is_same_v<T, int64_t>) 
        : data_(static_cast<int32_t>(val)) {}

    // Copy and move semantics
    Value(const Value&) = default;
    Value(Value&&) = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) = default;

    // Type checking
    ValueType getType() const {
        return static_cast<ValueType>(data_.index());
    }

    bool isNull() const {
        return std::holds_alternative<std::nullopt_t>(data_);
    }

    bool isBool() const {
        return std::holds_alternative<bool>(data_);
    }

    bool isInt32() const {
        return std::holds_alternative<int32_t>(data_);
    }

    bool isInt64() const {
        return std::holds_alternative<int64_t>(data_);
    }

    bool isDouble() const {
        return std::holds_alternative<double>(data_);
    }

    bool isString() const {
        return std::holds_alternative<std::string>(data_);
    }

    // Value getters with type checking
    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, bool>) {
            if (!isBool()) throw std::runtime_error("Value is not a boolean");
            return std::get<bool>(data_);
        }
        else if constexpr (std::is_same_v<T, int32_t>) {
            if (!isInt32()) throw std::runtime_error("Value is not an int32");
            return std::get<int32_t>(data_);
        }
        else if constexpr (std::is_same_v<T, int64_t>) {
            if (!isInt64()) throw std::runtime_error("Value is not an int64");
            return std::get<int64_t>(data_);
        }
        else if constexpr (std::is_same_v<T, double>) {
            if (!isDouble()) throw std::runtime_error("Value is not a double");
            return std::get<double>(data_);
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            if (!isString()) throw std::runtime_error("Value is not a string");
            return std::get<std::string>(data_);
        }
        else {
            static_assert(sizeof(T) == 0, "Unsupported type for Value::get()");
        }
    }

    // Safe getters that return optional
    template<typename T>
    std::optional<T> tryGet() const {
        try {
            return get<T>();
        } catch (...) {
            return std::nullopt;
        }
    }

    // Conversion operators for convenience
    explicit operator bool() const {
        if (isNull()) return false;
        if (isBool()) return get<bool>();
        if (isInt32()) return get<int32_t>() != 0;
        if (isInt64()) return get<int64_t>() != 0;
        if (isDouble()) return get<double>() != 0.0;
        if (isString()) return !get<std::string>().empty();
        return false;
    }

    // Comparison operators
    bool operator==(const Value& other) const {
        // Different types are never equal
        if (getType() != other.getType()) {
            return false;
        }
        
        // Handle each type explicitly
        switch (getType()) {
            case ValueType::Null:
                return true; // Both are null
            case ValueType::Boolean:
                return get<bool>() == other.get<bool>();
            case ValueType::Integer32:
                return get<int32_t>() == other.get<int32_t>();
            case ValueType::Integer64:
                return get<int64_t>() == other.get<int64_t>();
            case ValueType::Double:
                return get<double>() == other.get<double>();
            case ValueType::String:
                return get<std::string>() == other.get<std::string>();
            default:
                return false;
        }
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    bool operator<(const Value& other) const {
        // Null values are considered smallest
        if (isNull() && !other.isNull()) return true;
        if (!isNull() && other.isNull()) return false;
        if (isNull() && other.isNull()) return false;

        // Different types are ordered by their type index
        if (getType() != other.getType()) {
            return getType() < other.getType();
        }

        // Same type comparison
        switch (getType()) {
            case ValueType::Null:
                return false; // Null values are equal
            case ValueType::Boolean:
                return get<bool>() < other.get<bool>();
            case ValueType::Integer32:
                return get<int32_t>() < other.get<int32_t>();
            case ValueType::Integer64:
                return get<int64_t>() < other.get<int64_t>();
            case ValueType::Double:
                return get<double>() < other.get<double>();
            case ValueType::String:
                return get<std::string>() < other.get<std::string>();
            default:
                return false;
        }
    }

    // String representation
    std::string toString() const {
        return std::visit([](const auto& value) -> std::string {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::nullopt_t>) {
                return "NULL";
            }
            else if constexpr (std::is_same_v<T, bool>) {
                return value ? "true" : "false";
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(value);
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                return value;
            }
            else {
                return "UNKNOWN";
            }
        }, data_);
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const Value& value) {
        os << value.toString();
        return os;
    }
};

} // namespace scalerdb 