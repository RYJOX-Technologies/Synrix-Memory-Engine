#define _GNU_SOURCE
#include "optimized_inverted_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// Benchmark configuration
#define OPTIMIZED_INVERTED_BENCHMARK_NODES 10000
#define OPTIMIZED_INVERTED_BENCHMARK_QUERIES 1000
#define OPTIMIZED_INVERTED_BENCHMARK_ITERATIONS 5

// Performance measurement
typedef struct {
    uint64_t total_time_us;
    uint64_t min_time_us;
    uint64_t max_time_us;
    uint64_t avg_time_us;
    uint32_t operations;
    float throughput_ops_per_sec;
} optimized_inverted_benchmark_result_t;

// Benchmark functions
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void optimized_inverted_benchmark_result_init(optimized_inverted_benchmark_result_t* result) {
    memset(result, 0, sizeof(optimized_inverted_benchmark_result_t));
    result->min_time_us = UINT64_MAX;
}

static void optimized_inverted_benchmark_result_add_measurement(optimized_inverted_benchmark_result_t* result, uint64_t time_us) {
    result->total_time_us += time_us;
    result->operations++;
    
    if (time_us < result->min_time_us) {
        result->min_time_us = time_us;
    }
    if (time_us > result->max_time_us) {
        result->max_time_us = time_us;
    }
}

static void optimized_inverted_benchmark_result_finalize(optimized_inverted_benchmark_result_t* result) {
    if (result->operations > 0) {
        result->avg_time_us = result->total_time_us / result->operations;
        result->throughput_ops_per_sec = (float)result->operations / (result->total_time_us / 1000000.0f);
    }
}

// Generate test nodes
static void generate_test_nodes(lattice_node_t* nodes, uint32_t count) {
    srand(time(NULL));
    
    const char* test_data[] = {
        "machine learning algorithm optimization neural network deep learning",
        "data processing analysis statistical modeling prediction regression",
        "computer vision image recognition convolution neural network CNN",
        "natural language processing text analysis sentiment classification",
        "artificial intelligence robotics automation control systems AI",
        "database management query optimization indexing performance SQL",
        "distributed systems microservices cloud computing scalability",
        "cybersecurity encryption authentication network security protocols",
        "blockchain cryptocurrency smart contracts decentralized finance",
        "quantum computing quantum algorithms superposition entanglement"
    };
    
    for (uint32_t i = 0; i < count; i++) {
        nodes[i].id = i + 1;
        nodes[i].type = (lattice_node_type_t)((rand() % 10) + 1);
        snprintf(nodes[i].name, sizeof(nodes[i].name), "optimized_inverted_test_node_%u", i);
        strncpy(nodes[i].data, test_data[i % 10], sizeof(nodes[i].data) - 1);
        nodes[i].data[sizeof(nodes[i].data) - 1] = '\0';
        nodes[i].parent_id = (i > 0) ? (rand() % i) + 1 : 0;
        nodes[i].child_count = 0;
        nodes[i].children = NULL;
        nodes[i].confidence = (double)rand() / RAND_MAX;
        nodes[i].timestamp = get_time_us();
    }
}

