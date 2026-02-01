#define _GNU_SOURCE
#include "wal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/time.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>  // For _O_RDWR, _O_BINARY
#include <io.h>      // For _open_osfhandle
#endif

static int wal_verbose_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char* env = getenv("SYNRIX_WAL_VERBOSE");
        cached = (env && env[0] && strcmp(env, "0") != 0) ? 1 : 0;
    }
    return cached;
}

#define WAL_LOG_INFO(...) do { if (wal_verbose_enabled()) printf(__VA_ARGS__); } while(0)

// WAL magic number
#define WAL_MAGIC 0x57414C20  // "WAL " in ASCII

// WAL file header (State Ledger - tracks committed entries)
typedef struct {
    uint32_t magic;          // WAL_MAGIC
    uint32_t version;        // WAL format version
    uint64_t sequence;       // Current sequence number (total entries written)
    uint64_t checkpoint_sequence;  // Last checkpointed sequence
    uint64_t commit_count;   // Number of entries committed to disk (State Ledger)
    uint64_t last_valid_offset;  // Offset of last valid entry (for recovery)
} wal_file_header_t;

// Initialize WAL
int wal_init(wal_context_t* wal, const char* storage_path) {
    if (!wal || !storage_path) return -1;
    
    memset(wal, 0, sizeof(wal_context_t));
    
    // Build WAL file path (storage_path + ".wal")
    snprintf(wal->wal_path, sizeof(wal->wal_path), "%s.wal", storage_path);
    
    // Open or create WAL file
    #ifdef _WIN32
        // Windows: Use CreateFile with FILE_FLAG_WRITE_THROUGH for immediate durability
        // This bypasses Windows write cache, ensuring data is on disk immediately
        HANDLE hFile = CreateFileA(
            wal->wal_path,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_FLAG_WRITE_THROUGH | FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (hFile == INVALID_HANDLE_VALUE) {
            printf("[WAL] ERROR Failed to open WAL file: %s (error: %lu)\n", 
                   wal->wal_path, GetLastError());
            return -1;
        }
        // Convert Windows handle to file descriptor
        wal->wal_fd = _open_osfhandle((intptr_t)hFile, _O_RDWR | _O_BINARY);
        if (wal->wal_fd < 0) {
            CloseHandle(hFile);
            printf("[WAL] ERROR Failed to convert handle to file descriptor\n");
            return -1;
        }
    #else
        wal->wal_fd = open(wal->wal_path, O_RDWR | O_CREAT, 0644);
    #endif
    
    if (wal->wal_fd < 0) {
        printf("[WAL] ERROR Failed to open WAL file: %s (errno: %d)\n", 
               wal->wal_path, errno);
        return -1;
    }
    
    // Check if WAL file is new or existing
    struct stat st;
    if (fstat(wal->wal_fd, &st) == 0 && st.st_size > 0) {
        // Existing WAL file - read header
        wal_file_header_t header;
        if (pread(wal->wal_fd, &header, sizeof(header), 0) == sizeof(header)) {
            if (header.magic == WAL_MAGIC) {
                wal->sequence = header.sequence;
                wal->checkpoint_sequence = header.checkpoint_sequence;
                // Set file position to end of file (for appending)
                wal->file_pos = st.st_size;
                WAL_LOG_INFO("[WAL] OK Loaded existing WAL: sequence=%lu, checkpoint=%lu\n",
                             wal->sequence, wal->checkpoint_sequence);
            } else {
                // Invalid magic - initialize as new
                WAL_LOG_INFO("[WAL] WARN Invalid WAL magic, initializing as new\n");
                wal->sequence = 0;
                wal->checkpoint_sequence = 0;
                wal->file_pos = sizeof(wal_file_header_t);
            }
        } else {
            // Failed to read header - initialize as new
            wal->sequence = 0;
            wal->checkpoint_sequence = 0;
            wal->file_pos = sizeof(wal_file_header_t);
        }
    } else {
        // New WAL file - write header and pre-allocate for Windows
        // Windows: Pre-allocate 1MB to avoid lazy write metadata issues
        size_t initial_size = sizeof(wal_file_header_t);
        #ifdef _WIN32
            // Pre-allocate 1MB for Windows (avoids lazy write metadata hiding data)
            initial_size = 1024 * 1024;  // 1MB
        #endif
        
        if (ftruncate(wal->wal_fd, initial_size) != 0) {
            printf("[WAL] ERROR Failed to initialize WAL file size\n");
            close(wal->wal_fd);
            return -1;
        }
        
        #ifdef _WIN32
            // Windows: Explicitly set file end (SetEndOfFile)
            HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
            if (hFile != INVALID_HANDLE_VALUE) {
                LARGE_INTEGER li;
                li.QuadPart = initial_size;
                if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN)) {
                    SetEndOfFile(hFile);
                }
            }
        #endif
        
        wal_file_header_t header = {
            .magic = WAL_MAGIC,
            .version = 1,
            .sequence = 0,
            .checkpoint_sequence = 0,
            .commit_count = 0,
            .last_valid_offset = sizeof(wal_file_header_t)  // Start after header
        };
        if (pwrite(wal->wal_fd, &header, sizeof(header), 0) != sizeof(header)) {
            printf("[WAL] ERROR Failed to write WAL header\n");
            close(wal->wal_fd);
            return -1;
        }
        
        // Windows: Hard flush header immediately
        #ifdef _WIN32
            // Reuse hFile from above scope
            if (hFile != INVALID_HANDLE_VALUE) {
                FlushFileBuffers(hFile);
            }
        #endif
        
        fsync(wal->wal_fd);
        wal->sequence = 0;
        wal->checkpoint_sequence = 0;
        wal->file_pos = sizeof(wal_file_header_t);  // Start after header
        WAL_LOG_INFO("[WAL] OK Created new WAL file (pre-allocated %zu bytes)\n", initial_size);
    }
    
    wal->enabled = true;
    wal->entries_since_checkpoint = 0;
    
    // Initialize batching (disabled by default, enable via wal_set_batch_size)
    wal->write_buffer = NULL;
    wal->write_buffer_size = 0;
    wal->write_buffer_pos = 0;
    wal->batch_size = 0;  // 0 = no batching (immediate write)
    wal->batch_count = 0;
    
    // Initialize adaptive batching (Phase 4)
    // For NVMe optimization, allow much larger batches
    wal->min_batch_size = 1000;    // Minimum 1K entries
    wal->max_batch_size = 100000;  // Maximum 100K entries (NVMe-optimized)
    wal->last_adjust_time = 0;
    wal->adjustment_interval = 1000;  // 1 second
    wal->write_rate = 0;
    wal->write_count_window = 0;
    wal->window_start_time = 0;
    
    // Initialize background flush thread synchronization
    wal->flush_thread_running = false;
    wal->flush_thread_stop = false;
    wal->flush_requested = false;
    wal->flush_in_progress = false;
    wal->flush_sequence = 0;
    wal->flush_error = 0;
    
    if (pthread_mutex_init(&wal->flush_mutex, NULL) != 0) {
        printf("[WAL] ERROR Failed to initialize flush mutex\n");
        close(wal->wal_fd);
        return -1;
    }
    
    if (pthread_cond_init(&wal->flush_cond, NULL) != 0) {
        printf("[WAL] ERROR Failed to initialize flush condition variable\n");
        pthread_mutex_destroy(&wal->flush_mutex);
        close(wal->wal_fd);
        return -1;
    }
    
    return 0;
}

