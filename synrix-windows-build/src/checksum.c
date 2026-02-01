#define _GNU_SOURCE
#include "checksum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <zlib.h>  // For CRC32

#define CHECKSUM_MAGIC 0x434B5355  // "CKSU"
#define CHECKSUM_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t checksum_count;
    uint32_t reserved;
} checksum_file_header_t;

// Initialize checksum support
int checksum_init(checksum_context_t* ctx, 
                  persistent_lattice_t* lattice,
                  const char* checksum_file_path) {
    if (!ctx || !lattice || !checksum_file_path) return -1;
    
    memset(ctx, 0, sizeof(checksum_context_t));
    ctx->lattice = lattice;
    strncpy(ctx->checksum_file_path, checksum_file_path, sizeof(ctx->checksum_file_path) - 1);
    ctx->checksum_file_path[sizeof(ctx->checksum_file_path) - 1] = '\0';
    ctx->enabled = false;  // Disabled by default
    ctx->checksum_fd = -1;
    
    // Try to load existing checksums (if file exists)
    checksum_load(ctx);
    
    return 0;
}

// Enable/disable checksums
int checksum_enable(checksum_context_t* ctx, bool enable) {
    if (!ctx) return -1;
    
    ctx->enabled = enable;
    
    if (enable && ctx->checksum_fd < 0) {
        // Open checksum file for writing
        ctx->checksum_fd = open(ctx->checksum_file_path, O_CREAT | O_RDWR, 0644);
        if (ctx->checksum_fd < 0) {
            printf("[CHECKSUM] ERROR Failed to open checksum file: %s\n", ctx->checksum_file_path);
            ctx->enabled = false;
            return -1;
        }
    } else if (!enable && ctx->checksum_fd >= 0) {
        // Close checksum file
        close(ctx->checksum_fd);
        ctx->checksum_fd = -1;
    }
    
    return 0;
}

// Cleanup checksum context
void checksum_cleanup(checksum_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->checksum_fd >= 0) {
        close(ctx->checksum_fd);
    }
    
    if (ctx->checksums) {
        free(ctx->checksums);
    }
    
    memset(ctx, 0, sizeof(checksum_context_t));
}

// Calculate CRC32 checksum
uint32_t checksum_calculate(const void* data, size_t data_len) {
    if (!data || data_len == 0) return 0;
    
    return crc32(0, (const Bytef*)data, data_len);
}

// Store checksum for node
int checksum_store(checksum_context_t* ctx, uint64_t node_id, uint32_t checksum) {
    if (!ctx || !ctx->enabled) return 0;  // No-op if disabled
    
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    
    // Ensure capacity
    if (local_id >= ctx->checksum_capacity) {
        uint32_t new_capacity = (local_id + 1) * 2;
        uint32_t* new_checksums = (uint32_t*)realloc(ctx->checksums, 
                                                      new_capacity * sizeof(uint32_t));
        if (!new_checksums) return -1;
        memset(new_checksums + ctx->checksum_capacity, 0, 
               (new_capacity - ctx->checksum_capacity) * sizeof(uint32_t));
        ctx->checksums = new_checksums;
        ctx->checksum_capacity = new_capacity;
    }
    
    ctx->checksums[local_id] = checksum;
    return 0;
}

// Verify checksum for node
int checksum_verify(checksum_context_t* ctx, uint64_t node_id, uint32_t expected_checksum) {
    if (!ctx || !ctx->enabled) return 0;  // No-op if disabled (always valid)
    
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    
    if (local_id >= ctx->checksum_capacity || !ctx->checksums) {
        return -1;  // No checksum stored
    }
    
    uint32_t stored_checksum = ctx->checksums[local_id];
    if (stored_checksum == 0) {
        return 0;  // No checksum stored yet (new node)
    }
    
    if (stored_checksum != expected_checksum) {
        printf("[CHECKSUM] ERROR Checksum mismatch for node %lu: stored=0x%08X, expected=0x%08X\n",
               node_id, stored_checksum, expected_checksum);
        return -1;
    }
    
    return 0;
}

// Load checksums from file
int checksum_load(checksum_context_t* ctx) {
    if (!ctx) return -1;
    
    int fd = open(ctx->checksum_file_path, O_RDONLY);
    if (fd < 0) {
        // File doesn't exist yet - that's okay
        return 0;
    }
    
    // Read header
    checksum_file_header_t header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }
    
    if (header.magic != CHECKSUM_MAGIC) {
        printf("[CHECKSUM] ERROR Invalid checksum file magic\n");
        close(fd);
        return -1;
    }
    
    // Allocate checksum array
    ctx->checksum_capacity = header.checksum_count;
    ctx->checksums = (uint32_t*)calloc(ctx->checksum_capacity, sizeof(uint32_t));
    if (!ctx->checksums) {
        close(fd);
        return -1;
    }
    
    // Read checksums
    if (read(fd, ctx->checksums, header.checksum_count * sizeof(uint32_t)) != 
        (ssize_t)(header.checksum_count * sizeof(uint32_t))) {
        free(ctx->checksums);
        ctx->checksums = NULL;
        close(fd);
        return -1;
    }
    
    close(fd);
    printf("[CHECKSUM] OK Loaded %u checksums from %s\n", header.checksum_count, ctx->checksum_file_path);
    return 0;
}

// Save checksums to file
int checksum_save(checksum_context_t* ctx) {
    if (!ctx || !ctx->enabled) return 0;  // No-op if disabled
    
    int fd = open(ctx->checksum_file_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        printf("[CHECKSUM] ERROR Failed to open checksum file: %s\n", ctx->checksum_file_path);
        return -1;
    }
    
    // Write header
    checksum_file_header_t header = {
        .magic = CHECKSUM_MAGIC,
        .version = CHECKSUM_VERSION,
        .checksum_count = ctx->checksum_capacity,
        .reserved = 0
    };
    
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }
    
    // Write checksums
    if (ctx->checksum_capacity > 0 && ctx->checksums) {
        if (write(fd, ctx->checksums, ctx->checksum_capacity * sizeof(uint32_t)) != 
            (ssize_t)(ctx->checksum_capacity * sizeof(uint32_t))) {
            close(fd);
            return -1;
        }
    }
    
    fsync(fd);
    close(fd);
    
    printf("[CHECKSUM] OK Saved %u checksums to %s\n", ctx->checksum_capacity, ctx->checksum_file_path);
    return 0;
}

