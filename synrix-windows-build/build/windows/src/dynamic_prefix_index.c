/*
 * Dynamic Prefix Index Implementation
 * ====================================
 * 
 * Auto-detects and indexes any prefix pattern without hardcoding.
 * Uses linear array for prefix entries (O(n) lookup, but n = prefix count, typically < 100).
 */

#include "dynamic_prefix_index.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_ENTRY_CAPACITY 16
#define INITIAL_NODE_ID_CAPACITY 64

// Extract prefix from node name (e.g., "ISA_ADD" -> "ISA_", "QDRANT_COLLECTION:test" -> "QDRANT_COLLECTION:")
// Returns prefix length, or 0 if no valid prefix found
size_t extract_prefix(const char* node_name, char* prefix_out, size_t prefix_max) {
    if (!node_name || !prefix_out || prefix_max == 0) return 0;
    
    // Find first delimiter: "_" or ":"
    const char* underscore = strchr(node_name, '_');
    const char* colon = strchr(node_name, ':');
    const char* delimiter = NULL;
    
    if (underscore && colon) {
        delimiter = (underscore < colon) ? underscore : colon;
    } else if (underscore) {
        delimiter = underscore;
    } else if (colon) {
        delimiter = colon;
    } else {
        return 0; // No delimiter found
    }
    
    // Extract prefix (everything up to and including delimiter)
    size_t prefix_len = (size_t)(delimiter - node_name + 1);
    if (prefix_len >= prefix_max) return 0;
    
    strncpy(prefix_out, node_name, prefix_len);
    prefix_out[prefix_len] = '\0';
    
    return prefix_len;
}

// Initialize dynamic prefix index
void dynamic_prefix_index_init(dynamic_prefix_index_t* index) {
    if (!index) return;
    
    memset(index, 0, sizeof(dynamic_prefix_index_t));
    index->entry_capacity = INITIAL_ENTRY_CAPACITY;
    index->entries = (dynamic_prefix_entry_t*)malloc(index->entry_capacity * sizeof(dynamic_prefix_entry_t));
    if (!index->entries) {
        index->entry_capacity = 0;
        return;
    }
    memset(index->entries, 0, index->entry_capacity * sizeof(dynamic_prefix_entry_t));
}

// Cleanup dynamic prefix index
void dynamic_prefix_index_cleanup(dynamic_prefix_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        for (uint32_t i = 0; i < index->entry_count; i++) {
            if (index->entries[i].node_ids) {
                free(index->entries[i].node_ids);
            }
        }
        free(index->entries);
    }
    
    memset(index, 0, sizeof(dynamic_prefix_index_t));
}

// Find prefix entry by prefix string (linear search - O(n) where n = prefix count)
static dynamic_prefix_entry_t* find_entry(dynamic_prefix_index_t* index, const char* prefix) {
    if (!index || !prefix || !index->entries) return NULL;
    
    for (uint32_t i = 0; i < index->entry_count; i++) {
        if (strcmp(index->entries[i].prefix, prefix) == 0) {
            return &index->entries[i];
        }
    }
    
    return NULL;
}

// Find prefix entry by prefix string (public API)
dynamic_prefix_entry_t* dynamic_prefix_index_find(dynamic_prefix_index_t* index, const char* prefix) {
    return find_entry(index, prefix);
}

// Find or create prefix entry
dynamic_prefix_entry_t* dynamic_prefix_index_get_or_create(dynamic_prefix_index_t* index, const char* prefix) {
    if (!index || !prefix) return NULL;
    
    // Try to find existing entry
    dynamic_prefix_entry_t* entry = find_entry(index, prefix);
    if (entry) return entry;
    
    // Need to create new entry - check capacity
    if (index->entry_count >= index->entry_capacity) {
        // Grow entry array
        uint32_t new_capacity = index->entry_capacity * 2;
        dynamic_prefix_entry_t* new_entries = (dynamic_prefix_entry_t*)realloc(
            index->entries, new_capacity * sizeof(dynamic_prefix_entry_t));
        if (!new_entries) return NULL;
        
        // Zero out new entries
        memset(new_entries + index->entry_capacity, 0, 
               (new_capacity - index->entry_capacity) * sizeof(dynamic_prefix_entry_t));
        
        index->entries = new_entries;
        index->entry_capacity = new_capacity;
    }
    
    // Create new entry
    entry = &index->entries[index->entry_count++];
    strncpy(entry->prefix, prefix, sizeof(entry->prefix) - 1);
    entry->prefix[sizeof(entry->prefix) - 1] = '\0';
    entry->count = 0;
    entry->capacity = INITIAL_NODE_ID_CAPACITY;
    entry->node_ids = (uint64_t*)malloc(entry->capacity * sizeof(uint64_t));
    if (!entry->node_ids) {
        // Rollback entry count
        index->entry_count--;
        return NULL;
    }
    
    return entry;
}

// Add node to prefix index
void dynamic_prefix_index_add_node(dynamic_prefix_index_t* index, uint64_t node_id, const char* node_name) {
    if (!index || !node_name) return;
    
    // Extract prefix
    char prefix[64];
    if (extract_prefix(node_name, prefix, sizeof(prefix)) == 0) {
        return; // No valid prefix
    }
    
    // Get or create entry
    dynamic_prefix_entry_t* entry = dynamic_prefix_index_get_or_create(index, prefix);
    if (!entry) return;
    
    // Check if node already in list (avoid duplicates)
    for (uint32_t i = 0; i < entry->count; i++) {
        if (entry->node_ids[i] == node_id) {
            return; // Already indexed
        }
    }
    
    // Grow node_ids array if needed
    if (entry->count >= entry->capacity) {
        uint32_t new_capacity = entry->capacity * 2;
        uint64_t* new_node_ids = (uint64_t*)realloc(entry->node_ids, new_capacity * sizeof(uint64_t));
        if (!new_node_ids) return;
        
        entry->node_ids = new_node_ids;
        entry->capacity = new_capacity;
    }
    
    // Add node ID
    entry->node_ids[entry->count++] = node_id;
}

// Build index from all nodes (O(n) scan, discovers all prefixes automatically)
void dynamic_prefix_index_build(dynamic_prefix_index_t* index, 
                                const char** node_names, 
                                uint64_t* node_ids, 
                                uint32_t node_count) {
    if (!index || !node_names || !node_ids || node_count == 0) return;
    
    // Reset index
    dynamic_prefix_index_cleanup(index);
    dynamic_prefix_index_init(index);
    if (!index->entries) return;
    
    // Scan all nodes and add to index
    for (uint32_t i = 0; i < node_count; i++) {
        if (node_names[i]) {
            dynamic_prefix_index_add_node(index, node_ids[i], node_names[i]);
        }
    }
    
    index->built = true;
}

