#ifndef HIERARCHICAL_INDEXING_H
#define HIERARCHICAL_INDEXING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "advanced_indexing.h"

// ============================================================================
// HIERARCHICAL INDEXING - PHASE 3
// ============================================================================

// Tree node types
typedef enum {
    TREE_NODE_ROOT = 0,           // Root node
    TREE_NODE_INTERNAL = 1,       // Internal node
    TREE_NODE_LEAF = 2,           // Leaf node
    TREE_NODE_BRANCH = 3,         // Branch node
    TREE_NODE_TERMINAL = 4        // Terminal node
} tree_node_type_t;

// Hierarchical tree node with enhanced metadata
typedef struct {
    uint32_t node_id;             // Unique node identifier
    uint32_t parent_id;           // Parent node ID (0 for root)
    uint32_t* children;           // Array of child node IDs
    uint32_t child_count;         // Number of children
    uint32_t child_capacity;      // Capacity for children
    uint32_t level;               // Depth level (0 for root)
    char path[256];               // Hierarchical path (e.g., "/root/branch/leaf")
    char name[64];                // Node name
    char description[128];        // Node description
    tree_node_type_t type;        // Node type
    uint32_t subtree_size;        // Total size of subtree
    uint32_t leaf_count;          // Number of leaf nodes in subtree
    float weight;                 // Node weight for balancing
    uint64_t last_accessed;       // Last access timestamp
    uint32_t access_count;        // Access frequency
    bool is_balanced;             // Whether subtree is balanced
    float balance_factor;         // Balance factor (-1 to 1)
} hierarchical_tree_node_t;

// Hierarchical tree structure
typedef struct {
    hierarchical_tree_node_t* nodes;  // Node array
    uint32_t node_count;              // Number of nodes
    uint32_t node_capacity;           // Node capacity
    uint32_t root_id;                 // Root node ID
    uint32_t max_level;               // Maximum depth level
    uint32_t next_node_id;            // Next available node ID
    bool is_balanced;                 // Whether entire tree is balanced
    float global_balance_factor;      // Global balance factor
    uint64_t last_rebalance;          // Last rebalance timestamp
} hierarchical_tree_t;

// B+ tree node with enhanced features
typedef struct {
    uint32_t node_id;             // Node ID
    uint32_t* keys;               // Array of keys
    uint32_t* values;             // Array of values (for leaf nodes)
    uint32_t* children;           // Array of child node IDs (for internal nodes)
    uint32_t key_count;           // Number of keys
    uint32_t key_capacity;        // Key capacity
    uint32_t parent_id;           // Parent node ID
    uint32_t next_leaf;           // Next leaf node (for sequential access)
    uint32_t prev_leaf;           // Previous leaf node
    bool is_leaf;                 // Whether this is a leaf node
    uint32_t level;               // Node level
    uint64_t last_updated;        // Last update timestamp
    uint32_t access_count;        // Access frequency
    float utilization;            // Key utilization ratio
} enhanced_bplus_node_t;

// Enhanced B+ tree index
typedef struct {
    enhanced_bplus_node_t* nodes; // Node array
    uint32_t node_count;          // Number of nodes
    uint32_t node_capacity;       // Node capacity
    uint32_t root_id;             // Root node ID
    uint32_t leaf_head;           // First leaf node
    uint32_t leaf_tail;           // Last leaf node
    uint32_t order;               // B+ tree order (minimum degree)
    uint32_t height;              // Tree height
    uint32_t next_node_id;        // Next available node ID
    bool is_balanced;             // Whether tree is balanced
    uint64_t last_rebalance;      // Last rebalance timestamp
    uint32_t total_keys;          // Total number of keys
    float avg_utilization;        // Average node utilization
} enhanced_bplus_tree_t;