// Synchronous flush (for cleanup, checkpoint, etc.)
static int wal_flush_sync(wal_context_t* wal) {
    if (!wal || !wal->enabled || wal->wal_fd < 0) return -1;
    
    // If no batching or buffer is empty, nothing to flush
    if (wal->batch_size == 0 || wal->write_buffer_pos == 0) {
        return 0;
    }
    
    // Protect file_pos access and copy buffer with mutex to prevent race conditions
    uint8_t* flush_buffer = NULL;
    pthread_mutex_lock(&wal->flush_mutex);
    off_t current_file_pos = wal->file_pos;
    size_t buffer_pos = wal->write_buffer_pos;
    if (buffer_pos > 0) {
        // CRITICAL: Copy buffer contents while holding mutex to prevent race conditions
        flush_buffer = (uint8_t*)malloc(buffer_pos);
        if (flush_buffer) {
            memcpy(flush_buffer, wal->write_buffer, buffer_pos);
        } else {
            printf("[WAL] ERROR Failed to allocate flush buffer\n");
            pthread_mutex_unlock(&wal->flush_mutex);
            return -1;
        }
        wal->file_pos = current_file_pos + buffer_pos;  // Update inside mutex
        wal->write_buffer_pos = 0;  // Reset buffer
        wal->batch_count = 0;
    }
    pthread_mutex_unlock(&wal->flush_mutex);
    
    if (buffer_pos == 0 || !flush_buffer) {
        return 0;  // Nothing to flush
    }
    
    // Write buffered data using pwrite with explicit offset (avoids file position issues)
    ssize_t written = pwrite(wal->wal_fd, flush_buffer, buffer_pos, current_file_pos);
    free(flush_buffer);  // Free copied buffer
    if (written != (ssize_t)buffer_pos) {
        printf("[WAL] ERROR Failed to flush WAL buffer (written: %zd, expected: %zu)\n", 
               written, buffer_pos);
        return -1;
    }
    
    // CLEAR-AHEAD: Zero out the next header space after the flush
    // Windows pre-allocation leaves garbage data, so we must explicitly zero the next slot
    off_t next_header_pos = current_file_pos + buffer_pos;
    wal_entry_header_t zero_header = {0};
    if (next_header_pos + sizeof(wal_entry_header_t) <= 1024 * 1024) {  // Only if within pre-allocated 1MB
        pwrite(wal->wal_fd, &zero_header, sizeof(wal_entry_header_t), next_header_pos);
    }
    
    // Single fsync for entire batch
    if (fsync(wal->wal_fd) != 0) {
        printf("[WAL] ERROR Failed to fsync\n");
        return -1;
    }
    
    // Update file header with State Ledger (CRITICAL for recovery visibility)
    wal_file_header_t header = {
        .magic = WAL_MAGIC,
        .version = 1,
        .sequence = wal->sequence,
        .checkpoint_sequence = wal->checkpoint_sequence,
        .commit_count = wal->sequence,  // All entries committed (immediate write mode)
        .last_valid_offset = wal->file_pos  // Current file position = end of valid data
    };
    if (pwrite(wal->wal_fd, &header, sizeof(header), 0) != sizeof(header)) {
        printf("[WAL] WARN Failed to update WAL header\n");
    }
    
    // Windows-specific: Hard flush header immediately
    #ifdef _WIN32
        HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
        if (hFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(hFile);  // Force header to physical disk
        }
    #endif
    
    fsync(wal->wal_fd);
    return 0;
}

// Background flush thread function
static void* wal_flush_thread_func(void* arg) {
    wal_context_t* wal = (wal_context_t*)arg;
    
    while (!wal->flush_thread_stop) {
        pthread_mutex_lock(&wal->flush_mutex);
        
        // Wait for flush request or stop signal
        while (!wal->flush_requested && !wal->flush_thread_stop) {
            pthread_cond_wait(&wal->flush_cond, &wal->flush_mutex);
        }
        
        if (wal->flush_thread_stop) {
            pthread_mutex_unlock(&wal->flush_mutex);
            break;
        }
        
        // Mark flush in progress
        wal->flush_in_progress = true;
        wal->flush_requested = false;
        
        pthread_mutex_unlock(&wal->flush_mutex);
        
        // Perform flush (outside mutex to avoid blocking writers)
        // Save buffer position and COPY buffer contents atomically to prevent race conditions
        size_t buffer_pos = 0;
        uint32_t batch_count = 0;
        off_t current_file_pos = 0;
        uint8_t* flush_buffer = NULL;
        pthread_mutex_lock(&wal->flush_mutex);
        buffer_pos = wal->write_buffer_pos;
        batch_count = wal->batch_count;  // Save before reset
        current_file_pos = wal->file_pos;  // Read file_pos inside mutex
        if (buffer_pos > 0) {
            // CRITICAL: Copy buffer contents while holding mutex to prevent race conditions
            // Writers can modify buffer after we release mutex, so we must copy it now
            flush_buffer = (uint8_t*)malloc(buffer_pos);
            if (flush_buffer) {
                memcpy(flush_buffer, wal->write_buffer, buffer_pos);
            } else {
                printf("[WAL-FLUSH] ERROR Failed to allocate flush buffer\n");
                buffer_pos = 0;  // Skip flush if allocation fails
            }
            wal->write_buffer_pos = 0;  // Reset immediately so new writes can buffer
            wal->batch_count = 0;  // Reset batch count
            // Update file_pos inside mutex to prevent race conditions
            wal->file_pos = current_file_pos + buffer_pos;
        }
        pthread_mutex_unlock(&wal->flush_mutex);
        
        if (buffer_pos > 0 && flush_buffer) {
            printf("[WAL-FLUSH] OK Flushing batch: %u entries, %zu bytes (%.2f KB)\n", 
                   batch_count, buffer_pos, buffer_pos / 1024.0);
            
            // Write buffered data using pwrite with explicit offset (avoids file position issues)
            ssize_t written = pwrite(wal->wal_fd, flush_buffer, buffer_pos, current_file_pos);
            free(flush_buffer);  // Free copied buffer
            if (written != (ssize_t)buffer_pos) {
                wal->flush_error = -1;
                printf("[WAL-FLUSH] ERROR Failed to write buffer (written: %zd, expected: %zu)\n", 
                       written, buffer_pos);
            } else {
                // CLEAR-AHEAD: Zero out the next header space after the batch write
                // Windows pre-allocation leaves garbage data, so we must explicitly zero the next slot
                off_t next_header_pos = current_file_pos + buffer_pos;
                wal_entry_header_t zero_header = {0};
                if (next_header_pos + sizeof(wal_entry_header_t) <= 1024 * 1024) {  // Only if within pre-allocated 1MB
                    pwrite(wal->wal_fd, &zero_header, sizeof(wal_entry_header_t), next_header_pos);
                }
                
                // FORCE-COMMIT: Update State Ledger (commit_count) FIRST before flushing
                // This ensures recovery can see entries even if process crashes
                wal_file_header_t header = {
                    .magic = WAL_MAGIC,
                    .version = 1,
                    .sequence = wal->sequence,
                    .checkpoint_sequence = wal->checkpoint_sequence,
                    .commit_count = wal->sequence,  // All entries up to current sequence are committed
                    .last_valid_offset = current_file_pos + buffer_pos  // End of valid data
                };
                if (pwrite(wal->wal_fd, &header, sizeof(header), 0) != sizeof(header)) {
                    printf("[WAL-FLUSH] WARN Failed to update WAL header\n");
                    wal->flush_error = -1;
                } else {
                    // Single fsync for entire batch (major optimization)
                    if (fsync(wal->wal_fd) != 0) {
                        wal->flush_error = -1;
                        printf("[WAL-FLUSH] ERROR Failed to fsync\n");
                    } else {
                        wal->flush_error = 0;
                        
                        // Windows-specific: Hard flush using WIN32 API (FlushViewOfFile + FlushFileBuffers)
                        #ifdef _WIN32
                            HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
                            if (hFile != INVALID_HANDLE_VALUE) {
                                // FlushViewOfFile: Flush memory-mapped view (if any)
                                // Note: We're not using mmap for WAL writes, but this is the pattern
                                // FlushFileBuffers: Bypass Windows write cache, force to physical disk
                                if (!FlushFileBuffers(hFile)) {
                                    printf("[WAL-FLUSH] WARN FlushFileBuffers failed (error: %lu)\n", GetLastError());
                                }
                            }
                        #endif
                        
                        // Update flush sequence (for durability tracking)
                        wal->flush_sequence = wal->sequence;
                    }
                }
            }
        }
        
        pthread_mutex_lock(&wal->flush_mutex);
        wal->flush_in_progress = false;
        pthread_cond_broadcast(&wal->flush_cond);  // Notify waiters
        pthread_mutex_unlock(&wal->flush_mutex);
    }
    
    wal->flush_thread_running = false;
    return NULL;
}

