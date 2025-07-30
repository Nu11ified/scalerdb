#include <gtest/gtest.h>
#include "core/database.hpp"
#include "core/value.hpp"
#include "core/column.hpp"
#include <filesystem>
#include <cstdio>

using namespace scalerdb;

class SerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary directory for test files
        test_dir = std::filesystem::temp_directory_path() / "scalerdb_test";
        std::filesystem::create_directories(test_dir);
        
        // Generate test filename
        test_file = test_dir / "test_database.msgpack";
    }
    
    void TearDown() override {
        // Clean up test files
        if (std::filesystem::exists(test_file)) {
            std::filesystem::remove(test_file);
        }
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }
    
    std::filesystem::path test_dir;
    std::filesystem::path test_file;
};

TEST_F(SerializationTest, EmptyDatabaseRoundTrip) {
    // Create empty database
    Database original("test_db");
    
    // Save to file
    ASSERT_TRUE(original.save(test_file.string()));
    
    // Load into new database
    Database loaded;
    ASSERT_TRUE(loaded.load(test_file.string()));
    
    // Verify empty database state
    EXPECT_EQ(loaded.getTableCount(), 0);
    EXPECT_TRUE(loaded.isEmpty());
}

TEST_F(SerializationTest, SingleTableRoundTrip) {
    // Create database with one simple table
    Database original("test_db");
    
    // Create users table
    std::vector<Column> user_columns = {
        Column("id", ValueType::Integer32, false, true),  // Primary key
        Column("name", ValueType::String, false),
        Column("age", ValueType::Integer32, true),
        Column("active", ValueType::Boolean, false, false, Value(true))  // Default value
    };
    
    Table* users_table = original.createTable("users", std::move(user_columns), "id");
    
    // Insert test data
    Row user1(&users_table->getSchema());
    user1.setValue("id", Value(1));
    user1.setValue("name", Value(std::string("Alice")));
    user1.setValue("age", Value(30));
    user1.setValue("active", Value(true));
    users_table->insertRow(user1);
    
    Row user2(&users_table->getSchema());
    user2.setValue("id", Value(2));
    user2.setValue("name", Value(std::string("Bob")));
    user2.setValue("age", Value(25));
    user2.setValue("active", Value(false));
    users_table->insertRow(user2);
    
    // Save to file
    ASSERT_TRUE(original.save(test_file.string()));
    
    // Load into new database
    Database loaded;
    ASSERT_TRUE(loaded.load(test_file.string()));
    
    // Verify database structure
    EXPECT_EQ(loaded.getTableCount(), 1);
    EXPECT_TRUE(loaded.hasTable("users"));
    
    // Verify table structure
    const Table* loaded_table = loaded.getTable("users");
    ASSERT_NE(loaded_table, nullptr);
    EXPECT_EQ(loaded_table->getName(), "users");
    EXPECT_EQ(loaded_table->getRowCount(), 2);
    EXPECT_EQ(loaded_table->getPrimaryKeyColumnName(), "id");
    
    // Verify columns
    const auto& columns = loaded_table->getSchema();
    EXPECT_EQ(columns.size(), 4);
    
    EXPECT_EQ(columns[0].getName(), "id");
    EXPECT_EQ(columns[0].getType(), ValueType::Integer32);
    EXPECT_FALSE(columns[0].isNullable());
    EXPECT_TRUE(columns[0].isUnique());
    
    EXPECT_EQ(columns[1].getName(), "name");
    EXPECT_EQ(columns[1].getType(), ValueType::String);
    EXPECT_FALSE(columns[1].isNullable());
    
    EXPECT_EQ(columns[2].getName(), "age");
    EXPECT_EQ(columns[2].getType(), ValueType::Integer32);
    EXPECT_TRUE(columns[2].isNullable());
    
    EXPECT_EQ(columns[3].getName(), "active");
    EXPECT_EQ(columns[3].getType(), ValueType::Boolean);
    EXPECT_FALSE(columns[3].isNullable());
    // Note: default values are preserved
    EXPECT_TRUE(columns[3].getDefaultValue().has_value());
    EXPECT_TRUE(columns[3].getDefaultValue()->get<bool>());
    
    // Verify data
    const Row* row1 = loaded_table->findRowByPK(Value(1));
    ASSERT_NE(row1, nullptr);
    EXPECT_EQ(row1->getValue("id").get<int32_t>(), 1);
    EXPECT_EQ(row1->getValue("name").get<std::string>(), "Alice");
    EXPECT_EQ(row1->getValue("age").get<int32_t>(), 30);
    EXPECT_TRUE(row1->getValue("active").get<bool>());
    
    const Row* row2 = loaded_table->findRowByPK(Value(2));
    ASSERT_NE(row2, nullptr);
    EXPECT_EQ(row2->getValue("id").get<int32_t>(), 2);
    EXPECT_EQ(row2->getValue("name").get<std::string>(), "Bob");
    EXPECT_EQ(row2->getValue("age").get<int32_t>(), 25);
    EXPECT_FALSE(row2->getValue("active").get<bool>());
}