// Tree traversal types
typedef enum {
    TRAVERSAL_PREORDER = 0,       // Pre-order traversal
    TRAVERSAL_INORDER = 1,        // In-order traversal
    TRAVERSAL_POSTORDER = 2,      // Post-order traversal
    TRAVERSAL_LEVEL_ORDER = 3,    // Level-order (breadth-first) traversal
    TRAVERSAL_DEPTH_FIRST = 4,    // Depth-first traversal
    TRAVERSAL_BREADTH_FIRST = 5   // Breadth-first traversal
} tree_traversal_type_t;

// Tree traversal result
typedef struct {
    uint32_t* node_ids;           // Array of node IDs in traversal order
    uint32_t count;               // Number of nodes
    uint32_t capacity;            // Result capacity
    uint32_t traversal_time_us;   // Traversal time in microseconds
} tree_traversal_result_t;

// Tree search query
typedef struct {
    char path_pattern[256];       // Path pattern to match
    uint32_t min_level;           // Minimum level
    uint32_t max_level;           // Maximum level
    tree_node_type_t node_type;   // Node type filter
    float min_weight;             // Minimum weight
    float max_weight;             // Maximum weight
    uint32_t max_results;         // Maximum results to return
    bool use_regex;               // Use regex matching for paths
    bool include_subtrees;        // Include entire subtrees
} tree_search_query_t;

// Tree search result
typedef struct {
    uint32_t* node_ids;           // Matching node IDs
    uint32_t count;               // Number of matches
    uint32_t capacity;            // Result capacity
    char** paths;                 // Matching paths
    float* scores;                // Match scores
    uint32_t search_time_us;      // Search time in microseconds
} tree_search_result_t;

// Tree statistics
typedef struct {
    uint32_t total_nodes;         // Total number of nodes
    uint32_t leaf_nodes;          // Number of leaf nodes
    uint32_t internal_nodes;      // Number of internal nodes
    uint32_t max_depth;           // Maximum depth
    uint32_t min_depth;           // Minimum depth
    float avg_depth;              // Average depth
    float balance_factor;         // Overall balance factor
    uint32_t total_accesses;      // Total access count
    float avg_utilization;        // Average node utilization
    uint64_t last_rebalance;      // Last rebalance timestamp
} tree_statistics_t;

// Hierarchical indexing system
typedef struct {
    hierarchical_tree_t* tree;    // Hierarchical tree
    enhanced_bplus_tree_t* bplus_tree; // B+ tree for ordered access
    tree_statistics_t stats;      // Tree statistics
    bool is_initialized;          // Initialization status
    uint64_t last_update;         // Last update timestamp
} hierarchical_indexing_system_t;

// ============================================================================
// HIERARCHICAL TREE FUNCTIONS
// ============================================================================

// Create hierarchical tree
int hierarchical_tree_create(hierarchical_tree_t* tree, uint32_t initial_capacity);

// Add node to tree
int hierarchical_tree_add_node(hierarchical_tree_t* tree, const char* name, const char* description,
                              uint32_t parent_id, tree_node_type_t type, uint32_t* node_id);

// Remove node from tree
int hierarchical_tree_remove_node(hierarchical_tree_t* tree, uint32_t node_id);

// Find node by path
int hierarchical_tree_find_by_path(hierarchical_tree_t* tree, const char* path, uint32_t* node_id);

// Find nodes by pattern
int hierarchical_tree_find_by_pattern(hierarchical_tree_t* tree, const char* pattern, 
                                     uint32_t* node_ids, uint32_t* count);

// Get node children
int hierarchical_tree_get_children(hierarchical_tree_t* tree, uint32_t node_id, 
                                  uint32_t* children, uint32_t* count);

// Get node path
int hierarchical_tree_get_path(hierarchical_tree_t* tree, uint32_t node_id, char* path, size_t path_size);

// Traverse tree
int hierarchical_tree_traverse(hierarchical_tree_t* tree, uint32_t start_node_id, 
                              tree_traversal_type_t traversal_type, tree_traversal_result_t* result);

// Search tree
int hierarchical_tree_search(hierarchical_tree_t* tree, const tree_search_query_t* query, 
                            tree_search_result_t* result);