// Cleanup WAL
void wal_cleanup(wal_context_t* wal) {
    if (!wal) return;
    
    // Stop background flush thread
    if (wal->flush_thread_running) {
        pthread_mutex_lock(&wal->flush_mutex);
        wal->flush_thread_stop = true;
        wal->flush_requested = true;  // Wake thread
        pthread_cond_signal(&wal->flush_cond);
        pthread_mutex_unlock(&wal->flush_mutex);
        
        // Wait for thread to finish
        pthread_join(wal->flush_thread, NULL);
    }
    
    // Flush any pending writes (synchronous, since thread is stopped)
    if (wal->enabled && wal->batch_count > 0) {
        wal_flush_sync(wal);
    }
    
    // Cleanup thread synchronization
    pthread_cond_destroy(&wal->flush_cond);
    pthread_mutex_destroy(&wal->flush_mutex);
    
    // Free write buffer
    if (wal->write_buffer) {
        free(wal->write_buffer);
        wal->write_buffer = NULL;
    }
    
    if (wal->wal_fd >= 0) {
        close(wal->wal_fd);
        wal->wal_fd = -1;
    }
    
    memset(wal, 0, sizeof(wal_context_t));
}

// Flush WAL buffer to disk (non-blocking - signals background thread)
int wal_flush(wal_context_t* wal) {
    if (!wal || !wal->enabled || wal->wal_fd < 0) return -1;
    
    // If no batching or buffer is empty, nothing to flush
    if (wal->batch_size == 0 || wal->write_buffer_pos == 0) {
        return 0;
    }
    
    // Signal background flush thread (non-blocking)
    // NOTE: We don't reset write_buffer_pos or batch_count here - the flush thread will
    // read them and reset them after writing. This prevents race conditions and ensures
    // the flush thread sees the correct batch_count.
    pthread_mutex_lock(&wal->flush_mutex);
    wal->flush_requested = true;
    pthread_cond_signal(&wal->flush_cond);
    pthread_mutex_unlock(&wal->flush_mutex);
    
    return 0;
}

// Wait for flush to complete (for durability guarantees)
int wal_flush_wait(wal_context_t* wal, uint64_t sequence) {
    if (!wal || !wal->enabled) return -1;
    
    pthread_mutex_lock(&wal->flush_mutex);
    
    // Wait until flush sequence >= requested sequence
    while (wal->flush_sequence < sequence && wal->flush_thread_running) {
        pthread_cond_wait(&wal->flush_cond, &wal->flush_mutex);
    }
    
    int error = wal->flush_error;
    pthread_mutex_unlock(&wal->flush_mutex);
    
    return error;
}

