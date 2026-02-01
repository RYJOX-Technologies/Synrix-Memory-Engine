#define _GNU_SOURCE
#include "hierarchical_indexing.h"
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <regex.h>

// Forward declarations
static uint64_t get_time_us(void);
static void traverse_preorder(hierarchical_tree_t* tree, uint32_t node_id, tree_traversal_result_t* result);
static void traverse_inorder(hierarchical_tree_t* tree, uint32_t node_id, tree_traversal_result_t* result);
static void traverse_postorder(hierarchical_tree_t* tree, uint32_t node_id, tree_traversal_result_t* result);
static void traverse_level_order(hierarchical_tree_t* tree, uint32_t start_node_id, tree_traversal_result_t* result);

// ============================================================================
// HIERARCHICAL TREE FUNCTIONS
// ============================================================================

// Create hierarchical tree
int hierarchical_tree_create(hierarchical_tree_t* tree, uint32_t initial_capacity) {
    if (!tree || initial_capacity == 0) return -1;
    
    memset(tree, 0, sizeof(hierarchical_tree_t));
    tree->node_capacity = initial_capacity;
    tree->next_node_id = 1;
    tree->max_level = 0;
    
    tree->nodes = (hierarchical_tree_node_t*)calloc(initial_capacity, sizeof(hierarchical_tree_node_t));
    if (!tree->nodes) return -1;
    
    return 0;
}

// Add node to tree
int hierarchical_tree_add_node(hierarchical_tree_t* tree, const char* name, const char* description,
                              uint32_t parent_id, tree_node_type_t type, uint32_t* node_id) {
    if (!tree || !name || !node_id || tree->node_count >= tree->node_capacity) return -1;
    
    uint32_t new_node_id = tree->next_node_id++;
    hierarchical_tree_node_t* node = &tree->nodes[new_node_id - 1];
    
    // Initialize node
    node->node_id = new_node_id;
    node->parent_id = parent_id;
    node->child_count = 0;
    node->child_capacity = 10;
    node->children = (uint32_t*)calloc(10, sizeof(uint32_t));
    if (!node->children) return -1;
    
    strncpy(node->name, name, sizeof(node->name) - 1);
    if (description) {
        strncpy(node->description, description, sizeof(node->description) - 1);
    }
    node->type = type;
    node->weight = 1.0f;
    node->last_accessed = 0;
    node->access_count = 0;
    node->is_balanced = true;
    node->balance_factor = 0.0f;
    
    // Calculate level and path
    if (parent_id == 0) {
        node->level = 0;
        strcpy(node->path, "/");
    } else {
        hierarchical_tree_node_t* parent = &tree->nodes[parent_id - 1];
        node->level = parent->level + 1;
        snprintf(node->path, sizeof(node->path), "%s%s/", parent->path, name);
        
        // Add to parent's children
        if (parent->child_count < parent->child_capacity) {
            parent->children[parent->child_count] = new_node_id;
            parent->child_count++;
        }
    }
    
    // Update tree statistics
    tree->node_count++;
    if (node->level > tree->max_level) {
        tree->max_level = node->level;
    }
    
    // Update subtree statistics
    update_subtree_statistics(tree, new_node_id);
    
    *node_id = new_node_id;
    return 0;
}

// Remove node from tree
int hierarchical_tree_remove_node(hierarchical_tree_t* tree, uint32_t node_id) {
    if (!tree || node_id == 0 || node_id > tree->node_count) return -1;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    
    // Remove from parent's children
    if (node->parent_id > 0) {
        hierarchical_tree_node_t* parent = &tree->nodes[node->parent_id - 1];
        for (uint32_t i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == node_id) {
                // Shift remaining children
                for (uint32_t j = i; j < parent->child_count - 1; j++) {
                    parent->children[j] = parent->children[j + 1];
                }
                parent->child_count--;
                break;
            }
        }
    }
    
    // Free node resources
    if (node->children) {
        free(node->children);
    }
    
    // Mark node as deleted
    memset(node, 0, sizeof(hierarchical_tree_node_t));
    
    tree->node_count--;
    return 0;
}

