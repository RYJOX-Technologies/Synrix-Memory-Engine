#define _GNU_SOURCE
#include "advanced_indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// Benchmark configuration
#define BENCHMARK_NODES 10000
#define BENCHMARK_QUERIES 1000
#define BENCHMARK_ITERATIONS 10

// Performance measurement
typedef struct {
    uint64_t total_time_us;
    uint64_t min_time_us;
    uint64_t max_time_us;
    uint64_t avg_time_us;
    uint32_t operations;
    float throughput_ops_per_sec;
} benchmark_result_t;

// Benchmark functions
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void benchmark_result_init(benchmark_result_t* result) {
    memset(result, 0, sizeof(benchmark_result_t));
    result->min_time_us = UINT64_MAX;
}

static void benchmark_result_add_measurement(benchmark_result_t* result, uint64_t time_us) {
    result->total_time_us += time_us;
    result->operations++;
    
    if (time_us < result->min_time_us) {
        result->min_time_us = time_us;
    }
    if (time_us > result->max_time_us) {
        result->max_time_us = time_us;
    }
}

static void benchmark_result_finalize(benchmark_result_t* result) {
    if (result->operations > 0) {
        result->avg_time_us = result->total_time_us / result->operations;
        result->throughput_ops_per_sec = (float)result->operations / (result->total_time_us / 1000000.0f);
    }
}

// Generate test data
static void generate_test_nodes(lattice_node_t* nodes, uint32_t count) {
    srand(time(NULL));
    
    for (uint32_t i = 0; i < count; i++) {
        nodes[i].id = i + 1;
        nodes[i].type = (lattice_node_type_t)((rand() % 10) + 1);
        snprintf(nodes[i].name, sizeof(nodes[i].name), "test_node_%u", i);
        snprintf(nodes[i].data, sizeof(nodes[i].data), "test_data_%u", i);
        nodes[i].parent_id = (i > 0) ? (rand() % i) + 1 : 0;
        nodes[i].child_count = 0;
        nodes[i].children = NULL;
        nodes[i].confidence = (float)rand() / RAND_MAX;
        nodes[i].timestamp = get_time_us();
    }
}

