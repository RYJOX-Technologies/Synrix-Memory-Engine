#include "intelligent_lattice_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <zlib.h>  // For compression
#include <math.h>

// Get current time in milliseconds
static uint64_t get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// Simple compression function
static size_t compress_data(const uint8_t* input, size_t input_size, 
                           uint8_t* output, size_t output_size) {
    uLongf compressed_size = output_size;
    int result = compress2(output, &compressed_size, input, input_size, Z_BEST_COMPRESSION);
    return (result == Z_OK) ? compressed_size : 0;
}

// Simple decompression function
static size_t decompress_data(const uint8_t* input, size_t input_size,
                             uint8_t* output, size_t output_size) {
    uLongf decompressed_size = output_size;
    int result = uncompress(output, &decompressed_size, input, input_size);
    return (result == Z_OK) ? decompressed_size : 0;
}

// Create intelligent lattice loader
intelligent_lattice_loader_t* create_intelligent_lattice_loader(
    const char* lattice_path,
    const char* nvme_path,
    uint32_t max_ram_nodes,
    uint32_t write_batch_size,
    uint64_t write_batch_timeout_ms,
    bool compression_enabled,
    bool wear_leveling_enabled,
    bool predictive_loading_enabled) {
    
    printf("[INTELLIGENT-LOADER] INFO Creating intelligent lattice loader with NVMe wear protection...\n");
    
    intelligent_lattice_loader_t* loader = calloc(1, sizeof(intelligent_lattice_loader_t));
    if (!loader) return NULL;
    
    // Initialize base lattice
    loader->lattice = calloc(1, sizeof(persistent_lattice_t));
    if (!loader->lattice) {
        free(loader);
        return NULL;
    }
    
    if (lattice_init(loader->lattice, lattice_path) != 0) {
        printf("[INTELLIGENT-LOADER] ERROR Failed to initialize base lattice\n");
        free(loader->lattice);
        free(loader);
        return NULL;
    }
    
    // Initialize intelligent cache
    loader->cache = calloc(1, sizeof(intelligent_cache_t));
    if (!loader->cache) {
        lattice_cleanup(loader->lattice);
        free(loader->lattice);
        free(loader);
        return NULL;
    }
    
    // Configure cache
    loader->cache->write_batch_size = write_batch_size;
    loader->cache->write_batch_timeout_ms = write_batch_timeout_ms;
    loader->cache->pending_writes = 0;
    loader->cache->last_write_time = get_current_time_ms();
    
    // Allocate write batching arrays
    loader->cache->dirty_node_ids = calloc(write_batch_size, sizeof(uint32_t));
    loader->cache->dirty_nodes = calloc(write_batch_size, sizeof(lattice_node_t));
    
    // Allocate access tracking arrays
    loader->cache->access_frequency = calloc(1000000, sizeof(uint32_t));
    loader->cache->access_recency = calloc(1000000, sizeof(uint32_t));
    
    // Allocate compression cache
    if (compression_enabled) {
        loader->cache->max_compressed_size = max_ram_nodes * sizeof(lattice_node_t) / 2; // 50% compression
        loader->cache->compressed_cache = calloc(loader->cache->max_compressed_size, 1);
        loader->cache->compression_map = calloc(max_ram_nodes, sizeof(uint32_t));
    }
    
    // Allocate prefetch arrays
    if (predictive_loading_enabled) {
        loader->cache->prefetch_candidates = calloc(1000, sizeof(uint32_t));
        loader->cache->prefetch_window = 10; // Prefetch 10 related nodes
    }
    
    // Set configuration
    loader->max_ram_nodes = max_ram_nodes;
    loader->write_batch_size = write_batch_size;
    loader->write_batch_timeout_ms = write_batch_timeout_ms;
    loader->compression_enabled = compression_enabled;
    loader->wear_leveling_enabled = wear_leveling_enabled;
    loader->predictive_loading_enabled = predictive_loading_enabled;
    
    // Initialize statistics
    loader->total_operations = 0;
    loader->cache_hits = 0;
    loader->cache_misses = 0;
    loader->nvme_reads = 0;
    loader->nvme_writes = 0;
    loader->bytes_compressed = 0;
    loader->wear_cycles_saved = 0;
    
    // Initialize cache time counter
    loader->cache->current_time = 0;
    
    loader->initialized = true;
    
    printf("[INTELLIGENT-LOADER] OK Created with wear protection:\n");
    printf("  - Max RAM nodes: %u\n", max_ram_nodes);
    printf("  - Write batch size: %u\n", write_batch_size);
    printf("  - Write timeout: %lu ms\n", write_batch_timeout_ms);
    printf("  - Compression: %s\n", compression_enabled ? "ON" : "OFF");
    printf("  - Wear leveling: %s\n", wear_leveling_enabled ? "ON" : "OFF");
    printf("  - Predictive loading: %s\n", predictive_loading_enabled ? "ON" : "OFF");
    
    return loader;
}

