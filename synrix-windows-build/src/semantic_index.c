#define _GNU_SOURCE
#include "semantic_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// Hash table implementation with open addressing and linear probing
// Optimized for ARM64 and Jetson Orin Nano performance

// Constants
#define DEFAULT_CAPACITY 1024
#define MAX_LOAD_FACTOR 75  // 75% load factor
#define MAX_PROBE_DISTANCE 16
#define GROWTH_FACTOR 2

// Hash function (FNV-1a variant optimized for ARM64)
uint32_t semantic_hash(const char* key, size_t length) {
    if (!key || length == 0) return 0;
    
    uint32_t hash = 0x811c9dc5; // FNV offset basis
    const uint8_t* data = (const uint8_t*)key;
    
    // Unrolled loop for better performance
    while (length >= 4) {
        hash ^= data[0];
        hash *= 0x01000193; // FNV prime
        hash ^= data[1];
        hash *= 0x01000193;
        hash ^= data[2];
        hash *= 0x01000193;
        hash ^= data[3];
        hash *= 0x01000193;
        data += 4;
        length -= 4;
    }
    
    // Handle remaining bytes
    while (length > 0) {
        hash ^= *data++;
        hash *= 0x01000193;
        length--;
    }
    
    return hash;
}

uint32_t semantic_hash_combine(uint32_t hash1, uint32_t hash2) {
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

// Create semantic index
semantic_index_t* semantic_index_create(uint32_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = DEFAULT_CAPACITY;
    }
    
    // Round up to power of 2 for better performance
    uint32_t capacity = 1;
    while (capacity < initial_capacity) {
        capacity <<= 1;
    }
    
    semantic_index_t* index = (semantic_index_t*)malloc(sizeof(semantic_index_t));
    if (!index) return NULL;
    
    index->entries = (semantic_index_entry_t*)calloc(capacity, sizeof(semantic_index_entry_t));
    if (!index->entries) {
        free(index);
        return NULL;
    }
    
    index->capacity = capacity;
    index->size = 0;
    index->load_factor = 0;
    index->collision_count = 0;
    index->max_probe_distance = 0;
    
    return index;
}

void semantic_index_destroy(semantic_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        free(index->entries);
    }
    free(index);
}

// Get current timestamp in microseconds
static uint32_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000000 + tv.tv_usec);
}

// Find entry in hash table
static semantic_index_entry_t* find_entry(semantic_index_t* index, uint32_t node_id) {
    if (!index || !index->entries) return NULL;
    
    uint32_t hash = node_id;
    uint32_t capacity = index->capacity;
    uint32_t probe_distance = 0;
    
    // Linear probing
    uint32_t pos = hash & (capacity - 1);
    while (probe_distance < MAX_PROBE_DISTANCE) {
        semantic_index_entry_t* entry = &index->entries[pos];
        
        if (entry->node_id == 0) {
            // Empty slot found
            return NULL;
        }
        
        if (entry->node_id == node_id) {
            // Found the entry
            return entry;
        }
        
        pos = (pos + 1) & (capacity - 1);
        probe_distance++;
    }
    
    return NULL; // Not found within probe distance
}

// Add entry to hash table
static int add_entry(semantic_index_t* index, uint32_t node_id, uint32_t hash) {
    if (!index || !index->entries) return -1;
    
    uint32_t capacity = index->capacity;
    uint32_t probe_distance = 0;
    
    // Linear probing
    uint32_t pos = hash & (capacity - 1);
    while (probe_distance < MAX_PROBE_DISTANCE) {
        semantic_index_entry_t* entry = &index->entries[pos];
        
        if (entry->node_id == 0) {
            // Empty slot found
            entry->node_id = node_id;
            entry->hash = hash;
            entry->frequency = 1;
            entry->last_accessed = get_timestamp_us();
            entry->related_count = 0;
            
            index->size++;
            index->load_factor = (index->size * 100) / capacity;
            
            if (probe_distance > index->max_probe_distance) {
                index->max_probe_distance = probe_distance;
            }
            
            return 0;
        }
        
        if (entry->node_id == node_id) {
            // Entry already exists, update frequency
            entry->frequency++;
            entry->last_accessed = get_timestamp_us();
            return 0;
        }
        
        pos = (pos + 1) & (capacity - 1);
        probe_distance++;
    }
    
    // Collision occurred
    index->collision_count++;
    return -1; // Table full or too many collisions
}

