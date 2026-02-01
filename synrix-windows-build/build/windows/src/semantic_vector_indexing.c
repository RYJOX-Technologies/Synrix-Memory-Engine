#define _GNU_SOURCE
#include "semantic_vector_indexing.h"
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// ============================================================================
// VECTOR EMBEDDING FUNCTIONS
// ============================================================================

// Generate semantic embedding from text using simple hash-based approach
int generate_semantic_embedding(const char* text, float* embedding) {
    if (!text || !embedding) return -1;
    
    size_t text_len = strlen(text);
    if (text_len == 0) return -1;
    
    // Initialize embedding with zeros
    memset(embedding, 0, VECTOR_DIM * sizeof(float));
    
    // Simple hash-based embedding generation
    // This is a placeholder - in production, use proper word2vec/transformer models
    uint32_t hash = 0x811c9dc5; // FNV offset basis
    
    for (size_t i = 0; i < text_len; i++) {
        hash ^= (uint8_t)text[i];
        hash *= 0x01000193; // FNV prime
    }
    
    // Distribute hash across embedding dimensions
    for (int i = 0; i < VECTOR_DIM; i++) {
        embedding[i] = (float)((hash >> (i % 32)) & 0xFF) / 255.0f;
        embedding[i] = embedding[i] * 2.0f - 1.0f; // Normalize to [-1, 1]
    }
    
    // Add some text-based features
    for (size_t i = 0; i < text_len && i < VECTOR_DIM; i++) {
        embedding[i] += (float)((uint8_t)text[i] - 128) / 128.0f;
    }
    
    // Normalize the vector
    normalize_vector(embedding);
    
    return 0;
}

// Generate semantic embedding from lattice node
int generate_node_embedding(const lattice_node_t* node, float* embedding) {
    if (!node || !embedding) return -1;
    
    // Combine node name and data for embedding
    char combined_text[1024];
    snprintf(combined_text, sizeof(combined_text), "%s %s", node->name, node->data);
    
    return generate_semantic_embedding(combined_text, embedding);
}

// Calculate cosine similarity between two vectors
float calculate_cosine_similarity(const float* vec1, const float* vec2) {
    if (!vec1 || !vec2) return 0.0f;
    
    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    for (int i = 0; i < VECTOR_DIM; i++) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }
    
    if (norm1 == 0.0f || norm2 == 0.0f) return 0.0f;
    
    return dot_product / (sqrtf(norm1) * sqrtf(norm2));
}

// Calculate euclidean distance between two vectors
float calculate_euclidean_distance(const float* vec1, const float* vec2) {
    if (!vec1 || !vec2) return INFINITY;
    
    float distance = 0.0f;
    for (int i = 0; i < VECTOR_DIM; i++) {
        float diff = vec1[i] - vec2[i];
        distance += diff * diff;
    }
    
    return sqrtf(distance);
}

// Normalize vector to unit length
void normalize_vector(float* vector) {
    if (!vector) return;
    
    float magnitude = calculate_vector_magnitude(vector);
    if (magnitude == 0.0f) return;
    
    for (int i = 0; i < VECTOR_DIM; i++) {
        vector[i] /= magnitude;
    }
}

// Calculate vector magnitude
float calculate_vector_magnitude(const float* vector) {
    if (!vector) return 0.0f;
    
    float magnitude = 0.0f;
    for (int i = 0; i < VECTOR_DIM; i++) {
        magnitude += vector[i] * vector[i];
    }
    
    return sqrtf(magnitude);
}

// ============================================================================
// LSH (LOCALITY SENSITIVE HASHING) FUNCTIONS
// ============================================================================

