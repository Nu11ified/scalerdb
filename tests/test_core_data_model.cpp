#include <gtest/gtest.h>
#include "../src/core/database.hpp"
#include "../src/core/table.hpp"
#include "../src/core/row.hpp"
#include "../src/core/column.hpp"
#include "../src/core/value.hpp"

using namespace scalerdb;

class CoreDataModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test database
        db = std::make_unique<Database>("test_db");
        
        // Create a simple users table schema
        std::vector<Column> user_schema;
        user_schema.emplace_back("id", ValueType::Integer32, false, true);  // Primary key
        user_schema.emplace_back("name", ValueType::String, false, false);
        user_schema.emplace_back("age", ValueType::Integer32, true, false);
        user_schema.emplace_back("email", ValueType::String, true, true);   // Unique
        
        users_table = db->createTable("users", std::move(user_schema), "id");
    }

    std::unique_ptr<Database> db;
    Table* users_table;
};

// Value Tests
TEST(ValueTest, BasicConstructionAndAccess) {
    // Test null value
    Value null_val;
    EXPECT_TRUE(null_val.isNull());
    EXPECT_EQ(null_val.getType(), ValueType::Null);
    EXPECT_EQ(null_val.toString(), "NULL");

    // Test boolean value
    Value bool_val(true);
    EXPECT_TRUE(bool_val.isBool());
    EXPECT_EQ(bool_val.get<bool>(), true);
    EXPECT_EQ(bool_val.toString(), "true");

    // Test integer value
    Value int_val(42);
    EXPECT_TRUE(int_val.isInt32());
    EXPECT_EQ(int_val.get<int32_t>(), 42);
    EXPECT_EQ(int_val.toString(), "42");

    // Test string value
    Value str_val("hello");
    EXPECT_TRUE(str_val.isString());
    EXPECT_EQ(str_val.get<std::string>(), "hello");
    EXPECT_EQ(str_val.toString(), "hello");

    // Test double value
    Value double_val(3.14);
    EXPECT_TRUE(double_val.isDouble());
    EXPECT_DOUBLE_EQ(double_val.get<double>(), 3.14);
}

TEST(ValueTest, Comparisons) {
    Value val1(42);
    Value val2(42);
    Value val3(43);
    Value str_val("42");

    EXPECT_EQ(val1, val2);
    EXPECT_NE(val1, val3);
    EXPECT_NE(val1, str_val);  // Different types
    EXPECT_LT(val1, val3);
    EXPECT_LT(val1, str_val);  // Int32 < String in type ordering
}

TEST(ValueTest, TypeConversions) {
    Value val(42);
    
    // Test explicit bool conversion
    EXPECT_TRUE(static_cast<bool>(val));
    
    Value zero_val(0);
    EXPECT_FALSE(static_cast<bool>(zero_val));
    
    Value null_val;
    EXPECT_FALSE(static_cast<bool>(null_val));
}

// Column Tests
TEST(ColumnTest, BasicColumnProperties) {
    Column col("test_col", ValueType::Integer32, true, false);
    
    EXPECT_EQ(col.getName(), "test_col");
    EXPECT_EQ(col.getType(), ValueType::Integer32);
    EXPECT_TRUE(col.isNullable());
    EXPECT_FALSE(col.isUnique());
}

TEST(ColumnTest, ValidationConstraints) {
    Column col("age", ValueType::Integer32, false, false);
    
    // Add range constraint (18-100)
    col.addConstraint(Column::createRangeConstraint<int32_t>(18, 100));
    
    EXPECT_TRUE(col.validateValue(Value(25)));
    EXPECT_TRUE(col.validateValue(Value(18)));  // Boundary
    EXPECT_TRUE(col.validateValue(Value(100))); // Boundary
    EXPECT_FALSE(col.validateValue(Value(17))); // Too low
    EXPECT_FALSE(col.validateValue(Value(101))); // Too high
    EXPECT_FALSE(col.validateValue(Value()));    // Null not allowed
}

