#ifndef OPTIMIZED_VECTOR_INDEXING_H
#define OPTIMIZED_VECTOR_INDEXING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "persistent_lattice.h"

// ============================================================================
// OPTIMIZED VECTOR INDEXING - PHASE 2 OPTIMIZATION
// ============================================================================

// Optimized vector dimensions for better performance
#define OPTIMIZED_VECTOR_DIM 64  // Reduced from 128 for better cache performance
#define OPTIMIZED_LSH_FUNCTIONS 4  // Reduced from 8 for faster hashing
#define OPTIMIZED_MAX_CLUSTERS 16  // Reduced from 50 for faster clustering
#define OPTIMIZED_BATCH_SIZE 32  // Process vectors in batches
#define OPTIMIZED_CACHE_LINE_SIZE 64  // Cache line size for alignment

// SIMD-optimized vector operations
#ifdef __AVX2__
#define SIMD_WIDTH 8  // AVX2 processes 8 floats at once
#elif defined(__SSE4_1__)
#define SIMD_WIDTH 4  // SSE4.1 processes 4 floats at once
#else
#define SIMD_WIDTH 1  // Fallback to scalar
#endif

// Optimized vector structure with cache alignment
typedef struct {
    float data[OPTIMIZED_VECTOR_DIM] __attribute__((aligned(32)));  // 32-byte aligned for SIMD
    uint32_t node_id;
    uint32_t cluster_id;
    float magnitude;  // Precomputed for faster similarity
    uint32_t hash;    // Precomputed hash for fast comparison
} optimized_vector_t;

// Fast LSH index with optimized memory layout
typedef struct {
    optimized_vector_t* vectors;        // Cache-aligned vector array
    uint32_t* hash_buckets[OPTIMIZED_LSH_FUNCTIONS];  // Separate buckets per hash function
    uint32_t* bucket_sizes[OPTIMIZED_LSH_FUNCTIONS];  // Size of each bucket
    uint32_t* random_vectors[OPTIMIZED_LSH_FUNCTIONS]; // Random vectors for hashing
    float* random_offsets;              // Random offsets for hashing
    uint32_t vector_count;              // Number of vectors
    uint32_t vector_capacity;           // Capacity for vectors
    uint32_t bucket_capacity;           // Capacity per bucket
    bool is_optimized;                  // Whether index is optimized
} optimized_lsh_index_t;

// Fast clustering with precomputed centroids
typedef struct {
    optimized_vector_t* centroids;      // Cluster centroids
    uint32_t* cluster_assignments;     // Vector to cluster mapping
    uint32_t* cluster_sizes;           // Size of each cluster
    uint32_t* cluster_members;         // Members of each cluster
    uint32_t cluster_count;            // Number of clusters
    uint32_t vector_count;             // Number of vectors
    float* cluster_radii;              // Radius of each cluster
    bool is_optimized;                 // Whether clustering is optimized
} optimized_clustering_index_t;

// Fast similarity search result
typedef struct {
    uint32_t node_id;
    float similarity_score;
    float distance;
    uint32_t cluster_id;
    float cluster_confidence;
    uint32_t rank;
} optimized_search_result_t;

// Optimized vector indexing system
typedef struct {
    optimized_lsh_index_t* lsh_index;           // Fast LSH index
    optimized_clustering_index_t* clustering_index; // Fast clustering
    optimized_vector_t* vector_cache;           // Cached vectors
    uint32_t cache_size;                        // Cache size
    uint32_t cache_capacity;                    // Cache capacity
    bool use_simd;                              // Whether to use SIMD
    bool use_caching;                           // Whether to use caching
    uint64_t total_operations;                  // Total operations
    float avg_query_time;                       // Average query time
} optimized_vector_indexing_system_t;

// ============================================================================
// OPTIMIZED VECTOR OPERATIONS
// ============================================================================

// SIMD-optimized dot product
float optimized_dot_product(const float* vec1, const float* vec2);

