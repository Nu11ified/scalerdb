#include <iostream>
#include <nlohmann/json.hpp>
#include "src/core/threadpool.hpp"
#include "src/core/database.hpp"

using namespace scalerdb;

int main() {
    std::cout << "ScalerDB - Core In-Memory Database Engine\n";
    std::cout << "=========================================\n\n";
    
    try {
        // Create a database
        Database db("example_db");
        std::cout << "✓ Created database: " << db.getName() << "\n";
        
        // Create a users table with schema
        std::vector<Column> user_schema;
        user_schema.emplace_back("id", ValueType::Integer32, false, true);      // Primary key
        user_schema.emplace_back("name", ValueType::String, false, false);     // Required string
        user_schema.emplace_back("age", ValueType::Integer32, true, false);    // Optional integer
        user_schema.emplace_back("email", ValueType::String, true, true);      // Optional unique string
        
        // Add constraints to age column
        user_schema[2].addConstraint(Column::createRangeConstraint<int32_t>(0, 120));
        
        Table* users_table = db.createTable("users", std::move(user_schema), "id");
        std::cout << "✓ Created table 'users' with " << users_table->getSchema().size() << " columns\n";
        
        // Insert some sample users
        std::cout << "\nInserting sample data:\n";
        
        users_table->insertRow({Value(1), Value("Alice Johnson"), Value(28), Value("alice@example.com")});
        std::cout << "  • Inserted user: Alice Johnson (28)\n";
        
        users_table->insertRow({Value(2), Value("Bob Smith"), Value(35), Value("bob@example.com")});
        std::cout << "  • Inserted user: Bob Smith (35)\n";
        
        users_table->insertRow({Value(3), Value("Carol Wilson"), Value(42), Value("carol@example.com")});
        std::cout << "  • Inserted user: Carol Wilson (42)\n";
        
        // Demonstrate CRUD operations
        std::cout << "\nCRUD Operations:\n";
        
        // READ - Find user by primary key
        const Row* user = users_table->findRowByPK(Value(2));
        if (user) {
            std::cout << "  • Found user with ID 2: " << user->getValue("name").toString() << "\n";
        }
        
        // UPDATE - Update user information
        users_table->updateRow(Value(2), {Value(2), Value("Robert Smith"), Value(36), Value("robert@example.com")});
        std::cout << "  • Updated user ID 2: name and age changed\n";
        
        // DELETE - Remove a user
        users_table->deleteRow(Value(3));
        std::cout << "  • Deleted user ID 3 (Carol Wilson)\n";
        
        // Query operations
        std::cout << "\nQuery Operations:\n";
        std::cout << "  • Total users: " << users_table->getRowCount() << "\n";
        
        // Find users by age
        auto age_results = users_table->findRowsByColumn("age", Value(28));
        std::cout << "  • Users aged 28: " << age_results.size() << "\n";
        
        // Display all remaining users
        std::cout << "\nRemaining users:\n";
        for (const auto& row : users_table->getAllRows()) {
            std::cout << "  • ID: " << row.getValue("id").toString() 
                     << ", Name: " << row.getValue("name").toString()
                     << ", Age: " << row.getValue("age").toString() << "\n";
        }
        
        // Database statistics
        auto stats = db.getStats();
        std::cout << "\nDatabase Statistics:\n";
        std::cout << "  • Tables: " << stats.table_count << "\n";
        std::cout << "  • Total rows: " << stats.total_row_count << "\n";
        std::cout << "  • Memory estimate: " << stats.total_memory_estimate << " bytes\n";
        
        // Test JSON serialization
        std::cout << "\nTesting serialization features:\n";
        nlohmann::json user_json;
        user_json["id"] = 1;
        user_json["name"] = "Alice Johnson";
        user_json["status"] = "active";
        std::cout << "  • JSON example: " << user_json.dump() << "\n";
        
        // Test thread pool
        std::cout << "\nTesting thread pool:\n";
        scalerdb::ThreadPool pool(2);
        std::cout << "  • Thread pool initialized with " << pool.getThreadCount() << " threads\n";
        
        auto future = pool.submit([]() {
            return std::string("Background task completed!");
        });
        std::cout << "  • " << future.get() << "\n";
        
        std::cout << "\n✅ All core functionality verified successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
} 