TEST_F(SerializationTest, MultipleTablesRoundTrip) {
    // Create database with multiple tables
    Database original("ecommerce_db");
    
    // Create products table
    auto* products = original.createSimpleTable(
        "products",
        {
            {"id", ValueType::Integer32, false},
            {"name", ValueType::String, false},
            {"price", ValueType::Double, false},
            {"in_stock", ValueType::Boolean, false}
        },
        "id"
    );
    
    // Create orders table
    auto* orders = original.createSimpleTable(
        "orders",
        {
            {"order_id", ValueType::Integer64, false},
            {"customer_name", ValueType::String, false},
            {"total", ValueType::Double, false},
            {"shipped", ValueType::Boolean, true}
        },
        "order_id"
    );
    
    // Add data to products
    {
        Row product1(&products->getSchema());
        product1.setValue("id", Value(101));
        product1.setValue("name", Value(std::string("Laptop")));
        product1.setValue("price", Value(999.99));
        product1.setValue("in_stock", Value(true));
        products->insertRow(product1);
        
        Row product2(&products->getSchema());
        product2.setValue("id", Value(102));
        product2.setValue("name", Value(std::string("Mouse")));
        product2.setValue("price", Value(29.99));
        product2.setValue("in_stock", Value(false));
        products->insertRow(product2);
    }
    
    // Add data to orders
    {
        Row order1(&orders->getSchema());
        order1.setValue("order_id", Value(static_cast<int64_t>(1001)));
        order1.setValue("customer_name", Value(std::string("John Doe")));
        order1.setValue("total", Value(999.99));
        order1.setValue("shipped", Value(false));
        orders->insertRow(order1);
        
        Row order2(&orders->getSchema());
        order2.setValue("order_id", Value(static_cast<int64_t>(1002)));
        order2.setValue("customer_name", Value(std::string("Jane Smith")));
        order2.setValue("total", Value(59.98));
        order2.setValue("shipped", Value(true));
        orders->insertRow(order2);
    }
    
    // Save database
    ASSERT_TRUE(original.save(test_file.string()));
    
    // Load database
    Database loaded;
    ASSERT_TRUE(loaded.load(test_file.string()));
    
    // Verify structure
    EXPECT_EQ(loaded.getTableCount(), 2);
    EXPECT_TRUE(loaded.hasTable("products"));
    EXPECT_TRUE(loaded.hasTable("orders"));
    
    // Verify products table
    const Table* loaded_products = loaded.getTable("products");
    ASSERT_NE(loaded_products, nullptr);
    EXPECT_EQ(loaded_products->getRowCount(), 2);
    
    const Row* laptop = loaded_products->findRowByPK(Value(101));
    ASSERT_NE(laptop, nullptr);
    EXPECT_EQ(laptop->getValue("name").get<std::string>(), "Laptop");
    EXPECT_DOUBLE_EQ(laptop->getValue("price").get<double>(), 999.99);
    EXPECT_TRUE(laptop->getValue("in_stock").get<bool>());
    
    // Verify orders table
    const Table* loaded_orders = loaded.getTable("orders");
    ASSERT_NE(loaded_orders, nullptr);
    EXPECT_EQ(loaded_orders->getRowCount(), 2);
    
    const Row* order1001 = loaded_orders->findRowByPK(Value(static_cast<int64_t>(1001)));
    ASSERT_NE(order1001, nullptr);
    EXPECT_EQ(order1001->getValue("customer_name").get<std::string>(), "John Doe");
    EXPECT_DOUBLE_EQ(order1001->getValue("total").get<double>(), 999.99);
    EXPECT_FALSE(order1001->getValue("shipped").get<bool>());
}