TEST(ColumnTest, StringLengthConstraints) {
    Column col("name", ValueType::String, true, false);
    
    // Add length constraint (2-50 characters)
    col.addConstraint(Column::createLengthConstraint(2, 50));
    
    EXPECT_TRUE(col.validateValue(Value("ab")));    // Minimum length
    EXPECT_TRUE(col.validateValue(Value("hello")));
    EXPECT_FALSE(col.validateValue(Value("a")));    // Too short
    EXPECT_FALSE(col.validateValue(Value(std::string(51, 'x')))); // Too long
    EXPECT_TRUE(col.validateValue(Value()));        // Null allowed
}

// Row Tests
TEST_F(CoreDataModelTest, RowConstruction) {
    // Create a row with the users table schema
    Row row(&users_table->getSchema());
    
    EXPECT_EQ(row.size(), 4);
    EXPECT_FALSE(row.empty());
    
    // Set some values
    row.setValue("id", Value(1));
    row.setValue("name", Value("John Doe"));
    row.setValue("age", Value(30));
    
    EXPECT_EQ(row.getValue("id").get<int32_t>(), 1);
    EXPECT_EQ(row.getValue("name").get<std::string>(), "John Doe");
    EXPECT_EQ(row.getValue("age").get<int32_t>(), 30);
}

TEST_F(CoreDataModelTest, RowIndexAccess) {
    Row row(&users_table->getSchema());
    
    // Test index-based access
    row.setValue(0, Value(1));      // id
    row.setValue(1, Value("Jane"));  // name
    row.setValue(2, Value(25));     // age
    
    EXPECT_EQ(row[0].get<int32_t>(), 1);
    EXPECT_EQ(row[1].get<std::string>(), "Jane");
    EXPECT_EQ(row[2].get<int32_t>(), 25);
    
    // Test name-based access
    EXPECT_EQ(row["id"].get<int32_t>(), 1);
    EXPECT_EQ(row["name"].get<std::string>(), "Jane");
    EXPECT_EQ(row["age"].get<int32_t>(), 25);
}

// Table Tests - CRUD Operations
TEST_F(CoreDataModelTest, TableInsertRow) {
    // Test successful insertion
    std::vector<Value> values = {Value(1), Value("Alice"), Value(28), Value("alice@test.com")};
    EXPECT_TRUE(users_table->insertRow(values));
    EXPECT_EQ(users_table->getRowCount(), 1);
    
    // Test duplicate primary key
    std::vector<Value> duplicate_values = {Value(1), Value("Bob"), Value(30), Value("bob@test.com")};
    EXPECT_THROW(users_table->insertRow(duplicate_values), std::invalid_argument);
    EXPECT_EQ(users_table->getRowCount(), 1);
    
    // Test unique constraint violation (duplicate email)
    std::vector<Value> unique_violation = {Value(2), Value("Charlie"), Value(25), Value("alice@test.com")};
    EXPECT_THROW(users_table->insertRow(unique_violation), std::invalid_argument);
    EXPECT_EQ(users_table->getRowCount(), 1);
}

TEST_F(CoreDataModelTest, TableFindRowByPK) {
    // Insert test data
    std::vector<Value> values = {Value(1), Value("Alice"), Value(28), Value("alice@test.com")};
    users_table->insertRow(values);
    
    // Test finding existing row
    const Row* found_row = users_table->findRowByPK(Value(1));
    ASSERT_NE(found_row, nullptr);
    EXPECT_EQ(found_row->getValue("name").get<std::string>(), "Alice");
    EXPECT_EQ(found_row->getValue("age").get<int32_t>(), 28);
    
    // Test finding non-existent row
    const Row* not_found = users_table->findRowByPK(Value(999));
    EXPECT_EQ(not_found, nullptr);
}

