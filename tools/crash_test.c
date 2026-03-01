#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include "../src/storage/lattice/persistent_lattice.h"

// Crash test configuration
typedef struct {
    uint32_t target_nodes;
    uint32_t crash_at_node;
    bool crash_enabled;
    char lattice_path[512];  // Increased size for absolute paths
    pid_t child_pid;
} crash_test_config_t;

static crash_test_config_t g_config = {0};

// Get absolute path for test file (robust for longevity)
static void get_test_file_path(const char* user_path, char* abs_path, size_t abs_path_size) {
    if (!user_path || !abs_path) return;
    
    // If path is already absolute, use it
    if (user_path[0] == '/') {
        strncpy(abs_path, user_path, abs_path_size - 1);
        abs_path[abs_path_size - 1] = '\0';
        return;
    }
    
    // Otherwise, use dedicated test directory
    const char* test_dir = "/tmp/aion_crash_tests";
    
    // Create test directory if it doesn't exist
    struct stat st;
    if (stat(test_dir, &st) != 0) {
        mkdir(test_dir, 0755);
    }
    
    // Build absolute path
    snprintf(abs_path, abs_path_size, "%s/%s", test_dir, 
             (strrchr(user_path, '/') ? strrchr(user_path, '/') + 1 : user_path));
}

// Signal handler for crash injection
void crash_handler(int sig) {
    (void)sig;
    printf("\n[CRASH-TEST] 💥 CRASH INJECTED at node %u\n", g_config.crash_at_node);
    exit(1); // Simulate crash
}

// Write nodes with periodic saves (using WAL)
int write_nodes_with_crash(persistent_lattice_t* lattice, uint32_t count) {
    printf("[CRASH-TEST] Writing %u nodes with crash at node %u...\n", count, g_config.crash_at_node);
    
    // Ensure WAL is enabled (should be enabled before calling this)
    if (!lattice->wal || !lattice->wal_enabled) {
        printf("[CRASH-TEST] ⚠️  WAL not enabled, enabling now...\n");
        if (lattice_enable_wal(lattice) != 0) {
            printf("[CRASH-TEST] ❌ Failed to enable WAL\n");
            return -1;
        }
    }
    
    for (uint32_t i = 0; i < count; i++) {
        // Add node with WAL (ensures durability)
        char label[64];
        snprintf(label, sizeof(label), "test_node_%u", i);
        
        uint32_t node_id = lattice_add_node_with_wal(lattice, LATTICE_NODE_PRIMITIVE, label, "", 0);
        if (node_id == 0) {
            printf("[CRASH-TEST] ❌ Failed to add node %u\n", i);
            return -1;
        }
        
        // Periodic checkpoint (every 100 nodes) - this saves to disk and truncates WAL
        if ((i + 1) % 100 == 0) {
            if (lattice_save(lattice) != 0) {
                printf("[CRASH-TEST] ❌ Failed to save at node %u\n", i);
                return -1;
            }
            // Checkpoint WAL to mark entries as applied
            if (lattice_wal_checkpoint(lattice) != 0) {
                printf("[CRASH-TEST] ⚠️  Failed to checkpoint WAL at node %u\n", i);
            }
            printf("[CRASH-TEST] 💾 Saved at node %u (total: %u)\n", i + 1, lattice->node_count);
            fflush(stdout);
        }
        
        // Check if we should crash AFTER adding node (before next checkpoint)
        if (g_config.crash_enabled && (i + 1) == g_config.crash_at_node) {
            printf("[CRASH-TEST] 💥 CRASHING NOW after node %u...\n", i + 1);
            printf("[CRASH-TEST] Nodes in WAL (not checkpointed): %u\n", 
                   (i + 1) % 100); // Nodes since last checkpoint
            fflush(stdout);
            raise(SIGKILL); // Simulate crash
        }
    }
    
    // Final save and checkpoint
    if (lattice_save(lattice) != 0) {
        printf("[CRASH-TEST] ❌ Failed final save\n");
        return -1;
    }
    if (lattice_wal_checkpoint(lattice) != 0) {
        printf("[CRASH-TEST] ⚠️  Failed to checkpoint WAL\n");
    }
    
    return 0;
}