// Append entry to WAL (with batching support)
uint64_t wal_append(wal_context_t* wal, wal_operation_t operation,
                    uint64_t node_id, const void* data, uint32_t data_size) {
    // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
    // fprintf(stderr, "!!! DEBUG: ENTERED wal_append: wal=%p, enabled=%d, fd=%d, operation=%d, node_id=%llu\n",
    //        (void*)wal, wal ? wal->enabled : 0, wal ? wal->wal_fd : -1, operation, (unsigned long long)node_id);
    // fflush(stderr);
    
    if (!wal || !wal->enabled || wal->wal_fd < 0) {
        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
        // fprintf(stderr, "!!! DEBUG: wal_append early exit: wal=%p, enabled=%d, fd=%d\n",
        //        (void*)wal, wal ? wal->enabled : 0, wal ? wal->wal_fd : -1);
        // fflush(stderr);
        return 0;
    }
    
    // Increment sequence
    wal->sequence++;
    wal->entries_since_checkpoint++;
    
    // Build entry header
    wal_entry_header_t entry_header = {
        .sequence = wal->sequence,
        .operation = operation,
        .node_id = node_id,
        .data_size = data_size
    };
    
    size_t entry_size = sizeof(entry_header) + data_size;
    
    // If batching is enabled, buffer the write
    if (wal->batch_size > 0) {
        // Allocate buffer if needed (64KB default, grows if needed)
        if (!wal->write_buffer) {
            wal->write_buffer_size = 64 * 1024;  // 64KB initial
            wal->write_buffer = (uint8_t*)malloc(wal->write_buffer_size);
            if (!wal->write_buffer) {
                printf("[WAL] ERROR Failed to allocate write buffer\n");
                return 0;
            }
        }
        
        // Grow buffer if needed
        if (wal->write_buffer_pos + entry_size > wal->write_buffer_size) {
            size_t new_size = wal->write_buffer_size * 2;
            while (new_size < wal->write_buffer_pos + entry_size) {
                new_size *= 2;
            }
            uint8_t* new_buffer = (uint8_t*)realloc(wal->write_buffer, new_size);
            if (!new_buffer) {
                printf("[WAL] ERROR Failed to grow write buffer\n");
                return 0;
            }
            wal->write_buffer = new_buffer;
            wal->write_buffer_size = new_size;
        }
        
        // Copy entry to buffer
        memcpy(wal->write_buffer + wal->write_buffer_pos, &entry_header, sizeof(entry_header));
        wal->write_buffer_pos += sizeof(entry_header);
        
        if (data && data_size > 0) {
            memcpy(wal->write_buffer + wal->write_buffer_pos, data, data_size);
            wal->write_buffer_pos += data_size;
        }
        
        wal->batch_count++;
        
        // Adjust batch size adaptively (Phase 4)
        wal_adjust_batch_size(wal);
        
        // Flush if batch is full
        if (wal->batch_count >= wal->batch_size) {
            // Batch is full - trigger flush (should be rare with 50k batch size)
            if (wal_flush(wal) != 0) {
                return 0;
            }
        }
        
        return wal->sequence;
    }
    
    // No batching: immediate write (legacy mode)
    // Use tracked file position instead of lseek
    fprintf(stderr, "[WAL-DEBUG] Immediate write: file_pos=%llu, entry_size=%zu\n", 
           (unsigned long long)wal->file_pos, entry_size);
    fflush(stderr);
    
    #ifdef _WIN32
        // Windows: Use direct WriteFile API for guaranteed writes
        HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
        if (hFile == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "[WAL-ERROR] Invalid file handle\n");
            fflush(stderr);
            return 0;
        }
        
        // Set file pointer to current position
        LARGE_INTEGER li;
        li.QuadPart = wal->file_pos;
        if (!SetFilePointerEx(hFile, li, NULL, FILE_BEGIN)) {
            fprintf(stderr, "[WAL-ERROR] SetFilePointerEx failed: %lu\n", GetLastError());
            fflush(stderr);
            return 0;
        }
        
        // Write entry header using WriteFile
        DWORD written = 0;
        if (!WriteFile(hFile, &entry_header, sizeof(entry_header), &written, NULL)) {
            fprintf(stderr, "[WAL-ERROR] WriteFile (header) failed: %lu\n", GetLastError());
            fflush(stderr);
            return 0;
        }
        if (written != sizeof(entry_header)) {
            fprintf(stderr, "[WAL-ERROR] WriteFile (header) partial write: %lu/%zu\n", 
                   written, sizeof(entry_header));
            fflush(stderr);
            return 0;
        }
        wal->file_pos += sizeof(entry_header);
        
        // Write variable-length data if present
        if (data && data_size > 0) {
            written = 0;
            if (!WriteFile(hFile, data, data_size, &written, NULL)) {
                fprintf(stderr, "[WAL-ERROR] WriteFile (data) failed: %lu\n", GetLastError());
                fflush(stderr);
                return 0;
            }
            if (written != data_size) {
                fprintf(stderr, "[WAL-ERROR] WriteFile (data) partial write: %lu/%u\n", 
                       written, data_size);
                fflush(stderr);
                return 0;
            }
            wal->file_pos += data_size;
        }
        
        // Force flush immediately after write
        if (!FlushFileBuffers(hFile)) {
            fprintf(stderr, "[WAL-ERROR] FlushFileBuffers failed: %lu\n", GetLastError());
            fflush(stderr);
            return 0;
        }
        fprintf(stderr, "[WAL-DEBUG] WriteFile + FlushFileBuffers succeeded\n");
        fflush(stderr);
    #else
        // POSIX: Use pwrite
        ssize_t written = pwrite(wal->wal_fd, &entry_header, sizeof(entry_header), wal->file_pos);
        if (written != sizeof(entry_header)) {
            fprintf(stderr, "[WAL] ERROR Failed to write WAL entry header: written=%zd, expected=%zu, errno=%d\n",
                   written, sizeof(entry_header), errno);
            fflush(stderr);
            return 0;
        }
        wal->file_pos += sizeof(entry_header);
        
        // Write variable-length data if present
        if (data && data_size > 0) {
            written = pwrite(wal->wal_fd, data, data_size, wal->file_pos);
            if (written != (ssize_t)data_size) {
                fprintf(stderr, "[WAL] ERROR Failed to write WAL entry data: written=%zd, expected=%u, errno=%d\n",
                       written, data_size, errno);
                fflush(stderr);
                return 0;
            }
            wal->file_pos += data_size;
        }
        
        // Force to disk
        fsync(wal->wal_fd);
    #endif
    
    // CLEAR-AHEAD: Zero out the next header space to prevent "Incomplete Header" errors on recovery
    // Windows pre-allocation leaves garbage data, so we must explicitly zero the next slot
    wal_entry_header_t zero_header = {0};
    off_t next_header_pos = wal->file_pos;
    if (next_header_pos + sizeof(wal_entry_header_t) <= 1024 * 1024) {  // Only if within pre-allocated 1MB
        ssize_t zero_written = pwrite(wal->wal_fd, &zero_header, sizeof(wal_entry_header_t), next_header_pos);
        if (zero_written != sizeof(wal_entry_header_t)) {
            fprintf(stderr, "[WAL] WARN Failed to zero next header: written=%zd, errno=%d\n", zero_written, errno);
            fflush(stderr);
        }
    }
    
    // Force to disk (critical for durability)
    if (fsync(wal->wal_fd) != 0) {
        fprintf(stderr, "[WAL] WARN fsync failed: errno=%d\n", errno);
        fflush(stderr);
    }
    
    // Update file header with State Ledger (CRITICAL: must update commit_count and last_valid_offset)
    wal_file_header_t header = {
        .magic = WAL_MAGIC,
        .version = 1,
        .sequence = wal->sequence,
        .checkpoint_sequence = wal->checkpoint_sequence,
        .commit_count = wal->sequence,  // All entries committed (immediate write mode)
        .last_valid_offset = wal->file_pos  // Current file position = end of valid data
    };
    ssize_t header_written = pwrite(wal->wal_fd, &header, sizeof(header), 0);
    if (header_written != sizeof(header)) {
        fprintf(stderr, "[WAL] ERROR Failed to update WAL header: written=%zd, expected=%zu, errno=%d\n",
               header_written, sizeof(header), errno);
        fflush(stderr);
    } else {
        fprintf(stderr, "[WAL-DEBUG] Header updated: sequence=%llu, commit_count=%llu, last_valid_offset=%llu\n",
               (unsigned long long)wal->sequence, (unsigned long long)header.commit_count, 
               (unsigned long long)header.last_valid_offset);
        fflush(stderr);
    }
    
    // Windows-specific: Hard flush header immediately
    #ifdef _WIN32
        if (hFile != INVALID_HANDLE_VALUE) {
            if (!FlushFileBuffers(hFile)) {
                fprintf(stderr, "[WAL-ERROR] FlushFileBuffers (header) failed: %lu\n", GetLastError());
                fflush(stderr);
            }
        }
    #endif
    
    if (fsync(wal->wal_fd) != 0) {
        fprintf(stderr, "[WAL] WARN fsync (header) failed: errno=%d\n", errno);
        fflush(stderr);
    }
    
    fprintf(stderr, "[WAL-DEBUG] wal_append returning sequence=%llu\n", (unsigned long long)wal->sequence);
    fflush(stderr);
    
    return wal->sequence;
}

