#include "simple_unified_indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Get current time in microseconds
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Create simple unified indexing system
int simple_unified_indexing_system_create(simple_unified_indexing_system_t* system, const char* lattice_path) {
    if (!system || !lattice_path) return -1;
    
    memset(system, 0, sizeof(simple_unified_indexing_system_t));
    
    // Allocate and initialize lattice
    system->lattice = (persistent_lattice_t*)malloc(sizeof(persistent_lattice_t));
    if (!system->lattice) return -1;
    
    if (lattice_init(system->lattice, lattice_path) != 0) {
        free(system->lattice);
        system->lattice = NULL;
        return -1;
    }
    
    system->is_initialized = true;
    return 0;
}

// Destroy simple unified indexing system
int simple_unified_indexing_system_destroy(simple_unified_indexing_system_t* system) {
    if (!system) return -1;
    
    if (system->lattice) {
        lattice_cleanup(system->lattice);
        free(system->lattice);
        system->lattice = NULL;
    }
    
    memset(system, 0, sizeof(simple_unified_indexing_system_t));
    return 0;
}

// Add node to simple unified indexing system
int simple_unified_indexing_system_add_node(simple_unified_indexing_system_t* system, const lattice_node_t* node) {
    if (!system || !node || !system->is_initialized) return -1;
    
    // Add node to lattice
    uint32_t node_id = lattice_add_node(system->lattice, node->type, node->name, node->data, node->parent_id);
    return (node_id > 0) ? 0 : -1;
}

// Search in simple unified indexing system
int simple_unified_indexing_system_search(simple_unified_indexing_system_t* system, const char* query_text, 
                                         simple_search_result_t* results, uint32_t* count) {
    if (!system || !query_text || !results || !count || !system->is_initialized) return -1;
    
    uint64_t start_time = get_time_us();
    *count = 0;
    
    // Search by name
    uint32_t node_ids[1000];
    uint32_t found_count = lattice_find_nodes_by_name(system->lattice, query_text, node_ids, 1000);
    
    for (uint32_t i = 0; i < found_count && i < 1000; i++) {
        lattice_node_t* node = lattice_get_node(system->lattice, node_ids[i]);
        if (node) {
            results[*count].node_id = node->id;
            results[*count].relevance_score = 1.0f; // Exact match
            strncpy(results[*count].node_name, node->name, sizeof(results[*count].node_name) - 1);
            results[*count].node_name[sizeof(results[*count].node_name) - 1] = '\0';
            strncpy(results[*count].node_data, node->data, sizeof(results[*count].node_data) - 1);
            results[*count].node_data[sizeof(results[*count].node_data) - 1] = '\0';
            (*count)++;
        }
    }
    
    // Update statistics
    system->total_queries++;
    system->total_query_time_us += get_time_us() - start_time;
    
    return 0;
}

// Get system statistics
int simple_unified_indexing_system_get_stats(simple_unified_indexing_system_t* system, uint32_t* total_queries, uint64_t* avg_query_time_us) {
    if (!system || !total_queries || !avg_query_time_us) return -1;
    
    *total_queries = system->total_queries;
    *avg_query_time_us = (system->total_queries > 0) ? (system->total_query_time_us / system->total_queries) : 0;
    
    return 0;
}
