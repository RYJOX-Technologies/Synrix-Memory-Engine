#define _GNU_SOURCE
#include "advanced_indexing.h"
#include "semantic_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

// ============================================================================
// MULTI-DIMENSIONAL INDEXING IMPLEMENTATION
// ============================================================================

// B-tree implementation
int btree_index_create(btree_index_t* index, uint32_t order) {
    if (!index || order < 2) return -1;
    
    memset(index, 0, sizeof(btree_index_t));
    index->order = order;
    index->height = 0;
    index->node_count = 0;
    index->next_node_id = 1;
    
    // Allocate initial nodes
    index->nodes = (btree_node_t*)calloc(1000, sizeof(btree_node_t));
    if (!index->nodes) return -1;
    
    return 0;
}

int btree_index_insert(btree_index_t* index, multi_dim_entry_t* entry) {
    if (!index || !entry) return -1;
    
    // If tree is empty, create root
    if (index->node_count == 0) {
        btree_node_t* root = &index->nodes[0];
        root->entries = (multi_dim_entry_t*)calloc(index->order * 2 - 1, sizeof(multi_dim_entry_t));
        root->children = (uint32_t*)calloc(index->order * 2, sizeof(uint32_t));
        root->entry_count = 0;
        root->is_leaf = 1;
        root->parent_id = 0;
        
        root->entries[0] = *entry;
        root->entry_count = 1;
        index->root_id = 0;
        index->node_count = 1;
        index->height = 1;
        return 0;
    }
    
    // TODO: Implement full B-tree insertion with splitting
    // This is a simplified version for demonstration
    return 0;
}

int btree_index_search_range(btree_index_t* index, uint32_t min_key, uint32_t max_key,
                           uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    // TODO: Implement B-tree range search
    // This is a simplified version for demonstration
    return 0;
}

void btree_index_destroy(btree_index_t* index) {
    if (!index) return;
    
    if (index->nodes) {
        for (uint32_t i = 0; i < index->node_count; i++) {
            if (index->nodes[i].entries) {
                free(index->nodes[i].entries);
            }
            if (index->nodes[i].children) {
                free(index->nodes[i].children);
            }
        }
        free(index->nodes);
    }
    
    memset(index, 0, sizeof(btree_index_t));
}

// R-tree implementation
int rtree_index_create(rtree_index_t* index, uint32_t max_entries) {
    if (!index || max_entries < 2) return -1;
    
    memset(index, 0, sizeof(rtree_index_t));
    index->max_entries = max_entries;
    index->height = 0;
    index->node_count = 0;
    index->next_node_id = 1;
    
    // Allocate initial nodes
    index->nodes = (rtree_node_t*)calloc(1000, sizeof(rtree_node_t));
    if (!index->nodes) return -1;
    
    return 0;
}

int rtree_index_insert(rtree_index_t* index, spatial_entry_t* entry) {
    if (!index || !entry) return -1;
    
    // TODO: Implement R-tree insertion
    return 0;
}

int rtree_index_search_spatial(rtree_index_t* index, float* query_bounds,
                             uint32_t* results, uint32_t* count) {
    if (!index || !query_bounds || !results || !count) return -1;
    
    *count = 0;
    
    // TODO: Implement R-tree spatial search
    return 0;
}

void rtree_index_destroy(rtree_index_t* index) {
    if (!index) return;
    
    if (index->nodes) {
        for (uint32_t i = 0; i < index->node_count; i++) {
            if (index->nodes[i].entries) {
                free(index->nodes[i].entries);
            }
            if (index->nodes[i].children) {
                free(index->nodes[i].children);
            }
        }
        free(index->nodes);
    }
    
    memset(index, 0, sizeof(rtree_index_t));
}

// Composite index implementation
int composite_index_create(composite_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(composite_index_t));
    index->capacity = capacity;
    
    index->entries = (composite_entry_t*)calloc(capacity, sizeof(composite_entry_t));
    if (!index->entries) return -1;
    
    index->domain_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    index->complexity_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    index->performance_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    index->timestamp_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    
    if (!index->domain_index || !index->complexity_index || 
        !index->performance_index || !index->timestamp_index) {
        composite_index_destroy(index);
        return -1;
    }
    
    return 0;
}

