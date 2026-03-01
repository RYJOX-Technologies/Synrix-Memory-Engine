#ifndef ISOLATION_H
#define ISOLATION_H

#include <stdint.h>
#include <stdbool.h>
#include "seqlock.h"

#ifdef __cplusplus
extern "C" {
#endif

// Isolation layer for concurrent read/write safety
// Uses seqlock for lock-free reads (replaces pthread_rwlock)
struct isolation_context {
    seqlock_t seqlock;            // Sequence lock (lock-free reads)
    uint64_t read_version;       // Snapshot version for readers
    uint64_t write_version;      // Current write version
    uint64_t version_counter;    // Monotonic version counter
    bool enabled;               // Isolation enabled flag
};
typedef struct isolation_context isolation_context_t;

// Initialize isolation context
int isolation_init(isolation_context_t* isolation);

// Cleanup isolation context
void isolation_cleanup(isolation_context_t* isolation);

// Acquire read lock (multiple readers allowed)
int isolation_acquire_read_lock(isolation_context_t* isolation, uint64_t* snapshot_version);

// Release read lock
void isolation_release_read_lock(isolation_context_t* isolation);

// Acquire write lock (exclusive - no readers or writers)
int isolation_acquire_write_lock(isolation_context_t* isolation);

// Release write lock and increment version
void isolation_release_write_lock(isolation_context_t* isolation);

// Get current read version (for snapshot isolation)
uint64_t isolation_get_read_version(isolation_context_t* isolation);

// Get current write version
uint64_t isolation_get_write_version(isolation_context_t* isolation);

// Check if isolation is enabled
bool isolation_is_enabled(isolation_context_t* isolation);

#ifdef __cplusplus
}
#endif

#endif // ISOLATION_H

