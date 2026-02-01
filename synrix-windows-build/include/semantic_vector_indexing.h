#ifndef SEMANTIC_VECTOR_INDEXING_H
#define SEMANTIC_VECTOR_INDEXING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "advanced_indexing.h"

// ============================================================================
// SEMANTIC VECTOR INDEXING - PHASE 2
// ============================================================================

// Vector embedding dimensions and constants
#define VECTOR_DIM 128
#define MAX_VECTORS 100000
#define LSH_HASH_BITS 64
#define CLUSTERING_MAX_ITERATIONS 100
#define SIMILARITY_THRESHOLD 0.8f

// Semantic vector with enhanced metadata
typedef struct {
    uint32_t node_id;               // Lattice node ID
    float embedding[VECTOR_DIM];    // 128-dimensional semantic vector
    uint32_t cluster_id;            // K-means cluster assignment
    float cluster_confidence;       // Cluster membership confidence (0-1)
    uint32_t frequency;             // Usage frequency
    uint64_t last_accessed;         // Last access timestamp
    float semantic_weight;          // Semantic importance weight
    uint32_t related_vectors[8];    // Related vector IDs
    uint8_t related_count;          // Number of related vectors
} enhanced_semantic_vector_t;

// LSH (Locality Sensitive Hashing) with multiple hash functions
typedef struct {
    uint32_t node_id;               // Lattice node ID
    uint64_t lsh_hashes[8];         // Multiple LSH hash values
    uint32_t bucket_ids[8];         // Corresponding bucket IDs
    float similarity_threshold;     // Similarity threshold for this vector
    uint32_t hash_function_count;   // Number of hash functions used
    float collision_probability;    // Probability of hash collision
} enhanced_lsh_entry_t;

// LSH index with multiple hash functions
typedef struct {
    enhanced_lsh_entry_t* entries;  // LSH entries
    uint32_t* buckets;              // Hash buckets (array of lists)
    uint32_t* bucket_sizes;         // Size of each bucket
    uint32_t bucket_count;          // Number of buckets
    uint32_t hash_functions;        // Number of hash functions
    uint32_t vector_dim;            // Vector dimension
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Capacity
    float* random_vectors;          // Random vectors for LSH
    float* random_offsets;          // Random offsets for LSH
} enhanced_lsh_index_t;

// K-means clustering with enhanced features
typedef struct {
    uint32_t cluster_id;            // Cluster ID
    float centroid[VECTOR_DIM];     // Cluster centroid
    uint32_t* member_vectors;       // Vector IDs in this cluster
    uint32_t member_count;          // Number of members
    uint32_t member_capacity;       // Capacity for members
    float radius;                   // Cluster radius
    float density;                  // Cluster density
    float cohesion;                 // Cluster cohesion score
    float separation;               // Cluster separation score
    uint64_t last_update;           // Last update timestamp
    float stability_score;          // Cluster stability (0-1)
} enhanced_semantic_cluster_t;

// Enhanced clustering index
typedef struct {
    enhanced_semantic_cluster_t* clusters;  // Cluster array
    uint32_t cluster_count;         // Number of clusters
    uint32_t max_clusters;          // Maximum clusters
    uint32_t vector_dim;            // Vector dimension
    float convergence_threshold;    // Convergence threshold
    uint32_t max_iterations;        // Maximum iterations
    float inertia;                  // Sum of squared distances
    float silhouette_score;         // Silhouette score for quality
    bool converged;                 // Whether clustering converged
    uint32_t iteration_count;       // Number of iterations performed
} enhanced_clustering_index_t;

// Vector similarity search result
typedef struct {
    uint32_t node_id;               // Node ID
    float similarity_score;         // Similarity score (0-1)
    float distance;                 // Euclidean distance
    uint32_t cluster_id;            // Cluster ID
    float cluster_confidence;       // Cluster confidence
    uint32_t rank;                  // Result rank
} vector_similarity_result_t;

// Vector similarity search query
typedef struct {
    float query_vector[VECTOR_DIM]; // Query vector
    uint32_t max_results;           // Maximum results to return
    float min_similarity;           // Minimum similarity threshold
    uint32_t cluster_filter;        // Filter by cluster ID (0 = all)
    bool use_lsh;                   // Use LSH for fast search
    bool use_clustering;            // Use clustering for search
} vector_similarity_query_t;

// Semantic vector indexing system
typedef struct {
    enhanced_semantic_vector_t* vectors;    // Vector storage
    enhanced_lsh_index_t* lsh_index;        // LSH index
    enhanced_clustering_index_t* clustering; // Clustering index
    uint32_t vector_count;                  // Number of vectors
    uint32_t vector_capacity;               // Vector capacity
    bool is_initialized;                    // Initialization status
    uint64_t last_update;                   // Last update timestamp
} semantic_vector_indexing_system_t;