int composite_index_insert(composite_index_t* index, composite_entry_t* entry) {
    if (!index || !entry || index->count >= index->capacity) return -1;
    
    // Add entry
    index->entries[index->count] = *entry;
    
    // Update indexes
    index->domain_index[index->count] = index->count;
    index->complexity_index[index->count] = index->count;
    index->performance_index[index->count] = index->count;
    index->timestamp_index[index->count] = index->count;
    
    index->count++;
    return 0;
}

int composite_index_search_multi_criteria(composite_index_t* index,
                                        uint32_t domain_flags, uint32_t min_complexity,
                                        uint32_t min_performance, uint64_t min_timestamp,
                                        uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        composite_entry_t* entry = &index->entries[i];
        
        if ((domain_flags == 0 || (entry->domain_flags & domain_flags)) &&
            entry->complexity >= min_complexity &&
            entry->performance >= min_performance &&
            entry->timestamp >= min_timestamp) {
            results[*count] = entry->node_id;
            (*count)++;
        }
    }
    
    return 0;
}

void composite_index_destroy(composite_index_t* index) {
    if (!index) return;
    
    if (index->entries) free(index->entries);
    if (index->domain_index) free(index->domain_index);
    if (index->complexity_index) free(index->complexity_index);
    if (index->performance_index) free(index->performance_index);
    if (index->timestamp_index) free(index->timestamp_index);
    
    memset(index, 0, sizeof(composite_index_t));
}

// ============================================================================
// SEMANTIC VECTOR INDEXING IMPLEMENTATION
// ============================================================================

// LSH implementation
int lsh_index_create(lsh_index_t* index, uint32_t vector_dim, uint32_t hash_functions) {
    if (!index || vector_dim == 0 || hash_functions == 0) return -1;
    
    memset(index, 0, sizeof(lsh_index_t));
    index->vector_dim = vector_dim;
    index->hash_functions = hash_functions;
    index->bucket_count = 1 << (hash_functions / 2); // 2^(hash_functions/2) buckets
    index->capacity = 10000;
    
    index->entries = (lsh_entry_t*)calloc(index->capacity, sizeof(lsh_entry_t));
    index->buckets = (uint32_t*)calloc(index->bucket_count, sizeof(uint32_t));
    
    if (!index->entries || !index->buckets) {
        lsh_index_destroy(index);
        return -1;
    }
    
    return 0;
}

int lsh_index_insert(lsh_index_t* index, lsh_entry_t* entry) {
    if (!index || !entry || index->count >= index->capacity) return -1;
    
    index->entries[index->count] = *entry;
    index->count++;
    
    // Add to bucket
    uint32_t bucket_id = entry->bucket_id % index->bucket_count;
    // TODO: Implement proper bucket chaining
    
    return 0;
}

int lsh_index_search_similar(lsh_index_t* index, uint64_t query_hash, float threshold,
                           uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    // TODO: Implement LSH similarity search
    return 0;
}

void lsh_index_destroy(lsh_index_t* index) {
    if (!index) return;
    
    if (index->entries) free(index->entries);
    if (index->buckets) free(index->buckets);
    
    memset(index, 0, sizeof(lsh_index_t));
}

// Clustering implementation
int clustering_index_create(clustering_index_t* index, uint32_t max_clusters, uint32_t vector_dim) {
    if (!index || max_clusters == 0 || vector_dim == 0) return -1;
    
    memset(index, 0, sizeof(clustering_index_t));
    index->max_clusters = max_clusters;
    index->vector_dim = vector_dim;
    index->convergence_threshold = 0.001f;
    
    index->clusters = (semantic_cluster_t*)calloc(max_clusters, sizeof(semantic_cluster_t));
    if (!index->clusters) return -1;
    
    return 0;
}

