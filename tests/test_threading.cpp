#include <gtest/gtest.h>
#include "../src/core/database.hpp"
#include "../src/core/table.hpp"
#include "../src/core/threadpool.hpp"
#include <atomic>
#include <chrono>
#include <random>
#include <thread>
#include <vector>
#include <iostream>

using namespace scalerdb;

class ThreadingTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_unique<Database>("threading_test_db");
        
        // Create a test table with integer IDs and data
        std::vector<Column> schema;
        schema.emplace_back("id", ValueType::Integer32, false, true);  // Primary key
        schema.emplace_back("data", ValueType::Integer32, false, false);
        schema.emplace_back("timestamp", ValueType::Integer64, false, false);
        
        test_table = db->createTable("test_table", std::move(schema), "id");
        
        // Initialize with some base data
        for (int i = 0; i < initial_rows; ++i) {
            test_table->insertRow({Value(i), Value(i * 10), Value(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count())});
        }
    }

    std::unique_ptr<Database> db;
    Table* test_table;
    static constexpr int initial_rows = 100;
    static constexpr int test_duration_ms = 2000;
};

/**
 * Test with N reader threads and M writer threads
 * Readers perform findRowByPK operations
 * Writers perform insertRow operations
 * Use atomic counters to track operations and detect issues
 */
TEST_F(ThreadingTest, ReadersWritersStressTest) {
    const int num_readers = 8;
    const int num_writers = 4;
    const int writer_start_id = 1000;  // Start writer IDs from here to avoid conflicts
    
    // Atomic counters for tracking operations
    std::atomic<int> reads_completed{0};
    std::atomic<int> writes_completed{0};
    std::atomic<int> read_errors{0};
    std::atomic<int> write_errors{0};
    std::atomic<bool> stop_flag{false};
    
    // Vector to track successful writes for verification
    std::vector<std::atomic<bool>> write_success(num_writers * 100);
    for (auto& success : write_success) {
        success = false;
    }
    
    ThreadPool thread_pool(num_readers + num_writers);
    std::vector<std::future<void>> futures;
    
    // Submit reader tasks
    for (int reader_id = 0; reader_id < num_readers; ++reader_id) {
        futures.push_back(thread_pool.submit([this, reader_id, &reads_completed, &read_errors, &stop_flag]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, initial_rows - 1);
            
            int local_reads = 0;
            int local_errors = 0;
            
            while (!stop_flag.load()) {
                try {
                    int target_id = dis(gen);
                    const Row* row = test_table->findRowByPK(Value(target_id));
                    
                    if (row) {
                        // Verify data consistency
                        int id = row->getValue("id").get<int32_t>();
                        int data = row->getValue("data").get<int32_t>();
                        
                        if (id * 10 != data && id < initial_rows) {
                            // Data inconsistency detected in original rows
                            local_errors++;
                        }
                    }
                    local_reads++;
                    
                    // Small delay to allow writes to interleave
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                } catch (const std::exception& e) {
                    local_errors++;
                }
            }
            
            reads_completed.fetch_add(local_reads);
            read_errors.fetch_add(local_errors);
        }));
    }
    
    // Submit writer tasks
    for (int writer_id = 0; writer_id < num_writers; ++writer_id) {
        futures.push_back(thread_pool.submit([this, writer_id, writer_start_id, &writes_completed, &write_errors, &stop_flag, &write_success]() {
            int local_writes = 0;
            int local_errors = 0;
            int write_id = writer_start_id + writer_id * 100;
            
            while (!stop_flag.load()) {
                try {
                    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    
                    bool success = test_table->insertRow({
                        Value(write_id),
                        Value(write_id * 10), 
                        Value(now)
                    });
                    
                    if (success) {
                        write_success[writer_id * 100 + (write_id % 100)] = true;
                        local_writes++;
                    } else {
                        local_errors++;
                    }
                    
                    write_id++;
                    
                    // Small delay to allow reads to interleave
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                } catch (const std::exception& e) {
                    local_errors++;
                    write_id++;  // Move to next ID even on error
                }
            }
            
            writes_completed.fetch_add(local_writes);
            write_errors.fetch_add(local_errors);
        }));
    }
    
    // Let the test run for the specified duration
    std::this_thread::sleep_for(std::chrono::milliseconds(test_duration_ms));
    
    // Signal all threads to stop
    stop_flag = true;
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    // Verify results
    std::cout << "=== Threading Test Results ===" << std::endl;
    std::cout << "Reads completed: " << reads_completed.load() << std::endl;
    std::cout << "Writes completed: " << writes_completed.load() << std::endl;
    std::cout << "Read errors: " << read_errors.load() << std::endl;
    std::cout << "Write errors: " << write_errors.load() << std::endl;
    std::cout << "Final table size: " << test_table->getRowCount() << std::endl;
    
    // Assertions
    EXPECT_GT(reads_completed.load(), 0) << "No reads were completed";
    EXPECT_GT(writes_completed.load(), 0) << "No writes were completed";
    EXPECT_EQ(read_errors.load(), 0) << "Data consistency errors detected in reads";
    
    // Verify data integrity of original rows
    for (int i = 0; i < initial_rows; ++i) {
        const Row* row = test_table->findRowByPK(Value(i));
        ASSERT_NE(row, nullptr) << "Original row " << i << " was lost";
        
        int id = row->getValue("id").get<int32_t>();
        int data = row->getValue("data").get<int32_t>();
        EXPECT_EQ(id * 10, data) << "Data corruption in row " << i;
    }
    
    // Verify table size consistency
    size_t expected_min_size = initial_rows + writes_completed.load();
    EXPECT_GE(test_table->getRowCount(), expected_min_size) 
        << "Table size doesn't match expected minimum";
}