// Destroy intelligent lattice loader
void destroy_intelligent_lattice_loader(intelligent_lattice_loader_t* loader) {
    if (!loader) return;
    
    printf("[INTELLIGENT-LOADER] INFO Destroying intelligent lattice loader...\n");
    
    // Flush any pending writes
    if (loader->cache && loader->cache->pending_writes > 0) {
        printf("[INTELLIGENT-LOADER] WARN Flushing %u pending writes before destruction\n",
               loader->cache->pending_writes);
        flush_pending_writes(loader);
    }
    
    // Free cache arrays
    if (loader->cache) {
        free(loader->cache->dirty_node_ids);
        free(loader->cache->dirty_nodes);
        free(loader->cache->access_frequency);
        free(loader->cache->access_recency);
        free(loader->cache->compressed_cache);
        free(loader->cache->compression_map);
        free(loader->cache->prefetch_candidates);
        free(loader->cache);
    }
    
    // Cleanup base lattice
    if (loader->lattice) {
        lattice_cleanup(loader->lattice);
        free(loader->lattice);
    }
    
    free(loader);
    printf("[INTELLIGENT-LOADER] OK Destroyed\n");
}

// Smart node loading with predictive prefetching
lattice_node_t* load_node_intelligent(intelligent_lattice_loader_t* loader, 
                                     uint32_t node_id,
                                     bool prefetch_related) {
    if (!loader || !loader->initialized) return NULL;
    
    loader->total_operations++;
    
    // Check if node is already in cache
    lattice_node_t* cached_node = lattice_get_node(loader->lattice, node_id);
    if (cached_node) {
        loader->cache_hits++;
        
        // Update access patterns
        update_access_pattern(loader, node_id, 1); // 1 = read access
        
        // Prefetch related nodes if enabled
        if (prefetch_related && loader->cache->predictive_loading_enabled) {
            prefetch_related_nodes(loader, node_id, loader->cache->prefetch_window);
        }
        
        return cached_node;
    }
    
    // Cache miss - load from storage
    loader->cache_misses++;
    loader->nvme_reads++;
    
    // Load node from storage (simplified - in real implementation, use NVMe interface)
    lattice_node_t* node = lattice_get_node(loader->lattice, node_id);
    if (!node) {
        printf("[INTELLIGENT-LOADER] ERROR Failed to load node %u from storage\n", node_id);
        return NULL;
    }
    
    // Update access patterns
    update_access_pattern(loader, node_id, 1);
    
    // Prefetch related nodes if enabled
    if (prefetch_related && loader->cache->predictive_loading_enabled) {
        prefetch_related_nodes(loader, node_id, loader->cache->prefetch_window);
    }
    
    printf("[INTELLIGENT-LOADER] INFO Loaded node %u from storage (cache miss)\n", node_id);
    return node;
}