int clustering_index_add_vector(clustering_index_t* index, semantic_vector_t* vector) {
    if (!index || !vector) return -1;
    
    // TODO: Implement K-means clustering
    return 0;
}

int clustering_index_search_by_cluster(clustering_index_t* index, uint32_t cluster_id,
                                     uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    if (cluster_id >= index->cluster_count) return -1;
    
    semantic_cluster_t* cluster = &index->clusters[cluster_id];
    for (uint32_t i = 0; i < cluster->member_count; i++) {
        results[*count] = cluster->member_nodes[i];
        (*count)++;
    }
    
    return 0;
}

void clustering_index_destroy(clustering_index_t* index) {
    if (!index) return;
    
    if (index->clusters) {
        for (uint32_t i = 0; i < index->cluster_count; i++) {
            if (index->clusters[i].member_nodes) {
                free(index->clusters[i].member_nodes);
            }
        }
        free(index->clusters);
    }
    
    memset(index, 0, sizeof(clustering_index_t));
}

// ============================================================================
// HIERARCHICAL INDEXING IMPLEMENTATION
// ============================================================================

int hierarchical_index_create(hierarchical_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(hierarchical_index_t));
    index->capacity = capacity;
    
    index->entries = (hierarchical_entry_t*)calloc(capacity, sizeof(hierarchical_entry_t));
    if (!index->entries) return -1;
    
    return 0;
}

int hierarchical_index_insert(hierarchical_index_t* index, hierarchical_entry_t* entry) {
    if (!index || !entry || index->count >= index->capacity) return -1;
    
    index->entries[index->count] = *entry;
    index->count++;
    
    if (entry->level > index->max_level) {
        index->max_level = entry->level;
    }
    
    return 0;
}

int hierarchical_index_search_by_path(hierarchical_index_t* index, const char* path,
                                    uint32_t* results, uint32_t* count) {
    if (!index || !path || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        if (strstr(index->entries[i].path, path) != NULL) {
            results[*count] = index->entries[i].node_id;
            (*count)++;
        }
    }
    
    return 0;
}

void hierarchical_index_destroy(hierarchical_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        for (uint32_t i = 0; i < index->count; i++) {
            if (index->entries[i].children) {
                free(index->entries[i].children);
            }
        }
        free(index->entries);
    }
    
    memset(index, 0, sizeof(hierarchical_index_t));
}

// B+ tree implementation
int bplus_index_create(bplus_index_t* index, uint32_t order) {
    if (!index || order < 2) return -1;
    
    memset(index, 0, sizeof(bplus_index_t));
    index->order = order;
    index->height = 0;
    index->node_count = 0;
    index->next_node_id = 1;
    
    index->nodes = (bplus_node_t*)calloc(1000, sizeof(bplus_node_t));
    if (!index->nodes) return -1;
    
    return 0;
}

int bplus_index_insert(bplus_index_t* index, uint32_t node_id, uint32_t sort_key) {
    if (!index) return -1;
    
    // TODO: Implement B+ tree insertion
    return 0;
}

int bplus_index_search_range(bplus_index_t* index, uint32_t min_key, uint32_t max_key,
                           uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    // TODO: Implement B+ tree range search
    return 0;
}

void bplus_index_destroy(bplus_index_t* index) {
    if (!index) return;
    
    if (index->nodes) {
        for (uint32_t i = 0; i < index->node_count; i++) {
            if (index->nodes[i].children) {
                free(index->nodes[i].children);
            }
        }
        free(index->nodes);
    }
    
    memset(index, 0, sizeof(bplus_index_t));
}

// ============================================================================
// SPECIALIZED INDEXES IMPLEMENTATION
// ============================================================================

