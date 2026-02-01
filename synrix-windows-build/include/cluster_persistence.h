#ifndef CLUSTER_PERSISTENCE_H
#define CLUSTER_PERSISTENCE_H

#include <stdint.h>
#include <stdbool.h>
#include "persistent_lattice.h"

#ifdef __cplusplus
extern "C" {
#endif

// Persistent cluster structure (separate from node file)
typedef struct {
    uint32_t cluster_id;
    uint64_t* member_node_ids;   // Array of node IDs in cluster
    uint32_t member_count;
    float centroid[128];          // Cluster centroid (max 128 dimensions)
    uint32_t centroid_dim;        // Actual dimensions used
    float radius;                 // Cluster radius
    uint64_t created_timestamp;   // When cluster was created
    uint64_t last_updated;        // Last update timestamp
} persistent_cluster_t;

// Cluster persistence context
typedef struct {
    persistent_lattice_t* lattice;
    persistent_cluster_t* clusters;
    uint32_t cluster_count;
    uint32_t cluster_capacity;
    char cluster_file_path[512];
    bool enabled;
} cluster_persistence_t;

// Initialize cluster persistence (separate file, doesn't affect node access)
// Returns: 0 on success, -1 on error
int cluster_persistence_init(cluster_persistence_t* ctx, 
                             persistent_lattice_t* lattice,
                             const char* cluster_file_path);

// Cleanup cluster persistence
void cluster_persistence_cleanup(cluster_persistence_t* ctx);

// Save clusters to file (offline operation, doesn't affect node access)
// Returns: 0 on success, -1 on error
int cluster_persistence_save(cluster_persistence_t* ctx);

// Load clusters from file (offline operation, doesn't affect node access)
// Returns: 0 on success, -1 on error
int cluster_persistence_load(cluster_persistence_t* ctx);

// Add cluster (in-memory only, call save() to persist)
// Returns: 0 on success, -1 on error
int cluster_persistence_add_cluster(cluster_persistence_t* ctx,
                                    const persistent_cluster_t* cluster);

// Get cluster by ID
// Returns: pointer to cluster, or NULL if not found
persistent_cluster_t* cluster_persistence_get_cluster(cluster_persistence_t* ctx,
                                                      uint32_t cluster_id);

// Get all clusters
// Returns: number of clusters
uint32_t cluster_persistence_get_all_clusters(cluster_persistence_t* ctx,
                                             persistent_cluster_t** clusters);

#ifdef __cplusplus
}
#endif

#endif // CLUSTER_PERSISTENCE_H