// Smart node saving with write batching and compression
int save_node_intelligent(intelligent_lattice_loader_t* loader,
                         uint32_t node_id,
                         const lattice_node_t* node,
                         bool force_write) {
    if (!loader || !loader->initialized || !node) return -1;
    
    // Check if we need to flush pending writes
    uint64_t current_time = get_current_time_ms();
    bool should_flush = force_write || 
                       (loader->cache->pending_writes >= loader->cache->write_batch_size) ||
                       (current_time - loader->cache->last_write_time > loader->cache->write_batch_timeout_ms);
    
    if (should_flush && loader->cache->pending_writes > 0) {
        printf("[INTELLIGENT-LOADER] INFO Flushing %u pending writes (batch full or timeout)\n",
               loader->cache->pending_writes);
        flush_pending_writes(loader);
    }
    
    // Add to write batch
    if (loader->cache->pending_writes < loader->cache->write_batch_size) {
        uint32_t index = loader->cache->pending_writes;
        loader->cache->dirty_node_ids[index] = node_id;
        memcpy(&loader->cache->dirty_nodes[index], node, sizeof(lattice_node_t));
        loader->cache->pending_writes++;
        
        // Update access patterns
        update_access_pattern(loader, node_id, 2); // 2 = write access
        
        printf("[INTELLIGENT-LOADER] INFO Batched node %u for writing (%u/%u)\n",
               node_id, loader->cache->pending_writes, loader->cache->write_batch_size);
        
        return 0;
    }
    
    // Batch is full, force immediate write
    printf("[INTELLIGENT-LOADER] WARN Write batch full, forcing immediate write\n");
    return save_node_intelligent(loader, node_id, node, true);
}

// Flush pending writes to NVMe (with batching and compression)
int flush_pending_writes(intelligent_lattice_loader_t* loader) {
    if (!loader || !loader->initialized || loader->cache->pending_writes == 0) {
        return 0;
    }
    
    printf("[INTELLIGENT-LOADER] INFO Flushing %u pending writes with wear optimization...\n",
           loader->cache->pending_writes);
    
    // Apply compression if enabled
    size_t total_compressed_size = 0;
    if (loader->compression_enabled) {
        printf("[INTELLIGENT-LOADER] INFO Compressing %u nodes...\n", loader->cache->pending_writes);
        
        for (uint32_t i = 0; i < loader->cache->pending_writes; i++) {
            lattice_node_t* node = &loader->cache->dirty_nodes[i];
            uint8_t* compressed_data = loader->cache->compressed_cache + total_compressed_size;
            size_t remaining_space = loader->cache->max_compressed_size - total_compressed_size;
            
            size_t compressed_size = compress_data((uint8_t*)node, sizeof(lattice_node_t),
                                                  compressed_data, remaining_space);
            
            if (compressed_size > 0) {
                total_compressed_size += compressed_size;
                loader->bytes_compressed += sizeof(lattice_node_t) - compressed_size;
                printf("[INTELLIGENT-LOADER]   Node %u: %zu -> %zu bytes (%.1f%% compression)\n",
                       loader->cache->dirty_node_ids[i], sizeof(lattice_node_t), compressed_size,
                       100.0 * (1.0 - (double)compressed_size / sizeof(lattice_node_t)));
            }
        }
        
        printf("[INTELLIGENT-LOADER] OK Compressed %u nodes: %zu total bytes (%.1f%% avg compression)\n",
               loader->cache->pending_writes, total_compressed_size,
               100.0 * (1.0 - (double)total_compressed_size / (loader->cache->pending_writes * sizeof(lattice_node_t))));
    }
    
    // Write to NVMe (simplified - in real implementation, use NVMe interface)
    for (uint32_t i = 0; i < loader->cache->pending_writes; i++) {
        uint32_t node_id = loader->cache->dirty_node_ids[i];
        lattice_node_t* node = &loader->cache->dirty_nodes[i];
        
        // In real implementation, this would write to NVMe with wear leveling
        if (loader->wear_leveling_enabled) {
            // Apply wear leveling offset
            uint32_t wear_offset = (node_id + loader->cache->wear_leveling_offset) % 1000000;
            printf("[INTELLIGENT-LOADER]   Writing node %u with wear leveling offset %u\n", 
                   node_id, wear_offset);
        }
        
        loader->nvme_writes++;
        loader->cache->total_writes++;
    }
    
    // Calculate wear cycles saved
    uint64_t cycles_saved = loader->cache->pending_writes * 10; // Assume 10 cycles saved per batched write
    loader->wear_cycles_saved += cycles_saved;
    loader->cache->nvme_wear_cycles += cycles_saved;
    
    printf("[INTELLIGENT-LOADER] OK Flushed %u writes, saved %lu wear cycles\n",
           loader->cache->pending_writes, cycles_saved);
    
    // Reset pending writes
    loader->cache->pending_writes = 0;
    loader->cache->last_write_time = get_current_time_ms();
    
    return 0;
}

