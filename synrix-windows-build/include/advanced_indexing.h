#ifndef ADVANCED_INDEXING_H
#define ADVANCED_INDEXING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "persistent_lattice.h"

// ============================================================================
// MULTI-DIMENSIONAL INDEXING
// ============================================================================

// B-tree for range queries (complexity, performance, timestamp)
typedef struct {
    uint32_t node_id;
    uint32_t complexity;
    uint32_t performance;
    uint64_t timestamp;
    float semantic_score;
} multi_dim_entry_t;

// B-tree node structure
typedef struct btree_node {
    multi_dim_entry_t* entries;     // Entries in this node
    uint32_t* children;             // Child node IDs
    uint32_t entry_count;           // Number of entries
    uint32_t is_leaf;               // 1 if leaf, 0 if internal
    uint32_t parent_id;             // Parent node ID
} btree_node_t;

// B-tree index
typedef struct {
    uint32_t root_id;               // Root node ID
    uint32_t node_count;            // Total number of nodes
    uint32_t order;                 // B-tree order (minimum degree)
    uint32_t height;                // Tree height
    btree_node_t* nodes;            // Node array
    uint32_t next_node_id;          // Next available node ID
} btree_index_t;

// R-tree for spatial/domain queries
typedef struct {
    uint32_t node_id;
    float domain_coords[8];         // Multi-dimensional domain space
    float min_bounds[8];            // Minimum bounds
    float max_bounds[8];            // Maximum bounds
} spatial_entry_t;

// R-tree node
typedef struct rtree_node {
    spatial_entry_t* entries;       // Entries in this node
    uint32_t* children;             // Child node IDs
    uint32_t entry_count;           // Number of entries
    uint32_t is_leaf;               // 1 if leaf, 0 if internal
    float bounds[16];               // Bounding box (min/max for 8 dims)
} rtree_node_t;

// R-tree index
typedef struct {
    uint32_t root_id;               // Root node ID
    uint32_t node_count;            // Total number of nodes
    uint32_t max_entries;           // Maximum entries per node
    uint32_t height;                // Tree height
    rtree_node_t* nodes;            // Node array
    uint32_t next_node_id;          // Next available node ID
} rtree_index_t;

// Composite index for multi-criteria search
typedef struct {
    uint32_t node_id;
    uint32_t domain_flags;          // Bit flags for domains
    uint32_t complexity;            // Complexity level
    uint32_t performance;           // Performance score
    uint64_t timestamp;             // Creation time
    float semantic_score;           // Semantic relevance
    uint32_t pattern_type;          // Pattern type
    uint32_t evolution_generation;  // Evolution generation
} composite_entry_t;

// Composite index
typedef struct {
    composite_entry_t* entries;     // Index entries
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Capacity
    uint32_t* domain_index;         // Index by domain flags
    uint32_t* complexity_index;     // Index by complexity
    uint32_t* performance_index;    // Index by performance
    uint32_t* timestamp_index;      // Index by timestamp
} composite_index_t;

// ============================================================================
// SEMANTIC VECTOR INDEXING
// ============================================================================

// Vector similarity for semantic search
typedef struct {
    uint32_t node_id;
    float embedding[128];           // 128-dim semantic vector
    uint32_t cluster_id;            // K-means cluster
    float cluster_confidence;       // Cluster membership confidence
} semantic_vector_t;

// LSH (Locality Sensitive Hashing) for fast similarity
typedef struct {
    uint32_t node_id;
    uint64_t lsh_hash;              // LSH hash for fast similarity
    float similarity_threshold;     // Similarity threshold
    uint32_t bucket_id;             // LSH bucket ID
} lsh_entry_t;

// LSH index
typedef struct {
    lsh_entry_t* entries;           // LSH entries
    uint32_t* buckets;              // Hash buckets
    uint32_t bucket_count;          // Number of buckets
    uint32_t hash_functions;        // Number of hash functions
    uint32_t vector_dim;            // Vector dimension
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Capacity
} lsh_index_t;

// K-means clustering for semantic organization
typedef struct {
    uint32_t cluster_id;
    float centroid[128];            // Cluster centroid
    uint32_t* member_nodes;         // Nodes in this cluster
    uint32_t member_count;          // Number of members
    float radius;                   // Cluster radius
    float density;                  // Cluster density
} semantic_cluster_t;

