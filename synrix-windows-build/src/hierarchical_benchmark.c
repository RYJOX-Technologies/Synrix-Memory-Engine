#define _GNU_SOURCE
#include "hierarchical_indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// Benchmark configuration
#define HIERARCHICAL_BENCHMARK_NODES 3000
#define HIERARCHICAL_BENCHMARK_QUERIES 100
#define HIERARCHICAL_BENCHMARK_ITERATIONS 5

// Performance measurement
typedef struct {
    uint64_t total_time_us;
    uint64_t min_time_us;
    uint64_t max_time_us;
    uint64_t avg_time_us;
    uint32_t operations;
    float throughput_ops_per_sec;
} hierarchical_benchmark_result_t;

// Benchmark functions
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void hierarchical_benchmark_result_init(hierarchical_benchmark_result_t* result) {
    memset(result, 0, sizeof(hierarchical_benchmark_result_t));
    result->min_time_us = UINT64_MAX;
}

static void hierarchical_benchmark_result_add_measurement(hierarchical_benchmark_result_t* result, uint64_t time_us) {
    result->total_time_us += time_us;
    result->operations++;
    
    if (time_us < result->min_time_us) {
        result->min_time_us = time_us;
    }
    if (time_us > result->max_time_us) {
        result->max_time_us = time_us;
    }
}

static void hierarchical_benchmark_result_finalize(hierarchical_benchmark_result_t* result) {
    if (result->operations > 0) {
        result->avg_time_us = result->total_time_us / result->operations;
        result->throughput_ops_per_sec = (float)result->operations / (result->total_time_us / 1000000.0f);
    }
}

// Generate test nodes
static void generate_test_nodes(lattice_node_t* nodes, uint32_t count) {
    srand(time(NULL));
    
    for (uint32_t i = 0; i < count; i++) {
        nodes[i].id = i + 1;
        nodes[i].type = (lattice_node_type_t)((rand() % 10) + 1);
        snprintf(nodes[i].name, sizeof(nodes[i].name), "hierarchical_test_node_%u", i);
        snprintf(nodes[i].data, sizeof(nodes[i].data), "hierarchical_test_data_%u_with_tree_structure", i);
        nodes[i].parent_id = (i > 0) ? (rand() % i) + 1 : 0;
        nodes[i].child_count = 0;
        nodes[i].children = NULL;
        nodes[i].confidence = (double)rand() / RAND_MAX;
        nodes[i].timestamp = get_time_us();
    }
}

// Benchmark hierarchical tree operations
static hierarchical_benchmark_result_t benchmark_hierarchical_tree(void) {
    hierarchical_benchmark_result_t result;
    hierarchical_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Hierarchical Tree...\n");
    
    hierarchical_tree_t tree;
    if (hierarchical_tree_create(&tree, HIERARCHICAL_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create hierarchical tree\n");
        return result;
    }
    
    // Generate test nodes
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(HIERARCHICAL_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        hierarchical_tree_destroy(&tree);
        return result;
    }
    generate_test_nodes(test_nodes, HIERARCHICAL_BENCHMARK_NODES);
    
    // Benchmark insertions
    printf("  📝 Testing tree insertions...\n");
    for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_NODES; i++) {
            uint32_t node_id;
            hierarchical_tree_add_node(&tree, test_nodes[i].name, test_nodes[i].data, 
                                     0, TREE_NODE_LEAF, &node_id);
        }
        
        uint64_t end_time = get_time_us();
        hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing tree searches...\n");
    tree_search_query_t query;
    memset(&query, 0, sizeof(query));
    strcpy(query.path_pattern, "hierarchical_test");
    query.max_results = 100;
    query.min_level = 0;
    query.max_level = 10;
    query.use_regex = false;
    
    tree_search_result_t search_result;
    memset(&search_result, 0, sizeof(search_result));
    
    for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_QUERIES; i++) {
            hierarchical_tree_search(&tree, &query, &search_result);
        }
        
        uint64_t end_time = get_time_us();
        hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Clean up search result
    if (search_result.node_ids) free(search_result.node_ids);
    if (search_result.paths) {
        for (uint32_t i = 0; i < search_result.count; i++) {
            free(search_result.paths[i]);
        }
        free(search_result.paths);
    }
    if (search_result.scores) free(search_result.scores);
    
    hierarchical_benchmark_result_finalize(&result);
    
    // Get tree statistics
    tree_statistics_t stats;
    hierarchical_tree_get_statistics(&tree, &stats);
    
    printf("  ✅ Hierarchical Tree Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Total Nodes: %u\n", stats.total_nodes);
    printf("     Leaf Nodes: %u\n", stats.leaf_nodes);
    printf("     Max Depth: %u\n", stats.max_depth);
    printf("     Balance Factor: %.4f\n", stats.balance_factor);
    
    free(test_nodes);
    hierarchical_tree_destroy(&tree);
    
    return result;
}