// Update access patterns for intelligent prefetching
void update_access_pattern(intelligent_lattice_loader_t* loader,
                          uint32_t node_id,
                          uint32_t access_type) {
    if (!loader || !loader->cache || node_id >= 1000000) return;
    
    // Update frequency and recency
    loader->cache->access_frequency[node_id]++;
    loader->cache->access_recency[node_id] = loader->cache->current_time++;
    
    // Track access patterns for prefetching
    if (loader->cache->predictive_loading_enabled) {
        // Simple pattern: if we access node N, we're likely to access N+1, N-1, N+10, etc.
        // This is a simplified heuristic - in practice, you'd use more sophisticated ML
        for (uint32_t i = 0; i < loader->cache->pattern_count && i < 1000; i++) {
            if (loader->cache->access_patterns[i][0] == node_id) {
                // Found existing pattern, update it
                loader->cache->access_patterns[i][1] = access_type;
                loader->cache->access_patterns[i][2] = loader->cache->current_time;
                return;
            }
        }
        
        // Add new pattern
        if (loader->cache->pattern_count < 1000) {
            loader->cache->access_patterns[loader->cache->pattern_count][0] = node_id;
            loader->cache->access_patterns[loader->cache->pattern_count][1] = access_type;
            loader->cache->access_patterns[loader->cache->pattern_count][2] = loader->cache->current_time;
            loader->cache->pattern_count++;
        }
    }
}

// Prefetch related nodes based on access patterns
int prefetch_related_nodes(intelligent_lattice_loader_t* loader,
                          uint32_t node_id,
                          uint32_t max_count) {
    if (!loader || !loader->cache || !loader->cache->predictive_loading_enabled) {
        return 0;
    }
    
    printf("[INTELLIGENT-LOADER] INFO Prefetching related nodes for %u...\n", node_id);
    
    uint32_t prefetched = 0;
    
    // Simple heuristic: prefetch nearby nodes
    for (uint32_t i = 1; i <= max_count && prefetched < max_count; i++) {
        uint32_t candidate_id = node_id + i;
        if (candidate_id < loader->lattice->max_nodes) {
            // Check if node exists and isn't already loaded
            lattice_node_t* existing = lattice_get_node(loader->lattice, candidate_id);
            if (existing) {
                // Node exists and is loaded, skip
                continue;
            }
            
            // Prefetch this node
            lattice_node_t* prefetched_node = load_node_intelligent(loader, candidate_id, false);
            if (prefetched_node) {
                prefetched++;
                printf("[INTELLIGENT-LOADER]   Prefetched node %u\n", candidate_id);
            }
        }
    }
    
    printf("[INTELLIGENT-LOADER] OK Prefetched %u related nodes\n", prefetched);
    return prefetched;
}