// Find node by path
int hierarchical_tree_find_by_path(hierarchical_tree_t* tree, const char* path, uint32_t* node_id) {
    if (!tree || !path || !node_id) return -1;
    
    for (uint32_t i = 0; i < tree->node_capacity; i++) {
        hierarchical_tree_node_t* node = &tree->nodes[i];
        if (node->node_id != 0 && strcmp(node->path, path) == 0) {
            *node_id = node->node_id;
            return 0;
        }
    }
    
    return -1; // Not found
}

// Find nodes by pattern
int hierarchical_tree_find_by_pattern(hierarchical_tree_t* tree, const char* pattern, 
                                     uint32_t* node_ids, uint32_t* count) {
    if (!tree || !pattern || !node_ids || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < tree->node_capacity; i++) {
        hierarchical_tree_node_t* node = &tree->nodes[i];
        if (node->node_id != 0) {
            if (strstr(node->name, pattern) != NULL || strstr(node->path, pattern) != NULL) {
                node_ids[*count] = node->node_id;
                (*count)++;
            }
        }
    }
    
    return 0;
}

// Get node children
int hierarchical_tree_get_children(hierarchical_tree_t* tree, uint32_t node_id, 
                                  uint32_t* children, uint32_t* count) {
    if (!tree || node_id == 0 || !children || !count) return -1;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    if (node->node_id == 0) return -1;
    
    *count = node->child_count;
    memcpy(children, node->children, node->child_count * sizeof(uint32_t));
    
    return 0;
}

// Get node path
int hierarchical_tree_get_path(hierarchical_tree_t* tree, uint32_t node_id, char* path, size_t path_size) {
    if (!tree || node_id == 0 || !path || path_size == 0) return -1;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    if (node->node_id == 0) return -1;
    
    strncpy(path, node->path, path_size - 1);
    path[path_size - 1] = '\0';
    
    return 0;
}

// Traverse tree
int hierarchical_tree_traverse(hierarchical_tree_t* tree, uint32_t start_node_id, 
                              tree_traversal_type_t traversal_type, tree_traversal_result_t* result) {
    if (!tree || !result) return -1;
    
    result->count = 0;
    result->capacity = tree->node_count;
    result->node_ids = (uint32_t*)malloc(tree->node_count * sizeof(uint32_t));
    if (!result->node_ids) return -1;
    
    uint64_t start_time = get_time_us();
    
    // Perform traversal based on type
    switch (traversal_type) {
        case TRAVERSAL_PREORDER:
            traverse_preorder(tree, start_node_id, result);
            break;
        case TRAVERSAL_INORDER:
            traverse_inorder(tree, start_node_id, result);
            break;
        case TRAVERSAL_POSTORDER:
            traverse_postorder(tree, start_node_id, result);
            break;
        case TRAVERSAL_LEVEL_ORDER:
            traverse_level_order(tree, start_node_id, result);
            break;
        default:
            free(result->node_ids);
            return -1;
    }
    
    result->traversal_time_us = get_time_us() - start_time;
    return 0;
}

// Pre-order traversal helper
static void traverse_preorder(hierarchical_tree_t* tree, uint32_t node_id, tree_traversal_result_t* result) {
    if (node_id == 0) return;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    if (node->node_id == 0) return;
    
    // Visit current node
    result->node_ids[result->count++] = node_id;
    
    // Visit children
    for (uint32_t i = 0; i < node->child_count; i++) {
        traverse_preorder(tree, node->children[i], result);
    }
}

// In-order traversal helper
static void traverse_inorder(hierarchical_tree_t* tree, uint32_t node_id, tree_traversal_result_t* result) {
    if (node_id == 0) return;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    if (node->node_id == 0) return;
    
    // Visit left children first
    for (uint32_t i = 0; i < node->child_count / 2; i++) {
        traverse_inorder(tree, node->children[i], result);
    }
    
    // Visit current node
    result->node_ids[result->count++] = node_id;
    
    // Visit right children
    for (uint32_t i = node->child_count / 2; i < node->child_count; i++) {
        traverse_inorder(tree, node->children[i], result);
    }
}