/**
 * Test for deadlock detection
 * This test creates a scenario where threads might potentially deadlock
 * if locking isn't implemented correctly
 */
TEST_F(ThreadingTest, DeadlockDetectionTest) {
    const int num_threads = 16;
    const int operations_per_thread = 50;
    
    std::atomic<int> operations_completed{0};
    std::atomic<int> timeouts{0};
    
    ThreadPool thread_pool(num_threads);
    std::vector<std::future<void>> futures;
    
    for (int thread_id = 0; thread_id < num_threads; ++thread_id) {
        futures.push_back(thread_pool.submit([this, thread_id, operations_per_thread, &operations_completed, &timeouts]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> read_dis(0, initial_rows - 1);
            
            for (int op = 0; op < operations_per_thread; ++op) {
                auto start_time = std::chrono::steady_clock::now();
                
                try {
                    // Mix of read and write operations
                    if (op % 3 == 0) {
                        // Write operation
                        int write_id = 2000 + thread_id * 1000 + op;
                        test_table->insertRow({
                            Value(write_id),
                            Value(write_id * 10),
                            Value(std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count())
                        });
                    } else {
                        // Read operation
                        int read_id = read_dis(gen);
                        test_table->findRowByPK(Value(read_id));
                    }
                    
                    auto end_time = std::chrono::steady_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    
                    // If any operation takes more than 1 second, consider it a potential deadlock
                    if (duration.count() > 1000) {
                        timeouts.fetch_add(1);
                    }
                    
                    operations_completed.fetch_add(1);
                } catch (const std::exception& e) {
                    // Handle exceptions but continue
                }
            }
        }));
    }
    
    // Wait for all operations to complete with a timeout
    auto start_time = std::chrono::steady_clock::now();
    const auto timeout_duration = std::chrono::seconds(30);
    
    for (auto& future : futures) {
        auto remaining_time = timeout_duration - (std::chrono::steady_clock::now() - start_time);
        if (remaining_time <= std::chrono::milliseconds(0)) {
            ADD_FAILURE() << "Test timed out - possible deadlock detected";
            break;
        }
        
        if (future.wait_for(remaining_time) != std::future_status::ready) {
            ADD_FAILURE() << "Thread didn't complete within timeout - possible deadlock";
            break;
        }
    }
    
    std::cout << "=== Deadlock Detection Test Results ===" << std::endl;
    std::cout << "Operations completed: " << operations_completed.load() << std::endl;
    std::cout << "Timeouts (potential deadlocks): " << timeouts.load() << std::endl;
    
    // Verify no deadlocks occurred
    EXPECT_EQ(timeouts.load(), 0) << "Potential deadlocks detected (operations taking >1s)";
    EXPECT_EQ(operations_completed.load(), num_threads * operations_per_thread) 
        << "Not all operations completed";
}

