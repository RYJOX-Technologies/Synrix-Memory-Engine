#ifndef DYNAMIC_LATTICE_LOADER_H
#define DYNAMIC_LATTICE_LOADER_H

#include "persistent_lattice.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declaration for RAM-NVMe bridge (we'll create a C interface)
typedef struct ram_nvme_bridge ram_nvme_bridge_t;

// Dynamic lattice loader - loads nodes on-demand using RAM-NVMe bridge
typedef struct {
    persistent_lattice_t* lattice;           // Base lattice structure
    ram_nvme_bridge_t* nvme_bridge;         // RAM-NVMe bridge for efficient storage
    uint32_t* loaded_node_ids;              // Array of currently loaded node IDs
    uint32_t loaded_count;                  // Number of currently loaded nodes
    uint32_t max_loaded;                    // Maximum nodes to keep in RAM
    uint32_t access_counts[1000000];        // Access count for each node (for LRU)
    uint32_t last_access[1000000];          // Last access time for each node
    uint32_t current_time;                  // Current access time counter
    bool initialized;
} dynamic_lattice_loader_t;

// Create dynamic lattice loader
dynamic_lattice_loader_t* create_dynamic_lattice_loader(const char* lattice_path, 
                                                       const char* nvme_path,
                                                       size_t cache_size_mb);

// Destroy dynamic lattice loader
void destroy_dynamic_lattice_loader(dynamic_lattice_loader_t* loader);

// Load specific node on-demand
lattice_node_t* load_lattice_node(dynamic_lattice_loader_t* loader, uint32_t node_id);

// Load multiple nodes efficiently (batch loading)
int load_lattice_nodes_batch(dynamic_lattice_loader_t* loader, 
                            const uint32_t* node_ids, 
                            uint32_t count,
                            lattice_node_t* nodes_out);

// Find nodes by type (with dynamic loading)
int find_nodes_by_type_dynamic(dynamic_lattice_loader_t* loader,
                              lattice_node_type_t type,
                              uint32_t* node_ids_out,
                              uint32_t max_count);

// Preload frequently accessed nodes
int preload_frequent_nodes(dynamic_lattice_loader_t* loader, uint32_t count);

// Evict least recently used nodes
int evict_lru_nodes(dynamic_lattice_loader_t* loader, uint32_t count);

// Get memory usage statistics
void get_lattice_loader_stats(dynamic_lattice_loader_t* loader,
                             uint32_t* loaded_nodes,
                             uint32_t* total_nodes,
                             size_t* memory_usage_mb,
                             double* cache_hit_rate);

// Save node back to storage (if modified)
int save_lattice_node(dynamic_lattice_loader_t* loader, uint32_t node_id, 
                     const lattice_node_t* node);

// Batch save multiple nodes
int save_lattice_nodes_batch(dynamic_lattice_loader_t* loader,
                            const uint32_t* node_ids,
                            const lattice_node_t* nodes,
                            uint32_t count);

#endif // DYNAMIC_LATTICE_LOADER_H