// Post-order traversal helper
static void traverse_postorder(hierarchical_tree_t* tree, uint32_t node_id, tree_traversal_result_t* result) {
    if (node_id == 0) return;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    if (node->node_id == 0) return;
    
    // Visit children first
    for (uint32_t i = 0; i < node->child_count; i++) {
        traverse_postorder(tree, node->children[i], result);
    }
    
    // Visit current node
    result->node_ids[result->count++] = node_id;
}

// Level-order traversal helper
static void traverse_level_order(hierarchical_tree_t* tree, uint32_t start_node_id, tree_traversal_result_t* result) {
    if (start_node_id == 0) return;
    
    uint32_t* queue = (uint32_t*)malloc(tree->node_count * sizeof(uint32_t));
    if (!queue) return;
    
    uint32_t front = 0, rear = 0;
    queue[rear++] = start_node_id;
    
    while (front < rear) {
        uint32_t node_id = queue[front++];
        result->node_ids[result->count++] = node_id;
        
        hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
        for (uint32_t i = 0; i < node->child_count; i++) {
            queue[rear++] = node->children[i];
        }
    }
    
    free(queue);
}

// Search tree
int hierarchical_tree_search(hierarchical_tree_t* tree, const tree_search_query_t* query, 
                            tree_search_result_t* result) {
    if (!tree || !query || !result) return -1;
    
    result->count = 0;
    result->capacity = tree->node_count;
    result->node_ids = (uint32_t*)malloc(tree->node_count * sizeof(uint32_t));
    result->paths = (char**)malloc(tree->node_count * sizeof(char*));
    result->scores = (float*)malloc(tree->node_count * sizeof(float));
    
    if (!result->node_ids || !result->paths || !result->scores) {
        free(result->node_ids);
        free(result->paths);
        free(result->scores);
        return -1;
    }
    
    uint64_t start_time = get_time_us();
    
    for (uint32_t i = 0; i < tree->node_capacity; i++) {
        hierarchical_tree_node_t* node = &tree->nodes[i];
        if (node->node_id == 0) continue;
        
        // Check level constraints
        if (node->level < query->min_level || node->level > query->max_level) continue;
        
        // Check node type
        if (query->node_type != TREE_NODE_ROOT && node->type != query->node_type) continue;
        
        // Check weight constraints
        if (node->weight < query->min_weight || node->weight > query->max_weight) continue;
        
        // Check path pattern
        bool path_match = false;
        if (query->use_regex) {
            regex_t regex;
            if (regcomp(&regex, query->path_pattern, REG_EXTENDED) == 0) {
                if (regexec(&regex, node->path, 0, NULL, 0) == 0) {
                    path_match = true;
                }
                regfree(&regex);
            }
        } else {
            path_match = (strstr(node->path, query->path_pattern) != NULL);
        }
        
        if (path_match) {
            result->node_ids[result->count] = node->node_id;
            result->paths[result->count] = (char*)malloc(256);
            strcpy(result->paths[result->count], node->path);
            result->scores[result->count] = node->weight;
            result->count++;
            
            if (result->count >= query->max_results) break;
        }
    }
    
    result->search_time_us = get_time_us() - start_time;
    return 0;
}

// Rebalance tree
int hierarchical_tree_rebalance(hierarchical_tree_t* tree) {
    if (!tree) return -1;
    
    // Simple rebalancing: rebuild tree structure
    // In a real implementation, this would use AVL or Red-Black tree algorithms
    
    tree->is_balanced = true;
    tree->last_rebalance = get_time_us();
    
    return 0;
}

// Get tree statistics
int hierarchical_tree_get_statistics(hierarchical_tree_t* tree, tree_statistics_t* stats) {
    if (!tree || !stats) return -1;
    
    memset(stats, 0, sizeof(tree_statistics_t));
    
    stats->total_nodes = tree->node_count;
    stats->max_depth = tree->max_level;
    stats->min_depth = UINT32_MAX;
    stats->total_accesses = 0;
    
    float total_depth = 0.0f;
    
    for (uint32_t i = 0; i < tree->node_capacity; i++) {
        hierarchical_tree_node_t* node = &tree->nodes[i];
        if (node->node_id == 0) continue;
        
        if (node->type == TREE_NODE_LEAF) {
            stats->leaf_nodes++;
        } else {
            stats->internal_nodes++;
        }
        
        if (node->level < stats->min_depth) {
            stats->min_depth = node->level;
        }
        
        total_depth += node->level;
        stats->total_accesses += node->access_count;
    }
    
    stats->avg_depth = tree->node_count > 0 ? total_depth / tree->node_count : 0.0f;
    stats->balance_factor = calculate_tree_balance_factor(tree);
    stats->last_rebalance = tree->last_rebalance;
    
    return 0;
}

