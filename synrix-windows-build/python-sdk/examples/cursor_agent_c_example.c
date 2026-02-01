/*
 * Cursor AI Agent - Direct C Usage of SYNRIX CLI
 * 
 * This shows how Cursor AI (if written in C/C++) can use SYNRIX directly
 * without any Python dependencies - just pure C.
 * 
 * Compile with:
 *   gcc -o cursor_synrix_example cursor_agent_c_example.c
 * 
 * Or link directly to SYNRIX Engine:
 *   gcc -o cursor_synrix_example cursor_agent_c_example.c \
 *       -I../src/storage/lattice \
 *       ../src/storage/lattice/persistent_lattice.c \
 *       ../src/storage/lattice/wal.c \
 *       ../src/storage/lattice/isolation.c \
 *       ../src/storage/lattice/seqlock.c \
 *       -lm -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

// Option 1: Call CLI binary via system/subprocess
void synrix_write_via_cli(const char* lattice_path, const char* key, const char* value) {
    char command[1024];
    snprintf(command, sizeof(command), 
             "./synrix_cli write %s \"%s\" \"%s\" 2>/dev/null | grep '^{'",
             lattice_path, key, value);
    
    FILE* fp = popen(command, "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == '{') {
                printf("Response: %s", line);
                break;
            }
        }
        pclose(fp);
    }
}

// Option 2: Direct library linkage (if Cursor AI is in C/C++)
// This would link directly to the SYNRIX Engine
#ifdef LINK_DIRECTLY_TO_SYNRIX_ENGINE
#include "../src/storage/lattice/persistent_lattice.h"

void synrix_write_direct(const char* lattice_path, const char* key, const char* value) {
    persistent_lattice_t lattice;
    
    // Initialize lattice
    if (lattice_init(&lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Failed to initialize lattice\n");
        return;
    }
    
    // Add node
    uint64_t node_id = lattice_add_node(
        &lattice,
        LATTICE_NODE_LEARNING,
        key,
        value,
        0  // parent_id
    );
    
    if (node_id == 0) {
        fprintf(stderr, "Failed to add node\n");
        lattice_cleanup(&lattice);
        return;
    }
    
    // Save
    if (lattice_save(&lattice) != 0) {
        fprintf(stderr, "Failed to save lattice\n");
        lattice_cleanup(&lattice);
        return;
    }
    
    printf("✓ Written node_id: %lu\n", node_id);
    
    lattice_cleanup(&lattice);
}

void synrix_read_direct(const char* lattice_path, const char* key) {
    persistent_lattice_t lattice;
    
    if (lattice_init(&lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Failed to initialize lattice\n");
        return;
    }
    
    // Search for node
    uint64_t node_ids[100];
    uint32_t count = lattice_find_nodes_by_name(&lattice, key, node_ids, 100);
    
    if (count == 0) {
        printf("Not found\n");
        lattice_cleanup(&lattice);
        return;
    }
    
    // Get node
    lattice_node_t node;
    if (lattice_get_node_data(&lattice, node_ids[0], &node) == 0) {
        printf("✓ Found: %s = %s\n", node.name, node.data);
    }
    
    lattice_cleanup(&lattice);
}

void synrix_get_direct(const char* lattice_path, uint64_t node_id) {
    persistent_lattice_t lattice;
    
    if (lattice_init(&lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Failed to initialize lattice\n");
        return;
    }
    
    // O(1) direct lookup
    lattice_node_t node;
    if (lattice_get_node_data(&lattice, node_id, &node) == 0) {
        printf("✓ Found: %s = %s\n", node.name, node.data);
    } else {
        printf("Not found\n");
    }
    
    lattice_cleanup(&lattice);
}
#endif

int main(int argc, char** argv) {
    const char* lattice_path = "~/.cursor/synrix_memory.lattice";
    
    printf("Cursor AI Agent - Direct C Usage of SYNRIX\n");
    printf("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    
    // Option 1: Use CLI binary (works from any language)
    printf("\n1. Using CLI binary (subprocess):\n");
    synrix_write_via_cli(lattice_path, "pattern:c:memory", "Use malloc/free for dynamic allocation");
    
#ifdef LINK_DIRECTLY_TO_SYNRIX_ENGINE
    // Option 2: Direct library linkage (if Cursor AI is in C/C++)
    printf("\n2. Direct library linkage:\n");
    synrix_write_direct(lattice_path, "pattern:c:error", "Check return values");
    synrix_read_direct(lattice_path, "pattern:c:error");
    synrix_get_direct(lattice_path, 12345);
#endif
    
    printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("Cursor AI can use SYNRIX in pure C!\n");
    
    return 0;
}