// Bloom filter implementation
int bloom_filter_create(bloom_filter_t* filter, uint32_t expected_elements, float false_positive_rate) {
    if (!filter || expected_elements == 0 || false_positive_rate <= 0.0f || false_positive_rate >= 1.0f) {
        return -1;
    }
    
    memset(filter, 0, sizeof(bloom_filter_t));
    
    // Calculate optimal array size and hash count
    float ln2 = log(2.0);
    float ln_fp = log(false_positive_rate);
    
    filter->array_size = (uint32_t)(-(expected_elements * ln_fp) / (ln2 * ln2));
    filter->hash_count = (uint32_t)((filter->array_size / expected_elements) * ln2);
    
    // Round up to nearest byte
    filter->array_size = (filter->array_size + 7) / 8;
    
    filter->bit_array = (uint8_t*)calloc(filter->array_size, sizeof(uint8_t));
    if (!filter->bit_array) return -1;
    
    filter->element_count = 0;
    filter->false_positive_rate = false_positive_rate;
    
    return 0;
}

int bloom_filter_add(bloom_filter_t* filter, const char* key) {
    if (!filter || !key) return -1;
    
    size_t key_len = strlen(key);
    
    for (uint32_t i = 0; i < filter->hash_count; i++) {
        uint32_t hash = semantic_hash(key, key_len) + i;
        uint32_t bit_pos = hash % (filter->array_size * 8);
        uint32_t byte_pos = bit_pos / 8;
        uint32_t bit_offset = bit_pos % 8;
        
        filter->bit_array[byte_pos] |= (1 << bit_offset);
    }
    
    filter->element_count++;
    return 0;
}

bool bloom_filter_contains(bloom_filter_t* filter, const char* key) {
    if (!filter || !key) return false;
    
    size_t key_len = strlen(key);
    
    for (uint32_t i = 0; i < filter->hash_count; i++) {
        uint32_t hash = semantic_hash(key, key_len) + i;
        uint32_t bit_pos = hash % (filter->array_size * 8);
        uint32_t byte_pos = bit_pos / 8;
        uint32_t bit_offset = bit_pos % 8;
        
        if (!(filter->bit_array[byte_pos] & (1 << bit_offset))) {
            return false;
        }
    }
    
    return true;
}

void bloom_filter_destroy(bloom_filter_t* filter) {
    if (!filter) return;
    
    if (filter->bit_array) {
        free(filter->bit_array);
    }
    
    memset(filter, 0, sizeof(bloom_filter_t));
}

// Inverted index implementation
int inverted_index_create(inverted_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(inverted_index_t));
    index->capacity = capacity;
    index->total_documents = 0;
    
    index->entries = (inverted_index_entry_t*)calloc(capacity, sizeof(inverted_index_entry_t));
    if (!index->entries) return -1;
    
    return 0;
}

int inverted_index_add_term(inverted_index_t* index, const char* term, uint32_t node_id) {
    if (!index || !term || node_id == 0) return -1;
    
    // Find existing term or create new one
    uint32_t term_index = 0;
    bool found = false;
    
    for (uint32_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].term, term) == 0) {
            term_index = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        if (index->count >= index->capacity) return -1;
        
        term_index = index->count;
        strncpy(index->entries[term_index].term, term, sizeof(index->entries[term_index].term) - 1);
        index->entries[term_index].node_ids = (uint32_t*)calloc(1000, sizeof(uint32_t));
        index->entries[term_index].capacity = 1000;
        index->entries[term_index].count = 0;
        index->count++;
    }
    
    // Add node ID to term
    if (index->entries[term_index].count < index->entries[term_index].capacity) {
        index->entries[term_index].node_ids[index->entries[term_index].count] = node_id;
        index->entries[term_index].count++;
    }
    
    return 0;
}

int inverted_index_search_text(inverted_index_t* index, const char* query,
                             uint32_t* results, uint32_t* count) {
    if (!index || !query || !results || !count) return -1;
    
    *count = 0;
    
    // Simple term matching (TODO: implement proper text search)
    for (uint32_t i = 0; i < index->count; i++) {
        if (strstr(index->entries[i].term, query) != NULL) {
            for (uint32_t j = 0; j < index->entries[i].count; j++) {
                results[*count] = index->entries[i].node_ids[j];
                (*count)++;
            }
        }
    }
    
    return 0;
}