// Destroy hierarchical tree
void hierarchical_tree_destroy(hierarchical_tree_t* tree) {
    if (!tree) return;
    
    if (tree->nodes) {
        for (uint32_t i = 0; i < tree->node_capacity; i++) {
            if (tree->nodes[i].children) {
                free(tree->nodes[i].children);
            }
        }
        free(tree->nodes);
    }
    
    memset(tree, 0, sizeof(hierarchical_tree_t));
}

// ============================================================================
// B+ TREE FUNCTIONS
// ============================================================================

// Create enhanced B+ tree
int enhanced_bplus_tree_create(enhanced_bplus_tree_t* tree, uint32_t order) {
    if (!tree || order < 2) return -1;
    
    memset(tree, 0, sizeof(enhanced_bplus_tree_t));
    tree->order = order;
    tree->node_capacity = 1000;
    tree->next_node_id = 1;
    
    tree->nodes = (enhanced_bplus_node_t*)calloc(tree->node_capacity, sizeof(enhanced_bplus_node_t));
    if (!tree->nodes) return -1;
    
    return 0;
}

// Insert key-value pair
int enhanced_bplus_tree_insert(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t value) {
    if (!tree) return -1;
    
    // TODO: Implement full B+ tree insertion
    // This is a simplified version for demonstration
    
    if (tree->node_count == 0) {
        // Create root node
        enhanced_bplus_node_t* root = &tree->nodes[0];
        root->node_id = tree->next_node_id++;
        root->is_leaf = true;
        root->key_capacity = tree->order * 2 - 1;
        root->keys = (uint32_t*)calloc(root->key_capacity, sizeof(uint32_t));
        root->values = (uint32_t*)calloc(root->key_capacity, sizeof(uint32_t));
        
        if (!root->keys || !root->values) {
            free(root->keys);
            free(root->values);
            return -1;
        }
        
        root->keys[0] = key;
        root->values[0] = value;
        root->key_count = 1;
        
        tree->root_id = root->node_id;
        tree->node_count = 1;
        tree->height = 1;
        tree->leaf_head = root->node_id;
        tree->leaf_tail = root->node_id;
    }
    
    tree->total_keys++;
    return 0;
}

// Search for key
int enhanced_bplus_tree_search(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t* value) {
    if (!tree || !value) return -1;
    
    // TODO: Implement B+ tree search
    // This is a simplified version for demonstration
    
    return -1; // Not found
}

// Search range
int enhanced_bplus_tree_search_range(enhanced_bplus_tree_t* tree, uint32_t min_key, uint32_t max_key,
                                    uint32_t* keys, uint32_t* values, uint32_t* count) {
    if (!tree || !keys || !values || !count) return -1;
    
    *count = 0;
    
    // TODO: Implement B+ tree range search
    // This is a simplified version for demonstration
    
    return 0;
}

// Delete key
int enhanced_bplus_tree_delete(enhanced_bplus_tree_t* tree, uint32_t key) {
    if (!tree) return -1;
    
    // TODO: Implement B+ tree deletion
    // This is a simplified version for demonstration
    
    return 0;
}

// Get minimum key
int enhanced_bplus_tree_get_min(enhanced_bplus_tree_t* tree, uint32_t* key, uint32_t* value) {
    if (!tree || !key || !value) return -1;
    
    // TODO: Implement B+ tree min search
    // This is a simplified version for demonstration
    
    return -1; // Not found
}

// Get maximum key
int enhanced_bplus_tree_get_max(enhanced_bplus_tree_t* tree, uint32_t* key, uint32_t* value) {
    if (!tree || !key || !value) return -1;
    
    // TODO: Implement B+ tree max search
    // This is a simplified version for demonstration
    
    return -1; // Not found
}

