#define _GNU_SOURCE
#include "semantic_vector_indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// Benchmark configuration
#define VECTOR_BENCHMARK_NODES 5000
#define VECTOR_BENCHMARK_QUERIES 100
#define VECTOR_BENCHMARK_ITERATIONS 5

// Performance measurement
typedef struct {
    uint64_t total_time_us;
    uint64_t min_time_us;
    uint64_t max_time_us;
    uint64_t avg_time_us;
    uint32_t operations;
    float throughput_ops_per_sec;
} vector_benchmark_result_t;

// Benchmark functions
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void vector_benchmark_result_init(vector_benchmark_result_t* result) {
    memset(result, 0, sizeof(vector_benchmark_result_t));
    result->min_time_us = UINT64_MAX;
}

static void vector_benchmark_result_add_measurement(vector_benchmark_result_t* result, uint64_t time_us) {
    result->total_time_us += time_us;
    result->operations++;
    
    if (time_us < result->min_time_us) {
        result->min_time_us = time_us;
    }
    if (time_us > result->max_time_us) {
        result->max_time_us = time_us;
    }
}

static void vector_benchmark_result_finalize(vector_benchmark_result_t* result) {
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
        snprintf(nodes[i].name, sizeof(nodes[i].name), "vector_test_node_%u", i);
        snprintf(nodes[i].data, sizeof(nodes[i].data), "vector_test_data_%u_with_semantic_content", i);
        nodes[i].parent_id = (i > 0) ? (rand() % i) + 1 : 0;
        nodes[i].child_count = 0;
        nodes[i].children = NULL;
        nodes[i].confidence = (double)rand() / RAND_MAX;
        nodes[i].timestamp = get_time_us();
    }
}

