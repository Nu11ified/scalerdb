#include <iostream>
#include <nlohmann/json.hpp>
#include "BS_thread_pool.hpp"

int main() {
    std::cout << "ScalerDB - Starting up...\n";
    
    // Test JSON serialization (we'll add msgpack later if needed)
    std::vector<int> data = {1, 2, 3, 4, 5};
    nlohmann::json j = data;
    std::string serialized = j.dump();
    
    std::cout << "JSON serialization test: " << serialized.size() << " bytes\n";
    std::cout << "Serialized data: " << serialized << "\n";
    
    // Test thread pool
    BS::thread_pool pool(4);
    std::cout << "Thread pool initialized with " << pool.get_thread_count() << " threads\n";
    
    std::cout << "Setup successful!\n";
    return 0;
} 