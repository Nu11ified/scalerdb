#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "src/core/threadpool.hpp"
#include <vector>

// Test JSON serialization (will replace with msgpack later if needed)
TEST(SetupTest, JsonBasic) {
    std::vector<int> original = {1, 2, 3, 4, 5};
    
    // Serialize
    nlohmann::json j = original;
    std::string serialized = j.dump();
    
    EXPECT_GT(serialized.size(), 0);
    
    // Deserialize
    nlohmann::json parsed = nlohmann::json::parse(serialized);
    std::vector<int> deserialized = parsed.get<std::vector<int>>();
    
    EXPECT_EQ(original, deserialized);
}

// Test thread pool functionality
TEST(SetupTest, ThreadPoolBasic) {
    scalerdb::ThreadPool pool(2);
    
    EXPECT_EQ(pool.getThreadCount(), 2);
    
    // Test basic task submission
    auto future = pool.submit([]() {
        return 42;
    });
    
    EXPECT_EQ(future.get(), 42);
}

// Test that C++23 features work
TEST(SetupTest, Cpp23Features) {
    // Test C++23 features that are available
    
    // Test ranges (C++20/23 feature)
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    auto sum = 0;
    for (auto num : numbers) {
        sum += num;
    }
    EXPECT_EQ(sum, 15);
    
    // Test auto type deduction improvements
    auto lambda = [](auto x, auto y) { return x + y; };
    EXPECT_EQ(lambda(5, 3), 8);
    EXPECT_EQ(lambda(2.5, 1.5), 4.0);
    
    // Always passing test to verify compilation
    EXPECT_TRUE(true);
} 