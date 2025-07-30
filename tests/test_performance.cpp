#include <gtest/gtest.h>
#include "../src/core/database.hpp"
#include "../src/core/table.hpp"
#include "../src/core/threadpool.hpp"
#include <chrono>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <iomanip>
#include <thread>
#include <atomic>

using namespace scalerdb;
using namespace std::chrono;

/**
 * @brief Performance statistics calculator
 */
class PerformanceStats {
private:
    std::vector<double> latencies_ms_;
    
public:
    void addLatency(const std::chrono::nanoseconds& duration) {
        double ms = duration.count() / 1e6;
        latencies_ms_.push_back(ms);
    }
    
    void addLatency(double ms) {
        latencies_ms_.push_back(ms);
    }
    
    struct Stats {
        double min_ms = 0.0;
        double max_ms = 0.0;
        double mean_ms = 0.0;
        double p50_ms = 0.0;  // Median
        double p95_ms = 0.0;
        double p99_ms = 0.0;
        double p999_ms = 0.0;
        size_t count = 0;
        double total_ms = 0.0;
        double throughput_ops_per_sec = 0.0;
    };
    
    Stats calculate() const {
        Stats stats;
        if (latencies_ms_.empty()) return stats;
        
        std::vector<double> sorted = latencies_ms_;
        std::sort(sorted.begin(), sorted.end());
        
        stats.count = sorted.size();
        stats.min_ms = sorted.front();
        stats.max_ms = sorted.back();
        stats.total_ms = std::accumulate(sorted.begin(), sorted.end(), 0.0);
        stats.mean_ms = stats.total_ms / stats.count;
        stats.throughput_ops_per_sec = (stats.count * 1000.0) / stats.total_ms;
        
        // Calculate percentiles
        auto percentile = [&](double p) {
            size_t index = static_cast<size_t>((p / 100.0) * (stats.count - 1));
            return sorted[index];
        };
        
        stats.p50_ms = percentile(50.0);
        stats.p95_ms = percentile(95.0);
        stats.p99_ms = percentile(99.0);
        stats.p999_ms = percentile(99.9);
        
        return stats;
    }
    
    void clear() {
        latencies_ms_.clear();
    }
    
    void printStats(const std::string& operation) const {
        auto stats = calculate();
        std::cout << "\n=== " << operation << " Performance Stats ===\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Count:      " << stats.count << " operations\n";
        std::cout << "Total Time: " << stats.total_ms << " ms\n";
        std::cout << "Throughput: " << stats.throughput_ops_per_sec << " ops/sec\n";
        std::cout << "Latencies (ms):\n";
        std::cout << "  Min:  " << stats.min_ms << "\n";
        std::cout << "  Mean: " << stats.mean_ms << "\n";
        std::cout << "  P50:  " << stats.p50_ms << "\n";
        std::cout << "  P95:  " << stats.p95_ms << "\n";
        std::cout << "  P99:  " << stats.p99_ms << "\n";
        std::cout << "  P99.9:" << stats.p999_ms << "\n";
        std::cout << "  Max:  " << stats.max_ms << "\n";
    }
};

/**
 * @brief Simple profiler for hot spot detection
 */
class SimpleProfiler {
private:
    struct ProfilePoint {
        std::string name;
        std::atomic<uint64_t> call_count{0};
        std::atomic<uint64_t> total_time_ns{0};
        std::atomic<uint64_t> max_time_ns{0};
        
        ProfilePoint() = default;
        ProfilePoint(const ProfilePoint& other) 
            : name(other.name)
            , call_count(other.call_count.load())
            , total_time_ns(other.total_time_ns.load())
            , max_time_ns(other.max_time_ns.load()) {}
        
        ProfilePoint& operator=(const ProfilePoint& other) {
            if (this != &other) {
                name = other.name;
                call_count.store(other.call_count.load());
                total_time_ns.store(other.total_time_ns.load());
                max_time_ns.store(other.max_time_ns.load());
            }
            return *this;
        }
        