// Append ADD_NODE operation
uint64_t wal_append_add_node(wal_context_t* wal, uint64_t node_id,
                            uint8_t node_type, const char* name,
                            const char* node_data, uint64_t parent_id) {
    if (!wal || !name) return 0;
    
    // Pack data: type (1) | name_len (4) | name | data_len (4) | data | parent_id (8)
    size_t name_len = strlen(name);
    size_t data_len = node_data ? strlen(node_data) : 0;
    size_t total_size = 1 + 4 + name_len + 4 + data_len + 8;
    
    char* packed_data = (char*)malloc(total_size);
    if (!packed_data) return 0;
    
    size_t offset = 0;
    packed_data[offset++] = node_type;
    
    memcpy(packed_data + offset, &name_len, 4);
    offset += 4;
    memcpy(packed_data + offset, name, name_len);
    offset += name_len;
    
    memcpy(packed_data + offset, &data_len, 4);
    offset += 4;
    if (data_len > 0) {
        memcpy(packed_data + offset, node_data, data_len);
        offset += data_len;
    }
    
    memcpy(packed_data + offset, &parent_id, 8);
    offset += 8;
    
    uint64_t seq = wal_append(wal, WAL_OP_ADD_NODE, node_id, packed_data, total_size);
    free(packed_data);
    
    return seq;
}

// Append UPDATE_NODE operation
uint64_t wal_append_update_node(wal_context_t* wal, uint64_t node_id,
                                const char* new_data) {
    if (!wal || !new_data) return 0;
    
    size_t data_len = strlen(new_data);
    return wal_append(wal, WAL_OP_UPDATE_NODE, node_id, new_data, data_len);
}

// Append DELETE_NODE operation
uint64_t wal_append_delete_node(wal_context_t* wal, uint64_t node_id) {
    if (!wal) return 0;
    
    return wal_append(wal, WAL_OP_DELETE_NODE, node_id, NULL, 0);
}

// Append ADD_CHILD operation
uint64_t wal_append_add_child(wal_context_t* wal, uint64_t parent_id,
                             uint64_t child_id) {
    if (!wal) return 0;
    
    uint64_t data[2] = {parent_id, child_id};
    return wal_append(wal, WAL_OP_ADD_CHILD, parent_id, data, sizeof(data));
}

// Checkpoint WAL
int wal_checkpoint(wal_context_t* wal) {
    if (!wal || !wal->enabled || wal->wal_fd < 0) return -1;
    
    // Flush any pending writes before checkpointing (non-blocking signal)
    if (wal->batch_count > 0) {
        wal_flush(wal);
    }
    
    // Wait for all pending flushes to complete (durability guarantee)
    if (wal->batch_size > 0 && wal->flush_thread_running) {
        uint64_t current_sequence = wal->sequence;
        if (wal_flush_wait(wal, current_sequence) != 0) {
            printf("[WAL] ERROR Flush error during checkpoint\n");
            return -1;
        }
    }
    
    // Update checkpoint sequence
    wal->checkpoint_sequence = wal->sequence;
    wal->entries_since_checkpoint = 0;
    
    // Update file header with State Ledger
    wal_file_header_t header = {
        .magic = WAL_MAGIC,
        .version = 1,
        .sequence = wal->sequence,
        .checkpoint_sequence = wal->checkpoint_sequence,
        .commit_count = wal->checkpoint_sequence,  // Committed entries = checkpointed entries
        .last_valid_offset = wal->file_pos  // Current position = end of valid data
    };
    
    if (pwrite(wal->wal_fd, &header, sizeof(header), 0) != sizeof(header)) {
        printf("[WAL] ERROR Failed to update checkpoint in header\n");
        return -1;
    }
    
    // Windows-specific: Hard flush using WIN32 API (FlushViewOfFile + FlushFileBuffers)
    #ifdef _WIN32
        HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
        if (hFile != INVALID_HANDLE_VALUE) {
            // FlushFileBuffers: Bypass Windows write cache, force to physical disk
            if (!FlushFileBuffers(hFile)) {
                printf("[WAL] WARN FlushFileBuffers failed (error: %lu)\n", GetLastError());
            }
        }
    #endif
    
    fsync(wal->wal_fd);
    
    // CRITICAL: Truncate WAL file to remove checkpointed entries
    // This prevents old data from being read as new entries during recovery
    if (wal_truncate(wal) != 0) {
        printf("[WAL] WARN Failed to truncate WAL after checkpoint\n");
        // Continue anyway - checkpoint is still valid
    }
    
    printf("[WAL] OK Checkpointed at sequence %llu\n", (unsigned long long)wal->checkpoint_sequence);
    
    return 0;
}

