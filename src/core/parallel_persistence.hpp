#pragma once

#include "database.hpp"
#include "threadpool.hpp"
#include <future>
#include <vector>
#include <chrono>

namespace scalerdb {

/**
 * @brief Parallel persistence manager for high-performance database I/O
 * 
 * Addresses the major bottleneck identified in performance tests where
 * load operations take 71% of total time by parallelizing table processing.
 */
class ParallelPersistence {
private:
    ThreadPool& thread_pool_;
    
public:
    explicit ParallelPersistence(ThreadPool& pool) : thread_pool_(pool) {}
    
    /**
     * @brief Save database using parallel table serialization
     * 
     * Performance improvement: Each table serialized in parallel threads,
     * then results are concatenated. Expected 2-4x speedup for multi-table databases.
     */
    bool saveParallel(const Database& database, const std::string& filename);
    
    /**
     * @brief Load database using parallel chunk processing
     * 
     * Performance improvement: File is memory-mapped and parsed in chunks
     * to avoid the 13+ second load times we observed in benchmarks.
     */
    bool loadParallel(Database& database, const std::string& filename);
    
private:
    /**
     * @brief Serialize a single table to JSON string
     */
    std::string serializeTable(const Table& table, const std::string& table_name);
    
    /**
     * @brief Parse table from JSON chunk with row count hint
     */
    bool parseTableChunk(Database& database, const std::string& json_chunk, size_t expected_rows);
    
    /**
     * @brief Memory-map file for efficient parsing
     */
    struct MappedFile {
        const char* data;
        size_t size;
        void* handle; // Platform-specific handle
        
        MappedFile(const std::string& filename);
        ~MappedFile();
        bool isValid() const { return data != nullptr; }
    };
};

/**
 * @brief High-performance batch operations for bulk inserts
 * 
 * Addresses table_creation overhead (775ms average) by optimizing
 * batch insertions during database loading.
 */
class BatchInserter {
private:
    Table& table_;
    std::vector<Row> batch_;
    size_t batch_size_;
    
public:
    BatchInserter(Table& table, size_t batch_size = 1000) 
        : table_(table), batch_size_(batch_size) {
        batch_.reserve(batch_size_);
    }
    
    ~BatchInserter() {
        flush();
    }
    
    /**
     * @brief Add row to batch (deferred insertion)
     */
    void addRow(Row row) {
        batch_.emplace_back(std::move(row));
        if (batch_.size() >= batch_size_) {
            flush();
        }
    }
    
    /**
     * @brief Insert all batched rows with single lock acquisition
     * 
     * Performance improvement: Reduces lock overhead from O(n) to O(1)
     * Expected 5-10x speedup for bulk operations.
     */
    void flush();
};

/**
 * @brief Performance monitoring for persistence operations
 */
struct PersistenceMetrics {
    std::chrono::nanoseconds parse_time{0};
    std::chrono::nanoseconds serialize_time{0};
    std::chrono::nanoseconds io_time{0};
    size_t bytes_processed{0};
    size_t rows_processed{0};
    
    double getThroughputMBps() const {
        auto total_time = parse_time + serialize_time + io_time;
        if (total_time.count() == 0) return 0.0;
        return (bytes_processed / (1024.0 * 1024.0)) / (total_time.count() / 1e9);
    }
    
    void printReport() const {
        std::cout << "\n=== Persistence Performance Report ===\n";
        std::cout << "Bytes processed: " << (bytes_processed / 1024.0) << " KB\n";
        std::cout << "Rows processed: " << rows_processed << "\n";
        std::cout << "Parse time: " << (parse_time.count() / 1e6) << " ms\n";
        std::cout << "Serialize time: " << (serialize_time.count() / 1e6) << " ms\n";
        std::cout << "I/O time: " << (io_time.count() / 1e6) << " ms\n";
        std::cout << "Throughput: " << getThroughputMBps() << " MB/s\n";
    }
};

} // namespace scalerdb 