// Create enhanced LSH index
int enhanced_lsh_index_create(enhanced_lsh_index_t* index, uint32_t vector_dim, uint32_t hash_functions) {
    if (!index || vector_dim == 0 || hash_functions == 0) return -1;
    
    memset(index, 0, sizeof(enhanced_lsh_index_t));
    index->vector_dim = vector_dim;
    index->hash_functions = hash_functions;
    index->bucket_count = 1 << (hash_functions / 2); // 2^(hash_functions/2) buckets
    index->capacity = MAX_VECTORS;
    
    index->entries = (enhanced_lsh_entry_t*)calloc(index->capacity, sizeof(enhanced_lsh_entry_t));
    index->buckets = (uint32_t*)calloc(index->bucket_count * 100, sizeof(uint32_t)); // 100 vectors per bucket
    index->bucket_sizes = (uint32_t*)calloc(index->bucket_count, sizeof(uint32_t));
    index->random_vectors = (float*)calloc(hash_functions * vector_dim, sizeof(float));
    index->random_offsets = (float*)calloc(hash_functions, sizeof(float));
    
    if (!index->entries || !index->buckets || !index->bucket_sizes || 
        !index->random_vectors || !index->random_offsets) {
        enhanced_lsh_index_destroy(index);
        return -1;
    }
    
    // Initialize random vectors and offsets
    srand(time(NULL));
    for (uint32_t i = 0; i < hash_functions; i++) {
        for (uint32_t j = 0; j < vector_dim; j++) {
            index->random_vectors[i * vector_dim + j] = random_float_range(-1.0f, 1.0f);
        }
        index->random_offsets[i] = random_float_range(0.0f, 1.0f);
    }
    
    return 0;
}

// Add vector to LSH index
int enhanced_lsh_index_add_vector(enhanced_lsh_index_t* index, const float* vector, uint32_t node_id) {
    if (!index || !vector || node_id == 0 || index->count >= index->capacity) return -1;
    
    enhanced_lsh_entry_t* entry = &index->entries[index->count];
    entry->node_id = node_id;
    entry->hash_function_count = index->hash_functions;
    entry->similarity_threshold = SIMILARITY_THRESHOLD;
    
    // Generate LSH hashes
    for (uint32_t i = 0; i < index->hash_functions; i++) {
        entry->lsh_hashes[i] = generate_lsh_hash(vector, 
                                                &index->random_vectors[i * index->vector_dim],
                                                index->random_offsets[i]);
        entry->bucket_ids[i] = entry->lsh_hashes[i] % index->bucket_count;
    }
    
    // Add to buckets
    for (uint32_t i = 0; i < index->hash_functions; i++) {
        uint32_t bucket_id = entry->bucket_ids[i];
        if (index->bucket_sizes[bucket_id] < 100) { // Max 100 vectors per bucket
            index->buckets[bucket_id * 100 + index->bucket_sizes[bucket_id]] = index->count;
            index->bucket_sizes[bucket_id]++;
        }
    }
    
    index->count++;
    return 0;
}

// Search for similar vectors using LSH
int enhanced_lsh_index_search_similar(enhanced_lsh_index_t* index, const float* query_vector, 
                                     float threshold, vector_similarity_result_t* results, uint32_t* count) {
    (void)threshold; // Parameter reserved for future threshold filtering
    if (!index || !query_vector || !results || !count) return -1;
    
    *count = 0;
    
    // Generate query hashes
    uint64_t query_hashes[8];
    uint32_t query_buckets[8];
    
    for (uint32_t i = 0; i < index->hash_functions; i++) {
        query_hashes[i] = generate_lsh_hash(query_vector, 
                                           &index->random_vectors[i * index->vector_dim],
                                           index->random_offsets[i]);
        query_buckets[i] = query_hashes[i] % index->bucket_count;
    }
    
    // Search in buckets
    for (uint32_t i = 0; i < index->hash_functions; i++) {
        uint32_t bucket_id = query_buckets[i];
        uint32_t bucket_size = index->bucket_sizes[bucket_id];
        
        for (uint32_t j = 0; j < bucket_size && *count < 1000; j++) {
            uint32_t entry_idx = index->buckets[bucket_id * 100 + j];
            enhanced_lsh_entry_t* entry = &index->entries[entry_idx];
            
            // Check if hashes match
            bool hash_match = false;
            for (uint32_t k = 0; k < index->hash_functions; k++) {
                if (entry->lsh_hashes[k] == query_hashes[k]) {
                    hash_match = true;
                    break;
                }
            }
            
            if (hash_match) {
                results[*count].node_id = entry->node_id;
                results[*count].similarity_score = 1.0f; // Placeholder - would calculate actual similarity
                results[*count].distance = 0.0f;
                results[*count].cluster_id = 0;
                results[*count].cluster_confidence = 1.0f;
                results[*count].rank = *count + 1;
                (*count)++;
            }
        }
    }
    
    return 0;
}