        ProfilePoint(ProfilePoint&& other) noexcept 
            : name(std::move(other.name))
            , call_count(other.call_count.load())
            , total_time_ns(other.total_time_ns.load())
            , max_time_ns(other.max_time_ns.load()) {}
        
        ProfilePoint& operator=(ProfilePoint&& other) noexcept {
            if (this != &other) {
                name = std::move(other.name);
                call_count.store(other.call_count.load());
                total_time_ns.store(other.total_time_ns.load());
                max_time_ns.store(other.max_time_ns.load());
            }
            return *this;
        }
    };
    
    static std::vector<ProfilePoint> profile_points_;
    
public:
    class ScopedTimer {
    private:
        ProfilePoint& point_;
        high_resolution_clock::time_point start_;
        
    public:
        ScopedTimer(ProfilePoint& point) : point_(point), start_(high_resolution_clock::now()) {}
        
        ~ScopedTimer() {
            auto duration = high_resolution_clock::now() - start_;
            uint64_t ns = duration.count();
            point_.call_count.fetch_add(1);
            point_.total_time_ns.fetch_add(ns);
            
            // Update max time (simple approach, not perfectly thread-safe but good enough)
            uint64_t current_max = point_.max_time_ns.load();
            while (ns > current_max && !point_.max_time_ns.compare_exchange_weak(current_max, ns)) {
                // Retry if someone else updated max_time_ns
            }
        }
    };
    
    static size_t registerPoint(const std::string& name) {
        ProfilePoint point;
        point.name = name;
        profile_points_.push_back(std::move(point));
        return profile_points_.size() - 1;
    }
    
    static ScopedTimer time(size_t point_id) {
        return ScopedTimer(profile_points_[point_id]);
    }
    
    static void printResults() {
        std::cout << "\n=== Profiling Results ===\n";
        std::cout << std::left << std::setw(25) << "Function" 
                  << std::setw(12) << "Calls" 
                  << std::setw(15) << "Total (ms)"
                  << std::setw(15) << "Avg (μs)"
                  << std::setw(15) << "Max (μs)" << "\n";
        std::cout << std::string(80, '-') << "\n";
        
        for (const auto& point : profile_points_) {
            if (point.call_count.load() > 0) {
                double total_ms = point.total_time_ns.load() / 1e6;
                double avg_us = point.total_time_ns.load() / (1e3 * point.call_count.load());
                double max_us = point.max_time_ns.load() / 1e3;
                
                std::cout << std::left << std::setw(25) << point.name
                          << std::setw(12) << point.call_count.load()
                          << std::setw(15) << std::fixed << std::setprecision(3) << total_ms
                          << std::setw(15) << std::fixed << std::setprecision(1) << avg_us
                          << std::setw(15) << std::fixed << std::setprecision(1) << max_us << "\n";
            }
        }
    }
    
    static void reset() {
        for (auto& point : profile_points_) {
            point.call_count = 0;
            point.total_time_ns = 0;
            point.max_time_ns = 0;
        }
    }
};

std::vector<SimpleProfiler::ProfilePoint> SimpleProfiler::profile_points_;

// Profiling macros
#define PROFILE_POINT(name) \
    static size_t __prof_id = SimpleProfiler::registerPoint(name); \
    auto __prof_timer = SimpleProfiler::time(__prof_id);

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        SimpleProfiler::reset();
    }
    
    void TearDown() override {
        SimpleProfiler::printResults();
    }
};

/**
 * Test persistence performance with various data sizes
 */