TEST_F(CoreDataModelTest, TableUpdateRow) {
    // Insert test data
    std::vector<Value> values = {Value(1), Value("Alice"), Value(28), Value("alice@test.com")};
    users_table->insertRow(values);
    
    // Test successful update
    std::vector<Value> updated_values = {Value(1), Value("Alice Smith"), Value(29), Value("alice.smith@test.com")};
    EXPECT_TRUE(users_table->updateRow(Value(1), updated_values));
    
    const Row* updated_row = users_table->findRowByPK(Value(1));
    EXPECT_EQ(updated_row->getValue("name").get<std::string>(), "Alice Smith");
    EXPECT_EQ(updated_row->getValue("age").get<int32_t>(), 29);
    
    // Test updating non-existent row
    EXPECT_FALSE(users_table->updateRow(Value(999), updated_values));
}

TEST_F(CoreDataModelTest, TableDeleteRow) {
    // Insert test data
    std::vector<Value> values1 = {Value(1), Value("Alice"), Value(28), Value("alice@test.com")};
    std::vector<Value> values2 = {Value(2), Value("Bob"), Value(30), Value("bob@test.com")};
    users_table->insertRow(values1);
    users_table->insertRow(values2);
    
    EXPECT_EQ(users_table->getRowCount(), 2);
    
    // Test successful deletion
    EXPECT_TRUE(users_table->deleteRow(Value(1)));
    EXPECT_EQ(users_table->getRowCount(), 1);
    
    // Verify the correct row was deleted
    EXPECT_EQ(users_table->findRowByPK(Value(1)), nullptr);
    EXPECT_NE(users_table->findRowByPK(Value(2)), nullptr);
    
    // Test deleting non-existent row
    EXPECT_FALSE(users_table->deleteRow(Value(999)));
    EXPECT_EQ(users_table->getRowCount(), 1);
}

TEST_F(CoreDataModelTest, TableFindRowsByColumn) {
    // Insert test data
    users_table->insertRow({Value(1), Value("Alice"), Value(28), Value("alice@test.com")});
    users_table->insertRow({Value(2), Value("Bob"), Value(28), Value("bob@test.com")});
    users_table->insertRow({Value(3), Value("Charlie"), Value(30), Value("charlie@test.com")});
    
    // Find all users with age 28
    auto age_28_users = users_table->findRowsByColumn("age", Value(28));
    EXPECT_EQ(age_28_users.size(), 2);
    
    // Find all users with age 30
    auto age_30_users = users_table->findRowsByColumn("age", Value(30));
    EXPECT_EQ(age_30_users.size(), 1);
    EXPECT_EQ(age_30_users[0]->getValue("name").get<std::string>(), "Charlie");
    
    // Find users with non-existent age
    auto no_users = users_table->findRowsByColumn("age", Value(99));
    EXPECT_EQ(no_users.size(), 0);
}

// Database Tests
TEST(DatabaseTest, TableManagement) {
    Database db("test_db");
    
    EXPECT_EQ(db.getName(), "test_db");
    EXPECT_TRUE(db.isEmpty());
    EXPECT_EQ(db.getTableCount(), 0);
    
    // Create a table
    std::vector<Column> schema;
    schema.emplace_back("id", ValueType::Integer32, false, true);
    schema.emplace_back("name", ValueType::String, false, false);
    
    Table* table = db.createTable("test_table", std::move(schema), "id");
    EXPECT_NE(table, nullptr);
    EXPECT_FALSE(db.isEmpty());
    EXPECT_EQ(db.getTableCount(), 1);
    EXPECT_TRUE(db.hasTable("test_table"));
    
    // Test getting table
    Table* retrieved_table = db.getTable("test_table");
    EXPECT_EQ(retrieved_table, table);
    
    // Test getting non-existent table
    EXPECT_EQ(db.getTable("non_existent"), nullptr);
    
    // Test duplicate table creation
    std::vector<Column> schema2;
    schema2.emplace_back("id", ValueType::Integer32, false, true);
    EXPECT_THROW(db.createTable("test_table", std::move(schema2), "id"), std::invalid_argument);
}

