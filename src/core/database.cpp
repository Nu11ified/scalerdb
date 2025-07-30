#include "database.hpp"
#include "msgpack_types.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace scalerdb {

bool Database::save(const std::string& filename) const {
    try {
        // Create serializable representation
        SerializableDatabase serializable_db(*this);
        
        // Serialize to JSON (msgpack-compatible structure)
        nlohmann::json json_data = serializable_db;
        
        // Write to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        
        file << json_data.dump(4);  // Pretty print for debugging
        file.close();
        
        if (file.fail()) {
            std::cerr << "Failed to write data to file: " << filename << std::endl;
            return false;
        }
        
        std::cout << "Database successfully saved to: " << filename << std::endl;
        std::cout << "Saved " << tables_.size() << " tables" << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving database: " << e.what() << std::endl;
        return false;
    }
}

bool Database::load(const std::string& filename) {
    try {
        // Read JSON file
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return false;
        }
        
        nlohmann::json json_data;
        file >> json_data;
        file.close();
        
        if (file.fail()) {
            std::cerr << "Failed to read data from file: " << filename << std::endl;
            return false;
        }
        
        // Deserialize from JSON
        SerializableDatabase serializable_db = json_data;
        
        // Clear current state and load new data
        tables_.clear();
        
        // Convert back to Database format
        Database loaded_db = serializable_db.toDatabase();
        
        // Move the loaded tables to this database
        tables_ = std::move(loaded_db.tables_);
        
        std::cout << "Database successfully loaded from: " << filename << std::endl;
        std::cout << "Loaded " << tables_.size() << " tables" << std::endl;
        
        // Print summary of loaded tables
        for (const auto& pair : tables_) {
            std::cout << "  - Table '" << pair.first << "': " 
                      << pair.second->getRowCount() << " rows" << std::endl;
        }
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading database: " << e.what() << std::endl;
        return false;
    }
}

} // namespace scalerdb