// Rebalance tree
int hierarchical_tree_rebalance(hierarchical_tree_t* tree);

// Get tree statistics
int hierarchical_tree_get_statistics(hierarchical_tree_t* tree, tree_statistics_t* stats);

// Destroy hierarchical tree
void hierarchical_tree_destroy(hierarchical_tree_t* tree);

// ============================================================================
// B+ TREE FUNCTIONS
// ============================================================================

// Create enhanced B+ tree
int enhanced_bplus_tree_create(enhanced_bplus_tree_t* tree, uint32_t order);

// Insert key-value pair
int enhanced_bplus_tree_insert(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t value);

// Search for key
int enhanced_bplus_tree_search(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t* value);

// Search range
int enhanced_bplus_tree_search_range(enhanced_bplus_tree_t* tree, uint32_t min_key, uint32_t max_key,
                                    uint32_t* keys, uint32_t* values, uint32_t* count);

// Delete key
int enhanced_bplus_tree_delete(enhanced_bplus_tree_t* tree, uint32_t key);

// Get minimum key
int enhanced_bplus_tree_get_min(enhanced_bplus_tree_t* tree, uint32_t* key, uint32_t* value);

// Get maximum key
int enhanced_bplus_tree_get_max(enhanced_bplus_tree_t* tree, uint32_t* key, uint32_t* value);

// Get successor
int enhanced_bplus_tree_get_successor(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t* successor_key, uint32_t* successor_value);

// Get predecessor
int enhanced_bplus_tree_get_predecessor(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t* predecessor_key, uint32_t* predecessor_value);

// Traverse in order
int enhanced_bplus_tree_traverse_inorder(enhanced_bplus_tree_t* tree, uint32_t* keys, uint32_t* values, uint32_t* count);

// Rebalance tree
int enhanced_bplus_tree_rebalance(enhanced_bplus_tree_t* tree);

// Get tree statistics
int enhanced_bplus_tree_get_statistics(enhanced_bplus_tree_t* tree, tree_statistics_t* stats);

// Destroy B+ tree
void enhanced_bplus_tree_destroy(enhanced_bplus_tree_t* tree);

// ============================================================================
// HIERARCHICAL INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create hierarchical indexing system
int hierarchical_indexing_system_create(hierarchical_indexing_system_t* system);

// Add lattice node to hierarchical system
int hierarchical_indexing_system_add_node(hierarchical_indexing_system_t* system, const lattice_node_t* node);

// Search hierarchical system
int hierarchical_indexing_system_search(hierarchical_indexing_system_t* system, const tree_search_query_t* query,
                                       tree_search_result_t* result);

// Get system statistics
int hierarchical_indexing_system_get_statistics(hierarchical_indexing_system_t* system, tree_statistics_t* stats);

// Rebalance system
int hierarchical_indexing_system_rebalance(hierarchical_indexing_system_t* system);

// Destroy hierarchical indexing system
void hierarchical_indexing_system_destroy(hierarchical_indexing_system_t* system);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate tree balance factor
float calculate_tree_balance_factor(hierarchical_tree_t* tree);

// Calculate node weight
float calculate_node_weight(hierarchical_tree_node_t* node);

// Update subtree statistics
int update_subtree_statistics(hierarchical_tree_t* tree, uint32_t node_id);

// Find common ancestor
int find_common_ancestor(hierarchical_tree_t* tree, uint32_t node1_id, uint32_t node2_id, uint32_t* ancestor_id);

// Calculate tree height
uint32_t calculate_tree_height(hierarchical_tree_t* tree);

// Validate tree structure
bool validate_tree_structure(hierarchical_tree_t* tree);

// Generate tree visualization
int generate_tree_visualization(hierarchical_tree_t* tree, char* visualization, size_t size);

// Calculate path similarity
float calculate_path_similarity(const char* path1, const char* path2);

// Normalize path
int normalize_path(const char* input_path, char* output_path, size_t size);

#endif // HIERARCHICAL_INDEXING_H
