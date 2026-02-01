#ifndef SEQLOCK_H
#define SEQLOCK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

// Seqlock (Sequence Lock) for lock-free reads
// 
// Design:
// - Sequence counter: Even = no writer, odd = writer active
// - Readers: Check sequence before/after read, retry if changed
// - Writers: Increment to odd (acquire), increment to even (release)
//
// Advantages:
// - Readers are completely lock-free (no syscalls)
// - Multiple concurrent readers (no blocking)
// - Writers are exclusive (ACID guarantees)
// - Fast (atomic operations, no kernel calls)

typedef struct {
    atomic_uint_fast64_t sequence;  // Sequence counter (even = no writer, odd = writer active)
} seqlock_t;

// Initialize seqlock
void seqlock_init(seqlock_t* seqlock);

// Read lock (lock-free, retry if writer active)
// Returns: 0 on success, -1 on error
// snapshot: Output parameter for snapshot sequence number
int seqlock_read_lock(seqlock_t* seqlock, uint64_t* snapshot);

// Read unlock (no-op for seqlock, but validates sequence)
// snapshot: Snapshot sequence from read_lock
// Returns: 0 if read was consistent, -1 if writer modified data (should retry)
int seqlock_read_unlock(seqlock_t* seqlock, uint64_t snapshot);

// Write lock (exclusive - makes sequence odd)
// Returns: 0 on success, -1 on error
int seqlock_write_lock(seqlock_t* seqlock);

// Write unlock (makes sequence even, increments version)
// Returns: New sequence number (even)
uint64_t seqlock_write_unlock(seqlock_t* seqlock);

// Get current sequence (for snapshot versioning)
uint64_t seqlock_get_sequence(seqlock_t* seqlock);

#ifdef __cplusplus
}
#endif

#endif // SEQLOCK_H