// Recover from WAL (Phase 3: mmap'd for faster recovery)
int wal_recover(wal_context_t* wal,
                int (*apply_add_node)(void* ctx, uint64_t node_id, uint8_t type,
                                     const char* name, const char* data, uint64_t parent_id),
                int (*apply_update_node)(void* ctx, uint64_t node_id, const char* data),
                int (*apply_delete_node)(void* ctx, uint64_t node_id),
                int (*apply_add_child)(void* ctx, uint64_t parent_id, uint64_t child_id),
                void* apply_ctx) {
    if (!wal || wal->wal_fd < 0) return -1;
    
    // Get file size
    struct stat st;
    if (fstat(wal->wal_fd, &st) != 0 || st.st_size == 0) {
        printf("[WAL] WARN WAL file is empty or invalid\n");
        return 0;  // No recovery needed
    }
    
    size_t file_size = (size_t)st.st_size;
    
    // Memory-map WAL file for zero-copy recovery (Phase 3 optimization)
    void* wal_mmap = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, wal->wal_fd, 0);
    if (wal_mmap == MAP_FAILED) {
        printf("[WAL] WARN Failed to mmap WAL file, falling back to pread()\n");
        // Fallback to pread() implementation (not shown here for brevity)
        return -1;
    }
    
    // Read file header (State Ledger - direct pointer access - zero copy)
    wal_file_header_t* header = (wal_file_header_t*)wal_mmap;
    
    // State Ledger: Use commit_count and last_valid_offset to know where valid data ends
    // This prevents reading junk data that Windows lazy write might leave in the file
    size_t valid_data_end = file_size;
    if (header->commit_count > 0 && header->last_valid_offset > sizeof(wal_file_header_t) && 
        header->last_valid_offset < file_size) {
        valid_data_end = header->last_valid_offset;  // Only read up to last valid entry
        WAL_LOG_INFO("[WAL] INFO State Ledger: commit_count=%llu, reading up to offset %zu (file size: %zu)\n",
                     (unsigned long long)header->commit_count, valid_data_end, file_size);
    }
    
    if (header->magic != WAL_MAGIC) {
        printf("[WAL] ERROR Invalid WAL magic during recovery\n");
        munmap(wal_mmap, file_size);
        return -1;
    }
    
    // Start reading entries after header
    size_t offset = sizeof(wal_file_header_t);
    
    WAL_LOG_INFO("[WAL] INFO Recovering from WAL (mmap'd, checkpoint: %llu, current: %llu)...\n",
                 (unsigned long long)header->checkpoint_sequence, (unsigned long long)header->sequence);
    
    uint32_t entries_replayed = 0;
    
    // Validate file size is at least header size
    if (file_size < sizeof(wal_file_header_t)) {
    WAL_LOG_INFO("[WAL] WARN WAL file is too small (size: %zu, expected at least %zu)\n",
                     file_size, sizeof(wal_file_header_t));
        munmap(wal_mmap, file_size);
        return -1;
    }
    
    // Read entries until end of file (direct pointer access - zero copy)
    while (offset < file_size) {
        // Bounds check
        if (offset + sizeof(wal_entry_header_t) > file_size) {
            WAL_LOG_INFO("[WAL] WARN Incomplete entry header at offset %zu, stopping recovery\n", offset);
            break;
        }
        
        // Read entry header (direct pointer access)
        wal_entry_header_t* entry_header = (wal_entry_header_t*)((uint8_t*)wal_mmap + offset);
        
        // SENTINEL CHECK: If header is zeroed (sentinel), stop gracefully
        // This handles Windows pre-allocation where we zero the next header space
        if (entry_header->sequence == 0 && entry_header->operation == 0 && 
            entry_header->node_id == 0 && entry_header->data_size == 0) {
            // Reached sentinel (zeroed header) - end of valid data
            WAL_LOG_INFO("[WAL] INFO Reached sentinel (zeroed header) at offset %zu, stopping recovery\n", offset);
            break;
        }
        
        // Validate entry header to detect corruption early
        // Check sequence number first - it should be reasonable (not garbage)
        // Sequence should be > checkpoint_sequence and <= header->sequence
        // If sequence is way too large (> header->sequence + 1000), it's likely corruption
        if (entry_header->sequence > header->sequence + 1000 || 
            (entry_header->sequence > 0 && entry_header->sequence < header->checkpoint_sequence)) {
            // Corrupted sequence - truncate file at this offset
            printf("[WAL] WARN Invalid entry at offset %zu (seq: %lu is out of range, checkpoint: %lu, current: %lu), truncating\n",
                   offset, entry_header->sequence, header->checkpoint_sequence, header->sequence);
            
            // If corruption is detected very early (right after header), reinitialize the WAL file
            if (offset <= sizeof(wal_file_header_t) + 1024) {
                printf("[WAL] WARN Corruption detected very early, reinitializing WAL file\n");
                // Truncate to header size and reinitialize
                if (ftruncate(wal->wal_fd, sizeof(wal_file_header_t)) == 0) {
                    wal_file_header_t new_header = {
                        .magic = WAL_MAGIC,
                        .version = 1,
                        .sequence = header->sequence,
                        .checkpoint_sequence = header->checkpoint_sequence
                    };
                    pwrite(wal->wal_fd, &new_header, sizeof(new_header), 0);
                    fsync(wal->wal_fd);
                    printf("[WAL] OK Reinitialized WAL file\n");
                }
            } else {
                // Truncate file at this offset to remove corrupted data
                if (ftruncate(wal->wal_fd, offset) == 0) {
                    fsync(wal->wal_fd);
                    printf("[WAL] OK Truncated WAL file to remove corrupted data\n");
                }
            }
            break;
        }
        
        // If data_size is unreasonably large, it's definitely corruption
        if (entry_header->data_size > 1048576) {
            // Corrupted entry - truncate file at this offset
            printf("[WAL] WARN Invalid entry at offset %zu (seq: %lu, data_size: %u), truncating\n",
                   offset, entry_header->sequence, entry_header->data_size);
            
            // If corruption is detected very early (right after header), reinitialize the WAL file
            if (offset <= sizeof(wal_file_header_t) + 1024) {
                printf("[WAL] WARN Corruption detected very early, reinitializing WAL file\n");
                // Truncate to header size and reinitialize
                if (ftruncate(wal->wal_fd, sizeof(wal_file_header_t)) == 0) {
                    wal_file_header_t new_header = {
                        .magic = WAL_MAGIC,
                        .version = 1,
                        .sequence = header->sequence,
                        .checkpoint_sequence = header->checkpoint_sequence
                    };
                    pwrite(wal->wal_fd, &new_header, sizeof(new_header), 0);
                    fsync(wal->wal_fd);
                    printf("[WAL] OK Reinitialized WAL file\n");
                }
            } else {
                // Truncate file at this offset to remove corrupted data
                if (ftruncate(wal->wal_fd, offset) == 0) {
                    fsync(wal->wal_fd);
                    printf("[WAL] OK Truncated WAL file to remove corrupted data\n");
                }
            }
            break;
        }
        
        // Additional validation: sequence should be reasonable
        // Allow some tolerance for concurrent writes (sequence can be slightly ahead)
        if (entry_header->sequence == 0 && entry_header->data_size > 0) {
            // Sequence 0 with data is suspicious (sequence 0 should only be for empty entries)
            // But don't truncate unless data_size is also large (already checked above)
            // This is just a warning case, continue processing
        }
        
        offset += sizeof(wal_entry_header_t);
        
        // Skip entries before checkpoint (already applied)
        if (entry_header->sequence <= header->checkpoint_sequence) {
            // Bounds check for data size - if data_size is unreasonably large, this is likely corrupted data
            // Valid data_size should be < 1MB (1048576 bytes) for a single entry
            if (entry_header->data_size > 1048576) {
                // Corrupted checkpointed entry - truncation should have removed it but didn't
                // This can happen if truncation failed or file was corrupted
                // Since data_size is corrupted, we can't safely skip to next entry
                // Truncate the file at this point to remove all corrupted data
            printf("[WAL] WARN Found corrupted entry at offset %zu (data_size: %u), truncating WAL file\n",
                       offset - sizeof(wal_entry_header_t), entry_header->data_size);
                // Truncate file at this offset to remove corrupted data
                if (ftruncate(wal->wal_fd, offset - sizeof(wal_entry_header_t)) == 0) {
                    fsync(wal->wal_fd);
                    printf("[WAL] OK Truncated WAL file to remove corrupted data\n");
                }
                break;
            }
            if (offset + entry_header->data_size > file_size) {
                printf("[WAL] WARN Entry data extends beyond file, stopping recovery\n");
                break;
            }
            offset += entry_header->data_size;
            continue;
        }
        
        // Read entry data (direct pointer access - zero copy)
        const char* entry_data = NULL;
        if (entry_header->data_size > 0) {
            // Validate data_size is reasonable (sanity check for corruption)
            if (entry_header->data_size > 1048576) {
                // Corrupted entry - truncate file at this point to remove corrupted data
                printf("[WAL] WARN Found corrupted entry at offset %zu (data_size: %u), truncating WAL file\n",
                       offset - sizeof(wal_entry_header_t), entry_header->data_size);
                // Truncate file at this offset to remove corrupted data
                if (ftruncate(wal->wal_fd, offset - sizeof(wal_entry_header_t)) == 0) {
                    fsync(wal->wal_fd);
                    printf("[WAL] OK Truncated WAL file to remove corrupted data\n");
                }
                break;
            }
            // Bounds check
            if (offset + entry_header->data_size > file_size) {
                printf("[WAL] WARN Incomplete entry data at offset %zu (data_size: %u, file_size: %zu)\n", 
                       offset, entry_header->data_size, file_size);
                break;
            }
            
            entry_data = (const char*)((uint8_t*)wal_mmap + offset);
            offset += entry_header->data_size;
        }
        
        // Apply operation (using mmap'd data directly)
        int result = 0;
        switch (entry_header->operation) {
            case WAL_OP_ADD_NODE: {
                if (entry_data && entry_header->data_size >= 9) {
                    size_t data_offset = 0;
                    uint8_t type = entry_data[data_offset++];
                    
                    uint32_t name_len;
                    memcpy(&name_len, entry_data + data_offset, 4);
                    data_offset += 4;
                    
                    // Bounds check
                    if (data_offset + name_len > entry_header->data_size) {
                        printf("[WAL] WARN Invalid name length in entry\n");
                        break;
                    }
                    
                    char* name = (char*)malloc(name_len + 1);
                    memcpy(name, entry_data + data_offset, name_len);
                    name[name_len] = '\0';
                    data_offset += name_len;
                    
                    uint32_t node_data_len;
                    memcpy(&node_data_len, entry_data + data_offset, 4);
                    data_offset += 4;
                    
                    char* node_data = NULL;
                    if (node_data_len > 0) {
                        // Bounds check
                        if (data_offset + node_data_len > entry_header->data_size) {
                            printf("[WAL] WARN Invalid node data length in entry\n");
                            free(name);
                            break;
                        }
                        
                        node_data = (char*)malloc(node_data_len + 1);
                        memcpy(node_data, entry_data + data_offset, node_data_len);
                        node_data[node_data_len] = '\0';
                        data_offset += node_data_len;
                    }
                    
                    uint64_t parent_id;
                    // Read 64-bit parent_id (8 bytes for new format, 4 bytes for old format)
                    if (data_offset + 8 <= entry_header->data_size) {
                        // New format: 64-bit parent_id
                        memcpy(&parent_id, entry_data + data_offset, 8);
                    } else if (data_offset + 4 <= entry_header->data_size) {
                        // Old format: 32-bit parent_id (backward compatibility)
                        uint32_t parent_id_32;
                        memcpy(&parent_id_32, entry_data + data_offset, 4);
                        parent_id = (uint64_t)parent_id_32;
                    } else {
                        parent_id = 0;
                    }
                    
                    if (apply_add_node) {
                        result = apply_add_node(apply_ctx, entry_header->node_id, type,
                                               name, node_data, parent_id);
                    }
                    
                    free(name);
                    if (node_data) free(node_data);
                }
                break;
            }
            case WAL_OP_UPDATE_NODE: {
                if (entry_data && apply_update_node) {
                    char* data = (char*)malloc(entry_header->data_size + 1);
                    memcpy(data, entry_data, entry_header->data_size);
                    data[entry_header->data_size] = '\0';
                    result = apply_update_node(apply_ctx, entry_header->node_id, data);
                    free(data);
                }
                break;
            }
            case WAL_OP_DELETE_NODE: {
                if (apply_delete_node) {
                    result = apply_delete_node(apply_ctx, entry_header->node_id);
                }
                break;
            }
            case WAL_OP_ADD_CHILD: {
                if (entry_data && entry_header->data_size >= 8 && apply_add_child) {
                    uint64_t parent_id, child_id;
                    // Read 64-bit IDs (16 bytes for new format, 8 bytes for old format)
                    if (entry_header->data_size >= 16) {
                        // New format: 64-bit IDs
                        memcpy(&parent_id, entry_data, 8);
                        memcpy(&child_id, entry_data + 8, 8);
                    } else {
                        // Old format: 32-bit IDs (backward compatibility)
                        uint32_t parent_id_32, child_id_32;
                        memcpy(&parent_id_32, entry_data, 4);
                        memcpy(&child_id_32, entry_data + 4, 4);
                        parent_id = (uint64_t)parent_id_32;
                        child_id = (uint64_t)child_id_32;
                    }
                    result = apply_add_child(apply_ctx, parent_id, child_id);
                }
                break;
            }
            case WAL_OP_CHECKPOINT:
                // Checkpoint entries are markers, skip
                break;
            default:
                printf("[WAL] WARN Unknown operation type: %d\n", entry_header->operation);
                break;
        }
        
        if (result != 0) {
            printf("[WAL] WARN Failed to apply operation %d at sequence %lu\n",
                   entry_header->operation, entry_header->sequence);
            // Continue recovery (don't fail on single operation failure)
        } else {
            entries_replayed++;
        }
    }
    
    // Unmap WAL file
    munmap(wal_mmap, file_size);
    
    WAL_LOG_INFO("[WAL] OK Recovery complete (mmap'd): %u entries replayed\n", entries_replayed);
    
    return 0;
}

