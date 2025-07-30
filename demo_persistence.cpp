#include <iostream>
#include "src/core/database.hpp"

using namespace scalerdb;

int main() {
    std::cout << "=== ScalerDB Persistence Demo ===\n\n";
    
    // Create a database and populate it
    {
        std::cout << "1. Creating database and adding data...\n";
        Database db("demo_db");
        
        // Create a table
        std::vector<Column> schema;
        schema.emplace_back("id", ValueType::Integer32, false, true);
        schema.emplace_back("name", ValueType::String, false, false);
        schema.emplace_back("score", ValueType::Double, false, false);
        
        Table* table = db.createTable("highscores", std::move(schema), "id");
        
        // Add some data
        table->insertRow({Value(1), Value("Alice"), Value(95.5)});
        table->insertRow({Value(2), Value("Bob"), Value(87.2)});
        table->insertRow({Value(3), Value("Charlie"), Value(92.1)});
        
        std::cout << "   Added " << table->getRowCount() << " records to 'highscores' table\n";
        
        // Save to file
        std::cout << "2. Saving database to 'demo.json'...\n";
        if (db.save("demo.json")) {
            std::cout << "   ✓ Database saved successfully!\n";
        } else {
            std::cout << "   ✗ Failed to save database\n";
            return 1;
        }
    }
    // Database goes out of scope here - all in-memory data is lost
    
    std::cout << "\n3. Original database destroyed (in-memory data gone)\n";
    
    // Load the database back from file
    {
        std::cout << "4. Loading database from 'demo.json'...\n";
        Database restored_db;
        
        if (restored_db.load("demo.json")) {
            std::cout << "   ✓ Database loaded successfully!\n";
            
            // Verify the data is intact
            Table* table = restored_db.getTable("highscores");
            if (table) {
                std::cout << "   Restored table 'highscores' with " << table->getRowCount() << " records:\n";
                
                for (const auto& row : table->getAllRows()) {
                    std::cout << "     ID: " << row.getValue("id").toString()
                             << ", Name: " << row.getValue("name").toString()
                             << ", Score: " << row.getValue("score").toString() << "\n";
                }
            }
        } else {
            std::cout << "   ✗ Failed to load database\n";
            return 1;
        }
    }
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "ScalerDB is an IN-MEMORY database with OPTIONAL persistence:\n";
    std::cout << "• All operations happen in RAM for maximum speed\n";
    std::cout << "• Can save/load entire database to/from JSON files\n";
    std::cout << "• No automatic persistence - manual save/load required\n";
    std::cout << "• Perfect for high-performance applications that need occasional snapshots\n";
    
    return 0;
} 