TEST_F(PerformanceTest, PersistencePerformance) {
    const std::vector<size_t> data_sizes = {100, 1000, 10000, 50000};
    
    for (size_t size : data_sizes) {
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "Testing persistence with " << size << " records\n";
        std::cout << std::string(60, '=') << "\n";
        
        PerformanceStats save_stats, load_stats;
        Database db("perf_test_db");
        
        // Create test table
        {
            PROFILE_POINT("table_creation");
            std::vector<Column> schema;
            schema.emplace_back("id", ValueType::Integer32, false, true);
            schema.emplace_back("name", ValueType::String, false, false);
            schema.emplace_back("value", ValueType::Double, false, false);
            schema.emplace_back("timestamp", ValueType::Integer64, false, false);
            
            auto* table = db.createTable("test_data", std::move(schema), "id");
            
            // Insert test data
            std::cout << "Inserting " << size << " records...\n";
            auto insert_start = high_resolution_clock::now();
            
            for (size_t i = 0; i < size; ++i) {
                PROFILE_POINT("row_insertion");
                table->insertRow({
                    Value(static_cast<int32_t>(i)),
                    Value("User_" + std::to_string(i)),
                    Value(i * 3.14159),
                    Value(static_cast<int64_t>(duration_cast<milliseconds>(
                        system_clock::now().time_since_epoch()).count()))
                });
            }
            
            auto insert_duration = high_resolution_clock::now() - insert_start;
            double insert_time_ms = duration_cast<nanoseconds>(insert_duration).count() / 1e6;
            std::cout << "Insert time: " << insert_time_ms << " ms ("
                      << (size * 1000.0 / insert_time_ms) << " ops/sec)\n";
        }
        
        // Test save performance
        {
            std::cout << "Testing save performance...\n";
            std::string filename = "perf_test_" + std::to_string(size) + ".json";
            
            for (int trial = 0; trial < 5; ++trial) {
                PROFILE_POINT("database_save");
                auto start = high_resolution_clock::now();
                bool success = db.save(filename);
                auto duration = high_resolution_clock::now() - start;
                
                EXPECT_TRUE(success);
                save_stats.addLatency(duration);
            }
            
            save_stats.printStats("Database Save (" + std::to_string(size) + " records)");
        }
        
        // Test load performance
        {
            std::cout << "Testing load performance...\n";
            std::string filename = "perf_test_" + std::to_string(size) + ".json";
            
            for (int trial = 0; trial < 5; ++trial) {
                PROFILE_POINT("database_load");
                Database load_db;
                auto start = high_resolution_clock::now();
                bool success = load_db.load(filename);
                auto duration = high_resolution_clock::now() - start;
                
                EXPECT_TRUE(success);
                EXPECT_EQ(load_db.getTable("test_data")->getRowCount(), size);
                load_stats.addLatency(duration);
            }
            
            load_stats.printStats("Database Load (" + std::to_string(size) + " records)");
        }
        
        // Calculate data rates
        {
            std::ifstream file("perf_test_" + std::to_string(size) + ".json", std::ios::ate);
            if (file.is_open()) {
                size_t file_size = file.tellg();
                file.close();
                
                auto save_stats_calc = save_stats.calculate();
                auto load_stats_calc = load_stats.calculate();
                
                double save_mbps = (file_size / (1024.0 * 1024.0)) / (save_stats_calc.mean_ms / 1000.0);
                double load_mbps = (file_size / (1024.0 * 1024.0)) / (load_stats_calc.mean_ms / 1000.0);
                
                std::cout << "\nFile size: " << (file_size / 1024.0) << " KB\n";
                std::cout << "Save rate: " << save_mbps << " MB/s\n";
                std::cout << "Load rate: " << load_mbps << " MB/s\n";
                
                // Clean up
                std::remove(("perf_test_" + std::to_string(size) + ".json").c_str());
            }
        }
    }
}

/**
 * Test concurrent operation latencies
 */