// Truncate WAL (remove checkpointed entries)
int wal_truncate(wal_context_t* wal) {
    if (!wal || wal->wal_fd < 0) return -1;
    
    if (wal->checkpoint_sequence == 0) {
        // Nothing to truncate
        return 0;
    }
    
    // If all entries are checkpointed, truncate to header size
    if (wal->checkpoint_sequence >= wal->sequence) {
        // All entries checkpointed - truncate to header only
        off_t truncate_offset = sizeof(wal_file_header_t);
        if (ftruncate(wal->wal_fd, truncate_offset) != 0) {
            printf("[WAL] ERROR Failed to truncate WAL file\n");
            return -1;
        }
        
        #ifdef _WIN32
            // Windows-specific: Explicitly set file end after truncation
            HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
            if (hFile != INVALID_HANDLE_VALUE) {
                LARGE_INTEGER li;
                li.QuadPart = truncate_offset;
                if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN)) {
                    if (SetEndOfFile(hFile)) {
                        FlushFileBuffers(hFile);  // Force flush
                    }
                }
            }
        #endif
        
        wal->file_pos = truncate_offset;  // Update file position
        fsync(wal->wal_fd);  // Ensure truncation is on disk
        printf("[WAL] OK Truncated WAL to %ld bytes (all entries checkpointed)\n", truncate_offset);
        return 0;
    }
    
    // Find offset of first entry after checkpoint
    off_t truncate_offset = sizeof(wal_file_header_t);
    off_t file_size = lseek(wal->wal_fd, 0, SEEK_END);
    
    // Read entries to find checkpoint boundary
    off_t offset = sizeof(wal_file_header_t);
    while (offset < file_size) {
        wal_entry_header_t entry_header;
        ssize_t read_bytes = pread(wal->wal_fd, &entry_header, sizeof(entry_header), offset);
        
        if (read_bytes != sizeof(entry_header)) {
            // Invalid entry header - truncate here
            break;
        }
        
        // Validate entry header (sanity check for corruption)
        if (entry_header.data_size > 1048576) {
            // Corrupted entry - truncate before it
            printf("[WAL] WARN Found corrupted entry at offset %ld, truncating before it\n", offset);
            break;
        }
        
        if (entry_header.sequence > wal->checkpoint_sequence) {
            truncate_offset = offset;
            break;
        }
        
        offset += sizeof(entry_header) + entry_header.data_size;
        
        // Safety check to prevent infinite loop
        if (offset > file_size) {
            break;
        }
    }
    
    // Truncate file at checkpoint boundary
    if (truncate_offset > sizeof(wal_file_header_t)) {
        if (ftruncate(wal->wal_fd, truncate_offset) != 0) {
            printf("[WAL] ERROR Failed to truncate WAL file to offset %ld\n", truncate_offset);
            return -1;
        }
        
        #ifdef _WIN32
            // Windows-specific: Explicitly set file end after truncation
            // Windows doesn't automatically shrink files - need SetEndOfFile
            HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
            if (hFile != INVALID_HANDLE_VALUE) {
                LARGE_INTEGER li;
                li.QuadPart = truncate_offset;
                if (!SetFilePointerEx(hFile, li, NULL, FILE_BEGIN)) {
                    printf("[WAL] WARN SetFilePointerEx failed (error: %lu)\n", GetLastError());
                } else {
                    if (!SetEndOfFile(hFile)) {
                        printf("[WAL] WARN SetEndOfFile failed (error: %lu)\n", GetLastError());
                    } else {
                        // Force flush after truncation
                        FlushFileBuffers(hFile);
                    }
                }
            }
        #endif
        
        wal->file_pos = truncate_offset;  // Update file position
        
        // CRITICAL: Sync after truncation to ensure it's written to disk
        fsync(wal->wal_fd);
        
        printf("[WAL] OK Truncated WAL to %ld bytes (removed checkpointed entries)\n", truncate_offset);
    }
    
    return 0;
}

