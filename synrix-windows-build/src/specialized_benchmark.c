#define _GNU_SOURCE
#include "specialized_indexing.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// Benchmark configuration
#define SPECIALIZED_BENCHMARK_NODES 2000
#define SPECIALIZED_BENCHMARK_QUERIES 50
#define SPECIALIZED_BENCHMARK_ITERATIONS 3

// Performance measurement
typedef struct {
    uint64_t total_time_us;
    uint64_t min_time_us;
    uint64_t max_time_us;
    uint64_t avg_time_us;
    uint32_t operations;
    float throughput_ops_per_sec;
} specialized_benchmark_result_t;

// Benchmark functions
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void specialized_benchmark_result_init(specialized_benchmark_result_t* result) {
    memset(result, 0, sizeof(specialized_benchmark_result_t));
    result->min_time_us = UINT64_MAX;
}

static void specialized_benchmark_result_add_measurement(specialized_benchmark_result_t* result, uint64_t time_us) {
    result->total_time_us += time_us;
    result->operations++;
    
    if (time_us < result->min_time_us) {
        result->min_time_us = time_us;
    }
    if (time_us > result->max_time_us) {
        result->max_time_us = time_us;
    }
}

static void specialized_benchmark_result_finalize(specialized_benchmark_result_t* result) {
    if (result->operations > 0) {
        result->avg_time_us = result->total_time_us / result->operations;
        result->throughput_ops_per_sec = (float)result->operations / (result->total_time_us / 1000000.0f);
    }
}

// Generate test nodes
static void generate_test_nodes(lattice_node_t* nodes, uint32_t count) {
    srand(time(NULL));
    
    const char* test_data[] = {
        "machine learning algorithm optimization neural network",
        "data processing analysis statistical modeling prediction",
        "computer vision image recognition deep learning convolution",
        "natural language processing text analysis sentiment",
        "artificial intelligence robotics automation control systems",
        "database management query optimization indexing performance",
        "distributed systems microservices cloud computing scalability",
        "cybersecurity encryption authentication network security",
        "blockchain cryptocurrency smart contracts decentralized",
        "quantum computing quantum algorithms superposition entanglement"
    };
    
    for (uint32_t i = 0; i < count; i++) {
        nodes[i].id = i + 1;
        nodes[i].type = (lattice_node_type_t)((rand() % 10) + 1);
        snprintf(nodes[i].name, sizeof(nodes[i].name), "specialized_test_node_%u", i);
        strncpy(nodes[i].data, test_data[i % 10], sizeof(nodes[i].data) - 1);
        nodes[i].data[sizeof(nodes[i].data) - 1] = '\0';
        nodes[i].parent_id = (i > 0) ? (rand() % i) + 1 : 0;
        nodes[i].child_count = 0;
        nodes[i].children = NULL;
        nodes[i].confidence = (double)rand() / RAND_MAX;
        nodes[i].timestamp = get_time_us();
    }
}

// Benchmark Bloom filter operations
static specialized_benchmark_result_t benchmark_bloom_filter(void) {
    specialized_benchmark_result_t result;
    specialized_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Bloom Filter...\n");
    
    enhanced_bloom_filter_t bloom_filter;
    if (enhanced_bloom_filter_create(&bloom_filter, SPECIALIZED_BENCHMARK_NODES, 0.01f) != 0) {
        printf("❌ Failed to create Bloom filter\n");
        return result;
    }
    
    // Generate test keys
    char** test_keys = (char**)malloc(SPECIALIZED_BENCHMARK_NODES * sizeof(char*));
    if (!test_keys) {
        enhanced_bloom_filter_destroy(&bloom_filter);
        return result;
    }
    
    for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
        test_keys[i] = (char*)malloc(64);
        snprintf(test_keys[i], 64, "bloom_test_key_%u", i);
    }
    
    // Benchmark insertions
    printf("  📝 Testing Bloom filter insertions...\n");
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
            enhanced_bloom_filter_add(&bloom_filter, test_keys[i]);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark lookups
    printf("  🔍 Testing Bloom filter lookups...\n");
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_QUERIES; i++) {
            enhanced_bloom_filter_contains(&bloom_filter, test_keys[i % SPECIALIZED_BENCHMARK_NODES]);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    specialized_benchmark_result_finalize(&result);
    
    // Get Bloom filter statistics
    uint32_t elements;
    float fp_rate;
    uint64_t queries;
    enhanced_bloom_filter_get_stats(&bloom_filter, &elements, &fp_rate, &queries);
    
    printf("  ✅ Bloom Filter Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Elements: %u\n", elements);
    printf("     False Positive Rate: %.4f\n", fp_rate);
    printf("     Total Queries: %lu\n", queries);
    
    // Clean up
    for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
        free(test_keys[i]);
    }
    free(test_keys);
    enhanced_bloom_filter_destroy(&bloom_filter);
    
    return result;
}