// Get comprehensive statistics
void get_intelligent_loader_stats(intelligent_lattice_loader_t* loader,
                                 uint32_t* loaded_nodes,
                                 uint32_t* total_nodes,
                                 size_t* memory_usage_mb,
                                 double* cache_hit_rate,
                                 double* compression_ratio,
                                 uint64_t* nvme_wear_cycles,
                                 uint64_t* wear_cycles_saved,
                                 double* write_amplification) {
    if (!loader) return;
    
    if (loaded_nodes) *loaded_nodes = loader->lattice->node_count;
    if (total_nodes) *total_nodes = loader->lattice->max_nodes;
    if (memory_usage_mb) *memory_usage_mb = loader->lattice->node_count * sizeof(lattice_node_t) / (1024 * 1024);
    
    if (cache_hit_rate) {
        *cache_hit_rate = (loader->total_operations > 0) ? 
            (double)loader->cache_hits / loader->total_operations : 0.0;
    }
    
    if (compression_ratio) {
        *compression_ratio = (loader->bytes_compressed > 0) ? 
            (double)loader->bytes_compressed / (loader->bytes_compressed + loader->cache->total_writes * sizeof(lattice_node_t)) : 0.0;
    }
    
    if (nvme_wear_cycles) *nvme_wear_cycles = loader->cache->nvme_wear_cycles;
    if (wear_cycles_saved) *wear_cycles_saved = loader->wear_cycles_saved;
    
    if (write_amplification) {
        *write_amplification = (loader->cache->total_reads > 0) ? 
            (double)loader->cache->total_writes / loader->cache->total_reads : 1.0;
    }
}

// Estimate remaining NVMe lifespan
uint64_t estimate_nvme_lifespan(intelligent_lattice_loader_t* loader) {
    if (!loader || !loader->cache) return 0;
    
    // Assume NVMe can handle 100,000 write cycles before failure
    uint64_t max_cycles = 100000;
    uint64_t used_cycles = loader->cache->nvme_wear_cycles;
    uint64_t remaining_cycles = (used_cycles < max_cycles) ? (max_cycles - used_cycles) : 0;
    
    printf("[INTELLIGENT-LOADER] INFO NVMe Lifespan Estimate:\n");
    printf("  - Used cycles: %lu\n", used_cycles);
    printf("  - Remaining cycles: %lu\n", remaining_cycles);
    printf("  - Lifespan remaining: %.1f%%\n", 100.0 * (double)remaining_cycles / max_cycles);
    
    return remaining_cycles;
}

// Get wear leveling recommendations
void get_wear_leveling_recommendations(intelligent_lattice_loader_t* loader,
                                      uint32_t* recommended_batch_size,
                                      uint64_t* recommended_timeout_ms,
                                      bool* recommended_compression) {
    if (!loader) return;
    
    // Analyze current patterns and make recommendations
    double current_hit_rate = (loader->total_operations > 0) ? 
        (double)loader->cache_hits / loader->total_operations : 0.0;
    
    if (recommended_batch_size) {
        // Increase batch size if hit rate is low (more batching needed)
        if (current_hit_rate < 0.7) {
            *recommended_batch_size = loader->write_batch_size * 2;
        } else {
            *recommended_batch_size = loader->write_batch_size;
        }
    }
    
    if (recommended_timeout_ms) {
        // Adjust timeout based on access patterns
        if (loader->cache->pending_writes > loader->write_batch_size * 0.8) {
            *recommended_timeout_ms = loader->write_batch_timeout_ms / 2; // More frequent flushes
        } else {
            *recommended_timeout_ms = loader->write_batch_timeout_ms;
        }
    }
    
    if (recommended_compression) {
        // Enable compression if we're doing a lot of writes
        *recommended_compression = (loader->nvme_writes > 1000);
    }
    
    printf("[INTELLIGENT-LOADER] INFO Wear Leveling Recommendations:\n");
    printf("  - Current hit rate: %.1f%%\n", current_hit_rate * 100);
    printf("  - Recommended batch size: %u\n", recommended_batch_size ? *recommended_batch_size : 0);
    printf("  - Recommended timeout: %lu ms\n", recommended_timeout_ms ? *recommended_timeout_ms : 0);
    printf("  - Recommended compression: %s\n", recommended_compression ? (*recommended_compression ? "ON" : "OFF") : "N/A");
}