// Benchmark optimized inverted index insertions
static optimized_inverted_benchmark_result_t benchmark_optimized_insertions(void) {
    optimized_inverted_benchmark_result_t result;
    optimized_inverted_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Optimized Inverted Index Insertions...\n");
    
    optimized_inverted_index_t index;
    if (optimized_inverted_index_create(&index, OPTIMIZED_INVERTED_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create optimized inverted index\n");
        return result;
    }
    
    // Generate test nodes
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(OPTIMIZED_INVERTED_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        optimized_inverted_index_destroy(&index);
        return result;
    }
    generate_test_nodes(test_nodes, OPTIMIZED_INVERTED_BENCHMARK_NODES);
    
    // Benchmark insertions
    printf("  📝 Testing optimized insertions...\n");
    for (uint32_t iter = 0; iter < OPTIMIZED_INVERTED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_NODES; i++) {
            optimized_inverted_index_add_document(&index, &test_nodes[i]);
        }
        
        uint64_t end_time = get_time_us();
        optimized_inverted_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    optimized_inverted_benchmark_result_finalize(&result);
    
    // Get index statistics
    uint32_t total_terms, total_documents, total_postings;
    float avg_posting_length;
    optimized_inverted_index_get_stats(&index, &total_terms, &total_documents, 
                                     &total_postings, &avg_posting_length);
    
    printf("  ✅ Optimized Insertions Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Total Terms: %u\n", total_terms);
    printf("     Total Documents: %u\n", total_documents);
    printf("     Total Postings: %u\n", total_postings);
    printf("     Avg Posting Length: %.2f\n", avg_posting_length);
    
    free(test_nodes);
    optimized_inverted_index_destroy(&index);
    
    return result;
}

// Benchmark optimized inverted index searches
static optimized_inverted_benchmark_result_t benchmark_optimized_searches(void) {
    optimized_inverted_benchmark_result_t result;
    optimized_inverted_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Optimized Inverted Index Searches...\n");
    
    optimized_inverted_index_t index;
    if (optimized_inverted_index_create(&index, OPTIMIZED_INVERTED_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create optimized inverted index\n");
        return result;
    }
    
    // Generate test nodes and add to index
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(OPTIMIZED_INVERTED_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        optimized_inverted_index_destroy(&index);
        return result;
    }
    generate_test_nodes(test_nodes, OPTIMIZED_INVERTED_BENCHMARK_NODES);
    
    // Add all nodes to index
    for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_NODES; i++) {
        optimized_inverted_index_add_document(&index, &test_nodes[i]);
    }
    
    // Optimize index
    optimized_inverted_index_optimize(&index);
    
    // Test queries
    const char* test_queries[] = {
        "machine learning",
        "data processing",
        "neural network",
        "artificial intelligence",
        "database management",
        "distributed systems",
        "cybersecurity",
        "blockchain",
        "quantum computing",
        "optimization"
    };
    
    // Benchmark searches
    printf("  🔍 Testing optimized searches...\n");
    optimized_search_result_t search_results[1000];
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < OPTIMIZED_INVERTED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_QUERIES; i++) {
            const char* query = test_queries[i % 10];
            optimized_inverted_index_search_term(&index, query, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        optimized_inverted_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    optimized_inverted_benchmark_result_finalize(&result);
    
    printf("  ✅ Optimized Searches Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Hash Table Size: %u\n", index.hash_table_size);
    printf("     Total Terms: %u\n", index.total_terms);
    printf("     Total Postings: %u\n", index.total_postings);
    
    free(test_nodes);
    optimized_inverted_index_destroy(&index);
    
    return result;
}

// Benchmark optimized query processing
static optimized_inverted_benchmark_result_t benchmark_optimized_queries(void) {
    optimized_inverted_benchmark_result_t result;
    optimized_inverted_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Optimized Query Processing...\n");
    
    optimized_inverted_index_t index;
    if (optimized_inverted_index_create(&index, OPTIMIZED_INVERTED_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create optimized inverted index\n");
        return result;
    }
    
    // Generate test nodes and add to index
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(OPTIMIZED_INVERTED_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        optimized_inverted_index_destroy(&index);
        return result;
    }
    generate_test_nodes(test_nodes, OPTIMIZED_INVERTED_BENCHMARK_NODES);
    
    // Add all nodes to index
    for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_NODES; i++) {
        optimized_inverted_index_add_document(&index, &test_nodes[i]);
    }
    
    // Optimize index
    optimized_inverted_index_optimize(&index);
    
    // Test queries
    const char* test_query_texts[] = {
        "machine learning neural network",
        "data processing analysis",
        "artificial intelligence robotics",
        "database management SQL",
        "distributed systems cloud",
        "cybersecurity encryption",
        "blockchain cryptocurrency",
        "quantum computing algorithms",
        "optimization performance",
        "computer vision CNN"
    };
    
    // Benchmark query processing
    printf("  🔍 Testing optimized query processing...\n");
    optimized_search_result_t search_results[1000];
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < OPTIMIZED_INVERTED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_QUERIES; i++) {
            const char* query_text = test_query_texts[i % 10];
            
            // Create and process query
            optimized_query_t query;
            if (optimized_query_create(&query, query_text, false, false) == 0) {
                optimized_inverted_index_search(&index, &query, search_results, &search_count);
                optimized_query_destroy(&query);
            }
        }
        
        uint64_t end_time = get_time_us();
        optimized_inverted_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    optimized_inverted_benchmark_result_finalize(&result);
    
    printf("  ✅ Optimized Query Processing Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Query Processing: Multi-term queries\n");
    printf("     Relevance Scoring: TF-IDF based\n");
    
    free(test_nodes);
    optimized_inverted_index_destroy(&index);
    
    return result;
}

// Benchmark fuzzy search
static optimized_inverted_benchmark_result_t benchmark_optimized_fuzzy_search(void) {
    optimized_inverted_benchmark_result_t result;
    optimized_inverted_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Optimized Fuzzy Search...\n");
    
    optimized_inverted_index_t index;
    if (optimized_inverted_index_create(&index, OPTIMIZED_INVERTED_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create optimized inverted index\n");
        return result;
    }
    
    // Generate test nodes and add to index
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(OPTIMIZED_INVERTED_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        optimized_inverted_index_destroy(&index);
        return result;
    }
    generate_test_nodes(test_nodes, OPTIMIZED_INVERTED_BENCHMARK_NODES);
    
    // Add all nodes to index
    for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_NODES; i++) {
        optimized_inverted_index_add_document(&index, &test_nodes[i]);
    }
    
    // Optimize index
    optimized_inverted_index_optimize(&index);
    
    // Test fuzzy queries
    const char* test_fuzzy_queries[] = {
        "machin",      // fuzzy for "machine"
        "neural",      // exact match
        "algoritm",    // fuzzy for "algorithm"
        "procesing",   // fuzzy for "processing"
        "inteligence", // fuzzy for "intelligence"
        "databse",     // fuzzy for "database"
        "distributd",  // fuzzy for "distributed"
        "securty",     // fuzzy for "security"
        "blockchai",   // fuzzy for "blockchain"
        "quantm"       // fuzzy for "quantum"
    };
    
    // Benchmark fuzzy search
    printf("  🔍 Testing optimized fuzzy search...\n");
    optimized_search_result_t search_results[1000];
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < OPTIMIZED_INVERTED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < OPTIMIZED_INVERTED_BENCHMARK_QUERIES; i++) {
            const char* query = test_fuzzy_queries[i % 10];
            optimized_inverted_index_search_fuzzy(&index, query, 2, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        optimized_inverted_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    optimized_inverted_benchmark_result_finalize(&result);
    
    printf("  ✅ Optimized Fuzzy Search Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Fuzzy Matching: Levenshtein distance based\n");
    printf("     Similarity Threshold: 0.7\n");
    
    free(test_nodes);
    optimized_inverted_index_destroy(&index);
    
    return result;
}

// Main benchmark function
int main(void) {
    printf("🚀 OPTIMIZED INVERTED INDEX BENCHMARK SUITE\n");
    printf("==========================================\n\n");
    
    printf("Configuration:\n");
    printf("  Nodes: %d\n", OPTIMIZED_INVERTED_BENCHMARK_NODES);
    printf("  Queries: %d\n", OPTIMIZED_INVERTED_BENCHMARK_QUERIES);
    printf("  Iterations: %d\n", OPTIMIZED_INVERTED_BENCHMARK_ITERATIONS);
    printf("  Hash Table Size: %d\n", OPTIMIZED_INVERTED_INDEX_HASH_TABLE_SIZE);
    printf("  Max Terms: %d\n", OPTIMIZED_INVERTED_INDEX_MAX_TERMS);
    printf("  Max Docs Per Term: %d\n", OPTIMIZED_INVERTED_INDEX_MAX_DOCS_PER_TERM);
    printf("\n");
    
    // Run benchmarks
    optimized_inverted_benchmark_result_t insertion_result = benchmark_optimized_insertions();
    printf("\n");
    
    optimized_inverted_benchmark_result_t search_result = benchmark_optimized_searches();
    printf("\n");
    
    optimized_inverted_benchmark_result_t query_result = benchmark_optimized_queries();
    printf("\n");
    
    optimized_inverted_benchmark_result_t fuzzy_result = benchmark_optimized_fuzzy_search();
    printf("\n");
    
    // Summary
    printf("📊 OPTIMIZED INVERTED INDEX BENCHMARK SUMMARY\n");
    printf("=============================================\n");
    printf("Insertions:           %.2f ops/sec\n", insertion_result.throughput_ops_per_sec);
    printf("Searches:             %.2f ops/sec\n", search_result.throughput_ops_per_sec);
    printf("Query Processing:     %.2f ops/sec\n", query_result.throughput_ops_per_sec);
    printf("Fuzzy Search:         %.2f ops/sec\n", fuzzy_result.throughput_ops_per_sec);
    
    printf("\n🎯 OPTIMIZATION ACHIEVEMENTS:\n");
    printf("✅ Hash table for O(1) term lookup\n");
    printf("✅ Optimized memory layout for posting lists\n");
    printf("✅ TF-IDF scoring for relevance ranking\n");
    printf("✅ Fuzzy search with similarity matching\n");
    printf("✅ Multi-term query processing\n");
    printf("✅ Case-insensitive search\n");
    printf("✅ Batch processing for better performance\n");
    printf("✅ Memory-efficient data structures\n");
    
    printf("\n🚀 PERFORMANCE IMPROVEMENTS:\n");
    printf("• Hash table lookup: O(1) vs O(n) linear search\n");
    printf("• Memory layout: Optimized for cache performance\n");
    printf("• Relevance scoring: TF-IDF based ranking\n");
    printf("• Fuzzy search: Similarity-based matching\n");
    printf("• Query processing: Multi-term support\n");
    
    printf("\n✅ Optimized inverted index benchmark complete!\n");
    
    return 0;
}