// Benchmark composite index
static benchmark_result_t benchmark_composite_index(void) {
    benchmark_result_t result;
    benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Composite Index...\n");
    
    composite_index_t index;
    if (composite_index_create(&index, BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create composite index\n");
        return result;
    }
    
    // Generate test data
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        composite_index_destroy(&index);
        return result;
    }
    generate_test_nodes(test_nodes, BENCHMARK_NODES);
    
    // Benchmark insertions
    printf("  📝 Testing insertions...\n");
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
            composite_entry_t entry = {
                .node_id = test_nodes[i].id,
                .domain_flags = (uint32_t)(1 << (test_nodes[i].type % 32)),
                .complexity = (uint32_t)(test_nodes[i].confidence * 100),
                .performance = (uint32_t)(test_nodes[i].confidence * 100),
                .timestamp = test_nodes[i].timestamp,
                .semantic_score = (float)test_nodes[i].confidence,
                .pattern_type = test_nodes[i].type,
                .evolution_generation = 0
            };
            composite_index_insert(&index, &entry);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
        
        // Reset for next iteration
        index.count = 0;
    }
    
    // Benchmark searches
    printf("  🔍 Testing searches...\n");
    uint32_t* search_results = (uint32_t*)malloc(BENCHMARK_NODES * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_QUERIES; i++) {
            composite_index_search_multi_criteria(&index, 0, 0, 0, 0, 
                                                search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    benchmark_result_finalize(&result);
    
    printf("  ✅ Composite Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    free(search_results);
    free(test_nodes);
    composite_index_destroy(&index);
    
    return result;
}

// Benchmark bloom filter
static benchmark_result_t benchmark_bloom_filter(void) {
    benchmark_result_t result;
    benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Bloom Filter...\n");
    
    bloom_filter_t filter;
    if (bloom_filter_create(&filter, BENCHMARK_NODES, 0.01f) != 0) {
        printf("❌ Failed to create bloom filter\n");
        return result;
    }
    
    // Generate test keys
    char** test_keys = (char**)malloc(BENCHMARK_NODES * sizeof(char*));
    if (!test_keys) {
        bloom_filter_destroy(&filter);
        return result;
    }
    
    for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
        test_keys[i] = (char*)malloc(64);
        snprintf(test_keys[i], 64, "test_key_%u", i);
    }
    
    // Benchmark insertions
    printf("  📝 Testing insertions...\n");
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
            bloom_filter_add(&filter, test_keys[i]);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark lookups
    printf("  🔍 Testing lookups...\n");
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_QUERIES; i++) {
            bloom_filter_contains(&filter, test_keys[i % BENCHMARK_NODES]);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    benchmark_result_finalize(&result);
    
    printf("  ✅ Bloom Filter Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
        free(test_keys[i]);
    }
    free(test_keys);
    bloom_filter_destroy(&filter);
    
    return result;
}

// Benchmark inverted index
static benchmark_result_t benchmark_inverted_index(void) {
    benchmark_result_t result;
    benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Inverted Index...\n");
    
    inverted_index_t index;
    if (inverted_index_create(&index, BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create inverted index\n");
        return result;
    }
    
    // Generate test terms
    char** test_terms = (char**)malloc(BENCHMARK_NODES * sizeof(char*));
    if (!test_terms) {
        inverted_index_destroy(&index);
        return result;
    }
    
    for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
        test_terms[i] = (char*)malloc(32);
        snprintf(test_terms[i], 32, "term_%u", i);
    }
    
    // Benchmark insertions
    printf("  📝 Testing insertions...\n");
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
            inverted_index_add_term(&index, test_terms[i], i + 1);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing searches...\n");
    uint32_t* search_results = (uint32_t*)malloc(BENCHMARK_NODES * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_QUERIES; i++) {
            inverted_index_search_text(&index, "term", search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    benchmark_result_finalize(&result);
    
    printf("  ✅ Inverted Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
        free(test_terms[i]);
    }
    free(test_terms);
    free(search_results);
    inverted_index_destroy(&index);
    
    return result;
}

// Benchmark advanced indexing system
static benchmark_result_t benchmark_advanced_system(void) {
    benchmark_result_t result;
    benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Advanced Indexing System...\n");
    
    advanced_indexing_system_t system;
    if (advanced_indexing_system_create(&system) != 0) {
        printf("❌ Failed to create advanced indexing system\n");
        return result;
    }
    
    // Generate test data
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        advanced_indexing_system_destroy(&system);
        return result;
    }
    generate_test_nodes(test_nodes, BENCHMARK_NODES);
    
    // Benchmark insertions
    printf("  📝 Testing insertions...\n");
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_NODES; i++) {
            advanced_indexing_system_add_node(&system, &test_nodes[i]);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing searches...\n");
    uint32_t* search_results = (uint32_t*)malloc(BENCHMARK_NODES * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < BENCHMARK_QUERIES; i++) {
            advanced_indexing_system_search(&system, "test", search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    benchmark_result_finalize(&result);
    
    printf("  ✅ Advanced System Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    free(search_results);
    free(test_nodes);
    advanced_indexing_system_destroy(&system);
    
    return result;
}

// Main benchmark function
int main(void) {
    printf("🚀 ADVANCED INDEXING BENCHMARK SUITE\n");
    printf("=====================================\n\n");
    
    printf("Configuration:\n");
    printf("  Nodes: %d\n", BENCHMARK_NODES);
    printf("  Queries: %d\n", BENCHMARK_QUERIES);
    printf("  Iterations: %d\n\n", BENCHMARK_ITERATIONS);
    
    // Run benchmarks
    benchmark_result_t composite_result = benchmark_composite_index();
    printf("\n");
    
    benchmark_result_t bloom_result = benchmark_bloom_filter();
    printf("\n");
    
    benchmark_result_t inverted_result = benchmark_inverted_index();
    printf("\n");
    
    benchmark_result_t advanced_result = benchmark_advanced_system();
    printf("\n");
    
    // Summary
    printf("📊 BENCHMARK SUMMARY\n");
    printf("====================\n");
    printf("Composite Index:     %.2f ops/sec\n", composite_result.throughput_ops_per_sec);
    printf("Bloom Filter:        %.2f ops/sec\n", bloom_result.throughput_ops_per_sec);
    printf("Inverted Index:      %.2f ops/sec\n", inverted_result.throughput_ops_per_sec);
    printf("Advanced System:     %.2f ops/sec\n", advanced_result.throughput_ops_per_sec);
    
    printf("\n✅ Benchmark complete!\n");
    
    return 0;
}
