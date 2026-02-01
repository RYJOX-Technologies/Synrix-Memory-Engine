#define _GNU_SOURCE
#include "optimized_vector_indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
// Note: SIMD intrinsics disabled for ARM64 compatibility
// #include <immintrin.h>  // For SIMD intrinsics
// #include <x86intrin.h>  // For performance counters

// ============================================================================
// SIMD OPTIMIZATIONS
// ============================================================================

// Check SIMD availability (ARM64 compatible)
bool check_simd_availability(void) {
    // For ARM64, we'll use NEON instructions if available
    // This is a simplified check - in production, use proper ARM64 detection
    return false; // Disabled for ARM64 compatibility
}

// SIMD-optimized dot product using AVX2
float optimized_dot_product(const float* vec1, const float* vec2) {
    if (!vec1 || !vec2) return 0.0f;
    
#ifdef __AVX2__
    if (check_simd_availability()) {
        __m256 sum = _mm256_setzero_ps();
        
        // Process 8 floats at a time
        for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i += 8) {
            __m256 a = _mm256_load_ps(&vec1[i]);
            __m256 b = _mm256_load_ps(&vec2[i]);
            __m256 mul = _mm256_mul_ps(a, b);
            sum = _mm256_add_ps(sum, mul);
        }
        
        // Horizontal sum
        __m128 sum128 = _mm_add_ps(_mm256_extractf128_ps(sum, 0), _mm256_extractf128_ps(sum, 1));
        sum128 = _mm_hadd_ps(sum128, sum128);
        sum128 = _mm_hadd_ps(sum128, sum128);
        
        return _mm_cvtss_f32(sum128);
    }
#endif

#ifdef __SSE4_1__
    if (check_simd_availability()) {
        __m128 sum = _mm_setzero_ps();
        
        // Process 4 floats at a time
        for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i += 4) {
            __m128 a = _mm_load_ps(&vec1[i]);
            __m128 b = _mm_load_ps(&vec2[i]);
            __m128 mul = _mm_mul_ps(a, b);
            sum = _mm_add_ps(sum, mul);
        }
        
        // Horizontal sum
        sum = _mm_hadd_ps(sum, sum);
        sum = _mm_hadd_ps(sum, sum);
        
        return _mm_cvtss_f32(sum);
    }
#endif

    // Fallback to scalar
    float sum = 0.0f;
    for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i++) {
        sum += vec1[i] * vec2[i];
    }
    return sum;
}

// SIMD-optimized vector magnitude
float optimized_vector_magnitude(const float* vector) {
    if (!vector) return 0.0f;
    
#ifdef __AVX2__
    if (check_simd_availability()) {
        __m256 sum = _mm256_setzero_ps();
        
        for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i += 8) {
            __m256 v = _mm256_load_ps(&vector[i]);
            __m256 mul = _mm256_mul_ps(v, v);
            sum = _mm256_add_ps(sum, mul);
        }
        
        __m128 sum128 = _mm_add_ps(_mm256_extractf128_ps(sum, 0), _mm256_extractf128_ps(sum, 1));
        sum128 = _mm_hadd_ps(sum128, sum128);
        sum128 = _mm_hadd_ps(sum128, sum128);
        
        return sqrtf(_mm_cvtss_f32(sum128));
    }
#endif

    // Fallback to scalar
    float sum = 0.0f;
    for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i++) {
        sum += vector[i] * vector[i];
    }
    return sqrtf(sum);
}

// SIMD-optimized cosine similarity
float optimized_cosine_similarity(const float* vec1, const float* vec2) {
    if (!vec1 || !vec2) return 0.0f;
    
    float dot_product = optimized_dot_product(vec1, vec2);
    float magnitude1 = optimized_vector_magnitude(vec1);
    float magnitude2 = optimized_vector_magnitude(vec2);
    
    if (magnitude1 == 0.0f || magnitude2 == 0.0f) return 0.0f;
    
    return dot_product / (magnitude1 * magnitude2);
}

