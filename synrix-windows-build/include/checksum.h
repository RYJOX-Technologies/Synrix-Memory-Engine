#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <stdbool.h>
#include "persistent_lattice.h"

#ifdef __cplusplus
extern "C" {
#endif

// Optional CRC32 checksum support (disabled by default for performance)
// Checksums stored in separate file, doesn't affect node access performance

// Checksum context
typedef struct {
    persistent_lattice_t* lattice;
    char checksum_file_path[512];
    uint32_t* checksums;          // Array of checksums (indexed by local_id)
    uint32_t checksum_capacity;   // Capacity of checksum array
    bool enabled;                 // false by default
    int checksum_fd;              // File descriptor for checksum file
} checksum_context_t;

// Initialize checksum support (disabled by default)
// Returns: 0 on success, -1 on error
int checksum_init(checksum_context_t* ctx, 
                  persistent_lattice_t* lattice,
                  const char* checksum_file_path);

// Enable/disable checksums (disabled by default)
// Returns: 0 on success, -1 on error
int checksum_enable(checksum_context_t* ctx, bool enable);

// Cleanup checksum context
void checksum_cleanup(checksum_context_t* ctx);

// Calculate CRC32 checksum for node data
// Returns: checksum value
uint32_t checksum_calculate(const void* data, size_t data_len);

// Store checksum for node (called after node write)
// Returns: 0 on success, -1 on error
int checksum_store(checksum_context_t* ctx, uint64_t node_id, uint32_t checksum);

// Verify checksum for node (called before node read)
// Returns: 0 if valid, -1 if mismatch
int checksum_verify(checksum_context_t* ctx, uint64_t node_id, uint32_t expected_checksum);

// Load checksums from file
// Returns: 0 on success, -1 on error
int checksum_load(checksum_context_t* ctx);

// Save checksums to file
// Returns: 0 on success, -1 on error
int checksum_save(checksum_context_t* ctx);

#ifdef __cplusplus
}
#endif

#endif // CHECKSUM_H

