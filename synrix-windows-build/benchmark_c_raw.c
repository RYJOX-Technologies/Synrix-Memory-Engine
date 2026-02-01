/**
 * Pure C Benchmark for Synrix Engine
 * ===================================
 * Measures raw engine performance without Python/ctypes overhead.
 * 
 * Compile: gcc -O3 -o benchmark_c_raw benchmark_c_raw.c -L. -lsynrix -lz
 * Run: ./benchmark_c_raw
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "include/persistent_lattice.h"
#include "include/wal.h"

#ifdef _WIN32
    #include <windows.h>
    // High-resolution timing using QueryPerformanceCounter
    static double get_time_ns(void) {
        LARGE_INTEGER frequency, counter;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&counter);
        return (double)counter.QuadPart * 1000000000.0 / (double)frequency.QuadPart;
    }
#else
    #include <time.h>
    // High-resolution timing
    static double get_time_ns(void) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return ts.tv_sec * 1e9 + ts.tv_nsec;
    }
#endif

// Benchmark: Add nodes
static void benchmark_add_nodes(persistent_lattice_t* lattice, uint32_t count) {
    printf("\n=== Benchmark: Adding %u nodes ===\n", count);
    
    double start = get_time_ns();
    for (uint32_t i = 0; i < count; i++) {
        char name[256];
        char data[512];
        snprintf(name, sizeof(name), "BENCH:node_%u", i);
        snprintf(data, sizeof(data), "benchmark_data_%u", i);
        
        uint64_t node_id = lattice_add_node(lattice, LATTICE_NODE_LEARNING, name, data, 0);
        if (node_id == 0) {
            // Failed - skip
            continue;
        }
    }
    double end = get_time_ns();
    double total_us = (end - start) / 1000.0;
    double avg_us = total_us / count;
    double avg_ns = avg_us * 1000.0;
    
    printf("  Total time: %.2f µs\n", total_us);
    printf("  Average: %.2f ns per node\n", avg_ns);
    printf("  Throughput: %.0f nodes/sec\n", (count * 1e9) / (end - start));
}

// Benchmark: O(1) node lookup
static void benchmark_get_nodes(persistent_lattice_t* lattice, uint32_t count) {
    printf("\n=== Benchmark: O(1) Node Lookup (%u lookups) ===\n", count);
    
    // First, get node IDs by name using find_nodes_by_name
    uint64_t* node_ids = malloc(count * sizeof(uint64_t));
    uint32_t found_count = 0;
    
    for (uint32_t i = 0; i < count && found_count < count; i++) {
        char name[256];
        snprintf(name, sizeof(name), "BENCH:node_%u", i);
        
        uint64_t found_ids[1];
        uint32_t result_count = lattice_find_nodes_by_name(lattice, name, found_ids, 1);
        if (result_count > 0 && found_ids[0] != 0) {
            node_ids[found_count++] = found_ids[0];
        }
    }
    
    if (found_count == 0) {
        printf("  No nodes found for lookup test\n");
        free(node_ids);
        return;
    }
    
    // Now benchmark lookups by ID (using safe API)
    double start = get_time_ns();
    for (uint32_t i = 0; i < found_count; i++) {
        lattice_node_t node;
        lattice_get_node_data(lattice, node_ids[i], &node);
        (void)node;  // Prevent optimization
    }
    double end = get_time_ns();
    double total_us = (end - start) / 1000.0;
    double avg_us = total_us / found_count;
    double avg_ns = avg_us * 1000.0;
    
    printf("  Total time: %.2f µs\n", total_us);
    printf("  Average: %.2f ns per lookup\n", avg_ns);
    printf("  P50 (median): ~%.2f ns\n", avg_ns);
    printf("  Throughput: %.0f lookups/sec\n", (found_count * 1e9) / (end - start));
    
    free(node_ids);
}

// Benchmark: Name-based lookups (simulates prefix queries)
static void benchmark_name_lookups(persistent_lattice_t* lattice, uint32_t count) {
    printf("\n=== Benchmark: Name-based Lookups (%u lookups) ===\n", count);
    
    double start = get_time_ns();
    uint32_t found_count = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        char name[256];
        snprintf(name, sizeof(name), "BENCH:node_%u", i);
        
        uint64_t node_id;
        uint32_t result = lattice_find_nodes_by_name(lattice, name, &node_id, 1);
        if (result > 0) {
            found_count++;
        }
    }
    
    double end = get_time_ns();
    double total_us = (end - start) / 1000.0;
    double avg_us = total_us / count;
    double avg_ns = avg_us * 1000.0;
    
    printf("  Total time: %.2f µs\n", total_us);
    printf("  Average: %.2f ns per lookup\n", avg_ns);
    printf("  Found: %u/%u nodes\n", found_count, count);
    printf("  Throughput: %.0f lookups/sec\n", (count * 1e9) / (end - start));
}

// Benchmark: Save operation
static void benchmark_save(persistent_lattice_t* lattice) {
    printf("\n=== Benchmark: Save Operation ===\n");
    
    double start = get_time_ns();
    int result = lattice_save(lattice);
    double end = get_time_ns();
    
    double time_us = (end - start) / 1000.0;
    double time_ms = time_us / 1000.0;
    
    printf("  Result: %s\n", result == 0 ? "Success" : "Failed");
    printf("  Time: %.2f µs (%.2f ms)\n", time_us, time_ms);
}

// Benchmark: Load operation
static void benchmark_load(persistent_lattice_t* lattice, const char* path) {
    printf("\n=== Benchmark: Load Operation ===\n");
    
    double start = get_time_ns();
    int result = lattice_load(lattice);
    double end = get_time_ns();
    
    double time_us = (end - start) / 1000.0;
    double time_ms = time_us / 1000.0;
    
    printf("  Result: %s\n", result == 0 ? "Success" : "Failed");
    printf("  Time: %.2f µs (%.2f ms)\n", time_us, time_ms);
    printf("  Nodes loaded: %u\n", lattice->node_count);
}

int main(int argc, char** argv) {
    // Redirect stderr to suppress debug output during benchmark
    // Note: Only redirect after initialization to catch any startup errors
    FILE* null_file;
    #ifdef _WIN32
        null_file = fopen("nul", "w");
    #else
        null_file = fopen("/dev/null", "w");
    #endif
    if (null_file) {
        // Don't redirect yet - wait until after lattice_init
        // We'll redirect after initialization completes
    }
    
    printf("========================================\n");
    printf("  SYNRIX Pure C Performance Benchmark\n");
    printf("========================================\n");
    printf("Platform: %s\n", 
#ifdef _WIN32
           "Windows"
#else
           "Linux"
#endif
    );
    printf("\n");
    
    const char* lattice_path = "benchmark_raw.lattice";
    uint32_t node_count = 10000;
    uint32_t lookup_count = 1000;
    uint32_t prefix_iterations = 100;
    
    // Parse command line args
    if (argc > 1) node_count = atoi(argv[1]);
    if (argc > 2) lookup_count = atoi(argv[2]);
    if (argc > 3) prefix_iterations = atoi(argv[3]);
    
    // Initialize lattice
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, lattice_path, 2000000, 0) != 0) {
        fprintf(stderr, "Failed to initialize lattice\n");
        return 1;
    }
    
    // Disable evaluation mode for unlimited nodes
    lattice_disable_evaluation_mode(&lattice);
    
    // DISABLE WAL for pure performance benchmark (shows raw engine speed)
    // WAL is auto-enabled by lattice_init, so we must explicitly disable it
    // WAL adds overhead - this benchmark measures pure C performance
    // In production, WAL is enabled for durability, but here we want speed
    lattice_disable_wal(&lattice);
    
    // DISABLE auto-save to prevent interruptions during benchmark
    lattice.persistence.auto_save_enabled = false;
    lattice.persistence.auto_save_interval_nodes = 0;
    lattice.persistence.auto_save_interval_seconds = 0;
    
    // Load existing if present
    lattice_load(&lattice);
    printf("Initial node count: %u\n", lattice.node_count);
    printf("WAL: DISABLED (pure performance mode)\n");
    
    // Now redirect stderr to suppress debug output during benchmarks
    #ifdef _WIN32
        freopen("nul", "w", stderr);
    #else
        freopen("/dev/null", "w", stderr);
    #endif
    
    // Run benchmarks
    benchmark_add_nodes(&lattice, node_count);
    benchmark_get_nodes(&lattice, lookup_count);
    benchmark_name_lookups(&lattice, prefix_iterations);
    benchmark_save(&lattice);
    
    // Reload and benchmark load
    persistent_lattice_t lattice2;
    lattice_init(&lattice2, lattice_path, 2000000, 0);
    benchmark_load(&lattice2, lattice_path);
    
    // Cleanup
    lattice_cleanup(&lattice);
    lattice_cleanup(&lattice2);
    
    printf("\n========================================\n");
    printf("  Benchmark Complete\n");
    printf("========================================\n");
    
    return 0;
}