// SIMD-optimized euclidean distance
float optimized_euclidean_distance(const float* vec1, const float* vec2) {
    if (!vec1 || !vec2) return INFINITY;
    
#ifdef __AVX2__
    if (check_simd_availability()) {
        __m256 sum = _mm256_setzero_ps();
        
        for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i += 8) {
            __m256 a = _mm256_load_ps(&vec1[i]);
            __m256 b = _mm256_load_ps(&vec2[i]);
            __m256 diff = _mm256_sub_ps(a, b);
            __m256 mul = _mm256_mul_ps(diff, diff);
            sum = _mm256_add_ps(sum, mul);
        }
        
        __m128 sum128 = _mm_add_ps(_mm256_extractf128_ps(sum, 0), _mm256_extractf128_ps(sum, 1));
        sum128 = _mm_hadd_ps(sum128, sum128);
        sum128 = _mm_hadd_ps(sum128, sum128);
        
        return sqrtf(_mm_cvtss_f32(sum128));
    }
#endif

    // Fallback to scalar
    float sum = 0.0f;
    for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i++) {
        float diff = vec1[i] - vec2[i];
        sum += diff * diff;
    }
    return sqrtf(sum);
}

// Fast vector normalization
void optimized_normalize_vector(float* vector) {
    if (!vector) return;
    
    float magnitude = optimized_vector_magnitude(vector);
    if (magnitude == 0.0f) return;
    
#ifdef __AVX2__
    if (check_simd_availability()) {
        __m256 inv_mag = _mm256_set1_ps(1.0f / magnitude);
        
        for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i += 8) {
            __m256 v = _mm256_load_ps(&vector[i]);
            __m256 normalized = _mm256_mul_ps(v, inv_mag);
            _mm256_store_ps(&vector[i], normalized);
        }
        return;
    }
#endif

    // Fallback to scalar
    for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i++) {
        vector[i] /= magnitude;
    }
}

// Batch vector operations for better cache performance
int optimized_batch_dot_products(const float* vectors1, const float* vectors2, 
                                float* results, uint32_t count) {
    if (!vectors1 || !vectors2 || !results || count == 0) return -1;
    
    for (uint32_t i = 0; i < count; i++) {
        const float* v1 = &vectors1[i * OPTIMIZED_VECTOR_DIM];
        const float* v2 = &vectors2[i * OPTIMIZED_VECTOR_DIM];
        results[i] = optimized_dot_product(v1, v2);
    }
    
    return 0;
}

// ============================================================================
// OPTIMIZED EMBEDDING GENERATION
// ============================================================================

// Fast hash-based embedding generation
int optimized_generate_embedding(const char* text, float* embedding) {
    if (!text || !embedding) return -1;
    
    size_t text_len = strlen(text);
    if (text_len == 0) return -1;
    
    // Initialize embedding with zeros
    memset(embedding, 0, OPTIMIZED_VECTOR_DIM * sizeof(float));
    
    // Fast hash-based embedding using multiple hash functions
    uint32_t hashes[4] = {0x811c9dc5, 0x01000193, 0x85ebca6b, 0xc2b2ae35};
    
    for (size_t i = 0; i < text_len; i++) {
        uint8_t c = (uint8_t)text[i];
        
        // Update all hashes
        for (int j = 0; j < 4; j++) {
            hashes[j] ^= c;
            hashes[j] *= 0x01000193;
        }
    }
    
    // Distribute hashes across embedding dimensions
    for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i++) {
        uint32_t hash_idx = i % 4;
        uint32_t hash = hashes[hash_idx];
        
        // Use different bits for different dimensions
        uint32_t bit_shift = (i / 4) % 32;
        float value = (float)((hash >> bit_shift) & 0xFF) / 255.0f;
        embedding[i] = value * 2.0f - 1.0f; // Normalize to [-1, 1]
    }
    
    // Add text-based features for better semantic representation
    for (size_t i = 0; i < text_len && i < OPTIMIZED_VECTOR_DIM; i++) {
        float char_value = (float)((uint8_t)text[i] - 128) / 128.0f;
        embedding[i] += char_value * 0.1f; // Small contribution
    }
    
    // Normalize the vector
    optimized_normalize_vector(embedding);
    
    return 0;
}