// Get successor
int enhanced_bplus_tree_get_successor(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t* successor_key, uint32_t* successor_value) {
    if (!tree || !successor_key || !successor_value) return -1;
    
    // TODO: Implement B+ tree successor search
    // This is a simplified version for demonstration
    
    return -1; // Not found
}

// Get predecessor
int enhanced_bplus_tree_get_predecessor(enhanced_bplus_tree_t* tree, uint32_t key, uint32_t* predecessor_key, uint32_t* predecessor_value) {
    if (!tree || !predecessor_key || !predecessor_value) return -1;
    
    // TODO: Implement B+ tree predecessor search
    // This is a simplified version for demonstration
    
    return -1; // Not found
}

// Traverse in order
int enhanced_bplus_tree_traverse_inorder(enhanced_bplus_tree_t* tree, uint32_t* keys, uint32_t* values, uint32_t* count) {
    if (!tree || !keys || !values || !count) return -1;
    
    *count = 0;
    
    // TODO: Implement B+ tree inorder traversal
    // This is a simplified version for demonstration
    
    return 0;
}

// Rebalance tree
int enhanced_bplus_tree_rebalance(enhanced_bplus_tree_t* tree) {
    if (!tree) return -1;
    
    // TODO: Implement B+ tree rebalancing
    // This is a simplified version for demonstration
    
    tree->is_balanced = true;
    tree->last_rebalance = get_time_us();
    
    return 0;
}

// Get tree statistics
int enhanced_bplus_tree_get_statistics(enhanced_bplus_tree_t* tree, tree_statistics_t* stats) {
    if (!tree || !stats) return -1;
    
    memset(stats, 0, sizeof(tree_statistics_t));
    
    stats->total_nodes = tree->node_count;
    stats->total_accesses = tree->total_keys;
    stats->avg_utilization = tree->avg_utilization;
    stats->last_rebalance = tree->last_rebalance;
    
    return 0;
}

// Destroy B+ tree
void enhanced_bplus_tree_destroy(enhanced_bplus_tree_t* tree) {
    if (!tree) return;
    
    if (tree->nodes) {
        for (uint32_t i = 0; i < tree->node_capacity; i++) {
            if (tree->nodes[i].keys) free(tree->nodes[i].keys);
            if (tree->nodes[i].values) free(tree->nodes[i].values);
            if (tree->nodes[i].children) free(tree->nodes[i].children);
        }
        free(tree->nodes);
    }
    
    memset(tree, 0, sizeof(enhanced_bplus_tree_t));
}

// ============================================================================
// HIERARCHICAL INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create hierarchical indexing system
int hierarchical_indexing_system_create(hierarchical_indexing_system_t* system) {
    if (!system) return -1;
    
    memset(system, 0, sizeof(hierarchical_indexing_system_t));
    
    system->tree = (hierarchical_tree_t*)malloc(sizeof(hierarchical_tree_t));
    system->bplus_tree = (enhanced_bplus_tree_t*)malloc(sizeof(enhanced_bplus_tree_t));
    
    if (!system->tree || !system->bplus_tree) {
        hierarchical_indexing_system_destroy(system);
        return -1;
    }
    
    if (hierarchical_tree_create(system->tree, 10000) != 0) {
        hierarchical_indexing_system_destroy(system);
        return -1;
    }
    
    if (enhanced_bplus_tree_create(system->bplus_tree, 10) != 0) {
        hierarchical_indexing_system_destroy(system);
        return -1;
    }
    
    system->is_initialized = true;
    return 0;
}

// Add lattice node to hierarchical system
int hierarchical_indexing_system_add_node(hierarchical_indexing_system_t* system, const lattice_node_t* node) {
    if (!system || !node || !system->is_initialized) return -1;
    
    // Add to hierarchical tree
    uint32_t tree_node_id;
    if (hierarchical_tree_add_node(system->tree, node->name, node->data, 0, TREE_NODE_LEAF, &tree_node_id) != 0) {
        return -1;
    }
    
    // Add to B+ tree for ordered access
    if (enhanced_bplus_tree_insert(system->bplus_tree, node->id, tree_node_id) != 0) {
        return -1;
    }
    
    system->last_update = node->timestamp;
    return 0;
}

