#ifndef SIMPLE_UNIFIED_INDEXING_H
#define SIMPLE_UNIFIED_INDEXING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "persistent_lattice.h"

#ifdef __cplusplus
extern "C" {
#endif

// Simple unified indexing system - just wraps the basic lattice
typedef struct {
    persistent_lattice_t* lattice;
    bool is_initialized;
    uint32_t total_queries;
    uint64_t total_query_time_us;
} simple_unified_indexing_system_t;

// Simple search result
typedef struct {
    uint32_t node_id;
    float relevance_score;
    char node_name[64];
    char node_data[128];
} simple_search_result_t;

// Function declarations
int simple_unified_indexing_system_create(simple_unified_indexing_system_t* system, const char* lattice_path);
int simple_unified_indexing_system_destroy(simple_unified_indexing_system_t* system);
int simple_unified_indexing_system_add_node(simple_unified_indexing_system_t* system, const lattice_node_t* node);
int simple_unified_indexing_system_search(simple_unified_indexing_system_t* system, const char* query_text, 
                                         simple_search_result_t* results, uint32_t* count);
int simple_unified_indexing_system_get_stats(simple_unified_indexing_system_t* system, uint32_t* total_queries, uint64_t* avg_query_time_us);

#ifdef __cplusplus
}
#endif

#endif // SIMPLE_UNIFIED_INDEXING_H