// ============================================================================
// VECTOR EMBEDDING FUNCTIONS
// ============================================================================

// Generate semantic embedding from text
int generate_semantic_embedding(const char* text, float* embedding);

// Generate semantic embedding from lattice node
int generate_node_embedding(const lattice_node_t* node, float* embedding);

// Calculate cosine similarity between two vectors
float calculate_cosine_similarity(const float* vec1, const float* vec2);

// Calculate euclidean distance between two vectors
float calculate_euclidean_distance(const float* vec1, const float* vec2);

// Normalize vector to unit length
void normalize_vector(float* vector);

// Calculate vector magnitude
float calculate_vector_magnitude(const float* vector);

// ============================================================================
// LSH (LOCALITY SENSITIVE HASHING) FUNCTIONS
// ============================================================================

// Create enhanced LSH index
int enhanced_lsh_index_create(enhanced_lsh_index_t* index, uint32_t vector_dim, uint32_t hash_functions);

// Add vector to LSH index
int enhanced_lsh_index_add_vector(enhanced_lsh_index_t* index, const float* vector, uint32_t node_id);

// Search for similar vectors using LSH
int enhanced_lsh_index_search_similar(enhanced_lsh_index_t* index, const float* query_vector, 
                                     float threshold, vector_similarity_result_t* results, uint32_t* count);

// Generate LSH hash for vector
uint64_t generate_lsh_hash(const float* vector, const float* random_vector, float random_offset);

// Calculate LSH collision probability
float calculate_lsh_collision_probability(float similarity, uint32_t hash_functions);

// Destroy LSH index
void enhanced_lsh_index_destroy(enhanced_lsh_index_t* index);

// ============================================================================
// K-MEANS CLUSTERING FUNCTIONS
// ============================================================================

// Create enhanced clustering index
int enhanced_clustering_index_create(enhanced_clustering_index_t* index, uint32_t max_clusters, uint32_t vector_dim);

// Add vector to clustering index
int enhanced_clustering_index_add_vector(enhanced_clustering_index_t* index, const float* vector, uint32_t node_id);

// Perform K-means clustering
int enhanced_clustering_index_cluster(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count);

// Search vectors by cluster
int enhanced_clustering_index_search_by_cluster(enhanced_clustering_index_t* index, uint32_t cluster_id,
                                               vector_similarity_result_t* results, uint32_t* count);

// Calculate cluster quality metrics
float calculate_cluster_silhouette_score(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count);

// Update cluster centroids
int update_cluster_centroids(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count);

// Assign vectors to clusters
int assign_vectors_to_clusters(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count);

// Destroy clustering index
void enhanced_clustering_index_destroy(enhanced_clustering_index_t* index);

// ============================================================================
// SEMANTIC VECTOR INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create semantic vector indexing system
int semantic_vector_indexing_system_create(semantic_vector_indexing_system_t* system);

// Add lattice node to vector indexing system
int semantic_vector_indexing_system_add_node(semantic_vector_indexing_system_t* system, const lattice_node_t* node);

// Search for similar nodes using vector similarity
int semantic_vector_indexing_system_search_similar(semantic_vector_indexing_system_t* system, 
                                                  const vector_similarity_query_t* query,
                                                  vector_similarity_result_t* results, uint32_t* count);

// Update vector embeddings
int semantic_vector_indexing_system_update_embeddings(semantic_vector_indexing_system_t* system);

// Rebuild clustering
int semantic_vector_indexing_system_rebuild_clustering(semantic_vector_indexing_system_t* system);

// Get vector statistics
int semantic_vector_indexing_system_get_stats(semantic_vector_indexing_system_t* system, 
                                             uint32_t* vector_count, uint32_t* cluster_count, 
                                             float* avg_similarity, float* silhouette_score);

// Destroy semantic vector indexing system
void semantic_vector_indexing_system_destroy(semantic_vector_indexing_system_t* system);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Initialize random vectors for LSH
int initialize_lsh_random_vectors(enhanced_lsh_index_t* index);

// Calculate vector statistics
int calculate_vector_statistics(const enhanced_semantic_vector_t* vectors, uint32_t count,
                               float* mean_similarity, float* std_deviation, float* min_similarity, float* max_similarity);

// Sort similarity results by score
void sort_similarity_results(vector_similarity_result_t* results, uint32_t count);

// Calculate cluster cohesion and separation
int calculate_cluster_metrics(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count);

// Generate random float in range [0, 1]
float random_float(void);

// Generate random float in range [min, max]
float random_float_range(float min, float max);

#endif // SEMANTIC_VECTOR_INDEXING_H