TEST(DatabaseTest, TableNameRetrieval) {
    Database db("test_db");
    
    // Create multiple tables
    std::vector<Column> schema;
    schema.emplace_back("id", ValueType::Integer32, false, true);
    
    db.createTable("table1", schema, "id");
    db.createTable("table2", schema, "id");
    db.createTable("table3", schema, "id");
    
    auto table_names = db.getTableNames();
    EXPECT_EQ(table_names.size(), 3);
    
    // Note: unordered_map doesn't guarantee order, so we check contains
    EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "table1") != table_names.end());
    EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "table2") != table_names.end());
    EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "table3") != table_names.end());
}

TEST(DatabaseTest, DropTable) {
    Database db("test_db");
    
    std::vector<Column> schema;
    schema.emplace_back("id", ValueType::Integer32, false, true);
    
    db.createTable("test_table", schema, "id");
    EXPECT_TRUE(db.hasTable("test_table"));
    
    // Drop existing table
    EXPECT_TRUE(db.dropTable("test_table"));
    EXPECT_FALSE(db.hasTable("test_table"));
    EXPECT_EQ(db.getTableCount(), 0);
    
    // Drop non-existent table
    EXPECT_FALSE(db.dropTable("non_existent"));
}

TEST(DatabaseTest, SimpleTableCreation) {
    Database db("test_db");
    
    // Create table using the simple interface
    std::vector<std::tuple<std::string, ValueType, bool>> column_specs = {
        {"id", ValueType::Integer32, false},
        {"name", ValueType::String, false},
        {"active", ValueType::Boolean, true}
    };
    
    Table* table = db.createSimpleTable("simple_table", column_specs, "id");
    EXPECT_NE(table, nullptr);
    EXPECT_EQ(table->getSchema().size(), 3);
    EXPECT_EQ(table->getPrimaryKeyColumnName(), "id");
    
    // Verify primary key column properties
    const auto& schema = table->getSchema();
    const auto& pk_column = schema[0]; // Should be "id"
    EXPECT_FALSE(pk_column.isNullable());
    EXPECT_TRUE(pk_column.isUnique());
}

// Integration Tests
TEST_F(CoreDataModelTest, CompleteWorkflow) {
    // This test demonstrates a complete workflow with all CRUD operations
    
    // 1. Insert multiple users
    users_table->insertRow({Value(1), Value("Alice"), Value(28), Value("alice@test.com")});
    users_table->insertRow({Value(2), Value("Bob"), Value(30), Value("bob@test.com")});
    users_table->insertRow({Value(3), Value("Charlie"), Value(25), Value("charlie@test.com")});
    
    EXPECT_EQ(users_table->getRowCount(), 3);
    
    // 2. Query users
    const Row* alice = users_table->findRowByPK(Value(1));
    ASSERT_NE(alice, nullptr);
    EXPECT_EQ(alice->getValue("name").get<std::string>(), "Alice");
    
    // 3. Update a user
    users_table->updateRow(Value(2), {Value(2), Value("Robert"), Value(31), Value("robert@test.com")});
    const Row* updated_user = users_table->findRowByPK(Value(2));
    EXPECT_EQ(updated_user->getValue("name").get<std::string>(), "Robert");
    EXPECT_EQ(updated_user->getValue("age").get<int32_t>(), 31);
    
    // 4. Delete a user
    users_table->deleteRow(Value(3));
    EXPECT_EQ(users_table->getRowCount(), 2);
    EXPECT_EQ(users_table->findRowByPK(Value(3)), nullptr);
    
    // 5. Verify remaining users
    EXPECT_NE(users_table->findRowByPK(Value(1)), nullptr);
    EXPECT_NE(users_table->findRowByPK(Value(2)), nullptr);
    
    // 6. Test database statistics
    auto db_stats = db->getStats();
    EXPECT_EQ(db_stats.table_count, 1);
    EXPECT_EQ(db_stats.total_row_count, 2);
    EXPECT_GT(db_stats.total_memory_estimate, 0);
} 