// Clustering index
typedef struct {
    semantic_cluster_t* clusters;   // Cluster array
    uint32_t cluster_count;         // Number of clusters
    uint32_t max_clusters;          // Maximum clusters
    uint32_t vector_dim;            // Vector dimension
    float convergence_threshold;    // Convergence threshold
} clustering_index_t;

// ============================================================================
// HIERARCHICAL INDEXING
// ============================================================================

// Tree-based organization by domain/complexity
typedef struct {
    uint32_t node_id;
    uint32_t parent_id;
    uint32_t level;                 // Hierarchy level
    char path[256];                 // Hierarchical path
    uint32_t* children;             // Child node IDs
    uint32_t child_count;           // Number of children
    uint32_t subtree_size;          // Size of subtree
} hierarchical_entry_t;

// Hierarchical index
typedef struct {
    hierarchical_entry_t* entries;  // Hierarchical entries
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Capacity
    uint32_t root_id;               // Root node ID
    uint32_t max_level;             // Maximum hierarchy level
} hierarchical_index_t;

// B+ tree for ordered access
typedef struct {
    uint32_t node_id;
    uint32_t sort_key;              // Sortable key
    uint32_t* children;             // Child node IDs
    uint32_t child_count;           // Number of children
    uint32_t is_leaf;               // 1 if leaf, 0 if internal
    uint32_t parent_id;             // Parent node ID
    uint32_t next_leaf;             // Next leaf node (for sequential access)
    uint32_t prev_leaf;             // Previous leaf node
} bplus_node_t;

// B+ tree index
typedef struct {
    uint32_t root_id;               // Root node ID
    uint32_t leaf_head;             // First leaf node
    uint32_t leaf_tail;             // Last leaf node
    uint32_t node_count;            // Total number of nodes
    uint32_t order;                 // B+ tree order
    uint32_t height;                // Tree height
    bplus_node_t* nodes;            // Node array
    uint32_t next_node_id;          // Next available node ID
} bplus_index_t;

// ============================================================================
// SPECIALIZED INDEXES
// ============================================================================

// Bloom filter for fast negative lookups
typedef struct {
    uint8_t* bit_array;             // Bloom filter bits
    uint32_t array_size;            // Size in bits
    uint32_t hash_count;            // Number of hash functions
    uint32_t element_count;         // Number of elements
    uint32_t false_positive_rate;   // False positive rate (ppm)
} bloom_filter_t;

// Inverted index for text search
typedef struct {
    char term[64];                  // Search term
    uint32_t* node_ids;             // Nodes containing this term
    uint32_t count;                 // Number of nodes
    uint32_t capacity;              // Capacity
    float term_frequency;           // Term frequency
    float inverse_doc_frequency;    // Inverse document frequency
} inverted_index_entry_t;

// Inverted index
typedef struct {
    inverted_index_entry_t* entries; // Inverted index entries
    uint32_t count;                 // Number of terms
    uint32_t capacity;              // Capacity
    uint32_t total_documents;       // Total number of documents
} inverted_index_t;

// Temporal index for time-based queries
typedef struct {
    uint32_t node_id;
    uint64_t start_time;            // Start time
    uint64_t end_time;              // End time
    uint32_t duration;              // Duration
    uint32_t event_type;            // Event type
    uint32_t priority;              // Priority level
} temporal_entry_t;

// Temporal index
typedef struct {
    temporal_entry_t* entries;      // Temporal entries
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Capacity
    uint64_t time_range_start;      // Time range start
    uint64_t time_range_end;        // Time range end
} temporal_index_t;

// ============================================================================
// ADVANCED INDEXING SYSTEM
// ============================================================================

// Master indexing system
typedef struct {
    // Multi-dimensional indexes
    btree_index_t* complexity_btree;
    btree_index_t* performance_btree;
    btree_index_t* timestamp_btree;
    rtree_index_t* domain_rtree;
    composite_index_t* composite_index;
    
    // Semantic vector indexes
    semantic_vector_t* semantic_vectors;
    lsh_index_t* lsh_index;
    clustering_index_t* clustering_index;
    
    // Hierarchical indexes
    hierarchical_index_t* hierarchical_index;
    bplus_index_t* ordered_index;
    
    // Specialized indexes
    bloom_filter_t* bloom_filter;
    inverted_index_t* inverted_index;
    temporal_index_t* temporal_index;
    
    // Metadata
    uint32_t total_indexes;
    uint32_t total_entries;
    uint64_t last_update;
    bool is_initialized;
} advanced_indexing_system_t;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Multi-dimensional indexing
int btree_index_create(btree_index_t* index, uint32_t order);
int btree_index_insert(btree_index_t* index, multi_dim_entry_t* entry);
int btree_index_search_range(btree_index_t* index, uint32_t min_key, uint32_t max_key, 
                            uint32_t* results, uint32_t* count);