// Generate LSH hash for vector
uint64_t generate_lsh_hash(const float* vector, const float* random_vector, float random_offset) {
    if (!vector || !random_vector) return 0;
    
    float dot_product = 0.0f;
    for (int i = 0; i < VECTOR_DIM; i++) {
        dot_product += vector[i] * random_vector[i];
    }
    
    return (uint64_t)((dot_product + random_offset) * 1000000.0f);
}

// Calculate LSH collision probability
float calculate_lsh_collision_probability(float similarity, uint32_t hash_functions) {
    if (similarity < 0.0f || similarity > 1.0f) return 0.0f;
    
    // Simplified collision probability calculation
    float prob = 1.0f - similarity;
    return powf(prob, (float)hash_functions);
}

// Destroy LSH index
void enhanced_lsh_index_destroy(enhanced_lsh_index_t* index) {
    if (!index) return;
    
    if (index->entries) free(index->entries);
    if (index->buckets) free(index->buckets);
    if (index->bucket_sizes) free(index->bucket_sizes);
    if (index->random_vectors) free(index->random_vectors);
    if (index->random_offsets) free(index->random_offsets);
    
    memset(index, 0, sizeof(enhanced_lsh_index_t));
}

// ============================================================================
// K-MEANS CLUSTERING FUNCTIONS
// ============================================================================

// Create enhanced clustering index
int enhanced_clustering_index_create(enhanced_clustering_index_t* index, uint32_t max_clusters, uint32_t vector_dim) {
    if (!index || max_clusters == 0 || vector_dim == 0) return -1;
    
    memset(index, 0, sizeof(enhanced_clustering_index_t));
    index->max_clusters = max_clusters;
    index->vector_dim = vector_dim;
    index->convergence_threshold = 0.001f;
    index->max_iterations = CLUSTERING_MAX_ITERATIONS;
    
    index->clusters = (enhanced_semantic_cluster_t*)calloc(max_clusters, sizeof(enhanced_semantic_cluster_t));
    if (!index->clusters) return -1;
    
    // Initialize clusters
    for (uint32_t i = 0; i < max_clusters; i++) {
        index->clusters[i].cluster_id = i;
        index->clusters[i].member_capacity = 1000;
        index->clusters[i].member_vectors = (uint32_t*)calloc(1000, sizeof(uint32_t));
        if (!index->clusters[i].member_vectors) {
            enhanced_clustering_index_destroy(index);
            return -1;
        }
    }
    
    return 0;
}

// Add vector to clustering index
int enhanced_clustering_index_add_vector(enhanced_clustering_index_t* index, const float* vector, uint32_t node_id) {
    if (!index || !vector) return -1;
    
    // Find closest cluster
    uint32_t closest_cluster = 0;
    float min_distance = INFINITY;
    
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        float distance = calculate_euclidean_distance(vector, index->clusters[i].centroid);
        if (distance < min_distance) {
            min_distance = distance;
            closest_cluster = i;
        }
    }
    
    // Add to closest cluster
    if (index->clusters[closest_cluster].member_count < index->clusters[closest_cluster].member_capacity) {
        index->clusters[closest_cluster].member_vectors[index->clusters[closest_cluster].member_count] = node_id;
        index->clusters[closest_cluster].member_count++;
    }
    
    return 0;
}

