#define _GNU_SOURCE
#include "seqlock.h"
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>

// Initialize seqlock
void seqlock_init(seqlock_t* seqlock) {
    if (!seqlock) return;
    atomic_store(&seqlock->sequence, 0);
}

// Read lock (lock-free, retry if writer active)
// Returns: 0 on success, -1 on error
int seqlock_read_lock(seqlock_t* seqlock, uint64_t* snapshot) {
    if (!seqlock) return -1;
    
    // Retry loop: wait for writer to finish (sequence must be even)
    uint64_t seq;
    int retries = 0;
    const int MAX_RETRIES = 1000;  // Prevent infinite loops
    
    do {
        seq = atomic_load_explicit(&seqlock->sequence, memory_order_acquire);
        
        // If sequence is odd, writer is active - wait and retry
        if (seq & 1) {
            retries++;
            if (retries >= MAX_RETRIES) {
                // Too many retries - fallback or error
                return -1;
            }
            // Spin-wait (CPU yield to avoid busy-wait)
            // ARM-compatible: use yield instruction or memory barrier
            #if defined(__aarch64__) || defined(__arm__)
                __asm__ __volatile__("yield" ::: "memory");
            #elif defined(__x86_64__) || defined(__i386__)
                __asm__ __volatile__("pause" ::: "memory");
            #else
                // Portable: memory barrier
                __atomic_thread_fence(__ATOMIC_ACQUIRE);
            #endif
            continue;
        }
        
        // Sequence is even - no writer active
        break;
    } while (1);
    
    // Store snapshot sequence
    if (snapshot) {
        *snapshot = seq;
    }
    
    return 0;
}

// Read unlock (validates sequence didn't change during read)
// Returns: 0 if read was consistent, -1 if writer modified data (should retry)
int seqlock_read_unlock(seqlock_t* seqlock, uint64_t snapshot) {
    if (!seqlock) return -1;
    
    // Check if sequence changed (writer modified data during read)
    uint64_t current_seq = atomic_load_explicit(&seqlock->sequence, memory_order_acquire);
    
    if (current_seq != snapshot) {
        // Sequence changed - writer modified data during read
        // Caller should retry the read operation
        return -1;
    }
    
    // Sequence unchanged - read was consistent
    return 0;
}

// Write lock (exclusive - makes sequence odd)
// Returns: 0 on success, -1 on error
int seqlock_write_lock(seqlock_t* seqlock) {
    if (!seqlock) return -1;
    
    // Wait for current sequence to be even (no active writer)
    uint64_t seq;
    int retries = 0;
    const int MAX_RETRIES = 10000;  // Increased for high contention
    
    do {
        seq = atomic_load_explicit(&seqlock->sequence, memory_order_acquire);
        
        // If sequence is odd, another writer is active - wait
        if (seq & 1) {
            retries++;
            if (retries >= MAX_RETRIES) {
                return -1;  // Timeout
            }
            // Exponential backoff for high contention
            if (retries > 100) {
                usleep(1);  // 1us sleep after 100 retries
            } else if (retries > 10) {
                // CPU yield (architecture-specific)
                #if defined(__aarch64__) || defined(__arm__)
                    __asm__ __volatile__("yield" ::: "memory");
                #elif defined(__x86_64__) || defined(__i386__)
                    __asm__ __volatile__("pause" ::: "memory");
                #else
                    __atomic_thread_fence(__ATOMIC_ACQUIRE);
                #endif
            }
            continue;
        }
        
        // Try to acquire lock (increment to odd)
        // Use compare-and-swap to ensure atomicity
        uint64_t expected = seq;
        if (atomic_compare_exchange_weak_explicit(&seqlock->sequence, &expected, seq + 1,
                                                  memory_order_acquire, memory_order_relaxed)) {
            // Successfully acquired lock (sequence is now odd)
            return 0;
        }
        
        // CAS failed - retry (another writer beat us)
        retries++;
        if (retries >= MAX_RETRIES) {
            return -1;
        }
    } while (1);
}

// Write unlock (makes sequence even, increments version)
// Returns: New sequence number (even)
uint64_t seqlock_write_unlock(seqlock_t* seqlock) {
    if (!seqlock) return 0;
    
    // Increment sequence (odd -> even, increments version)
    uint64_t new_seq = atomic_fetch_add_explicit(&seqlock->sequence, 1, memory_order_release) + 1;
    
    return new_seq;
}

// Get current sequence (for snapshot versioning)
uint64_t seqlock_get_sequence(seqlock_t* seqlock) {
    if (!seqlock) return 0;
    return atomic_load_explicit(&seqlock->sequence, memory_order_acquire);
}

