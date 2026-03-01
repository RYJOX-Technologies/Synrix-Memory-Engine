#define _GNU_SOURCE
#include "../src/storage/lattice/persistent_lattice.h"
#include "../src/storage/lattice/wal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>

// Test configuration
#define TEST_LATTICE_PATH "/tmp/wal_test.lattice"
#define TEST_NODES_COUNT 1000

// Test 1: Basic WAL functionality
static int test_basic_wal(void) {
    printf("\n========================================\n");
    printf("TEST 1: Basic WAL Functionality\n");
    printf("========================================\n\n");
    
    // Remove old test files
    unlink(TEST_LATTICE_PATH);
    unlink(TEST_LATTICE_PATH ".wal");
    
    // Initialize lattice
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, TEST_LATTICE_PATH, 10000) != 0) {
        printf("❌ Failed to initialize lattice\n");
        return -1;
    }
    
    // Enable WAL
    if (lattice_enable_wal(&lattice) != 0) {
        printf("❌ Failed to enable WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    printf("✅ WAL enabled\n");
    
    // Add nodes with WAL
    printf("Adding %d nodes with WAL...\n", TEST_NODES_COUNT);
    for (uint32_t i = 0; i < TEST_NODES_COUNT; i++) {
        char name[64];
        char data[128];
        snprintf(name, sizeof(name), "test_node_%u", i);
        snprintf(data, sizeof(data), "test_data_%u", i);
        
        uint32_t node_id = lattice_add_node_with_wal(&lattice, 
                                                     LATTICE_NODE_PRIMITIVE,
                                                     name, data, 0);
        if (node_id == 0) {
            printf("❌ Failed to add node %u\n", i);
            lattice_cleanup(&lattice);
            return -1;
        }
        
        if ((i + 1) % 100 == 0) {
            printf("  Added %u nodes...\n", i + 1);
        }
    }
    
    printf("✅ Added %d nodes with WAL\n", TEST_NODES_COUNT);
    
    // Checkpoint WAL
    printf("Checkpointing WAL...\n");
    if (lattice_wal_checkpoint(&lattice) != 0) {
        printf("❌ Failed to checkpoint WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    printf("✅ WAL checkpointed\n");
    
    // Save lattice
    if (lattice_save(&lattice) != 0) {
        printf("❌ Failed to save lattice\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    printf("✅ Lattice saved\n");
    printf("Total nodes: %u\n", lattice.total_nodes);
    
    lattice_cleanup(&lattice);
    
    printf("\n✅ TEST 1 PASSED\n");
    return 0;
}

// Test 2: WAL Recovery
static int test_wal_recovery(void) {
    printf("\n========================================\n");
    printf("TEST 2: WAL Recovery\n");
    printf("========================================\n\n");
    
    // Initialize lattice (should load existing file)
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, TEST_LATTICE_PATH, 10000) != 0) {
        printf("❌ Failed to initialize lattice\n");
        return -1;
    }
    
    // Load existing lattice
    if (lattice_load(&lattice) != 0) {
        printf("❌ Failed to load lattice\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    printf("✅ Loaded lattice: %u nodes\n", lattice.total_nodes);
    
    // Enable WAL
    if (lattice_enable_wal(&lattice) != 0) {
        printf("❌ Failed to enable WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    // Recover from WAL
    printf("Recovering from WAL...\n");
    if (lattice_recover_from_wal(&lattice) != 0) {
        printf("❌ Failed to recover from WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    printf("✅ Recovery complete\n");
    printf("Total nodes after recovery: %u\n", lattice.total_nodes);
    
    // Verify nodes exist
    uint32_t verified = 0;
    for (uint32_t i = 0; i < TEST_NODES_COUNT; i++) {
        char expected_name[64];
        snprintf(expected_name, sizeof(expected_name), "test_node_%u", i);
        
        // Try to find node by name (simplified - in real test would use proper lookup)
        for (uint32_t j = 0; j < lattice.node_count; j++) {
            if (strcmp(lattice.nodes[j].name, expected_name) == 0) {
                verified++;
                break;
            }
        }
    }
    
    printf("Verified %u nodes exist\n", verified);
    
    if (verified < TEST_NODES_COUNT * 0.9) {
        printf("❌ Too few nodes verified (expected at least %u, got %u)\n",
               (uint32_t)(TEST_NODES_COUNT * 0.9), verified);
        lattice_cleanup(&lattice);
        return -1;
    }
    
    lattice_cleanup(&lattice);
    
    printf("\n✅ TEST 2 PASSED\n");
    return 0;
}

// Test 3: Crash Recovery (simulated)
static int test_crash_recovery(void) {
    printf("\n========================================\n");
    printf("TEST 3: Crash Recovery (Simulated)\n");
    printf("========================================\n\n");
    
    // Remove old test files
    unlink(TEST_LATTICE_PATH);
    unlink(TEST_LATTICE_PATH ".wal");
    
    // Initialize lattice
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, TEST_LATTICE_PATH, 10000) != 0) {
        printf("❌ Failed to initialize lattice\n");
        return -1;
    }
    
    // Enable WAL
    if (lattice_enable_wal(&lattice) != 0) {
        printf("❌ Failed to enable WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    // Add some nodes
    printf("Adding 500 nodes before crash simulation...\n");
    for (uint32_t i = 0; i < 500; i++) {
        char name[64];
        char data[128];
        snprintf(name, sizeof(name), "crash_test_node_%u", i);
        snprintf(data, sizeof(data), "crash_test_data_%u", i);
        
        uint32_t node_id = lattice_add_node_with_wal(&lattice,
                                                     LATTICE_NODE_PRIMITIVE,
                                                     name, data, 0);
        if (node_id == 0) {
            printf("❌ Failed to add node %u\n", i);
            lattice_cleanup(&lattice);
            return -1;
        }
    }
    
    printf("✅ Added 500 nodes\n");
    
    // Save lattice (simulate checkpoint)
    if (lattice_save(&lattice) != 0) {
        printf("❌ Failed to save lattice\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    // Checkpoint WAL
    if (lattice_wal_checkpoint(&lattice) != 0) {
        printf("❌ Failed to checkpoint WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    printf("✅ Checkpointed at 500 nodes\n");
    
    // Add more nodes (these will be in WAL but not checkpointed)
    printf("Adding 200 more nodes (will be in WAL, not checkpointed)...\n");
    for (uint32_t i = 500; i < 700; i++) {
        char name[64];
        char data[128];
        snprintf(name, sizeof(name), "crash_test_node_%u", i);
        snprintf(data, sizeof(data), "crash_test_data_%u", i);
        
        uint32_t node_id = lattice_add_node_with_wal(&lattice,
                                                     LATTICE_NODE_PRIMITIVE,
                                                     name, data, 0);
        if (node_id == 0) {
            printf("❌ Failed to add node %u\n", i);
            lattice_cleanup(&lattice);
            return -1;
        }
    }
    
    printf("✅ Added 200 more nodes (total: 700)\n");
    printf("Simulating crash (not saving lattice, WAL has uncheckpointed entries)...\n");
    
    // Don't save lattice - simulate crash
    // WAL should have entries for nodes 500-699
    
    lattice_cleanup(&lattice);
    
    // Now "recover" - initialize new lattice and recover from WAL
    printf("\nRecovering from crash...\n");
    
    persistent_lattice_t recovered_lattice;
    if (lattice_init(&recovered_lattice, TEST_LATTICE_PATH, 10000) != 0) {
        printf("❌ Failed to initialize recovered lattice\n");
        return -1;
    }
    
    // Load lattice (should have 500 nodes from checkpoint)
    if (lattice_load(&recovered_lattice) != 0) {
        printf("❌ Failed to load recovered lattice\n");
        lattice_cleanup(&recovered_lattice);
        return -1;
    }
    
    printf("✅ Loaded lattice: %u nodes (from checkpoint)\n", recovered_lattice.total_nodes);
    
    // Enable WAL and recover
    if (lattice_enable_wal(&recovered_lattice) != 0) {
        printf("❌ Failed to enable WAL on recovery\n");
        lattice_cleanup(&recovered_lattice);
        return -1;
    }
    
    // Recover from WAL (should replay nodes 500-699)
    printf("Recovering from WAL...\n");
    if (lattice_recover_from_wal(&recovered_lattice) != 0) {
        printf("❌ Failed to recover from WAL\n");
        lattice_cleanup(&recovered_lattice);
        return -1;
    }
    
    printf("✅ Recovery complete\n");
    printf("Total nodes after recovery: %u\n", recovered_lattice.total_nodes);
    
    // Verify we recovered the uncheckpointed nodes
    if (recovered_lattice.total_nodes < 700) {
        printf("❌ Expected at least 700 nodes after recovery, got %u\n",
               recovered_lattice.total_nodes);
        lattice_cleanup(&recovered_lattice);
        return -1;
    }
    
    printf("✅ Recovered uncheckpointed nodes (expected ~700, got %u)\n",
           recovered_lattice.total_nodes);
    
    lattice_cleanup(&recovered_lattice);
    
    printf("\n✅ TEST 3 PASSED\n");
    return 0;
}

// Test 4: WAL Statistics
static int test_wal_statistics(void) {
    printf("\n========================================\n");
    printf("TEST 4: WAL Statistics\n");
    printf("========================================\n\n");
    
    // Initialize lattice
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, TEST_LATTICE_PATH, 10000) != 0) {
        printf("❌ Failed to initialize lattice\n");
        return -1;
    }
    
    // Enable WAL
    if (lattice_enable_wal(&lattice) != 0) {
        printf("❌ Failed to enable WAL\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    // Add some nodes
    for (uint32_t i = 0; i < 100; i++) {
        char name[64];
        snprintf(name, sizeof(name), "stat_test_node_%u", i);
        lattice_add_node_with_wal(&lattice, LATTICE_NODE_PRIMITIVE, name, "", 0);
    }
    
    // Get WAL statistics
    uint64_t total_entries, checkpointed_entries, pending_entries;
    wal_get_stats(lattice.wal, &total_entries, &checkpointed_entries, &pending_entries);
    
    printf("WAL Statistics:\n");
    printf("  Total entries: %lu\n", total_entries);
    printf("  Checkpointed entries: %lu\n", checkpointed_entries);
    printf("  Pending entries: %lu\n", pending_entries);
    
    if (total_entries < 100) {
        printf("❌ Expected at least 100 entries, got %lu\n", total_entries);
        lattice_cleanup(&lattice);
        return -1;
    }
    
    // Checkpoint
    lattice_wal_checkpoint(&lattice);
    
    // Get updated statistics
    wal_get_stats(lattice.wal, &total_entries, &checkpointed_entries, &pending_entries);
    
    printf("\nAfter checkpoint:\n");
    printf("  Total entries: %lu\n", total_entries);
    printf("  Checkpointed entries: %lu\n", checkpointed_entries);
    printf("  Pending entries: %lu\n", pending_entries);
    
    if (checkpointed_entries != total_entries) {
        printf("❌ After checkpoint, checkpointed should equal total\n");
        lattice_cleanup(&lattice);
        return -1;
    }
    
    lattice_cleanup(&lattice);
    
    printf("\n✅ TEST 4 PASSED\n");
    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("========================================\n");
    printf("SYNRIX WAL Test Suite\n");
    printf("========================================\n");
    
    int tests_passed = 0;
    int tests_failed = 0;
    
    // Run tests
    if (test_basic_wal() == 0) {
        tests_passed++;
    } else {
        tests_failed++;
    }
    
    if (test_wal_recovery() == 0) {
        tests_passed++;
    } else {
        tests_failed++;
    }
    
    if (test_crash_recovery() == 0) {
        tests_passed++;
    } else {
        tests_failed++;
    }
    
    if (test_wal_statistics() == 0) {
        tests_passed++;
    } else {
        tests_failed++;
    }
    
    // Summary
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Total tests: %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✅ ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("\n❌ SOME TESTS FAILED\n");
        return 1;
    }
}