// Perform K-means clustering
int enhanced_clustering_index_cluster(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count) {
    if (!index || !vectors || vector_count == 0) return -1;
    
    // Initialize centroids randomly
    srand(time(NULL));
    for (uint32_t i = 0; i < index->max_clusters; i++) {
        uint32_t random_idx = rand() % vector_count;
        memcpy(index->clusters[i].centroid, vectors[random_idx].embedding, VECTOR_DIM * sizeof(float));
        index->clusters[i].member_count = 0;
    }
    
    index->cluster_count = index->max_clusters;
    index->iteration_count = 0;
    index->converged = false;
    
    // K-means iterations
    for (uint32_t iter = 0; iter < index->max_iterations; iter++) {
        index->iteration_count = iter + 1;
        
        // Assign vectors to clusters
        if (assign_vectors_to_clusters(index, vectors, vector_count) != 0) {
            return -1;
        }
        
        // Update centroids
        if (update_cluster_centroids(index, vectors, vector_count) != 0) {
            return -1;
        }
        
        // Check convergence
        float total_inertia = 0.0f;
        for (uint32_t i = 0; i < index->cluster_count; i++) {
            for (uint32_t j = 0; j < index->clusters[i].member_count; j++) {
                uint32_t vector_idx = index->clusters[i].member_vectors[j];
                float distance = calculate_euclidean_distance(vectors[vector_idx].embedding, index->clusters[i].centroid);
                total_inertia += distance * distance;
            }
        }
        
        if (iter > 0 && fabsf(index->inertia - total_inertia) < index->convergence_threshold) {
            index->converged = true;
            break;
        }
        
        index->inertia = total_inertia;
    }
    
    // Calculate final metrics
    calculate_cluster_metrics(index, vectors, vector_count);
    index->silhouette_score = calculate_cluster_silhouette_score(index, vectors, vector_count);
    
    return 0;
}

// Search vectors by cluster
int enhanced_clustering_index_search_by_cluster(enhanced_clustering_index_t* index, uint32_t cluster_id,
                                               vector_similarity_result_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    if (cluster_id >= index->cluster_count) return -1;
    
    enhanced_semantic_cluster_t* cluster = &index->clusters[cluster_id];
    for (uint32_t i = 0; i < cluster->member_count && *count < 1000; i++) {
        results[*count].node_id = cluster->member_vectors[i];
        results[*count].similarity_score = 1.0f; // Placeholder
        results[*count].distance = 0.0f;
        results[*count].cluster_id = cluster_id;
        results[*count].cluster_confidence = cluster->stability_score;
        results[*count].rank = *count + 1;
        (*count)++;
    }
    
    return 0;
}

// Calculate cluster quality metrics
float calculate_cluster_silhouette_score(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count) {
    if (!index || !vectors || vector_count == 0) return 0.0f;
    
    float total_silhouette = 0.0f;
    uint32_t valid_vectors = 0;
    
    for (uint32_t i = 0; i < vector_count; i++) {
        float a = 0.0f; // Average distance within cluster
        float b = INFINITY; // Minimum average distance to other clusters
        
        // Calculate a (intra-cluster distance)
        uint32_t cluster_id = vectors[i].cluster_id;
        if (cluster_id < index->cluster_count) {
            uint32_t cluster_size = index->clusters[cluster_id].member_count;
            if (cluster_size > 1) {
                for (uint32_t j = 0; j < cluster_size; j++) {
                    uint32_t other_vector_idx = index->clusters[cluster_id].member_vectors[j];
                    if (other_vector_idx != i) {
                        a += calculate_euclidean_distance(vectors[i].embedding, vectors[other_vector_idx].embedding);
                    }
                }
                a /= (cluster_size - 1);
            }
        }
        
        // Calculate b (inter-cluster distance)
        for (uint32_t c = 0; c < index->cluster_count; c++) {
            if (c != cluster_id) {
                float cluster_distance = 0.0f;
                uint32_t cluster_size = index->clusters[c].member_count;
                if (cluster_size > 0) {
                    for (uint32_t j = 0; j < cluster_size; j++) {
                        uint32_t other_vector_idx = index->clusters[c].member_vectors[j];
                        cluster_distance += calculate_euclidean_distance(vectors[i].embedding, vectors[other_vector_idx].embedding);
                    }
                    cluster_distance /= cluster_size;
                    if (cluster_distance < b) {
                        b = cluster_distance;
                    }
                }
            }
        }
        
        if (b != INFINITY) {
            float silhouette = (b - a) / fmaxf(a, b);
            total_silhouette += silhouette;
            valid_vectors++;
        }
    }
    
    return valid_vectors > 0 ? total_silhouette / valid_vectors : 0.0f;
}