void btree_index_destroy(btree_index_t* index);

int rtree_index_create(rtree_index_t* index, uint32_t max_entries);
int rtree_index_insert(rtree_index_t* index, spatial_entry_t* entry);
int rtree_index_search_spatial(rtree_index_t* index, float* query_bounds, 
                              uint32_t* results, uint32_t* count);
void rtree_index_destroy(rtree_index_t* index);

int composite_index_create(composite_index_t* index, uint32_t capacity);
int composite_index_insert(composite_index_t* index, composite_entry_t* entry);
int composite_index_search_multi_criteria(composite_index_t* index, 
                                         uint32_t domain_flags, uint32_t min_complexity,
                                         uint32_t min_performance, uint64_t min_timestamp,
                                         uint32_t* results, uint32_t* count);
void composite_index_destroy(composite_index_t* index);

// Semantic vector indexing
int lsh_index_create(lsh_index_t* index, uint32_t vector_dim, uint32_t hash_functions);
int lsh_index_insert(lsh_index_t* index, lsh_entry_t* entry);
int lsh_index_search_similar(lsh_index_t* index, uint64_t query_hash, float threshold,
                            uint32_t* results, uint32_t* count);
void lsh_index_destroy(lsh_index_t* index);

int clustering_index_create(clustering_index_t* index, uint32_t max_clusters, uint32_t vector_dim);
int clustering_index_add_vector(clustering_index_t* index, semantic_vector_t* vector);
int clustering_index_search_by_cluster(clustering_index_t* index, uint32_t cluster_id,
                                      uint32_t* results, uint32_t* count);
void clustering_index_destroy(clustering_index_t* index);

// Hierarchical indexing
int hierarchical_index_create(hierarchical_index_t* index, uint32_t capacity);
int hierarchical_index_insert(hierarchical_index_t* index, hierarchical_entry_t* entry);
int hierarchical_index_search_by_path(hierarchical_index_t* index, const char* path,
                                     uint32_t* results, uint32_t* count);
void hierarchical_index_destroy(hierarchical_index_t* index);

int bplus_index_create(bplus_index_t* index, uint32_t order);
int bplus_index_insert(bplus_index_t* index, uint32_t node_id, uint32_t sort_key);
int bplus_index_search_range(bplus_index_t* index, uint32_t min_key, uint32_t max_key,
                            uint32_t* results, uint32_t* count);
void bplus_index_destroy(bplus_index_t* index);

// Specialized indexes
int bloom_filter_create(bloom_filter_t* filter, uint32_t expected_elements, float false_positive_rate);
int bloom_filter_add(bloom_filter_t* filter, const char* key);
bool bloom_filter_contains(bloom_filter_t* filter, const char* key);
void bloom_filter_destroy(bloom_filter_t* filter);

int inverted_index_create(inverted_index_t* index, uint32_t capacity);
int inverted_index_add_term(inverted_index_t* index, const char* term, uint32_t node_id);
int inverted_index_search_text(inverted_index_t* index, const char* query,
                              uint32_t* results, uint32_t* count);
void inverted_index_destroy(inverted_index_t* index);

int temporal_index_create(temporal_index_t* index, uint32_t capacity);
int temporal_index_insert(temporal_index_t* index, temporal_entry_t* entry);
int temporal_index_search_time_range(temporal_index_t* index, uint64_t start_time, uint64_t end_time,
                                   uint32_t* results, uint32_t* count);
void temporal_index_destroy(temporal_index_t* index);

// Master indexing system
int advanced_indexing_system_create(advanced_indexing_system_t* system);
int advanced_indexing_system_add_node(advanced_indexing_system_t* system, lattice_node_t* node);
int advanced_indexing_system_search(advanced_indexing_system_t* system, const char* query,
                                   uint32_t* results, uint32_t* count);
void advanced_indexing_system_destroy(advanced_indexing_system_t* system);

#endif // ADVANCED_INDEXING_H
