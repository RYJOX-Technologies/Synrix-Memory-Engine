#ifndef WAL_H
#define WAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>  // for off_t
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// WAL operation types
typedef enum {
    WAL_OP_ADD_NODE = 1,
    WAL_OP_UPDATE_NODE = 2,
    WAL_OP_DELETE_NODE = 3,
    WAL_OP_ADD_CHILD = 4,
    WAL_OP_CHECKPOINT = 5
} wal_operation_t;

// WAL entry structure
// Format: sequence (8 bytes) | operation (4 bytes) | node_id (8 bytes) | data_size (4 bytes) | data (variable)
typedef struct {
    uint64_t sequence;      // Monotonic sequence number
    wal_operation_t operation;  // Operation type
    uint64_t node_id;       // Node ID (64-bit: device_id << 32 | local_id)
    uint32_t data_size;     // Size of variable-length data
    // Variable-length data follows (node name, data, etc.)
} wal_entry_header_t;

// WAL context
struct wal_context {
    int wal_fd;             // File descriptor for WAL file
    char wal_path[512];     // Path to WAL file
    uint64_t sequence;      // Current sequence number
    uint64_t checkpoint_sequence;  // Last checkpointed sequence
    uint32_t entries_since_checkpoint;  // Entries since last checkpoint
    bool enabled;           // WAL enabled flag
    
    // Batching support (optimization)
    uint8_t* write_buffer;  // Write buffer for batching
    size_t write_buffer_size;  // Total buffer size
    size_t write_buffer_pos;   // Current position in buffer
    uint32_t batch_size;      // Max entries per batch (0 = no batching)
    uint32_t batch_count;     // Current entries in batch
    off_t file_pos;           // Current file position (avoids lseek)
    
    // Adaptive batching (Phase 4: Performance optimization)
    uint32_t min_batch_size;      // Minimum batch size (1K)
    uint32_t max_batch_size;      // Maximum batch size (100K)
    uint64_t last_adjust_time;    // Last time batch size was adjusted (ms)
    uint32_t adjustment_interval; // Adjust every N ms (1000ms = 1 second)
    uint32_t write_rate;          // Writes per second (rolling average)
    uint32_t write_count_window;  // Writes in current window
    uint64_t window_start_time;   // Start time of current window (ms)
    
    // Background flush thread (Phase 2: Performance optimization)
    pthread_t flush_thread;      // Background flush thread
    bool flush_thread_running;   // Thread running flag
    bool flush_thread_stop;      // Stop thread flag
    pthread_mutex_t flush_mutex; // Mutex for flush synchronization
    pthread_cond_t flush_cond;   // Condition variable for flush signal
    bool flush_requested;        // Flush requested flag
    bool flush_in_progress;      // Flush in progress flag
    uint64_t flush_sequence;     // Sequence number of last flushed entry
    int flush_error;             // Error code from flush thread (0 = success)
};
typedef struct wal_context wal_context_t;

// WAL functions
// Initialize WAL
int wal_init(wal_context_t* wal, const char* storage_path);

// Cleanup WAL
void wal_cleanup(wal_context_t* wal);

// Append entry to WAL (returns sequence number)
uint64_t wal_append(wal_context_t* wal, wal_operation_t operation, 
                    uint64_t node_id, const void* data, uint32_t data_size);

// Append ADD_NODE operation
uint64_t wal_append_add_node(wal_context_t* wal, uint64_t node_id,
                            uint8_t node_type, const char* name, 
                            const char* node_data, uint64_t parent_id);

// Append UPDATE_NODE operation
uint64_t wal_append_update_node(wal_context_t* wal, uint64_t node_id,
                               const char* new_data);

// Append DELETE_NODE operation
uint64_t wal_append_delete_node(wal_context_t* wal, uint64_t node_id);

// Append ADD_CHILD operation
uint64_t wal_append_add_child(wal_context_t* wal, uint64_t parent_id,
                             uint64_t child_id);

// Checkpoint WAL (mark entries as applied, can truncate)
int wal_checkpoint(wal_context_t* wal);

// Recover from WAL (replay entries)
int wal_recover(wal_context_t* wal, 
                int (*apply_add_node)(void* ctx, uint64_t node_id, uint8_t type,
                                     const char* name, const char* data, uint64_t parent_id),
                int (*apply_update_node)(void* ctx, uint64_t node_id, const char* data),
                int (*apply_delete_node)(void* ctx, uint64_t node_id),
                int (*apply_add_child)(void* ctx, uint64_t parent_id, uint64_t child_id),
                void* apply_ctx);

// Truncate WAL (remove checkpointed entries)
int wal_truncate(wal_context_t* wal);

// Get WAL statistics
void wal_get_stats(wal_context_t* wal, uint64_t* total_entries, 
                  uint64_t* checkpointed_entries, uint64_t* pending_entries);

// Flush WAL buffer to disk (non-blocking - signals background thread)
int wal_flush(wal_context_t* wal);

// Wait for flush to complete (for durability guarantees)
int wal_flush_wait(wal_context_t* wal, uint64_t sequence);

// Enable/configure WAL batching (0 = disable, N = batch size)
int wal_set_batch_size(wal_context_t* wal, uint32_t batch_size);

// Enable adaptive batching (Phase 4: Dynamic batch sizing)
int wal_enable_adaptive_batching(wal_context_t* wal, uint32_t min_batch, uint32_t max_batch);

// Adjust batch size based on write rate (called periodically)
void wal_adjust_batch_size(wal_context_t* wal);

#ifdef __cplusplus
}
#endif

#endif // WAL_H