// Update cluster centroids
int update_cluster_centroids(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count) {
    (void)vector_count; // Parameter used for API consistency
    if (!index || !vectors) return -1;
    
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        enhanced_semantic_cluster_t* cluster = &index->clusters[i];
        
        if (cluster->member_count == 0) continue;
        
        // Reset centroid
        memset(cluster->centroid, 0, VECTOR_DIM * sizeof(float));
        
        // Sum all vectors in cluster
        for (uint32_t j = 0; j < cluster->member_count; j++) {
            uint32_t vector_idx = cluster->member_vectors[j];
            for (int k = 0; k < VECTOR_DIM; k++) {
                cluster->centroid[k] += vectors[vector_idx].embedding[k];
            }
        }
        
        // Average to get centroid
        for (int k = 0; k < VECTOR_DIM; k++) {
            cluster->centroid[k] /= cluster->member_count;
        }
    }
    
    return 0;
}

// Assign vectors to clusters
int assign_vectors_to_clusters(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count) {
    if (!index || !vectors) return -1;
    
    // Reset cluster memberships
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        index->clusters[i].member_count = 0;
    }
    
    // Assign each vector to closest cluster
    for (uint32_t i = 0; i < vector_count; i++) {
        uint32_t closest_cluster = 0;
        float min_distance = INFINITY;
        
        for (uint32_t j = 0; j < index->cluster_count; j++) {
            float distance = calculate_euclidean_distance(vectors[i].embedding, index->clusters[j].centroid);
            if (distance < min_distance) {
                min_distance = distance;
                closest_cluster = j;
            }
        }
        
        // Add to closest cluster
        enhanced_semantic_cluster_t* cluster = &index->clusters[closest_cluster];
        if (cluster->member_count < cluster->member_capacity) {
            cluster->member_vectors[cluster->member_count] = i;
            cluster->member_count++;
        }
    }
    
    return 0;
}

// Calculate cluster cohesion and separation
int calculate_cluster_metrics(enhanced_clustering_index_t* index, const enhanced_semantic_vector_t* vectors, uint32_t vector_count) {
    (void)vector_count; // Parameter used for API consistency
    if (!index || !vectors) return -1;
    
    for (uint32_t i = 0; i < index->cluster_count; i++) {
        enhanced_semantic_cluster_t* cluster = &index->clusters[i];
        
        if (cluster->member_count == 0) {
            cluster->cohesion = 0.0f;
            cluster->separation = 0.0f;
            cluster->density = 0.0f;
            cluster->radius = 0.0f;
            continue;
        }
        
        // Calculate cohesion (average intra-cluster distance)
        float total_distance = 0.0f;
        for (uint32_t j = 0; j < cluster->member_count; j++) {
            uint32_t vector_idx = cluster->member_vectors[j];
            float distance = calculate_euclidean_distance(vectors[vector_idx].embedding, cluster->centroid);
            total_distance += distance;
        }
        cluster->cohesion = total_distance / cluster->member_count;
        cluster->radius = cluster->cohesion;
        
        // Calculate density (members per unit volume)
        cluster->density = (float)cluster->member_count / (cluster->radius + 1.0f);
        
        // Calculate separation (minimum distance to other clusters)
        cluster->separation = INFINITY;
        for (uint32_t j = 0; j < index->cluster_count; j++) {
            if (i != j && index->clusters[j].member_count > 0) {
                float distance = calculate_euclidean_distance(cluster->centroid, index->clusters[j].centroid);
                if (distance < cluster->separation) {
                    cluster->separation = distance;
                }
            }
        }
        
        // Calculate stability score
        cluster->stability_score = fminf(1.0f, cluster->density / (cluster->separation + 1.0f));
    }
    
    return 0;
}

