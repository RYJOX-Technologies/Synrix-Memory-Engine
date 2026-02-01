#ifndef INTELLIGENT_LATTICE_LOADER_H
#define INTELLIGENT_LATTICE_LOADER_H

#include "persistent_lattice.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// NVMe wear protection and intelligent caching
typedef struct {
    // Write batching for wear reduction
    uint32_t write_batch_size;              // Batch writes to reduce cycles
    uint32_t pending_writes;                // Number of pending writes
    uint32_t* dirty_node_ids;               // Nodes that need writing
    lattice_node_t* dirty_nodes;            // Dirty node data
    uint64_t last_write_time;               // Last batch write time
    uint64_t write_batch_timeout_ms;        // Max time before forced write
    
    // Access pattern tracking for intelligent prefetching
    uint32_t* access_frequency;             // How often each node is accessed
    uint32_t* access_recency;               // How recently each node was accessed
    uint32_t* access_patterns[1000];        // Access pattern sequences
    uint32_t pattern_count;                 // Number of tracked patterns
    
    // Memory management with compression
    uint8_t* compressed_cache;              // Compressed node cache
    size_t compressed_size;                 // Current compressed cache size
    size_t max_compressed_size;             // Maximum compressed cache size
    uint32_t* compression_map;              // Maps node_id to compressed offset
    
    // Wear leveling and write optimization
    uint64_t total_writes;                  // Total write operations
    uint64_t total_reads;                   // Total read operations
    uint64_t write_amplification;           // Write amplification factor
    uint32_t wear_leveling_offset;          // Offset for wear leveling
    
    // Predictive loading
    bool predictive_loading_enabled;        // Enable predictive prefetching
    uint32_t prefetch_window;               // How many nodes to prefetch
    uint32_t* prefetch_candidates;          // Nodes to prefetch next
    uint32_t current_time;                  // Current access time counter
    
    // Statistics and monitoring
    double cache_hit_rate;                  // Current cache hit rate
    double compression_ratio;               // Current compression ratio
    uint64_t nvme_wear_cycles;              // Estimated NVMe wear cycles
    uint64_t bytes_saved_compression;       // Bytes saved through compression
} intelligent_cache_t;

// Intelligent lattice loader with NVMe wear protection
typedef struct {
    persistent_lattice_t* lattice;           // Base lattice structure
    intelligent_cache_t* cache;             // Intelligent caching system
    void* nvme_interface;                   // NVMe interface (C++ bridge)
    
    // Configuration
    uint32_t max_ram_nodes;                 // Maximum nodes to keep in RAM
    uint32_t write_batch_size;              // Batch size for writes
    uint64_t write_batch_timeout_ms;        // Timeout for batch writes
    bool compression_enabled;               // Enable compression
    bool wear_leveling_enabled;             // Enable wear leveling
    bool predictive_loading_enabled;        // Enable predictive loading
    
    // Statistics
    uint64_t total_operations;              // Total operations performed
    uint64_t cache_hits;                    // Cache hits
    uint64_t cache_misses;                  // Cache misses
    uint64_t nvme_reads;                    // NVMe read operations
    uint64_t nvme_writes;                   // NVMe write operations
    uint64_t bytes_compressed;              // Total bytes compressed
    uint64_t wear_cycles_saved;             // Wear cycles saved through optimization
    
    bool initialized;
} intelligent_lattice_loader_t;

// Create intelligent lattice loader with wear protection
intelligent_lattice_loader_t* create_intelligent_lattice_loader(
    const char* lattice_path,
    const char* nvme_path,
    uint32_t max_ram_nodes,
    uint32_t write_batch_size,
    uint64_t write_batch_timeout_ms,
    bool compression_enabled,
    bool wear_leveling_enabled,
    bool predictive_loading_enabled);

// Destroy intelligent lattice loader
void destroy_intelligent_lattice_loader(intelligent_lattice_loader_t* loader);

// Smart node loading with predictive prefetching
lattice_node_t* load_node_intelligent(intelligent_lattice_loader_t* loader, 
                                     uint32_t node_id,
                                     bool prefetch_related);

// Batch load nodes with intelligent batching
int load_nodes_batch_intelligent(intelligent_lattice_loader_t* loader,
                                const uint32_t* node_ids,
                                uint32_t count,
                                lattice_node_t* nodes_out);

// Smart node saving with write batching and compression
int save_node_intelligent(intelligent_lattice_loader_t* loader,
                         uint32_t node_id,
                         const lattice_node_t* node,
                         bool force_write);

// Batch save nodes with wear optimization
int save_nodes_batch_intelligent(intelligent_lattice_loader_t* loader,
                                const uint32_t* node_ids,
                                const lattice_node_t* nodes,
                                uint32_t count,
                                bool force_write);

// Flush pending writes to NVMe (with batching)
int flush_pending_writes(intelligent_lattice_loader_t* loader);

// Prefetch related nodes based on access patterns
int prefetch_related_nodes(intelligent_lattice_loader_t* loader,
                          uint32_t node_id,
                          uint32_t max_count);

// Update access patterns for intelligent prefetching
void update_access_pattern(intelligent_lattice_loader_t* loader,
                          uint32_t node_id,
                          uint32_t access_type);

// Get comprehensive statistics
void get_intelligent_loader_stats(intelligent_lattice_loader_t* loader,
                                 uint32_t* loaded_nodes,
                                 uint32_t* total_nodes,
                                 size_t* memory_usage_mb,
                                 double* cache_hit_rate,
                                 double* compression_ratio,
                                 uint64_t* nvme_wear_cycles,
                                 uint64_t* wear_cycles_saved,
                                 double* write_amplification);

// Optimize cache for current access patterns
int optimize_cache_intelligent(intelligent_lattice_loader_t* loader);

// Enable/disable features dynamically
int set_compression_enabled(intelligent_lattice_loader_t* loader, bool enabled);
int set_wear_leveling_enabled(intelligent_lattice_loader_t* loader, bool enabled);
int set_predictive_loading_enabled(intelligent_lattice_loader_t* loader, bool enabled);

// Emergency flush (force all pending writes)
int emergency_flush(intelligent_lattice_loader_t* loader);

// Estimate remaining NVMe lifespan
uint64_t estimate_nvme_lifespan(intelligent_lattice_loader_t* loader);

// Get wear leveling recommendations
void get_wear_leveling_recommendations(intelligent_lattice_loader_t* loader,
                                      uint32_t* recommended_batch_size,
                                      uint64_t* recommended_timeout_ms,
                                      bool* recommended_compression);

#endif // INTELLIGENT_LATTICE_LOADER_H