// SIMD-optimized vector magnitude
float optimized_vector_magnitude(const float* vector);

// SIMD-optimized cosine similarity
float optimized_cosine_similarity(const float* vec1, const float* vec2);

// SIMD-optimized euclidean distance
float optimized_euclidean_distance(const float* vec1, const float* vec2);

// Fast vector normalization
void optimized_normalize_vector(float* vector);

// Batch vector operations
int optimized_batch_dot_products(const float* vectors1, const float* vectors2, 
                                float* results, uint32_t count);

// Fast hash-based embedding generation
int optimized_generate_embedding(const char* text, float* embedding);

// Precompute vector properties
void optimized_precompute_vector_properties(optimized_vector_t* vector);

// ============================================================================
// OPTIMIZED LSH FUNCTIONS
// ============================================================================

// Create optimized LSH index
int optimized_lsh_index_create(optimized_lsh_index_t* index, uint32_t capacity);

// Add vector to optimized LSH index
int optimized_lsh_index_add_vector(optimized_lsh_index_t* index, const optimized_vector_t* vector);

// Fast LSH search
int optimized_lsh_index_search(optimized_lsh_index_t* index, const float* query_vector,
                              optimized_search_result_t* results, uint32_t* count);

// Optimize LSH index for faster queries
int optimized_lsh_index_optimize(optimized_lsh_index_t* index);

// Destroy optimized LSH index
void optimized_lsh_index_destroy(optimized_lsh_index_t* index);

// ============================================================================
// OPTIMIZED CLUSTERING FUNCTIONS
// ============================================================================

// Create optimized clustering index
int optimized_clustering_index_create(optimized_clustering_index_t* index, uint32_t max_clusters);

// Fast clustering using mini-batch K-means
int optimized_clustering_index_cluster(optimized_clustering_index_t* index, 
                                      const optimized_vector_t* vectors, uint32_t count);

// Fast cluster search
int optimized_clustering_index_search(optimized_clustering_index_t* index, 
                                     const float* query_vector, uint32_t* cluster_id, float* confidence);

// Optimize clustering for faster queries
int optimized_clustering_index_optimize(optimized_clustering_index_t* index);

// Destroy optimized clustering index
void optimized_clustering_index_destroy(optimized_clustering_index_t* index);

// ============================================================================
// OPTIMIZED VECTOR INDEXING SYSTEM
// ============================================================================

// Create optimized vector indexing system
int optimized_vector_indexing_system_create(optimized_vector_indexing_system_t* system, 
                                           uint32_t capacity);

// Add lattice node to optimized system
int optimized_vector_indexing_system_add_node(optimized_vector_indexing_system_t* system, 
                                             const lattice_node_t* node);

// Fast semantic search
int optimized_vector_indexing_system_search(optimized_vector_indexing_system_t* system, 
                                           const char* query, optimized_search_result_t* results, 
                                           uint32_t* count);

// Optimize entire system
int optimized_vector_indexing_system_optimize(optimized_vector_indexing_system_t* system);

// Get system statistics
int optimized_vector_indexing_system_get_stats(optimized_vector_indexing_system_t* system, 
                                              uint32_t* total_vectors, float* avg_query_time);

// Destroy optimized system
void optimized_vector_indexing_system_destroy(optimized_vector_indexing_system_t* system);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Check SIMD availability
bool check_simd_availability(void);

// Enable/disable SIMD
void set_simd_enabled(bool enabled);

// Memory alignment utilities
void* aligned_malloc(size_t size, size_t alignment);
void aligned_free(void* ptr);

// Cache optimization utilities
void prefetch_vector(const optimized_vector_t* vector);
void prefetch_vectors(const optimized_vector_t* vectors, uint32_t count);

// Performance monitoring
uint64_t get_cycle_count(void);
float cycles_to_seconds(uint64_t cycles);

#endif // OPTIMIZED_VECTOR_INDEXING_H