/**
 * Test concurrent reads only (should all succeed with shared locks)
 */
TEST_F(ThreadingTest, ConcurrentReadsTest) {
    const int num_readers = 20;
    const int reads_per_thread = 100;
    
    std::atomic<int> total_reads{0};
    std::atomic<int> successful_reads{0};
    
    ThreadPool thread_pool(num_readers);
    std::vector<std::future<void>> futures;
    
    for (int reader_id = 0; reader_id < num_readers; ++reader_id) {
        futures.push_back(thread_pool.submit([this, reads_per_thread, &total_reads, &successful_reads]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, initial_rows - 1);
            
            for (int read = 0; read < reads_per_thread; ++read) {
                int target_id = dis(gen);
                const Row* row = test_table->findRowByPK(Value(target_id));
                
                total_reads.fetch_add(1);
                if (row != nullptr) {
                    successful_reads.fetch_add(1);
                }
            }
        }));
    }
    
    // Wait for all reads to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    std::cout << "=== Concurrent Reads Test Results ===" << std::endl;
    std::cout << "Total reads: " << total_reads.load() << std::endl;
    std::cout << "Successful reads: " << successful_reads.load() << std::endl;
    
    // All reads should succeed since we're only reading existing data
    EXPECT_EQ(total_reads.load(), num_readers * reads_per_thread);
    EXPECT_EQ(successful_reads.load(), total_reads.load()) << "Some reads failed unexpectedly";
}

/**
 * Test that demonstrates the locking is working correctly
 * by showing that operations are properly serialized
 */
TEST_F(ThreadingTest, LockingVerificationTest) {
    const int num_writers = 5;
    const int writes_per_thread = 20;
    
    std::atomic<int> write_conflicts{0};
    std::vector<std::atomic<int>> writer_counts(num_writers);
    for (auto& count : writer_counts) {
        count = 0;
    }
    
    ThreadPool thread_pool(num_writers);
    std::vector<std::future<void>> futures;
    
    for (int writer_id = 0; writer_id < num_writers; ++writer_id) {
        futures.push_back(thread_pool.submit([this, writer_id, writes_per_thread, &write_conflicts, &writer_counts]() {
            for (int write = 0; write < writes_per_thread; ++write) {
                try {
                    int write_id = 3000 + writer_id * 1000 + write;
                    bool success = test_table->insertRow({
                        Value(write_id),
                        Value(write_id * 10),
                        Value(std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count())
                    });
                    
                    if (success) {
                        writer_counts[writer_id].fetch_add(1);
                    } else {
                        write_conflicts.fetch_add(1);
                    }
                } catch (const std::exception& e) {
                    write_conflicts.fetch_add(1);
                }
            }
        }));
    }
    
    // Wait for all writes to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    int total_successful_writes = 0;
    for (int i = 0; i < num_writers; ++i) {
        total_successful_writes += writer_counts[i].load();
    }
    
    std::cout << "=== Locking Verification Test Results ===" << std::endl;
    std::cout << "Total successful writes: " << total_successful_writes << std::endl;
    std::cout << "Write conflicts: " << write_conflicts.load() << std::endl;
    
    // Since each writer uses different IDs, there should be no conflicts
    EXPECT_EQ(write_conflicts.load(), 0) << "Unexpected write conflicts detected";
    EXPECT_EQ(total_successful_writes, num_writers * writes_per_thread) 
        << "Not all writes completed successfully";
} 