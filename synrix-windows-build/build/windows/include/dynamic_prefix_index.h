/*
 * Dynamic Prefix Index - Auto-detects and indexes any prefix pattern
 * ===================================================================
 * 
 * This system automatically discovers prefixes (e.g., "ISA_", "QDRANT_COLLECTION:")
 * and builds O(k) indexes for them, making the system truly plug-and-play.
 * No hardcoding required - any new prefix is automatically indexed.
 */

#ifndef DYNAMIC_PREFIX_INDEX_H
#define DYNAMIC_PREFIX_INDEX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Dynamic prefix entry - stores IDs for a specific prefix
typedef struct {
    char prefix[64];              // Prefix string (e.g., "ISA_", "QDRANT_COLLECTION:")
    uint32_t count;              // Number of nodes with this prefix
    uint32_t capacity;            // Allocated capacity
    uint64_t* node_ids;           // Array of node IDs with this prefix
} dynamic_prefix_entry_t;

// Dynamic prefix index - automatically discovers and indexes all prefixes
typedef struct {
    uint32_t entry_count;         // Number of discovered prefixes
    uint32_t entry_capacity;      // Allocated capacity for entries
    dynamic_prefix_entry_t* entries; // Array of prefix entries
    bool built;                   // Whether index has been built
} dynamic_prefix_index_t;

// Initialize dynamic prefix index
void dynamic_prefix_index_init(dynamic_prefix_index_t* index);

// Cleanup dynamic prefix index
void dynamic_prefix_index_cleanup(dynamic_prefix_index_t* index);

// Extract prefix from node name (e.g., "ISA_ADD" -> "ISA_", "QDRANT_COLLECTION:test" -> "QDRANT_COLLECTION:")
// Returns prefix length, or 0 if no valid prefix found
size_t extract_prefix(const char* node_name, char* prefix_out, size_t prefix_max);

// Find or create prefix entry
dynamic_prefix_entry_t* dynamic_prefix_index_get_or_create(dynamic_prefix_index_t* index, const char* prefix);

// Add node to prefix index
void dynamic_prefix_index_add_node(dynamic_prefix_index_t* index, uint64_t node_id, const char* node_name);

// Find prefix entry by prefix string
dynamic_prefix_entry_t* dynamic_prefix_index_find(dynamic_prefix_index_t* index, const char* prefix);

// Build index from all nodes (O(n) scan, discovers all prefixes automatically)
void dynamic_prefix_index_build(dynamic_prefix_index_t* index, 
                                const char** node_names, 
                                uint64_t* node_ids, 
                                uint32_t node_count);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_PREFIX_INDEX_H