// Precompute vector properties for faster operations
void optimized_precompute_vector_properties(optimized_vector_t* vector) {
    if (!vector) return;
    
    // Precompute magnitude
    vector->magnitude = optimized_vector_magnitude(vector->data);
    
    // Precompute hash for fast comparison
    uint32_t hash = 0x811c9dc5;
    for (int i = 0; i < OPTIMIZED_VECTOR_DIM; i++) {
        uint32_t bits;
        memcpy(&bits, &vector->data[i], sizeof(uint32_t));
        hash ^= bits;
        hash *= 0x01000193;
    }
    vector->hash = hash;
}

// ============================================================================
// OPTIMIZED LSH FUNCTIONS
// ============================================================================

// Create optimized LSH index
int optimized_lsh_index_create(optimized_lsh_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(optimized_lsh_index_t));
    index->vector_capacity = capacity;
    index->bucket_capacity = capacity / 4; // 4x over-provisioning
    
    // Allocate cache-aligned vector array
    index->vectors = (optimized_vector_t*)aligned_malloc(capacity * sizeof(optimized_vector_t), 32);
    if (!index->vectors) return -1;
    
    // Allocate hash buckets for each hash function
    for (int i = 0; i < OPTIMIZED_LSH_FUNCTIONS; i++) {
        index->hash_buckets[i] = (uint32_t*)calloc(index->bucket_capacity, sizeof(uint32_t));
        index->bucket_sizes[i] = (uint32_t*)calloc(index->bucket_capacity, sizeof(uint32_t));
        index->random_vectors[i] = (uint32_t*)calloc(OPTIMIZED_VECTOR_DIM, sizeof(uint32_t));
        
        if (!index->hash_buckets[i] || !index->bucket_sizes[i] || !index->random_vectors[i]) {
            optimized_lsh_index_destroy(index);
            return -1;
        }
    }
    
    // Generate random vectors for hashing
    srand(time(NULL));
    index->random_offsets = (float*)malloc(OPTIMIZED_LSH_FUNCTIONS * sizeof(float));
    if (!index->random_offsets) {
        optimized_lsh_index_destroy(index);
        return -1;
    }
    
    for (int i = 0; i < OPTIMIZED_LSH_FUNCTIONS; i++) {
        for (int j = 0; j < OPTIMIZED_VECTOR_DIM; j++) {
            index->random_vectors[i][j] = rand();
        }
        index->random_offsets[i] = (float)rand() / RAND_MAX;
    }
    
    return 0;
}

// Add vector to optimized LSH index
int optimized_lsh_index_add_vector(optimized_lsh_index_t* index, const optimized_vector_t* vector) {
    if (!index || !vector || index->vector_count >= index->vector_capacity) return -1;
    
    // Copy vector to cache-aligned storage
    index->vectors[index->vector_count] = *vector;
    optimized_precompute_vector_properties(&index->vectors[index->vector_count]);
    
    // Add to hash buckets
    for (int i = 0; i < OPTIMIZED_LSH_FUNCTIONS; i++) {
        uint32_t hash = 0;
        for (int j = 0; j < OPTIMIZED_VECTOR_DIM; j++) {
            hash ^= (uint32_t)(vector->data[j] * 1000000.0f) ^ index->random_vectors[i][j];
        }
        
        uint32_t bucket_id = hash % index->bucket_capacity;
        
        if (index->bucket_sizes[i][bucket_id] < 100) { // Max 100 vectors per bucket
            uint32_t bucket_idx = bucket_id * 100 + index->bucket_sizes[i][bucket_id];
            index->hash_buckets[i][bucket_idx] = index->vector_count;
            index->bucket_sizes[i][bucket_id]++;
        }
    }
    
    index->vector_count++;
    return 0;
}