// Resize hash table
int semantic_index_resize(semantic_index_t* index, uint32_t new_capacity) {
    if (!index || new_capacity < index->size) return -1;
    
    // Round up to power of 2
    uint32_t capacity = 1;
    while (capacity < new_capacity) {
        capacity <<= 1;
    }
    
    semantic_index_entry_t* old_entries = index->entries;
    uint32_t old_capacity = index->capacity;
    
    // Allocate new table
    index->entries = (semantic_index_entry_t*)calloc(capacity, sizeof(semantic_index_entry_t));
    if (!index->entries) {
        index->entries = old_entries;
        return -1;
    }
    
    index->capacity = capacity;
    uint32_t old_size = index->size;
    index->size = 0;
    index->collision_count = 0;
    index->max_probe_distance = 0;
    
    // Rehash all entries
    for (uint32_t i = 0; i < old_capacity; i++) {
        semantic_index_entry_t* old_entry = &old_entries[i];
        if (old_entry->node_id != 0) {
            add_entry(index, old_entry->node_id, old_entry->hash);
        }
    }
    
    free(old_entries);
    return 0;
}

// Add entry to index
int semantic_index_add(semantic_index_t* index, uint32_t node_id, 
                      const char* semantic_key, uint32_t hash) {
    if (!index || node_id == 0) return -1;
    
    // Check if we need to resize
    if (index->load_factor > MAX_LOAD_FACTOR) {
        if (semantic_index_resize(index, index->capacity * GROWTH_FACTOR) != 0) {
            return -1;
        }
    }
    
    // Calculate hash if not provided
    if (hash == 0 && semantic_key) {
        hash = semantic_hash(semantic_key, strlen(semantic_key));
    }
    
    return add_entry(index, node_id, hash);
}

// Remove entry from index
int semantic_index_remove(semantic_index_t* index, uint32_t node_id) {
    if (!index || !index->entries) return -1;
    
    semantic_index_entry_t* entry = find_entry(index, node_id);
    if (!entry) return -1;
    
    // Clear the entry
    memset(entry, 0, sizeof(semantic_index_entry_t));
    index->size--;
    index->load_factor = (index->size * 100) / index->capacity;
    
    return 0;
}

// Update entry in index
int semantic_index_update(semantic_index_t* index, uint32_t node_id, 
                         const char* semantic_key, uint32_t hash) {
    if (!index || node_id == 0) return -1;
    
    semantic_index_entry_t* entry = find_entry(index, node_id);
    if (!entry) return -1;
    
    // Update hash if provided
    if (hash != 0) {
        entry->hash = hash;
    } else if (semantic_key) {
        entry->hash = semantic_hash(semantic_key, strlen(semantic_key));
    }
    
    entry->last_accessed = get_timestamp_us();
    return 0;
}

// Query index
semantic_query_result_t* semantic_index_query(semantic_index_t* index,
                                            const char* query_key,
                                            semantic_query_type_t query_type,
                                            uint32_t max_results) {
    if (!index || !query_key || max_results == 0) return NULL;
    
    uint32_t start_time = get_timestamp_us();
    uint32_t query_hash = semantic_hash(query_key, strlen(query_key));
    
    semantic_query_result_t* result = (semantic_query_result_t*)malloc(sizeof(semantic_query_result_t));
    if (!result) return NULL;
    
    result->node_ids = (uint32_t*)malloc(max_results * sizeof(uint32_t));
    result->scores = (float*)malloc(max_results * sizeof(float));
    if (!result->node_ids || !result->scores) {
        free(result->node_ids);
        free(result->scores);
        free(result);
        return NULL;
    }
    
    result->count = 0;
    result->capacity = max_results;
    
    // Search for matching entries
    for (uint32_t i = 0; i < index->capacity && result->count < max_results; i++) {
        semantic_index_entry_t* entry = &index->entries[i];
        if (entry->node_id == 0) continue;
        
        float score = 0.0f;
        bool match = false;
        
        switch (query_type) {
            case SEMANTIC_QUERY_EXACT:
                match = (entry->hash == query_hash);
                score = match ? 1.0f : 0.0f;
                break;
                
            case SEMANTIC_QUERY_SIMILAR:
                // Calculate similarity based on hash distance
                {
                    uint32_t hash_diff = entry->hash ^ query_hash;
                    uint32_t bit_count = __builtin_popcount(hash_diff);
                    score = 1.0f - (float)bit_count / 32.0f;
                    match = (score > 0.5f);
                }
                break;
                
            case SEMANTIC_QUERY_FREQUENCY:
                match = true;
                score = (float)entry->frequency / 100.0f;
                break;
                
            default:
                match = true;
                score = 0.5f;
                break;
        }
        
        if (match && score > 0.0f) {
            result->node_ids[result->count] = entry->node_id;
            result->scores[result->count] = score;
            result->count++;
        }
    }
    
    result->query_time_us = get_timestamp_us() - start_time;
    return result;
}