void inverted_index_destroy(inverted_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        for (uint32_t i = 0; i < index->count; i++) {
            if (index->entries[i].node_ids) {
                free(index->entries[i].node_ids);
            }
        }
        free(index->entries);
    }
    
    memset(index, 0, sizeof(inverted_index_t));
}

// Temporal index implementation
int temporal_index_create(temporal_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(temporal_index_t));
    index->capacity = capacity;
    index->time_range_start = UINT64_MAX;
    index->time_range_end = 0;
    
    index->entries = (temporal_entry_t*)calloc(capacity, sizeof(temporal_entry_t));
    if (!index->entries) return -1;
    
    return 0;
}

int temporal_index_insert(temporal_index_t* index, temporal_entry_t* entry) {
    if (!index || !entry || index->count >= index->capacity) return -1;
    
    index->entries[index->count] = *entry;
    index->count++;
    
    if (entry->start_time < index->time_range_start) {
        index->time_range_start = entry->start_time;
    }
    if (entry->end_time > index->time_range_end) {
        index->time_range_end = entry->end_time;
    }
    
    return 0;
}

int temporal_index_search_time_range(temporal_index_t* index, uint64_t start_time, uint64_t end_time,
                                   uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        temporal_entry_t* entry = &index->entries[i];
        
        // Check for time overlap
        if (entry->start_time <= end_time && entry->end_time >= start_time) {
            results[*count] = entry->node_id;
            (*count)++;
        }
    }
    
    return 0;
}

void temporal_index_destroy(temporal_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        free(index->entries);
    }
    
    memset(index, 0, sizeof(temporal_index_t));
}

// ============================================================================
// MASTER INDEXING SYSTEM IMPLEMENTATION
// ============================================================================

int advanced_indexing_system_create(advanced_indexing_system_t* system) {
    if (!system) return -1;
    
    memset(system, 0, sizeof(advanced_indexing_system_t));
    
    // Create multi-dimensional indexes
    system->complexity_btree = (btree_index_t*)malloc(sizeof(btree_index_t));
    system->performance_btree = (btree_index_t*)malloc(sizeof(btree_index_t));
    system->timestamp_btree = (btree_index_t*)malloc(sizeof(btree_index_t));
    system->domain_rtree = (rtree_index_t*)malloc(sizeof(rtree_index_t));
    system->composite_index = (composite_index_t*)malloc(sizeof(composite_index_t));
    
    if (!system->complexity_btree || !system->performance_btree || 
        !system->timestamp_btree || !system->domain_rtree || !system->composite_index) {
        advanced_indexing_system_destroy(system);
        return -1;
    }
    
    btree_index_create(system->complexity_btree, 10);
    btree_index_create(system->performance_btree, 10);
    btree_index_create(system->timestamp_btree, 10);
    rtree_index_create(system->domain_rtree, 10);
    composite_index_create(system->composite_index, 10000);
    
    // Create semantic vector indexes
    system->semantic_vectors = (semantic_vector_t*)calloc(10000, sizeof(semantic_vector_t));
    system->lsh_index = (lsh_index_t*)malloc(sizeof(lsh_index_t));
    system->clustering_index = (clustering_index_t*)malloc(sizeof(clustering_index_t));
    
    if (!system->semantic_vectors || !system->lsh_index || !system->clustering_index) {
        advanced_indexing_system_destroy(system);
        return -1;
    }
    
    lsh_index_create(system->lsh_index, 128, 8);
    clustering_index_create(system->clustering_index, 100, 128);
    
    // Create hierarchical indexes
    system->hierarchical_index = (hierarchical_index_t*)malloc(sizeof(hierarchical_index_t));
    system->ordered_index = (bplus_index_t*)malloc(sizeof(bplus_index_t));
    
    if (!system->hierarchical_index || !system->ordered_index) {
        advanced_indexing_system_destroy(system);
        return -1;
    }
    
    hierarchical_index_create(system->hierarchical_index, 10000);
    bplus_index_create(system->ordered_index, 10);
    
    // Create specialized indexes
    system->bloom_filter = (bloom_filter_t*)malloc(sizeof(bloom_filter_t));
    system->inverted_index = (inverted_index_t*)malloc(sizeof(inverted_index_t));
    system->temporal_index = (temporal_index_t*)malloc(sizeof(temporal_index_t));
    
    if (!system->bloom_filter || !system->inverted_index || !system->temporal_index) {
        advanced_indexing_system_destroy(system);
        return -1;
    }
    
    bloom_filter_create(system->bloom_filter, 100000, 0.01f);
    inverted_index_create(system->inverted_index, 10000);
    temporal_index_create(system->temporal_index, 10000);
    
    system->total_indexes = 12;
    system->is_initialized = true;
    
    return 0;
}