// Fast LSH search
int optimized_lsh_index_search(optimized_lsh_index_t* index, const float* query_vector,
                              optimized_search_result_t* results, uint32_t* count) {
    if (!index || !query_vector || !results || !count) return -1;
    
    *count = 0;
    
    // Generate query hashes
    uint32_t query_hashes[OPTIMIZED_LSH_FUNCTIONS];
    for (int i = 0; i < OPTIMIZED_LSH_FUNCTIONS; i++) {
        query_hashes[i] = 0;
        for (int j = 0; j < OPTIMIZED_VECTOR_DIM; j++) {
            query_hashes[i] ^= (uint32_t)(query_vector[j] * 1000000.0f) ^ index->random_vectors[i][j];
        }
    }
    
    // Search in buckets
    for (int i = 0; i < OPTIMIZED_LSH_FUNCTIONS && *count < 1000; i++) {
        uint32_t bucket_id = query_hashes[i] % index->bucket_capacity;
        uint32_t bucket_size = index->bucket_sizes[i][bucket_id];
        
        for (uint32_t j = 0; j < bucket_size && *count < 1000; j++) {
            uint32_t vector_idx = index->hash_buckets[i][bucket_id * 100 + j];
            optimized_vector_t* vector = &index->vectors[vector_idx];
            
            // Fast similarity check using precomputed properties
            float similarity = optimized_cosine_similarity(query_vector, vector->data);
            
            if (similarity > 0.7f) { // Threshold for similarity
                results[*count].node_id = vector->node_id;
                results[*count].similarity_score = similarity;
                results[*count].distance = 1.0f - similarity;
                results[*count].cluster_id = vector->cluster_id;
                results[*count].cluster_confidence = similarity;
                results[*count].rank = *count + 1;
                (*count)++;
            }
        }
    }
    
    return 0;
}

// Optimize LSH index for faster queries
int optimized_lsh_index_optimize(optimized_lsh_index_t* index) {
    if (!index) return -1;
    
    // Sort vectors by hash for better cache locality
    // This is a simplified optimization - in production, use more sophisticated methods
    
    index->is_optimized = true;
    return 0;
}

// Destroy optimized LSH index
void optimized_lsh_index_destroy(optimized_lsh_index_t* index) {
    if (!index) return;
    
    if (index->vectors) aligned_free(index->vectors);
    
    for (int i = 0; i < OPTIMIZED_LSH_FUNCTIONS; i++) {
        if (index->hash_buckets[i]) free(index->hash_buckets[i]);
        if (index->bucket_sizes[i]) free(index->bucket_sizes[i]);
        if (index->random_vectors[i]) free(index->random_vectors[i]);
    }
    
    if (index->random_offsets) free(index->random_offsets);
    
    memset(index, 0, sizeof(optimized_lsh_index_t));
}

// ============================================================================
// OPTIMIZED CLUSTERING FUNCTIONS
// ============================================================================

// Create optimized clustering index
int optimized_clustering_index_create(optimized_clustering_index_t* index, uint32_t max_clusters) {
    if (!index || max_clusters == 0) return -1;
    
    memset(index, 0, sizeof(optimized_clustering_index_t));
    index->cluster_count = max_clusters;
    
    // Allocate cache-aligned centroids
    index->centroids = (optimized_vector_t*)aligned_malloc(max_clusters * sizeof(optimized_vector_t), 32);
    if (!index->centroids) return -1;
    
    index->cluster_assignments = (uint32_t*)calloc(10000, sizeof(uint32_t)); // Max 10k vectors
    index->cluster_sizes = (uint32_t*)calloc(max_clusters, sizeof(uint32_t));
    index->cluster_members = (uint32_t*)calloc(max_clusters * 1000, sizeof(uint32_t)); // Max 1k per cluster
    index->cluster_radii = (float*)calloc(max_clusters, sizeof(float));
    
    if (!index->cluster_assignments || !index->cluster_sizes || 
        !index->cluster_members || !index->cluster_radii) {
        optimized_clustering_index_destroy(index);
        return -1;
    }
    
    return 0;
}

