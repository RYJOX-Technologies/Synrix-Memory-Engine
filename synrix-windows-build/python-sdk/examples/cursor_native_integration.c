/*
 * Cursor AI - Native SYNRIX Integration Example
 * 
 * This is how Cursor links to SYNRIX directly - pure C, zero overhead
 * Just #include and link - feels like native system calls
 */

#include "synrix_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Open SYNRIX (lattice stays in memory - zero overhead)
    synrix_t* synrix = synrix_open("~/.cursor/synrix_memory.lattice", 100000);
    if (!synrix) {
        fprintf(stderr, "Failed to open SYNRIX\n");
        return 1;
    }
    
    // Write - feels like a native call
    uint64_t node_id = synrix_write(synrix, "pattern:python:error", "Use try/except");
    printf("Written: node_id=%lu\n", node_id);
    
    // Read - direct memory access, no subprocess
    char value[1024];
    uint64_t found_id = synrix_read(synrix, "pattern:python:error", value, sizeof(value));
    if (found_id) {
        printf("Read: %s\n", value);
    }
    
    // O(1) lookup - sub-microsecond
    char key[512];
    char val[1024];
    if (synrix_get(synrix, node_id, key, sizeof(key), val, sizeof(val))) {
        printf("Get: %s = %s\n", key, val);
    }
    
    // Search - O(k) semantic query
    uint64_t node_ids[100];
    char* keys[100];
    char* values[100];
    
    // Allocate result buffers
    for (int i = 0; i < 100; i++) {
        keys[i] = malloc(512);
        values[i] = malloc(1024);
    }
    
    uint32_t count = synrix_search(synrix, "pattern:", 100, node_ids, keys, values);
    printf("Search found %u results\n", count);
    
    // Cleanup
    for (int i = 0; i < 100; i++) {
        free(keys[i]);
        free(values[i]);
    }
    
    synrix_close(synrix);
    
    return 0;
}

/*
 * Compile with:
 *   gcc -o cursor_synrix cursor_native_integration.c -L. -lsynrix -I../src/cli
 * 
 * Or link statically:
 *   gcc -o cursor_synrix cursor_native_integration.c libsynrix.a -I../src/cli
 * 
 * For Cursor: Just add to build system:
 *   - Include: synrix_api.h
 *   - Link: -lsynrix (or libsynrix.a)
 *   - That's it - zero overhead, native calls
 */