// Get WAL statistics
void wal_get_stats(wal_context_t* wal, uint64_t* total_entries,
                  uint64_t* checkpointed_entries, uint64_t* pending_entries) {
    if (!wal) return;
    
    if (total_entries) *total_entries = wal->sequence;
    if (checkpointed_entries) *checkpointed_entries = wal->checkpoint_sequence;
    if (pending_entries) *pending_entries = wal->sequence - wal->checkpoint_sequence;
}

// Get current time in milliseconds
static uint64_t get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

// Adjust batch size based on write rate (Phase 4: Adaptive batching)
void wal_adjust_batch_size(wal_context_t* wal) {
    if (!wal || wal->batch_size == 0) return;  // Adaptive batching only works with batching enabled
    
    uint64_t now = get_time_ms();
    
    // Update write rate (rolling window)
    if (wal->window_start_time == 0) {
        wal->window_start_time = now;
        wal->write_count_window = 0;
    }
    
    wal->write_count_window++;
    
    // Calculate write rate every second
    uint64_t window_duration = now - wal->window_start_time;
    if (window_duration >= 1000) {  // 1 second window
        wal->write_rate = (wal->write_count_window * 1000) / (uint32_t)window_duration;
        wal->write_count_window = 0;
        wal->window_start_time = now;
    }
    
    // Adjust batch size periodically
    if (now - wal->last_adjust_time < wal->adjustment_interval) {
        return;  // Too soon to adjust
    }
    
    wal->last_adjust_time = now;
    
    uint32_t old_batch_size = wal->batch_size;
    
    // Adaptive algorithm:
    // - High write rate (>10K writes/sec): Increase batch size (better throughput)
    // - Low write rate (<1K writes/sec): Decrease batch size (lower latency)
    // - Medium write rate: Keep current size
    
    if (wal->write_rate > 10000) {
        // High write rate: Increase batch size (up to max)
        wal->batch_size = (uint32_t)(wal->batch_size * 1.2);
        if (wal->batch_size > wal->max_batch_size) {
            wal->batch_size = wal->max_batch_size;
        }
    } else if (wal->write_rate < 1000 && wal->write_rate > 0) {
        // Low write rate: Decrease batch size (down to min)
        wal->batch_size = (uint32_t)(wal->batch_size * 0.8);
        if (wal->batch_size < wal->min_batch_size) {
            wal->batch_size = wal->min_batch_size;
        }
    }
    // Medium write rate: Keep current size
    
    if (old_batch_size != wal->batch_size) {
        // Batch size changed - log for debugging
        // printf("[WAL] Adjusted batch size: %u -> %u (write rate: %u/sec)\n",
        //        old_batch_size, wal->batch_size, wal->write_rate);
    }
}

// Enable adaptive batching (Phase 4)
int wal_enable_adaptive_batching(wal_context_t* wal, uint32_t min_batch, uint32_t max_batch) {
    if (!wal) return -1;
    
    if (min_batch == 0 || max_batch == 0 || min_batch > max_batch) {
        return -1;  // Invalid parameters
    }
    
    wal->min_batch_size = min_batch;
    wal->max_batch_size = max_batch;
    
    // Set initial batch size to midpoint
    uint32_t initial_batch = (min_batch + max_batch) / 2;
    if (wal_set_batch_size(wal, initial_batch) != 0) {
        return -1;
    }
    
    wal->last_adjust_time = get_time_ms();
    wal->window_start_time = 0;
    wal->write_rate = 0;
    wal->write_count_window = 0;
    
    return 0;
}

// Enable/configure WAL batching (0 = disable, N = batch size)
int wal_set_batch_size(wal_context_t* wal, uint32_t batch_size) {
    if (!wal) return -1;
    
    // Flush any pending writes before changing batch size
    if (wal->batch_count > 0) {
        wal_flush(wal);
    }
    
    // Stop background flush thread if disabling batching
    if (wal->batch_size > 0 && batch_size == 0 && wal->flush_thread_running) {
        pthread_mutex_lock(&wal->flush_mutex);
        wal->flush_thread_stop = true;
        wal->flush_requested = true;
        pthread_cond_signal(&wal->flush_cond);
        pthread_mutex_unlock(&wal->flush_mutex);
        
        pthread_join(wal->flush_thread, NULL);
        wal->flush_thread_running = false;
    }
    
    wal->batch_size = batch_size;
    
    // If disabling batching, free buffer
    if (batch_size == 0 && wal->write_buffer) {
        free(wal->write_buffer);
        wal->write_buffer = NULL;
        wal->write_buffer_size = 0;
    }
    
    // Start background flush thread if enabling batching
    if (wal->batch_size > 0 && !wal->flush_thread_running) {
        wal->flush_thread_stop = false;
        wal->flush_requested = false;
        wal->flush_in_progress = false;
        wal->flush_sequence = wal->sequence;
        wal->flush_error = 0;
        
        if (pthread_create(&wal->flush_thread, NULL, wal_flush_thread_func, wal) != 0) {
            printf("[WAL] ERROR Failed to start background flush thread\n");
            wal->batch_size = 0;  // Disable batching if thread creation fails
            return -1;
        }
        
        wal->flush_thread_running = true;
    }
    
    return 0;
}