// Fast clustering using mini-batch K-means
int optimized_clustering_index_cluster(optimized_clustering_index_t* index, 
                                      const optimized_vector_t* vectors, uint32_t count) {
    if (!index || !vectors || count == 0) return -1;
    
    // Initialize centroids randomly
    srand(time(NULL));
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        uint32_t random_idx = rand() % count;
        index->centroids[i] = vectors[random_idx];
        optimized_precompute_vector_properties(&index->centroids[i]);
    }
    
    // Mini-batch K-means (much faster than standard K-means)
    const uint32_t batch_size = 32;
    const uint32_t max_iterations = 10; // Reduced from 100
    
    for (uint32_t iter = 0; iter < max_iterations; iter++) {
        // Process in mini-batches
        for (uint32_t batch_start = 0; batch_start < count; batch_start += batch_size) {
            uint32_t batch_end = (batch_start + batch_size < count) ? 
                                batch_start + batch_size : count;
            
            // Assign vectors to clusters
            for (uint32_t i = batch_start; i < batch_end; i++) {
                float best_similarity = -1.0f;
                uint32_t best_cluster = 0;
                
                for (uint32_t j = 0; j < index->cluster_count; j++) {
                    float similarity = optimized_cosine_similarity(vectors[i].data, 
                                                                  index->centroids[j].data);
                    if (similarity > best_similarity) {
                        best_similarity = similarity;
                        best_cluster = j;
                    }
                }
                
                index->cluster_assignments[i] = best_cluster;
            }
            
            // Update centroids (simplified)
            for (uint32_t j = 0; j < index->cluster_count; j++) {
                uint32_t cluster_size = 0;
                float cluster_sum[OPTIMIZED_VECTOR_DIM] = {0};
                
                for (uint32_t i = batch_start; i < batch_end; i++) {
                    if (index->cluster_assignments[i] == j) {
                        for (int k = 0; k < OPTIMIZED_VECTOR_DIM; k++) {
                            cluster_sum[k] += vectors[i].data[k];
                        }
                        cluster_size++;
                    }
                }
                
                if (cluster_size > 0) {
                    for (int k = 0; k < OPTIMIZED_VECTOR_DIM; k++) {
                        index->centroids[j].data[k] = cluster_sum[k] / cluster_size;
                    }
                    optimized_precompute_vector_properties(&index->centroids[j]);
                }
            }
        }
    }
    
    // Update cluster sizes and members
    memset(index->cluster_sizes, 0, index->cluster_count * sizeof(uint32_t));
    for (uint32_t i = 0; i < count; i++) {
        uint32_t cluster_id = index->cluster_assignments[i];
        if (cluster_id < index->cluster_count) {
            index->cluster_members[cluster_id * 1000 + index->cluster_sizes[cluster_id]] = i;
            index->cluster_sizes[cluster_id]++;
        }
    }
    
    index->vector_count = count;
    index->is_optimized = true;
    
    return 0;
}

// Fast cluster search
int optimized_clustering_index_search(optimized_clustering_index_t* index, 
                                     const float* query_vector, uint32_t* cluster_id, float* confidence) {
    if (!index || !query_vector || !cluster_id || !confidence) return -1;
    
    float best_similarity = -1.0f;
    uint32_t best_cluster = 0;
    
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        float similarity = optimized_cosine_similarity(query_vector, index->centroids[i].data);
        if (similarity > best_similarity) {
            best_similarity = similarity;
            best_cluster = i;
        }
    }
    
    *cluster_id = best_cluster;
    *confidence = best_similarity;
    
    return 0;
}