int advanced_indexing_system_add_node(advanced_indexing_system_t* system, lattice_node_t* node) {
    if (!system || !node || !system->is_initialized) return -1;
    
    // Add to composite index
    composite_entry_t composite_entry = {
        .node_id = node->id,
        .domain_flags = (uint32_t)(1 << (node->type % 32)),
        .complexity = (uint32_t)(node->confidence * 100),
        .performance = (uint32_t)(node->confidence * 100),
        .timestamp = node->timestamp,
        .semantic_score = (float)node->confidence,
        .pattern_type = node->type,
        .evolution_generation = 0
    };
    
    composite_index_insert(system->composite_index, &composite_entry);
    
    // Add to bloom filter
    bloom_filter_add(system->bloom_filter, node->name);
    
    // Add to inverted index
    inverted_index_add_term(system->inverted_index, node->name, node->id);
    
    system->total_entries++;
    system->last_update = node->timestamp;
    
    return 0;
}

int advanced_indexing_system_search(advanced_indexing_system_t* system, const char* query,
                                  uint32_t* results, uint32_t* count) {
    if (!system || !query || !results || !count || !system->is_initialized) return -1;
    
    *count = 0;
    
    // Use inverted index for text search
    inverted_index_search_text(system->inverted_index, query, results, count);
    
    return 0;
}

void advanced_indexing_system_destroy(advanced_indexing_system_t* system) {
    if (!system) return;
    
    // Destroy multi-dimensional indexes
    if (system->complexity_btree) {
        btree_index_destroy(system->complexity_btree);
        free(system->complexity_btree);
    }
    if (system->performance_btree) {
        btree_index_destroy(system->performance_btree);
        free(system->performance_btree);
    }
    if (system->timestamp_btree) {
        btree_index_destroy(system->timestamp_btree);
        free(system->timestamp_btree);
    }
    if (system->domain_rtree) {
        rtree_index_destroy(system->domain_rtree);
        free(system->domain_rtree);
    }
    if (system->composite_index) {
        composite_index_destroy(system->composite_index);
        free(system->composite_index);
    }
    
    // Destroy semantic vector indexes
    if (system->semantic_vectors) {
        free(system->semantic_vectors);
    }
    if (system->lsh_index) {
        lsh_index_destroy(system->lsh_index);
        free(system->lsh_index);
    }
    if (system->clustering_index) {
        clustering_index_destroy(system->clustering_index);
        free(system->clustering_index);
    }
    
    // Destroy hierarchical indexes
    if (system->hierarchical_index) {
        hierarchical_index_destroy(system->hierarchical_index);
        free(system->hierarchical_index);
    }
    if (system->ordered_index) {
        bplus_index_destroy(system->ordered_index);
        free(system->ordered_index);
    }
    
    // Destroy specialized indexes
    if (system->bloom_filter) {
        bloom_filter_destroy(system->bloom_filter);
        free(system->bloom_filter);
    }
    if (system->inverted_index) {
        inverted_index_destroy(system->inverted_index);
        free(system->inverted_index);
    }
    if (system->temporal_index) {
        temporal_index_destroy(system->temporal_index);
        free(system->temporal_index);
    }
    
    memset(system, 0, sizeof(advanced_indexing_system_t));
}