void semantic_query_result_destroy(semantic_query_result_t* result) {
    if (!result) return;
    
    free(result->node_ids);
    free(result->scores);
    free(result);
}

// Find similar entries
int semantic_index_find_similar(semantic_index_t* index, uint32_t node_id,
                               semantic_query_result_t* result) {
    if (!index || node_id == 0 || !result) return -1;
    
    semantic_index_entry_t* entry = find_entry(index, node_id);
    if (!entry) return -1;
    
    // Find entries with similar hash
    uint32_t count = 0;
    for (uint32_t i = 0; i < index->capacity && count < result->capacity; i++) {
        semantic_index_entry_t* other = &index->entries[i];
        if (other->node_id == 0 || other->node_id == node_id) continue;
        
        uint32_t hash_diff = entry->hash ^ other->hash;
        uint32_t bit_count = __builtin_popcount(hash_diff);
        float similarity = 1.0f - (float)bit_count / 32.0f;
        
        if (similarity > 0.3f) {
            result->node_ids[count] = other->node_id;
            result->scores[count] = similarity;
            count++;
        }
    }
    
    result->count = count;
    return 0;
}

// Find related entries
int semantic_index_find_related(semantic_index_t* index, uint32_t node_id,
                               semantic_query_result_t* result) {
    if (!index || node_id == 0 || !result) return -1;
    
    semantic_index_entry_t* entry = find_entry(index, node_id);
    if (!entry) return -1;
    
    // Return related nodes
    uint32_t count = 0;
    for (uint32_t i = 0; i < entry->related_count && count < result->capacity; i++) {
        result->node_ids[count] = entry->related_nodes[i];
        result->scores[count] = 1.0f;
        count++;
    }
    
    result->count = count;
    return 0;
}

// Find evolution chain
int semantic_index_find_evolution_chain(semantic_index_t* index, uint32_t node_id,
                                       semantic_query_result_t* result) {
    if (!index || node_id == 0 || !result) return -1;
    
    // This would require additional data structures to track evolution
    // For now, return empty result
    result->count = 0;
    return 0;
}

// Get index statistics
void semantic_index_get_stats(semantic_index_t* index, 
                             uint32_t* size, uint32_t* capacity,
                             uint32_t* collisions, uint32_t* max_probe) {
    if (!index) return;
    
    if (size) *size = index->size;
    if (capacity) *capacity = index->capacity;
    if (collisions) *collisions = index->collision_count;
    if (max_probe) *max_probe = index->max_probe_distance;
}

float semantic_index_get_load_factor(semantic_index_t* index) {
    if (!index) return 0.0f;
    return (float)index->load_factor / 100.0f;
}

// Optimize index
int semantic_index_optimize(semantic_index_t* index) {
    if (!index) return -1;
    
    // Resize if load factor is too low
    if (index->load_factor < 25 && index->capacity > DEFAULT_CAPACITY) {
        return semantic_index_resize(index, index->capacity / 2);
    }
    
    return 0;
}

// Save index to file
int semantic_index_save(semantic_index_t* index, const char* filename) {
    if (!index || !filename) return -1;
    
    FILE* file = fopen(filename, "wb");
    if (!file) return -1;
    
    // Write header
    fwrite(&index->capacity, sizeof(uint32_t), 1, file);
    fwrite(&index->size, sizeof(uint32_t), 1, file);
    
    // Write entries
    fwrite(index->entries, sizeof(semantic_index_entry_t), index->capacity, file);
    
    fclose(file);
    return 0;
}

// Load index from file
int semantic_index_load(semantic_index_t* index, const char* filename) {
    if (!index || !filename) return -1;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return -1;
    
    // Read header
    uint32_t capacity, size;
    if (fread(&capacity, sizeof(uint32_t), 1, file) != 1 ||
        fread(&size, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Resize if needed
    if (capacity != index->capacity) {
        if (semantic_index_resize(index, capacity) != 0) {
            fclose(file);
            return -1;
        }
    }
    
    // Read entries
    if (fread(index->entries, sizeof(semantic_index_entry_t), capacity, file) != capacity) {
        fclose(file);
        return -1;
    }
    
    index->size = size;
    index->load_factor = (size * 100) / capacity;
    
    fclose(file);
    return 0;
}