TEST_F(PerformanceTest, ConcurrentOperationLatencies) {
    const int num_threads = 8;
    const int operations_per_thread = 1000;
    
    std::cout << "\nTesting concurrent operation latencies with " 
              << num_threads << " threads, " << operations_per_thread 
              << " ops each\n";
    
    Database db("concurrent_perf_test");
    
    // Create test table
    std::vector<Column> schema;
    schema.emplace_back("id", ValueType::Integer32, false, true);
    schema.emplace_back("data", ValueType::String, false, false);
    
    auto* table = db.createTable("concurrent_test", std::move(schema), "id");
    
    // Pre-populate with some data for reads
    for (int i = 0; i < 1000; ++i) {
        table->insertRow({Value(i), Value("Data_" + std::to_string(i))});
    }
    
    ThreadPool thread_pool(num_threads);
    std::vector<std::future<PerformanceStats>> futures;
    
    std::atomic<int> next_write_id{10000}; // Start writes from 10000 to avoid conflicts
    
    // Launch threads
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        futures.push_back(thread_pool.submit([this, table, thread_id, operations_per_thread, &next_write_id]() {
            PerformanceStats thread_stats;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> read_dis(0, 999);
            std::uniform_int_distribution<> operation_dis(0, 2); // 0=read, 1=write, 2=read
            
            for (int op = 0; op < operations_per_thread; ++op) {
                int operation_type = operation_dis(gen);
                
                if (operation_type == 1) { // Write operation
                    PROFILE_POINT("concurrent_write");
                    auto start = high_resolution_clock::now();
                    
                    int write_id = next_write_id.fetch_add(1);
                    try {
                        table->insertRow({
                            Value(write_id),
                            Value("ConcurrentData_" + std::to_string(write_id))
                        });
                    } catch (const std::exception& e) {
                        // Handle duplicate key errors gracefully
                    }
                    
                    auto duration = high_resolution_clock::now() - start;
                    thread_stats.addLatency(duration);
                } else { // Read operation
                    PROFILE_POINT("concurrent_read");
                    auto start = high_resolution_clock::now();
                    
                    int read_id = read_dis(gen);
                    table->findRowByPK(Value(read_id));
                    
                    auto duration = high_resolution_clock::now() - start;
                    thread_stats.addLatency(duration);
                }
            }
            
            return thread_stats;
        }));
    }
    
    // Collect results
    PerformanceStats combined_stats;
    for (auto& future : futures) {
        auto thread_stats = future.get();
        auto stats = thread_stats.calculate();
        for (size_t i = 0; i < stats.count; ++i) {
            // Note: We can't get individual latencies easily, so we approximate
            combined_stats.addLatency(stats.mean_ms);
        }
    }
    
    combined_stats.printStats("Concurrent Operations");
    
    std::cout << "Final table size: " << table->getRowCount() << " rows\n";
}

/**
 * Test cache behavior simulation
 */
TEST_F(PerformanceTest, CacheBehaviorTest) {
    std::cout << "\nTesting cache behavior patterns...\n";
    
    Database db("cache_test");
    std::vector<Column> schema;
    schema.emplace_back("id", ValueType::Integer32, false, true);
    schema.emplace_back("payload", ValueType::String, false, false);
    
    auto* table = db.createTable("cache_test", std::move(schema), "id");
    
    // Create data with varying payload sizes to test cache behavior
    const int num_records = 10000;
    std::cout << "Creating " << num_records << " records with varying payload sizes...\n";
    
    PerformanceStats insert_stats;
    for (int i = 0; i < num_records; ++i) {
        PROFILE_POINT("cache_test_insert");
        
        // Create payloads of different sizes to cause cache misses
        std::string payload(100 + (i % 1000), 'A' + (i % 26));
        
        auto start = high_resolution_clock::now();
        table->insertRow({Value(i), Value(payload)});
        auto duration = high_resolution_clock::now() - start;
        
        insert_stats.addLatency(duration);
    }
    
    insert_stats.printStats("Variable-Size Inserts");
    
    // Test sequential vs random access patterns
    std::cout << "Testing access patterns...\n";
    
    PerformanceStats sequential_stats, random_stats;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_records - 1);
    
    // Sequential access
    for (int i = 0; i < 1000; ++i) {
        PROFILE_POINT("sequential_access");
        auto start = high_resolution_clock::now();
        table->findRowByPK(Value(i));
        auto duration = high_resolution_clock::now() - start;
        sequential_stats.addLatency(duration);
    }
    
    // Random access
    for (int i = 0; i < 1000; ++i) {
        PROFILE_POINT("random_access");
        auto start = high_resolution_clock::now();
        table->findRowByPK(Value(dis(gen)));
        auto duration = high_resolution_clock::now() - start;
        random_stats.addLatency(duration);
    }
    
    sequential_stats.printStats("Sequential Access");
    random_stats.printStats("Random Access");
} 