// Benchmark B+ tree operations
static hierarchical_benchmark_result_t benchmark_bplus_tree(void) {
    hierarchical_benchmark_result_t result;
    hierarchical_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking B+ Tree...\n");
    
    enhanced_bplus_tree_t bplus_tree;
    if (enhanced_bplus_tree_create(&bplus_tree, 10) != 0) {
        printf("❌ Failed to create B+ tree\n");
        return result;
    }
    
    // Benchmark insertions
    printf("  📝 Testing B+ tree insertions...\n");
    for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_NODES; i++) {
            enhanced_bplus_tree_insert(&bplus_tree, i + 1, i + 1);
        }
        
        uint64_t end_time = get_time_us();
        hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing B+ tree searches...\n");
    uint32_t* keys = (uint32_t*)malloc(1000 * sizeof(uint32_t));
    uint32_t* values = (uint32_t*)malloc(1000 * sizeof(uint32_t));
    uint32_t count = 0;
    
    for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_QUERIES; i++) {
            enhanced_bplus_tree_search_range(&bplus_tree, i, i + 100, keys, values, &count);
        }
        
        uint64_t end_time = get_time_us();
        hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    hierarchical_benchmark_result_finalize(&result);
    
    // Get B+ tree statistics
    tree_statistics_t stats;
    enhanced_bplus_tree_get_statistics(&bplus_tree, &stats);
    
    printf("  ✅ B+ Tree Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Total Nodes: %u\n", stats.total_nodes);
    printf("     Total Keys: %u\n", bplus_tree.total_keys);
    printf("     Avg Utilization: %.4f\n", stats.avg_utilization);
    
    free(keys);
    free(values);
    enhanced_bplus_tree_destroy(&bplus_tree);
    
    return result;
}

// Benchmark tree traversals
static hierarchical_benchmark_result_t benchmark_tree_traversals(void) {
    hierarchical_benchmark_result_t result;
    hierarchical_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Tree Traversals...\n");
    
    hierarchical_tree_t tree;
    if (hierarchical_tree_create(&tree, HIERARCHICAL_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create hierarchical tree\n");
        return result;
    }
    
    // Build tree structure
    for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_NODES; i++) {
        uint32_t node_id;
        char name[64];
        snprintf(name, sizeof(name), "traversal_node_%u", i);
        hierarchical_tree_add_node(&tree, name, "traversal test node", 0, TREE_NODE_LEAF, &node_id);
    }
    
    // Benchmark different traversal types
    tree_traversal_type_t traversal_types[] = {
        TRAVERSAL_PREORDER,
        TRAVERSAL_INORDER,
        TRAVERSAL_POSTORDER,
        TRAVERSAL_LEVEL_ORDER
    };
    
    const char* traversal_names[] = {
        "Pre-order",
        "In-order", 
        "Post-order",
        "Level-order"
    };
    
    for (uint32_t t = 0; t < 4; t++) {
        printf("  🔍 Testing %s traversal...\n", traversal_names[t]);
        
        for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
            uint64_t start_time = get_time_us();
            
            tree_traversal_result_t traversal_result;
            memset(&traversal_result, 0, sizeof(traversal_result));
            
            hierarchical_tree_traverse(&tree, 1, traversal_types[t], &traversal_result);
            
            uint64_t end_time = get_time_us();
            hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
            
            // Clean up
            if (traversal_result.node_ids) {
                free(traversal_result.node_ids);
            }
        }
    }
    
    hierarchical_benchmark_result_finalize(&result);
    
    printf("  ✅ Tree Traversal Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    hierarchical_tree_destroy(&tree);
    
    return result;
}