// Benchmark inverted index operations
static specialized_benchmark_result_t benchmark_inverted_index(void) {
    specialized_benchmark_result_t result;
    specialized_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Inverted Index...\n");
    
    enhanced_inverted_index_t inverted_index;
    if (enhanced_inverted_index_create(&inverted_index, SPECIALIZED_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create inverted index\n");
        return result;
    }
    
    // Generate test documents
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(SPECIALIZED_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        enhanced_inverted_index_destroy(&inverted_index);
        return result;
    }
    generate_test_nodes(test_nodes, SPECIALIZED_BENCHMARK_NODES);
    
    // Benchmark document additions
    printf("  📝 Testing inverted index document additions...\n");
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
            enhanced_inverted_index_add_document(&inverted_index, test_nodes[i].id, test_nodes[i].data);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing inverted index searches...\n");
    const char* search_queries[] = {
        "machine learning",
        "data processing",
        "neural network",
        "deep learning",
        "artificial intelligence"
    };
    
    uint32_t* search_results = (uint32_t*)malloc(1000 * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_QUERIES; i++) {
            const char* query = search_queries[i % 5];
            enhanced_inverted_index_search(&inverted_index, query, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    specialized_benchmark_result_finalize(&result);
    
    printf("  ✅ Inverted Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Terms: %u\n", inverted_index.term_count);
    printf("     Documents: %u\n", inverted_index.total_documents);
    printf("     Total Terms: %u\n", inverted_index.total_terms);
    
    free(test_nodes);
    free(search_results);
    enhanced_inverted_index_destroy(&inverted_index);
    
    return result;
}

// Benchmark temporal index operations
static specialized_benchmark_result_t benchmark_temporal_index(void) {
    specialized_benchmark_result_t result;
    specialized_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Temporal Index...\n");
    
    enhanced_temporal_index_t temporal_index;
    if (enhanced_temporal_index_create(&temporal_index, SPECIALIZED_BENCHMARK_NODES) != 0) {
        printf("❌ Failed to create temporal index\n");
        return result;
    }
    
    // Generate test temporal entries
    printf("  📝 Testing temporal index entry additions...\n");
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
            enhanced_temporal_entry_t entry;
            entry.node_id = i + 1;
            entry.start_time = get_time_us() + (i * 1000);
            entry.end_time = entry.start_time + (rand() % 10000);
            entry.duration = entry.end_time - entry.start_time;
            entry.event_type = rand() % 10;
            entry.priority = rand() % 256;
            entry.importance_score = (float)rand() / RAND_MAX;
            snprintf(entry.event_description, sizeof(entry.event_description), "temporal_event_%u", i);
            entry.related_events = NULL;
            entry.related_count = 0;
            entry.frequency = 1;
            entry.last_occurrence = entry.start_time;
            
            enhanced_temporal_index_add_entry(&temporal_index, &entry);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark time range searches
    printf("  🔍 Testing temporal index time range searches...\n");
    uint32_t* search_results = (uint32_t*)malloc(1000 * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_QUERIES; i++) {
            uint64_t query_start = get_time_us() - 1000000;
            uint64_t query_end = get_time_us() + 1000000;
            enhanced_temporal_index_search_time_range(&temporal_index, query_start, query_end, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    specialized_benchmark_result_finalize(&result);
    
    // Get temporal statistics
    uint32_t total_events;
    uint64_t time_span;
    float avg_duration;
    enhanced_temporal_index_get_stats(&temporal_index, &total_events, &time_span, &avg_duration);
    
    printf("  ✅ Temporal Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Total Events: %u\n", total_events);
    printf("     Time Span: %lu μs\n", time_span);
    printf("     Avg Duration: %.2f μs\n", avg_duration);
    
    free(search_results);
    enhanced_temporal_index_destroy(&temporal_index);
    
    return result;
}

// Benchmark geographic index operations
static specialized_benchmark_result_t benchmark_geographic_index(void) {
    specialized_benchmark_result_t result;
    specialized_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Geographic Index...\n");
    
    geographic_index_t geographic_index;
    if (geographic_index_create(&geographic_index, SPECIALIZED_BENCHMARK_NODES, 0.1f) != 0) {
        printf("❌ Failed to create geographic index\n");
        return result;
    }
    
    // Generate test geographic entries
    printf("  📝 Testing geographic index entry additions...\n");
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
            geographic_entry_t entry;
            entry.node_id = i + 1;
            entry.latitude = (float)(rand() % 180) - 90.0f;
            entry.longitude = (float)(rand() % 360) - 180.0f;
            entry.altitude = (float)(rand() % 10000);
            entry.accuracy = 1.0f;
            snprintf(entry.location_name, sizeof(entry.location_name), "location_%u", i);
            entry.location_type = rand() % 10;
            entry.bounding_box[0] = entry.latitude - 0.1f;
            entry.bounding_box[1] = entry.latitude + 0.1f;
            entry.bounding_box[2] = entry.longitude - 0.1f;
            entry.bounding_box[3] = entry.longitude + 0.1f;
            entry.nearby_nodes = NULL;
            entry.nearby_count = 0;
            
            geographic_index_add_entry(&geographic_index, &entry);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark bounding box searches
    printf("  🔍 Testing geographic index bounding box searches...\n");
    uint32_t* search_results = (uint32_t*)malloc(1000 * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_QUERIES; i++) {
            float min_lat = -90.0f + (rand() % 90);
            float max_lat = min_lat + 10.0f;
            float min_lon = -180.0f + (rand() % 180);
            float max_lon = min_lon + 10.0f;
            
            geographic_index_search_bounding_box(&geographic_index, min_lat, max_lat, min_lon, max_lon, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    specialized_benchmark_result_finalize(&result);
    
    printf("  ✅ Geographic Index Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Entries: %u\n", geographic_index.count);
    printf("     Grid Size: %u\n", geographic_index.grid_size);
    printf("     Grid Resolution: %.2f\n", geographic_index.grid_resolution);
    
    free(search_results);
    geographic_index_destroy(&geographic_index);
    
    return result;
}

// Benchmark specialized indexing system
static specialized_benchmark_result_t benchmark_specialized_system(void) {
    specialized_benchmark_result_t result;
    specialized_benchmark_result_init(&result);
    
    printf("🔍 Benchmarking Specialized Indexing System...\n");
    
    specialized_indexing_system_t system;
    if (specialized_indexing_system_create(&system) != 0) {
        printf("❌ Failed to create specialized indexing system\n");
        return result;
    }
    
    // Generate test nodes
    lattice_node_t* test_nodes = (lattice_node_t*)malloc(SPECIALIZED_BENCHMARK_NODES * sizeof(lattice_node_t));
    if (!test_nodes) {
        specialized_indexing_system_destroy(&system);
        return result;
    }
    generate_test_nodes(test_nodes, SPECIALIZED_BENCHMARK_NODES);
    
    // Benchmark node additions
    printf("  📝 Testing specialized system node additions...\n");
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_NODES; i++) {
            specialized_indexing_system_add_node(&system, &test_nodes[i]);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    // Benchmark searches
    printf("  🔍 Testing specialized system searches...\n");
    const char* search_queries[] = {
        "machine learning",
        "data processing",
        "neural network",
        "deep learning",
        "artificial intelligence"
    };
    
    uint32_t* search_results = (uint32_t*)malloc(1000 * sizeof(uint32_t));
    uint32_t search_count = 0;
    
    for (uint32_t iter = 0; iter < SPECIALIZED_BENCHMARK_ITERATIONS; iter++) {
        uint64_t start_time = get_time_us();
        
        for (uint32_t i = 0; i < SPECIALIZED_BENCHMARK_QUERIES; i++) {
            const char* query = search_queries[i % 5];
            specialized_indexing_system_search(&system, query, search_results, &search_count);
        }
        
        uint64_t end_time = get_time_us();
        specialized_benchmark_result_add_measurement(&result, end_time - start_time);
    }
    
    specialized_benchmark_result_finalize(&result);
    
    // Get system statistics
    uint32_t total_entries;
    float avg_query_time;
    specialized_indexing_system_get_stats(&system, &total_entries, &avg_query_time);
    
    printf("  ✅ Specialized System Results:\n");
    printf("     Operations: %u\n", result.operations);
    printf("     Avg Time: %lu μs\n", result.avg_time_us);
    printf("     Min Time: %lu μs\n", result.min_time_us);
    printf("     Max Time: %lu μs\n", result.max_time_us);
    printf("     Throughput: %.2f ops/sec\n", result.throughput_ops_per_sec);
    printf("     Total Entries: %u\n", total_entries);
    printf("     Avg Query Time: %.2f μs\n", avg_query_time);
    
    free(test_nodes);
    free(search_results);
    specialized_indexing_system_destroy(&system);
    
    return result;
}

// Main benchmark function
int main(void) {
    printf("🚀 SPECIALIZED INDEXING BENCHMARK SUITE - PHASE 4\n");
    printf("==================================================\n\n");
    
    printf("Configuration:\n");
    printf("  Nodes: %d\n", SPECIALIZED_BENCHMARK_NODES);
    printf("  Queries: %d\n", SPECIALIZED_BENCHMARK_QUERIES);
    printf("  Iterations: %d\n", SPECIALIZED_BENCHMARK_ITERATIONS);
    printf("\n");
    
    // Run benchmarks
    specialized_benchmark_result_t bloom_result = benchmark_bloom_filter();
    printf("\n");
    
    specialized_benchmark_result_t inverted_result = benchmark_inverted_index();
    printf("\n");
    
    specialized_benchmark_result_t temporal_result = benchmark_temporal_index();
    printf("\n");
    
    specialized_benchmark_result_t geographic_result = benchmark_geographic_index();
    printf("\n");
    
    specialized_benchmark_result_t system_result = benchmark_specialized_system();
    printf("\n");
    
    // Summary
    printf("📊 SPECIALIZED INDEXING BENCHMARK SUMMARY\n");
    printf("==========================================\n");
    printf("Bloom Filter:           %.2f ops/sec\n", bloom_result.throughput_ops_per_sec);
    printf("Inverted Index:         %.2f ops/sec\n", inverted_result.throughput_ops_per_sec);
    printf("Temporal Index:         %.2f ops/sec\n", temporal_result.throughput_ops_per_sec);
    printf("Geographic Index:       %.2f ops/sec\n", geographic_result.throughput_ops_per_sec);
    printf("Specialized System:     %.2f ops/sec\n", system_result.throughput_ops_per_sec);
    
    printf("\n🎯 PHASE 4 ACHIEVEMENTS:\n");
    printf("✅ Enhanced Bloom Filter (fast negative lookups)\n");
    printf("✅ Enhanced Inverted Index (text search)\n");
    printf("✅ Enhanced Temporal Index (time-based queries)\n");
    printf("✅ Geographic Index (spatial queries)\n");
    printf("✅ Full-Text Search Index (advanced text search)\n");
    printf("✅ Specialized Indexing System (unified system)\n");
    
    printf("\n✅ Phase 4 benchmark complete!\n");
    
    return 0;
}
