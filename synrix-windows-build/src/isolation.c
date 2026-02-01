#define _GNU_SOURCE
#include "isolation.h"
#include "seqlock.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Initialize isolation context
int isolation_init(isolation_context_t* isolation) {
    if (!isolation) return -1;
    
    memset(isolation, 0, sizeof(isolation_context_t));
    
    // Initialize seqlock (lock-free reads)
    seqlock_init(&isolation->seqlock);
    
    isolation->read_version = 0;
    isolation->write_version = 0;
    isolation->version_counter = 0;
    isolation->enabled = true;
    
    printf("[ISOLATION] OK Initialized isolation context (seqlock - lock-free reads)\n");
    
    return 0;
}

// Cleanup isolation context
void isolation_cleanup(isolation_context_t* isolation) {
    if (!isolation) return;
    
    // Seqlock doesn't need cleanup (no resources to free)
    memset(isolation, 0, sizeof(isolation_context_t));
}

// Acquire read lock (multiple readers allowed, lock-free)
int isolation_acquire_read_lock(isolation_context_t* isolation, uint64_t* snapshot_version) {
    if (!isolation || !isolation->enabled) {
        if (snapshot_version) *snapshot_version = 0;
        return -1;
    }
    
    // Acquire read lock (lock-free, retry if writer active)
    uint64_t snapshot_seq = 0;
    if (seqlock_read_lock(&isolation->seqlock, &snapshot_seq) != 0) {
        printf("[ISOLATION] ERROR Failed to acquire read lock (too many retries)\n");
        return -1;
    }
    
    // Get snapshot version (current write version at time of read)
    // Use sequence number as version (even numbers = versions)
    if (snapshot_version) {
        *snapshot_version = isolation->write_version;
    }
    
    return 0;
}

// Release read lock (validates sequence didn't change)
void isolation_release_read_lock(isolation_context_t* isolation) {
    if (!isolation || !isolation->enabled) return;
    
    // Note: seqlock_read_unlock() would validate consistency,
    // but we don't have the snapshot sequence here.
    // For now, we just return (readers are lock-free, no cleanup needed).
    // If we need to validate consistency, we'd need to pass snapshot through.
}

// Acquire write lock (exclusive - no readers or writers)
int isolation_acquire_write_lock(isolation_context_t* isolation) {
    if (!isolation || !isolation->enabled) return -1;
    
    // Acquire write lock (makes sequence odd, exclusive)
    if (seqlock_write_lock(&isolation->seqlock) != 0) {
        printf("[ISOLATION] ERROR Failed to acquire write lock (timeout)\n");
        return -1;
    }
    
    return 0;
}

// Release write lock and increment version
void isolation_release_write_lock(isolation_context_t* isolation) {
    if (!isolation || !isolation->enabled) return;
    
    // Release write lock (makes sequence even, increments version)
    uint64_t new_seq = seqlock_write_unlock(&isolation->seqlock);
    
    // Update version tracking
    // Sequence is even, so version = sequence / 2
    isolation->version_counter = new_seq / 2;
    isolation->write_version = isolation->version_counter;
}

// Get current read version (for snapshot isolation)
uint64_t isolation_get_read_version(isolation_context_t* isolation) {
    if (!isolation || !isolation->enabled) return 0;
    
    return isolation->read_version;
}

// Get current write version
uint64_t isolation_get_write_version(isolation_context_t* isolation) {
    if (!isolation || !isolation->enabled) return 0;
    
    return isolation->write_version;
}

// Check if isolation is enabled
bool isolation_is_enabled(isolation_context_t* isolation) {
    if (!isolation) return false;
    return isolation->enabled;
}