TEST_F(SerializationTest, AllValueTypesRoundTrip) {
    // Create database with table containing all value types
    Database original("types_test");
    
    std::vector<Column> columns = {
        Column("id", ValueType::Integer32, false, true),
        Column("null_col", ValueType::String, true),
        Column("bool_col", ValueType::Boolean, false),
        Column("int32_col", ValueType::Integer32, false),
        Column("int64_col", ValueType::Integer64, false),
        Column("double_col", ValueType::Double, false),
        Column("string_col", ValueType::String, false)
    };
    
    Table* types_table = original.createTable("types", std::move(columns), "id");
    
    // Insert row with all types
    Row test_row(&types_table->getSchema());
    test_row.setValue("id", Value(1));
    test_row.setValue("null_col", Value());  // NULL value
    test_row.setValue("bool_col", Value(true));
    test_row.setValue("int32_col", Value(std::numeric_limits<int32_t>::max()));
    test_row.setValue("int64_col", Value(std::numeric_limits<int64_t>::min()));
    test_row.setValue("double_col", Value(3.14159));
    test_row.setValue("string_col", Value(std::string("Hello, MessagePack! 🚀")));
    types_table->insertRow(test_row);
    
    // Save and load
    ASSERT_TRUE(original.save(test_file.string()));
    
    Database loaded;
    ASSERT_TRUE(loaded.load(test_file.string()));
    
    // Verify all value types
    const Table* loaded_table = loaded.getTable("types");
    ASSERT_NE(loaded_table, nullptr);
    
    const Row* loaded_row = loaded_table->findRowByPK(Value(1));
    ASSERT_NE(loaded_row, nullptr);
    
    EXPECT_EQ(loaded_row->getValue("id").get<int32_t>(), 1);
    EXPECT_TRUE(loaded_row->getValue("null_col").isNull());
    EXPECT_TRUE(loaded_row->getValue("bool_col").get<bool>());
    EXPECT_EQ(loaded_row->getValue("int32_col").get<int32_t>(), std::numeric_limits<int32_t>::max());
    EXPECT_EQ(loaded_row->getValue("int64_col").get<int64_t>(), std::numeric_limits<int64_t>::min());
    EXPECT_DOUBLE_EQ(loaded_row->getValue("double_col").get<double>(), 3.14159);
    EXPECT_EQ(loaded_row->getValue("string_col").get<std::string>(), "Hello, MessagePack! 🚀");
}

TEST_F(SerializationTest, SaveLoadFailureHandling) {
    Database db("test_db");
    
    // Test saving to invalid path
    EXPECT_FALSE(db.save("/invalid/path/that/does/not/exist/file.msgpack"));
    
    // Test loading non-existent file
    Database empty_db;
    EXPECT_FALSE(empty_db.load("non_existent_file.msgpack"));
    
    // Test loading invalid file (create a text file instead of msgpack)
    std::string invalid_file = test_dir / "invalid.msgpack";
    std::ofstream invalid(invalid_file);
    invalid << "This is not a valid msgpack file";
    invalid.close();
    
    EXPECT_FALSE(empty_db.load(invalid_file));
}

TEST_F(SerializationTest, LargeDatasetRoundTrip) {
    // Test with larger dataset
    Database original("large_test");
    
    auto* large_table = original.createSimpleTable(
        "large_table",
        {
            {"id", ValueType::Integer32, false},
            {"data", ValueType::String, false},
            {"value", ValueType::Double, false}
        },
        "id"
    );
    
    // Insert 1000 rows
    const size_t num_rows = 1000;
    for (size_t i = 0; i < num_rows; ++i) {
        Row row(&large_table->getSchema());
        row.setValue("id", Value(static_cast<int32_t>(i)));
        row.setValue("data", Value(std::string("Row data for entry ") + std::to_string(i)));
        row.setValue("value", Value(static_cast<double>(i) * 1.5));
        large_table->insertRow(row);
    }
    
    // Save and load
    ASSERT_TRUE(original.save(test_file.string()));
    
    Database loaded;
    ASSERT_TRUE(loaded.load(test_file.string()));
    
    // Verify size
    const Table* loaded_table = loaded.getTable("large_table");
    ASSERT_NE(loaded_table, nullptr);
    EXPECT_EQ(loaded_table->getRowCount(), num_rows);
    
    // Spot check some rows
    const Row* first = loaded_table->findRowByPK(Value(0));
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->getValue("data").get<std::string>(), "Row data for entry 0");
    EXPECT_DOUBLE_EQ(first->getValue("value").get<double>(), 0.0);
    
    const Row* middle = loaded_table->findRowByPK(Value(500));
    ASSERT_NE(middle, nullptr);
    EXPECT_EQ(middle->getValue("data").get<std::string>(), "Row data for entry 500");
    EXPECT_DOUBLE_EQ(middle->getValue("value").get<double>(), 750.0);
    
    const Row* last = loaded_table->findRowByPK(Value(999));
    ASSERT_NE(last, nullptr);
    EXPECT_EQ(last->getValue("data").get<std::string>(), "Row data for entry 999");
    EXPECT_DOUBLE_EQ(last->getValue("value").get<double>(), 1498.5);
}