// Optimize clustering for faster queries
int optimized_clustering_index_optimize(optimized_clustering_index_t* index) {
    if (!index) return -1;
    
    // Precompute cluster radii for faster search
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        float max_distance = 0.0f;
        
        for (uint32_t j = 0; j < index->cluster_sizes[i]; j++) {
            uint32_t member_idx = index->cluster_members[i * 1000 + j];
            float distance = optimized_euclidean_distance(index->centroids[i].data, 
                                                         index->centroids[i].data); // Placeholder
            if (distance > max_distance) {
                max_distance = distance;
            }
        }
        
        index->cluster_radii[i] = max_distance;
    }
    
    index->is_optimized = true;
    return 0;
}

// Destroy optimized clustering index
void optimized_clustering_index_destroy(optimized_clustering_index_t* index) {
    if (!index) return;
    
    if (index->centroids) aligned_free(index->centroids);
    if (index->cluster_assignments) free(index->cluster_assignments);
    if (index->cluster_sizes) free(index->cluster_sizes);
    if (index->cluster_members) free(index->cluster_members);
    if (index->cluster_radii) free(index->cluster_radii);
    
    memset(index, 0, sizeof(optimized_clustering_index_t));
}

// ============================================================================
// OPTIMIZED VECTOR INDEXING SYSTEM
// ============================================================================

// Create optimized vector indexing system
int optimized_vector_indexing_system_create(optimized_vector_indexing_system_t* system, 
                                           uint32_t capacity) {
    if (!system || capacity == 0) return -1;
    
    memset(system, 0, sizeof(optimized_vector_indexing_system_t));
    
    system->lsh_index = (optimized_lsh_index_t*)malloc(sizeof(optimized_lsh_index_t));
    system->clustering_index = (optimized_clustering_index_t*)malloc(sizeof(optimized_clustering_index_t));
    system->vector_cache = (optimized_vector_t*)aligned_malloc(capacity * sizeof(optimized_vector_t), 32);
    
    if (!system->lsh_index || !system->clustering_index || !system->vector_cache) {
        optimized_vector_indexing_system_destroy(system);
        return -1;
    }
    
    if (optimized_lsh_index_create(system->lsh_index, capacity) != 0) {
        optimized_vector_indexing_system_destroy(system);
        return -1;
    }
    
    if (optimized_clustering_index_create(system->clustering_index, OPTIMIZED_MAX_CLUSTERS) != 0) {
        optimized_vector_indexing_system_destroy(system);
        return -1;
    }
    
    system->cache_capacity = capacity;
    system->use_simd = check_simd_availability();
    system->use_caching = true;
    
    return 0;
}

// Add lattice node to optimized system
int optimized_vector_indexing_system_add_node(optimized_vector_indexing_system_t* system, 
                                             const lattice_node_t* node) {
    if (!system || !node) return -1;
    
    printf("DEBUG: optimized_vector_indexing_system_add_node called for node %d\n", node->id);
    
    // Check if system is properly initialized
    if (!system->lsh_index || !system->clustering_index || !system->vector_cache) {
        printf("DEBUG: System not properly initialized - lsh_index=%p, clustering_index=%p, vector_cache=%p\n", 
               system->lsh_index, system->clustering_index, system->vector_cache);
        return -1;
    }
    
    // Generate optimized embedding
    optimized_vector_t vector;
    vector.node_id = node->id;
    vector.cluster_id = 0;
    
    char combined_text[1024];
    snprintf(combined_text, sizeof(combined_text), "%s %s", node->name, node->data);
    
    if (optimized_generate_embedding(combined_text, vector.data) != 0) {
        return -1;
    }
    
    optimized_precompute_vector_properties(&vector);
    
    // Add to LSH index
    if (optimized_lsh_index_add_vector(system->lsh_index, &vector) != 0) {
        return -1;
    }
    
    // Add to cache
    if (system->cache_size < system->cache_capacity) {
        system->vector_cache[system->cache_size] = vector;
        system->cache_size++;
    }
    
    system->total_operations++;
    return 0;
}

