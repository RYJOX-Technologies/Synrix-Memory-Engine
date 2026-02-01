#ifndef SEMANTIC_INDEX_H
#define SEMANTIC_INDEX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Semantic index for fast lookup and reasoning
typedef struct {
    uint32_t node_id;           // Lattice node ID
    uint32_t hash;              // Semantic hash
    uint32_t frequency;         // Usage frequency
    uint32_t last_accessed;     // Last access timestamp
    uint32_t related_nodes[8];  // Related node IDs
    uint8_t related_count;      // Number of related nodes
} semantic_index_entry_t;

// Hash map for semantic indexing
typedef struct {
    semantic_index_entry_t* entries;  // Hash table entries
    uint32_t capacity;                // Table capacity
    uint32_t size;                    // Current size
    uint32_t load_factor;             // Load factor (percentage)
    uint32_t collision_count;         // Collision count
    uint32_t max_probe_distance;      // Maximum probe distance
} semantic_index_t;

// Semantic query types
typedef enum {
    SEMANTIC_QUERY_EXACT = 0,         // Exact match
    SEMANTIC_QUERY_SIMILAR = 1,       // Similar patterns
    SEMANTIC_QUERY_RELATED = 2,       // Related patterns
    SEMANTIC_QUERY_EVOLUTION = 3,     // Evolution chain
    SEMANTIC_QUERY_DOMAIN = 4,        // Domain-specific
    SEMANTIC_QUERY_COMPLEXITY = 5,    // Complexity-based
    SEMANTIC_QUERY_PERFORMANCE = 6,   // Performance-based
    SEMANTIC_QUERY_FREQUENCY = 7      // Frequency-based
} semantic_query_type_t;

// Query result structure
typedef struct {
    uint32_t* node_ids;               // Result node IDs
    uint32_t count;                   // Number of results
    uint32_t capacity;                // Result capacity
    float* scores;                    // Relevance scores
    uint32_t query_time_us;           // Query time in microseconds
} semantic_query_result_t;

// Function declarations
semantic_index_t* semantic_index_create(uint32_t initial_capacity);
void semantic_index_destroy(semantic_index_t* index);

// Index management
int semantic_index_add(semantic_index_t* index, uint32_t node_id, 
                      const char* semantic_key, uint32_t hash);
int semantic_index_remove(semantic_index_t* index, uint32_t node_id);
int semantic_index_update(semantic_index_t* index, uint32_t node_id, 
                         const char* semantic_key, uint32_t hash);

// Query operations
semantic_query_result_t* semantic_index_query(semantic_index_t* index,
                                            const char* query_key,
                                            semantic_query_type_t query_type,
                                            uint32_t max_results);
void semantic_query_result_destroy(semantic_query_result_t* result);

// Semantic reasoning
int semantic_index_find_similar(semantic_index_t* index, uint32_t node_id,
                               semantic_query_result_t* result);
int semantic_index_find_related(semantic_index_t* index, uint32_t node_id,
                               semantic_query_result_t* result);
int semantic_index_find_evolution_chain(semantic_index_t* index, uint32_t node_id,
                                       semantic_query_result_t* result);

// Index statistics
void semantic_index_get_stats(semantic_index_t* index, 
                             uint32_t* size, uint32_t* capacity,
                             uint32_t* collisions, uint32_t* max_probe);
float semantic_index_get_load_factor(semantic_index_t* index);

// Hash functions
uint32_t semantic_hash(const char* key, size_t length);
uint32_t semantic_hash_combine(uint32_t hash1, uint32_t hash2);

// Performance optimization
int semantic_index_resize(semantic_index_t* index, uint32_t new_capacity);
int semantic_index_optimize(semantic_index_t* index);

// Persistence
int semantic_index_save(semantic_index_t* index, const char* filename);
int semantic_index_load(semantic_index_t* index, const char* filename);

#endif // SEMANTIC_INDEX_H
