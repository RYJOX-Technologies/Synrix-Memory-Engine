/*
 * Query Latency Diagnostic - Per-query nanosecond measurement
 * ===========================================================
 *
 * Answers: Are 96/192 ns real per-query latencies?
 *
 * Measures:
 *   A) lattice_find_nodes_by_name (prefix search = find_by_prefix)
 *   B) lattice_get_node_data (O(1) direct read)
 *
 * Output: min, max, avg, distribution histogram (ns buckets)
 *
 * Build: gcc -O2 -o query_latency_diagnostic query_latency_diagnostic.c \
 *          -I../src -I../src/storage/lattice \
 *          ../src/storage/lattice/persistent_lattice.c [other deps...]
 *
 * Or use run script. Run 1000+ iterations for distribution.
 */

#define _POSIX_C_SOURCE 200809L
#include "../src/storage/lattice/persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int compare_uint64(const void* a, const void* b) {
    uint64_t da = *(const uint64_t*)a, db = *(const uint64_t*)b;
    return (da > db) - (da < db);
}

/* Histogram buckets: 0-50, 50-100, 100-200, 200-500, 500-1000, 1-5us, 5-10us, 10us+ */
static void print_histogram(uint64_t* times_ns, uint32_t count) {
    uint32_t b[8] = {0};
    for (uint32_t i = 0; i < count; i++) {
        double us = times_ns[i] / 1000.0;
        if (us < 0.05) b[0]++;
        else if (us < 0.1) b[1]++;
        else if (us < 0.2) b[2]++;
        else if (us < 0.5) b[3]++;
        else if (us < 1.0) b[4]++;
        else if (us < 5.0) b[5]++;
        else if (us < 10.0) b[6]++;
        else b[7]++;
    }
    printf("  Distribution:\n");
    printf("    <50 ns:    %5u (%.1f%%)\n", b[0], 100.0 * b[0] / count);
    printf("    50-100:   %5u (%.1f%%)\n", b[1], 100.0 * b[1] / count);
    printf("    100-200:  %5u (%.1f%%)\n", b[2], 100.0 * b[2] / count);
    printf("    200-500:  %5u (%.1f%%)\n", b[3], 100.0 * b[3] / count);
    printf("    500ns-1μs:%5u (%.1f%%)\n", b[4], 100.0 * b[4] / count);
    printf("    1-5 μs:   %5u (%.1f%%)\n", b[5], 100.0 * b[5] / count);
    printf("    5-10 μs:  %5u (%.1f%%)\n", b[6], 100.0 * b[6] / count);
    printf("    >10 μs:   %5u (%.1f%%)\n", b[7], 100.0 * b[7] / count);
}

static void run_test(const char* name, persistent_lattice_t* lattice,
                     uint32_t iterations, int test_o1) {
    printf("\n══════════════════════════════════════════════════════════╗\n");
    printf("  %s\n", name);
    printf("══════════════════════════════════════════════════════════╝\n\n");

    uint64_t* times = (uint64_t*)malloc(sizeof(uint64_t) * iterations);
    if (!times) { fprintf(stderr, "malloc failed\n"); return; }

    /* Find a prefix that exists (extended benchmark uses QDRANT_POINT:test_collection:, O1_TEST_NODE:) */
    const char* prefix = "QDRANT_POINT:test_collection:";
    uint64_t ids[64];
    uint32_t n = lattice_find_nodes_by_name(lattice, prefix, ids, 64);
    if (n == 0) {
        prefix = "O1_TEST_NODE:";
        n = lattice_find_nodes_by_name(lattice, prefix, ids, 64);
    }
    if (n == 0) {
        prefix = "LEARNING_";
        n = lattice_find_nodes_by_name(lattice, prefix, ids, 64);
    }
    if (n == 0) {
        printf("  No prefix matches - run extended benchmark first to seed lattice.\n");
        free(times);
        return;
    }

    /* Warmup */
    for (int i = 0; i < 100; i++) {
        if (test_o1) {
            lattice_node_t node;
            lattice_get_node_data(lattice, ids[i % n], &node);
        } else {
            lattice_find_nodes_by_name(lattice, prefix, ids, 64);
        }
    }

    uint32_t success = 0;
    for (uint32_t i = 0; i < iterations; i++) {
        uint64_t start = get_time_ns();
        if (test_o1) {
            lattice_node_t node;
            lattice_get_node_data(lattice, ids[i % n], &node);
        } else {
            lattice_find_nodes_by_name(lattice, prefix, ids, 64);
        }
        uint64_t elapsed = get_time_ns() - start;
        times[success++] = elapsed;
    }

    qsort(times, success, sizeof(uint64_t), compare_uint64);
    uint64_t sum = 0;
    for (uint32_t i = 0; i < success; i++) sum += times[i];

    printf("  Iterations: %u\n", success);
    printf("  Min:    %lu ns\n", (unsigned long)times[0]);
    printf("  Max:    %lu ns\n", (unsigned long)times[success - 1]);
    printf("  Avg:    %.2f ns\n", (double)sum / success);
    printf("  p50:    %lu ns\n", (unsigned long)times[success / 2]);
    printf("  p99:    %lu ns\n", (unsigned long)times[(uint32_t)(success * 0.99)]);
    print_histogram(times, success);
    free(times);
}

/* Create minimal lattice if path doesn't exist (for standalone runs) */
static int ensure_minimal_lattice(persistent_lattice_t* lattice, const char* path) {
    if (access(path, F_OK) == 0) return 0;  /* exists */
    printf("Creating minimal lattice at %s...\n", path);
    if (lattice_init(lattice, path, 10000, 0) != 0) return -1;
    lattice_disable_evaluation_mode(lattice);
    lattice_configure_persistence(lattice, false, 0, 0, false);
    for (int i = 0; i < 200; i++) {
        char name[64], data[64];
        snprintf(name, sizeof(name), "QDRANT_POINT:test_collection:%d", i);
        snprintf(data, sizeof(data), "data_%d", i);
        if (lattice_add_node(lattice, LATTICE_NODE_PATTERN, name, data, 0) == 0) break;
    }
    lattice_build_prefix_index(lattice);
    lattice_save(lattice);
    lattice_cleanup(lattice);
    return 0;
}

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "/tmp/query_latency_diagnostic.lattice";
    uint32_t iters = 1000;
    if (argc > 2) iters = (uint32_t)atoi(argv[2]);

    printf("Query Latency Diagnostic\n");
    printf("=======================\n");
    printf("Lattice: %s | Iterations: %u\n", path, iters);

    if (access(path, F_OK) != 0) {
        persistent_lattice_t tmp;
        memset(&tmp, 0, sizeof(tmp));
        if (ensure_minimal_lattice(&tmp, path) != 0) {
            fprintf(stderr, "Failed to create lattice. Run extended benchmark first:\n");
            fprintf(stderr, "  ./tools/run_extended_p99_benchmark.sh 1000 50000\n");
            return 1;
        }
    }

    persistent_lattice_t lattice;
    memset(&lattice, 0, sizeof(lattice));
    if (lattice_init(&lattice, path, 100000, 0) != 0) {
        fprintf(stderr, "Failed to init lattice: %s\n", path);
        return 1;
    }
    lattice_load(&lattice);

    run_test("A) lattice_find_nodes_by_name (prefix search = find_by_prefix)", &lattice, iters, 0);
    run_test("B) lattice_get_node_data (O(1) direct read)", &lattice, iters, 1);

    lattice_cleanup(&lattice);
    printf("\n");
    return 0;
}