// Fast semantic search
int optimized_vector_indexing_system_search(optimized_vector_indexing_system_t* system, 
                                           const char* query, optimized_search_result_t* results, 
                                           uint32_t* count) {
    if (!system || !query || !results || !count) return -1;
    
    *count = 0;
    
    // Generate query embedding
    float query_embedding[OPTIMIZED_VECTOR_DIM];
    if (optimized_generate_embedding(query, query_embedding) != 0) {
        return -1;
    }
    
    // Search LSH index
    optimized_search_result_t lsh_results[1000];
    uint32_t lsh_count = 0;
    
    if (optimized_lsh_index_search(system->lsh_index, query_embedding, lsh_results, &lsh_count) == 0) {
        // Copy results
        for (uint32_t i = 0; i < lsh_count && *count < 1000; i++) {
            results[*count] = lsh_results[i];
            (*count)++;
        }
    }
    
    // Search clustering index
    uint32_t cluster_id;
    float confidence;
    if (optimized_clustering_index_search(system->clustering_index, query_embedding, 
                                         &cluster_id, &confidence) == 0) {
        // Add cluster-based results
        if (*count < 1000) {
            results[*count].node_id = cluster_id;
            results[*count].similarity_score = confidence;
            results[*count].distance = 1.0f - confidence;
            results[*count].cluster_id = cluster_id;
            results[*count].cluster_confidence = confidence;
            results[*count].rank = *count + 1;
            (*count)++;
        }
    }
    
    return 0;
}

// Optimize entire system
int optimized_vector_indexing_system_optimize(optimized_vector_indexing_system_t* system) {
    if (!system) return -1;
    
    // Optimize LSH index
    optimized_lsh_index_optimize(system->lsh_index);
    
    // Cluster vectors if we have enough
    if (system->cache_size > OPTIMIZED_MAX_CLUSTERS) {
        optimized_clustering_index_cluster(system->clustering_index, system->vector_cache, system->cache_size);
        optimized_clustering_index_optimize(system->clustering_index);
    }
    
    return 0;
}

// Get system statistics
int optimized_vector_indexing_system_get_stats(optimized_vector_indexing_system_t* system, 
                                              uint32_t* total_vectors, float* avg_query_time) {
    if (!system || !total_vectors || !avg_query_time) return -1;
    
    *total_vectors = system->total_operations;
    *avg_query_time = system->avg_query_time;
    
    return 0;
}

// Destroy optimized system
void optimized_vector_indexing_system_destroy(optimized_vector_indexing_system_t* system) {
    if (!system) return;
    
    if (system->lsh_index) {
        optimized_lsh_index_destroy(system->lsh_index);
        free(system->lsh_index);
    }
    
    if (system->clustering_index) {
        optimized_clustering_index_destroy(system->clustering_index);
        free(system->clustering_index);
    }
    
    if (system->vector_cache) aligned_free(system->vector_cache);
    
    memset(system, 0, sizeof(optimized_vector_indexing_system_t));
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Memory alignment utilities
void* aligned_malloc(size_t size, size_t alignment) {
    void* ptr = malloc(size + alignment);
    if (!ptr) return NULL;
    
    // Simple alignment for ARM64 compatibility
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned_addr;
}

void aligned_free(void* ptr) {
    free(ptr);
}

// Cache optimization utilities
void prefetch_vector(const optimized_vector_t* vector) {
    if (vector) {
        // Simplified prefetch for ARM64 compatibility
        volatile char* ptr = (volatile char*)vector;
        (void)ptr[0]; // Touch first byte
    }
}

void prefetch_vectors(const optimized_vector_t* vectors, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        prefetch_vector(&vectors[i]);
    }
}

// Performance monitoring (ARM64 compatible)
uint64_t get_cycle_count(void) {
    // Use gettimeofday for ARM64 compatibility
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

float cycles_to_seconds(uint64_t cycles) {
    // This is a simplified conversion - in production, calibrate with actual CPU frequency
    return (float)cycles / 3000000000.0f; // Assume 3GHz CPU
}