// Destroy clustering index
void enhanced_clustering_index_destroy(enhanced_clustering_index_t* index) {
    if (!index) return;
    
    if (index->clusters) {
        for (uint32_t i = 0; i < index->max_clusters; i++) {
            if (index->clusters[i].member_vectors) {
                free(index->clusters[i].member_vectors);
            }
        }
        free(index->clusters);
    }
    
    memset(index, 0, sizeof(enhanced_clustering_index_t));
}

// ============================================================================
// SEMANTIC VECTOR INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create semantic vector indexing system
int semantic_vector_indexing_system_create(semantic_vector_indexing_system_t* system) {
    if (!system) return -1;
    
    memset(system, 0, sizeof(semantic_vector_indexing_system_t));
    
    system->vector_capacity = MAX_VECTORS;
    system->vectors = (enhanced_semantic_vector_t*)calloc(system->vector_capacity, sizeof(enhanced_semantic_vector_t));
    if (!system->vectors) return -1;
    
    system->lsh_index = (enhanced_lsh_index_t*)malloc(sizeof(enhanced_lsh_index_t));
    system->clustering = (enhanced_clustering_index_t*)malloc(sizeof(enhanced_clustering_index_t));
    
    if (!system->lsh_index || !system->clustering) {
        semantic_vector_indexing_system_destroy(system);
        return -1;
    }
    
    if (enhanced_lsh_index_create(system->lsh_index, VECTOR_DIM, 8) != 0) {
        semantic_vector_indexing_system_destroy(system);
        return -1;
    }
    
    if (enhanced_clustering_index_create(system->clustering, 100, VECTOR_DIM) != 0) {
        semantic_vector_indexing_system_destroy(system);
        return -1;
    }
    
    system->is_initialized = true;
    return 0;
}

// Add lattice node to vector indexing system
int semantic_vector_indexing_system_add_node(semantic_vector_indexing_system_t* system, const lattice_node_t* node) {
    if (!system || !node || !system->is_initialized || system->vector_count >= system->vector_capacity) return -1;
    
    enhanced_semantic_vector_t* vector = &system->vectors[system->vector_count];
    vector->node_id = node->id;
    vector->frequency = 1;
    vector->last_accessed = node->timestamp;
    vector->semantic_weight = (float)node->confidence;
    vector->related_count = 0;
    
    // Generate embedding
    if (generate_node_embedding(node, vector->embedding) != 0) {
        return -1;
    }
    
    // Add to LSH index
    enhanced_lsh_index_add_vector(system->lsh_index, vector->embedding, node->id);
    
    // Add to clustering index
    enhanced_clustering_index_add_vector(system->clustering, vector->embedding, node->id);
    
    system->vector_count++;
    system->last_update = node->timestamp;
    
    return 0;
}

// Search for similar nodes using vector similarity
int semantic_vector_indexing_system_search_similar(semantic_vector_indexing_system_t* system, 
                                                  const vector_similarity_query_t* query,
                                                  vector_similarity_result_t* results, uint32_t* count) {
    if (!system || !query || !results || !count || !system->is_initialized) return -1;
    
    *count = 0;
    
    if (query->use_lsh) {
        // Use LSH for fast search
        enhanced_lsh_index_search_similar(system->lsh_index, query->query_vector, 
                                        query->min_similarity, results, count);
    } else {
        // Use brute force search
        for (uint32_t i = 0; i < system->vector_count && *count < query->max_results; i++) {
            float similarity = calculate_cosine_similarity(query->query_vector, system->vectors[i].embedding);
            
            if (similarity >= query->min_similarity) {
                if (query->cluster_filter == 0 || system->vectors[i].cluster_id == query->cluster_filter) {
                    results[*count].node_id = system->vectors[i].node_id;
                    results[*count].similarity_score = similarity;
                    results[*count].distance = calculate_euclidean_distance(query->query_vector, system->vectors[i].embedding);
                    results[*count].cluster_id = system->vectors[i].cluster_id;
                    results[*count].cluster_confidence = system->vectors[i].cluster_confidence;
                    results[*count].rank = *count + 1;
                    (*count)++;
                }
            }
        }
        
        // Sort results by similarity
        sort_similarity_results(results, *count);
    }
    
    return 0;
}