// Benchmark hierarchical indexing system
static hierarchical_benchmark_result_t benchmark_hierarchical_system(void) {
    hierarchical_benchmark_result_t result;
    hierarchical_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Hierarchical Indexing System...\n");
    
    hierarchical_indexing_system_t system;
    if (hierarchical_indexing_system_create(&system) != 0) {
        printf("❌ Failed to create hierarchical indexing system\n");
        return result;
    }
    
    // Generate test nodes
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(HIERARCHICAL_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        hierarchical_indexing_system_destroy(&system);
        return result;
    }
    generate_test_nodes(test_nodes, HIERARCHICAL_BENCHMARK_NODES);
    
    // Benchmark insertions
    printf("  📝 Testing system insertions...\n");
    for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_NODES; i++) {
            hierarchical_indexing_system_add_node(&system, &test_nodes[i]);
        }
        
        uint64_t end_time = get_time_us();
        hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing system searches...\n");
    tree_search_query_t query;
    memset(&query, 0, sizeof(query));
    strcpy(query.path_pattern, "hierarchical_test");
    query.max_results = 100;
    query.min_level = 0;
    query.max_level = 10;
    query.use_regex = false;
    
    tree_search_result_t search_result;
    memset(&search_result, 0, sizeof(search_result));
    
    for (uint32_t iter = 0; iter < HIERARCHICAL_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < HIERARCHICAL_BENCHMARK_QUERIES; i++) {
            hierarchical_indexing_system_search(&system, &query, &search_result);
        }
        
        uint64_t end_time = get_time_us();
        hierarchical_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    hierarchical_benchmark_result_finalize(&result);
    
    // Get system statistics
    tree_statistics_t stats;
    hierarchical_indexing_system_get_statistics(&system, &stats);
    
    printf("  ✅ Hierarchical System Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Total Nodes: %u\n", stats.total_nodes);
    printf("     Leaf Nodes: %u\n", stats.leaf_nodes);
    printf("     Max Depth: %u\n", stats.max_depth);
    printf("     Balance Factor: %.4f\n", stats.balance_factor);
    
    // Clean up search result
    if (search_result.node_ids) free(search_result.node_ids);
    if (search_result.paths) {
        for (uint32_t i = 0; i < search_result.count; i++) {
            free(search_result.paths[i]);
        }
        free(search_result.paths);
    }
    if (search_result.scores) free(search_result.scores);
    
    free(test_nodes);
    hierarchical_indexing_system_destroy(&system);
    
    return result;
}

// Main benchmark function
int main(void) {
    printf("🚀 HIERARCHICAL INDEXING BENCHMARK SUITE - PHASE 3\n");
    printf("==================================================\n\n");
    
    printf("Configuration:\n");
    printf("  Nodes: %d\n", HIERARCHICAL_BENCHMARK_NODES);
    printf("  Queries: %d\n", HIERARCHICAL_BENCHMARK_QUERIES);
    printf("  Iterations: %d\n", HIERARCHICAL_BENCHMARK_ITERATIONS);
    printf("\n");
    
    // Run benchmarks
    hierarchical_benchmark_result_t tree_result = benchmark_hierarchical_tree();
    printf("\n");
    
    hierarchical_benchmark_result_t bplus_result = benchmark_bplus_tree();
    printf("\n");
    
    hierarchical_benchmark_result_t traversal_result = benchmark_tree_traversals();
    printf("\n");
    
    hierarchical_benchmark_result_t system_result = benchmark_hierarchical_system();
    printf("\n");
    
    // Summary
    printf("📊 HIERARCHICAL INDEXING BENCHMARK SUMMARY\n");
    printf("===========================================\n");
    printf("Hierarchical Tree:    %.2f ops/sec\n", tree_result.throughput_ops_per_sec);
    printf("B+ Tree:              %.2f ops/sec\n", bplus_result.throughput_ops_per_sec);
    printf("Tree Traversals:      %.2f ops/sec\n", traversal_result.throughput_ops_per_sec);
    printf("Hierarchical System:  %.2f ops/sec\n", system_result.throughput_ops_per_sec);
    
    printf("\n🎯 PHASE 3 ACHIEVEMENTS:\n");
    printf("✅ Hierarchical Tree Structure\n");
    printf("✅ B+ Tree for Ordered Access\n");
    printf("✅ Multiple Tree Traversal Types\n");
    printf("✅ Tree Search and Pattern Matching\n");
    printf("✅ Tree Statistics and Metrics\n");
    printf("✅ Hierarchical Indexing System\n");
    
    printf("\n✅ Phase 3 benchmark complete!\n");
    
    return 0;
}