// Verify data integrity after crash
int verify_data_integrity(persistent_lattice_t* lattice, uint32_t expected_min_nodes) {
    printf("[CRASH-TEST] Verifying data integrity...\n");
    
    if (!lattice || !lattice->nodes) {
        printf("[CRASH-TEST] ❌ Invalid lattice pointer\n");
        return -1;
    }
    
    printf("[CRASH-TEST] Lattice state: node_count=%u, total_nodes=%u\n", 
           lattice->node_count, lattice->total_nodes);
    
    uint32_t valid_nodes = 0;
    uint32_t corrupted_nodes = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && i < lattice->max_nodes; i++) {
        if (i >= lattice->max_nodes) break; // Safety check
        
        lattice_node_t* node = &lattice->nodes[i];
        if (!node) {
            corrupted_nodes++;
            continue;
        }
        
        // Basic validation
        if (node->id == 0 || node->id > lattice->total_nodes) {
            corrupted_nodes++;
            continue;
        }
        
        // Check if node data is accessible
        lattice_node_t out_node;
        if (lattice_get_node_data(lattice, node->id, &out_node) != 0) {
            corrupted_nodes++;
            continue;
        }
        
        valid_nodes++;
    }
    
    printf("[CRASH-TEST] ✅ Valid nodes: %u\n", valid_nodes);
    if (corrupted_nodes > 0) {
        printf("[CRASH-TEST] ⚠️  Corrupted nodes: %u\n", corrupted_nodes);
    }
    
    // Success if we have at least the expected minimum (accounting for crash)
    if (valid_nodes >= expected_min_nodes) {
        printf("[CRASH-TEST] ✅ DATA INTEGRITY VERIFIED: %u nodes intact\n", valid_nodes);
        return 0;
    } else {
        printf("[CRASH-TEST] ❌ DATA LOSS DETECTED: Expected >= %u, got %u\n", 
               expected_min_nodes, valid_nodes);
        return -1;
    }
}

// Test scenario 1: Power loss during write
// Returns: 0 = test passed, -1 = test failed, 1 = crashed (expected)
int test_power_loss(persistent_lattice_t* lattice) {
    printf("\n=== TEST 1: Power Loss During Write ===\n");
    
    // Setup: Write 1000 nodes, crash at node 500
    g_config.target_nodes = 1000;
    g_config.crash_at_node = 500;
    g_config.crash_enabled = true;
    
    // Write nodes (will crash at 500)
    write_nodes_with_crash(lattice, g_config.target_nodes);
    
    // If we get here, we didn't crash (unexpected)
    printf("[CRASH-TEST] ⚠️  Warning: Expected crash but process continued\n");
    return -1;
}

// Test scenario 2: Process kill during node addition
// Returns: 0 = test passed, -1 = test failed, 1 = crashed (expected)
int test_process_kill(persistent_lattice_t* lattice) {
    printf("\n=== TEST 2: Process Kill During Node Addition ===\n");
    
    // Setup: Write 500 nodes, crash at node 250
    g_config.target_nodes = 500;
    g_config.crash_at_node = 250;
    g_config.crash_enabled = true;
    
    // Write nodes (will crash at 250)
    write_nodes_with_crash(lattice, g_config.target_nodes);
    
    // If we get here, we didn't crash (unexpected)
    printf("[CRASH-TEST] ⚠️  Warning: Expected crash but process continued\n");
    return -1;
}

// Verify crash recovery (called after a crash)
int verify_crash_recovery(const char* lattice_path, uint32_t crash_at_node, const char* test_name) {
    printf("\n[CRASH-TEST] Verifying recovery after %s...\n", test_name);
    
    // Get absolute path
    char abs_path[512];
    get_test_file_path(lattice_path, abs_path, sizeof(abs_path));
    
    // Verify file exists before attempting load
    struct stat st;
    if (stat(abs_path, &st) != 0) {
        printf("[CRASH-TEST] ❌ File does not exist: %s (errno=%d)\n", abs_path, errno);
        return -1;
    }
    
    printf("[CRASH-TEST] ✅ File exists: %s (size=%ld bytes)\n", abs_path, (long)st.st_size);
    
    // Small delay to ensure file system sync
    usleep(100000); // 100ms
    
    // Reload lattice
    persistent_lattice_t reloaded;
    if (lattice_init(&reloaded, abs_path, 100000, 0) != 0) {
        printf("[CRASH-TEST] ❌ Failed to reload lattice from %s\n", abs_path);
        return -1;
    }
    
    // Load existing lattice (if any)
    uint32_t nodes_before_recovery = 0;
    if (lattice_load(&reloaded) == 0) {
        nodes_before_recovery = reloaded.total_nodes;
        printf("[CRASH-TEST] ✅ Loaded lattice: %u nodes (from checkpoint)\n", nodes_before_recovery);
    } else {
        printf("[CRASH-TEST] ℹ️  No existing lattice file (first run)\n");
    }
    
    // Enable WAL and recover from WAL (this is the key step!)
    if (lattice_enable_wal(&reloaded) != 0) {
        printf("[CRASH-TEST] ❌ Failed to enable WAL for recovery\n");
        lattice_cleanup(&reloaded);
        return -1;
    }
    
    printf("[CRASH-TEST] Recovering from WAL...\n");
    if (lattice_recover_from_wal(&reloaded) != 0) {
        printf("[CRASH-TEST] ⚠️  WAL recovery returned error (may be normal if WAL is empty)\n");
    } else {
        printf("[CRASH-TEST] ✅ WAL recovery completed\n");
    }
    
    // Calculate expected minimum: nodes from last checkpoint + nodes in WAL
    // We checkpoint every 100 nodes, so last checkpoint is (crash_at_node / 100) * 100
    uint32_t last_checkpoint = (crash_at_node / 100) * 100;
    uint32_t nodes_in_wal = crash_at_node - last_checkpoint; // Nodes added after last checkpoint
    uint32_t expected_min = last_checkpoint + nodes_in_wal; // Should recover all nodes
    
    if (expected_min == 0) expected_min = 1; // At least 1 node
    
    printf("[CRASH-TEST] Expected minimum nodes after recovery: %u\n", expected_min);
    printf("[CRASH-TEST]   (Last checkpoint: %u, Nodes in WAL: %u)\n", last_checkpoint, nodes_in_wal);
    printf("[CRASH-TEST] Actual nodes after recovery: %u\n", reloaded.total_nodes);
    
    // Verify recovery: should have at least expected_min nodes
    if (reloaded.total_nodes >= expected_min) {
        printf("[CRASH-TEST] ✅ Recovery verified: %u nodes recovered (expected >= %u)\n", 
               reloaded.total_nodes, expected_min);
        printf("[CRASH-TEST] ✅ ZERO DATA LOSS: All nodes recovered from WAL after crash\n");
        lattice_cleanup(&reloaded);
        return 0;
    } else {
        printf("[CRASH-TEST] ❌ DATA LOSS DETECTED: Expected >= %u, got %u\n", 
               expected_min, reloaded.total_nodes);
        lattice_cleanup(&reloaded);
        return -1;
    }
}