// Update vector embeddings
int semantic_vector_indexing_system_update_embeddings(semantic_vector_indexing_system_t* system) {
    if (!system || !system->is_initialized) return -1;
    
    // Rebuild LSH index
    enhanced_lsh_index_destroy(system->lsh_index);
    if (enhanced_lsh_index_create(system->lsh_index, VECTOR_DIM, 8) != 0) {
        return -1;
    }
    
    // Rebuild clustering
    return semantic_vector_indexing_system_rebuild_clustering(system);
}

// Rebuild clustering
int semantic_vector_indexing_system_rebuild_clustering(semantic_vector_indexing_system_t* system) {
    if (!system || !system->is_initialized) return -1;
    
    // Rebuild clustering index
    enhanced_clustering_index_destroy(system->clustering);
    if (enhanced_clustering_index_create(system->clustering, 100, VECTOR_DIM) != 0) {
        return -1;
    }
    
    // Perform clustering
    return enhanced_clustering_index_cluster(system->clustering, system->vectors, system->vector_count);
}

// Get vector statistics
int semantic_vector_indexing_system_get_stats(semantic_vector_indexing_system_t* system, 
                                             uint32_t* vector_count, uint32_t* cluster_count, 
                                             float* avg_similarity, float* silhouette_score) {
    if (!system || !vector_count || !cluster_count || !avg_similarity || !silhouette_score) return -1;
    
    *vector_count = system->vector_count;
    *cluster_count = system->clustering->cluster_count;
    *silhouette_score = system->clustering->silhouette_score;
    
    // Calculate average similarity
    float total_similarity = 0.0f;
    uint32_t comparisons = 0;
    
    for (uint32_t i = 0; i < system->vector_count; i++) {
        for (uint32_t j = i + 1; j < system->vector_count; j++) {
            total_similarity += calculate_cosine_similarity(system->vectors[i].embedding, system->vectors[j].embedding);
            comparisons++;
        }
    }
    
    *avg_similarity = comparisons > 0 ? total_similarity / comparisons : 0.0f;
    
    return 0;
}

// Destroy semantic vector indexing system
void semantic_vector_indexing_system_destroy(semantic_vector_indexing_system_t* system) {
    if (!system) return;
    
    if (system->vectors) free(system->vectors);
    if (system->lsh_index) {
        enhanced_lsh_index_destroy(system->lsh_index);
        free(system->lsh_index);
    }
    if (system->clustering) {
        enhanced_clustering_index_destroy(system->clustering);
        free(system->clustering);
    }
    
    memset(system, 0, sizeof(semantic_vector_indexing_system_t));
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Sort similarity results by score
void sort_similarity_results(vector_similarity_result_t* results, uint32_t count) {
    if (!results || count == 0) return;
    
    // Simple bubble sort by similarity score (descending)
    for (uint32_t i = 0; i < count - 1; i++) {
        for (uint32_t j = 0; j < count - i - 1; j++) {
            if (results[j].similarity_score < results[j + 1].similarity_score) {
                vector_similarity_result_t temp = results[j];
                results[j] = results[j + 1];
                results[j + 1] = temp;
            }
        }
    }
}

// Generate random float in range [0, 1]
float random_float(void) {
    return (float)rand() / RAND_MAX;
}

// Generate random float in range [min, max]
float random_float_range(float min, float max) {
    return min + (max - min) * random_float();
}