// Search hierarchical system
int hierarchical_indexing_system_search(hierarchical_indexing_system_t* system, const tree_search_query_t* query,
                                       tree_search_result_t* result) {
    if (!system || !query || !result || !system->is_initialized) return -1;
    
    return hierarchical_tree_search(system->tree, query, result);
}

// Get system statistics
int hierarchical_indexing_system_get_statistics(hierarchical_indexing_system_t* system, tree_statistics_t* stats) {
    if (!system || !stats || !system->is_initialized) return -1;
    
    return hierarchical_tree_get_statistics(system->tree, stats);
}

// Rebalance system
int hierarchical_indexing_system_rebalance(hierarchical_indexing_system_t* system) {
    if (!system || !system->is_initialized) return -1;
    
    int tree_result = hierarchical_tree_rebalance(system->tree);
    int bplus_result = enhanced_bplus_tree_rebalance(system->bplus_tree);
    
    return (tree_result == 0 && bplus_result == 0) ? 0 : -1;
}

// Destroy hierarchical indexing system
void hierarchical_indexing_system_destroy(hierarchical_indexing_system_t* system) {
    if (!system) return;
    
    if (system->tree) {
        hierarchical_tree_destroy(system->tree);
        free(system->tree);
    }
    
    if (system->bplus_tree) {
        enhanced_bplus_tree_destroy(system->bplus_tree);
        free(system->bplus_tree);
    }
    
    memset(system, 0, sizeof(hierarchical_indexing_system_t));
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate tree balance factor
float calculate_tree_balance_factor(hierarchical_tree_t* tree) {
    if (!tree || tree->node_count == 0) return 0.0f;
    
    float total_balance = 0.0f;
    uint32_t valid_nodes = 0;
    
    for (uint32_t i = 0; i < tree->node_capacity; i++) {
        hierarchical_tree_node_t* node = &tree->nodes[i];
        if (node->node_id != 0) {
            total_balance += node->balance_factor;
            valid_nodes++;
        }
    }
    
    return valid_nodes > 0 ? total_balance / valid_nodes : 0.0f;
}

// Calculate node weight
float calculate_node_weight(hierarchical_tree_node_t* node) {
    if (!node) return 0.0f;
    
    // Simple weight calculation based on access count and subtree size
    return (float)node->access_count + (float)node->subtree_size * 0.1f;
}

// Update subtree statistics
int update_subtree_statistics(hierarchical_tree_t* tree, uint32_t node_id) {
    if (!tree || node_id == 0) return -1;
    
    hierarchical_tree_node_t* node = &tree->nodes[node_id - 1];
    if (node->node_id == 0) return -1;
    
    // Update subtree size
    node->subtree_size = 1; // Count self
    node->leaf_count = (node->type == TREE_NODE_LEAF) ? 1 : 0;
    
    for (uint32_t i = 0; i < node->child_count; i++) {
        uint32_t child_id = node->children[i];
        hierarchical_tree_node_t* child = &tree->nodes[child_id - 1];
        if (child->node_id != 0) {
            node->subtree_size += child->subtree_size;
            node->leaf_count += child->leaf_count;
        }
    }
    
    // Update weight
    node->weight = calculate_node_weight(node);
    
    // Update balance factor
    if (node->child_count <= 1) {
        node->balance_factor = 0.0f;
    } else {
        // Simple balance factor calculation
        uint32_t left_size = 0, right_size = 0;
        for (uint32_t i = 0; i < node->child_count / 2; i++) {
            left_size += tree->nodes[node->children[i] - 1].subtree_size;
        }
        for (uint32_t i = node->child_count / 2; i < node->child_count; i++) {
            right_size += tree->nodes[node->children[i] - 1].subtree_size;
        }
        
        if (left_size + right_size > 0) {
            node->balance_factor = (float)(right_size - left_size) / (left_size + right_size);
        } else {
            node->balance_factor = 0.0f;
        }
    }
    
    // Update parent if exists
    if (node->parent_id > 0) {
        update_subtree_statistics(tree, node->parent_id);
    }
    
    return 0;
}

// Get current timestamp in microseconds
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