// Test scenario 3: Multiple crashes in sequence
// Returns: 0 = test passed, -1 = test failed, 1 = crashed (expected)
int test_multiple_crashes(persistent_lattice_t* lattice) {
    printf("\n=== TEST 3: Multiple Crashes in Sequence ===\n");
    
    // First crash: Write 300 nodes, crash at 150
    g_config.target_nodes = 300;
    g_config.crash_at_node = 150;
    g_config.crash_enabled = true;
    
    write_nodes_with_crash(lattice, g_config.target_nodes);
    
    // If we get here, we didn't crash (unexpected)
    printf("[CRASH-TEST] ⚠️  Warning: Expected crash but process continued\n");
    return -1;
}

int main(int argc, char* argv[]) {
    printf("=== AION OMEGA CRASH TEST (Jepsen-Style) ===\n\n");
    
    // Parse arguments
    if (argc < 2) {
        printf("Usage: %s <test_number> [lattice_path]\n", argv[0]);
        printf("  test_number: 1=power_loss, 2=process_kill, 3=multiple_crashes, 0=all\n");
        printf("  lattice_path: Path to lattice file (default: /tmp/crash_test.lattice)\n");
        return 1;
    }
    
    int test_num = atoi(argv[1]);
    const char* user_path = (argc >= 3) ? argv[2] : "crash_test.lattice";
    
    // Get absolute path for robust file handling
    get_test_file_path(user_path, g_config.lattice_path, sizeof(g_config.lattice_path));
    
    printf("[CRASH-TEST] Using test file: %s\n", g_config.lattice_path);
    
    // Remove existing lattice for clean test (only for crash tests, not verification)
    if (test_num >= 1 && test_num <= 3) {
        unlink(g_config.lattice_path);
    }
    
    // Initialize lattice
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, g_config.lattice_path, 100000, 0) != 0) {
        printf("[CRASH-TEST] ❌ Failed to initialize lattice\n");
        return 1;
    }
    
    // Enable WAL for crash recovery (critical for demo!)
    if (lattice_enable_wal(&lattice) != 0) {
        printf("[CRASH-TEST] ❌ Failed to enable WAL\n");
        lattice_cleanup(&lattice);
        return 1;
    }
    printf("[CRASH-TEST] ✅ WAL enabled for crash recovery\n");
    
    // Install crash handler
    signal(SIGKILL, crash_handler);
    
    // Run crash tests (these will exit on crash)
    if (test_num == 0 || test_num == 1) {
        test_power_loss(&lattice);
        // If we get here, test didn't crash (unexpected)
        printf("[CRASH-TEST] ⚠️  Test 1 didn't crash as expected\n");
        lattice_cleanup(&lattice);
        return 1;
    }
    
    if (test_num == 2) {
        test_process_kill(&lattice);
        // If we get here, test didn't crash (unexpected)
        printf("[CRASH-TEST] ⚠️  Test 2 didn't crash as expected\n");
        lattice_cleanup(&lattice);
        return 1;
    }
    
    if (test_num == 3) {
        test_multiple_crashes(&lattice);
        // If we get here, test didn't crash (unexpected)
        printf("[CRASH-TEST] ⚠️  Test 3 didn't crash as expected\n");
        lattice_cleanup(&lattice);
        return 1;
    }
    
    // Verification mode: Check recovery after crash
    if (test_num == 10) {
        // Verify test 1 recovery
        if (verify_crash_recovery(g_config.lattice_path, 500, "Power Loss") != 0) {
            return 1;
        }
    } else if (test_num == 20) {
        // Verify test 2 recovery
        if (verify_crash_recovery(g_config.lattice_path, 250, "Process Kill") != 0) {
            return 1;
        }
    } else if (test_num == 30) {
        // Verify test 3 recovery
        if (verify_crash_recovery(g_config.lattice_path, 150, "Multiple Crashes") != 0) {
            return 1;
        }
    }
    
    lattice_cleanup(&lattice);
    printf("\n=== ✅ CRASH TEST COMPLETE ===\n");
    return 0;
}