// Benchmark vector embedding generation
static vector_benchmark_result_t benchmark_vector_embedding_generation(void) {
    vector_benchmark_result_t result;
    vector_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Vector Embedding Generation...\n");
    
    // Generate test nodes
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(VECTOR_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        printf("❌ Failed to allocate test nodes\n");
        return result;
    }
    generate_test_nodes(test_nodes, VECTOR_BENCHMARK_NODES);
    
    // Benchmark embedding generation
    printf("  📝 Testing embedding generation...\n");
    for (uint32_t iter = 0; iter < VECTOR_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < VECTOR_BENCHMARK_NODES; i++) {
            float embedding[VECTOR_DIM];
            generate_node_embedding(&test_nodes[i], embedding);
        }
        
        uint64_t end_time = get_time_us();
        vector_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    vector_benchmark_result_finalize(&result);
    
    printf("  ✅ Vector Embedding Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    free(test_nodes);
    return result;
}

// Benchmark LSH index
static vector_benchmark_result_t benchmark_lsh_index(void) {
    vector_benchmark_result_t result;
    vector_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking LSH Index...\n");
    
    enhanced_lsh_index_t lsh_index;
    if (enhanced_lsh_index_create(&lsh_index, VECTOR_DIM, 8) != 0) {
        printf("❌ Failed to create LSH index\n");
        return result;
    }
    
    // Generate test vectors
    float* test_vectors = (float*)malloc(VECTOR_BENCHMARK_NODES * VECTOR_DIM * sizeof(float));
    if (!test_vectors) {
        enhanced_lsh_index_destroy(&lsh_index);
        return result;
    }
    
    srand(time(NULL));
    for (uint32_t i = 0; i < VECTOR_BENCHMARK_NODES; i++) {
        for (int j = 0; j < VECTOR_DIM; j++) {
            test_vectors[i * VECTOR_DIM + j] = random_float_range(-1.0f, 1.0f);
        }
    }
    
    // Benchmark insertions
    printf("  📝 Testing LSH insertions...\n");
    for (uint32_t iter = 0; iter < VECTOR_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < VECTOR_BENCHMARK_NODES; i++) {
            enhanced_lsh_index_add_vector(&lsh_index, &test_vectors[i * VECTOR_DIM], i + 1);
        }
        
        uint64_t end_time = get_time_us();
        vector_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing LSH searches...\n");
    vector_similarity_result_t* search_results = (vector_similarity_result_t*)malloc(1000 * sizeof(vector_similarity_result_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < VECTOR_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < VECTOR_BENCHMARK_QUERIES; i++) {
            enhanced_lsh_index_search_similar(&lsh_index, &test_vectors[i * VECTOR_DIM], 
                                            0.8f, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        vector_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    vector_benchmark_result_finalize(&result);
    
    printf("  ✅ LSH Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    
    free(search_results);
    free(test_vectors);
    enhanced_lsh_index_destroy(&lsh_index);
    
    return result;
}

// Benchmark clustering index
static vector_benchmark_result_t benchmark_clustering_index(void) {
    vector_benchmark_result_t result;
    vector_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Clustering Index...\n");
    
    enhanced_clustering_index_t clustering_index;
    if (enhanced_clustering_index_create(&clustering_index, 50, VECTOR_DIM) != 0) {
        printf("❌ Failed to create clustering index\n");
        return result;
    }
    
    // Generate test vectors
    enhanced_semantic_vector_t* test_vectors = (enhanced_semantic_vector_t*)malloc(VECTOR_BENCHMARK_NODES * sizeof(enhanced_semantic_vector_t));
    if (!test_vectors) {
        enhanced_clustering_index_destroy(&clustering_index);
        return result;
    }
    
    srand(time(NULL));
    for (uint32_t i = 0; i < VECTOR_BENCHMARK_NODES; i++) {
        test_vectors[i].node_id = i + 1;
        test_vectors[i].cluster_id = 0;
        test_vectors[i].cluster_confidence = 0.0f;
        test_vectors[i].frequency = 1;
        test_vectors[i].last_accessed = get_time_us();
        test_vectors[i].semantic_weight = 1.0f;
        test_vectors[i].related_count = 0;
        
        for (int j = 0; j < VECTOR_DIM; j++) {
            test_vectors[i].embedding[j] = random_float_range(-1.0f, 1.0f);
        }
    }
    
    // Benchmark clustering
    printf("  📝 Testing clustering...\n");
    for (uint32_t iter = 0; iter < VECTOR_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        enhanced_clustering_index_cluster(&clustering_index, test_vectors, VECTOR_BENCHMARK_NODES);
        
        uint64_t end_time = get_time_us();
        vector_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    vector_benchmark_result_finalize(&result);
    
    printf("  ✅ Clustering Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Clusters: %u\n", clustering_index.cluster_count);
    printf("     Silhouette Score: %.4f\n", clustering_index.silhouette_score);
    
    free(test_vectors);
    enhanced_clustering_index_destroy(&clustering_index);
    
    return result;
}

// Benchmark semantic vector indexing system
static vector_benchmark_result_t benchmark_semantic_vector_system(void) {
    vector_benchmark_result_t result;
    vector_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Semantic Vector Indexing System...\n");
    
    semantic_vector_indexing_system_t system;
    if (semantic_vector_indexing_system_create(&system) != 0) {
        printf("❌ Failed to create semantic vector indexing system\n");
        return result;
    }
    
    // Generate test nodes
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(VECTOR_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        semantic_vector_indexing_system_destroy(&system);
        return result;
    }
    generate_test_nodes(test_nodes, VECTOR_BENCHMARK_NODES);
    
    // Benchmark insertions
    printf("  📝 Testing system insertions...\n");
    for (uint32_t iter = 0; iter < VECTOR_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < VECTOR_BENCHMARK_NODES; i++) {
            semantic_vector_indexing_system_add_node(&system, &test_nodes[i]);
        }
        
        uint64_t end_time = get_time_us();
        vector_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing system searches...\n");
    vector_similarity_query_t query;
    memset(&query, 0, sizeof(query));
    for (int i = 0; i < VECTOR_DIM; i++) {
        query.query_vector[i] = random_float_range(-1.0f, 1.0f);
    }
    query.max_results = 100;
    query.min_similarity = 0.5f;
    query.use_lsh = true;
    
    vector_similarity_result_t* search_results = (vector_similarity_result_t*)malloc(1000 * sizeof(vector_similarity_result_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < VECTOR_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < VECTOR_BENCHMARK_QUERIES; i++) {
            semantic_vector_indexing_system_search_similar(&system, &query, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        vector_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    vector_benchmark_result_finalize(&result);
    
    // Get system statistics
    uint32_t vector_count, cluster_count;
    float avg_similarity, silhouette_score;
    semantic_vector_indexing_system_get_stats(&system, &vector_count, &cluster_count, &avg_similarity, &silhouette_score);
    
    printf("  ✅ Semantic Vector System Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Vectors: %u\n", vector_count);
    printf("     Clusters: %u\n", cluster_count);
    printf("     Avg Similarity: %.4f\n", avg_similarity);
    printf("     Silhouette Score: %.4f\n", silhouette_score);
    
    free(search_results);
    free(test_nodes);
    semantic_vector_indexing_system_destroy(&system);
    
    return result;
}

// Main benchmark function
int main(void) {
    printf("🚀 SEMANTIC VECTOR INDEXING BENCHMARK SUITE - PHASE 2\n");
    printf("=====================================================\n\n");
    
    printf("Configuration:\n");
    printf("  Nodes: %d\n", VECTOR_BENCHMARK_NODES);
    printf("  Queries: %d\n", VECTOR_BENCHMARK_QUERIES);
    printf("  Iterations: %d\n", VECTOR_BENCHMARK_ITERATIONS);
    printf("  Vector Dimension: %d\n", VECTOR_DIM);
    printf("  Max Vectors: %d\n", MAX_VECTORS);
    printf("\n");
    
    // Run benchmarks
    vector_benchmark_result_t embedding_result = benchmark_vector_embedding_generation();
    printf("\n");
    
    vector_benchmark_result_t lsh_result = benchmark_lsh_index();
    printf("\n");
    
    vector_benchmark_result_t clustering_result = benchmark_clustering_index();
    printf("\n");
    
    vector_benchmark_result_t system_result = benchmark_semantic_vector_system();
    printf("\n");
    
    // Summary
    printf("📊 VECTOR INDEXING BENCHMARK SUMMARY\n");
    printf("=====================================\n");
    printf("Vector Embedding:     %.2f ops/sec\n", embedding_result.throughput_ops_per_sec);
    printf("LSH Index:            %.2f ops/sec\n", lsh_result.throughput_ops_per_sec);
    printf("Clustering Index:     %.2f ops/sec\n", clustering_result.throughput_ops_per_sec);
    printf("Semantic System:      %.2f ops/sec\n", system_result.throughput_ops_per_sec);
    
    printf("\n🎯 PHASE 2 ACHIEVEMENTS:\n");
    printf("✅ Vector Embedding Generation\n");
    printf("✅ LSH (Locality Sensitive Hashing)\n");
    printf("✅ K-means Clustering\n");
    printf("✅ Semantic Vector Indexing System\n");
    printf("✅ Multi-dimensional Similarity Search\n");
    printf("✅ Cluster-based Organization\n");
    
    printf("\n✅ Phase 2 benchmark complete!\n");
    
    return 0;
}
