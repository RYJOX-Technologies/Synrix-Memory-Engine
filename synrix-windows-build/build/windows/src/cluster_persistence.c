#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "cluster_persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

// Include Windows compatibility for fsync
#ifdef _WIN32
#include "sys/mman.h"  // Contains fsync implementation for Windows
#endif

// File format:
// [Header: magic, version, cluster_count]
// [Cluster 1: cluster_id, member_count, centroid_dim, radius, timestamp, last_updated, centroid[], member_ids[]]
// [Cluster 2: ...]
// ...

#define CLUSTER_MAGIC 0x434C5553  // "CLUS"
#define CLUSTER_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t cluster_count;
    uint32_t reserved;
} cluster_file_header_t;

// Initialize cluster persistence
int cluster_persistence_init(cluster_persistence_t* ctx, 
                             persistent_lattice_t* lattice,
                             const char* cluster_file_path) {
    if (!ctx || !lattice || !cluster_file_path) return -1;
    
    memset(ctx, 0, sizeof(cluster_persistence_t));
    ctx->lattice = lattice;
    strncpy(ctx->cluster_file_path, cluster_file_path, sizeof(ctx->cluster_file_path) - 1);
    ctx->cluster_file_path[sizeof(ctx->cluster_file_path) - 1] = '\0';
    ctx->cluster_capacity = 100;  // Initial capacity
    ctx->enabled = true;
    
    // Allocate cluster array
    ctx->clusters = (persistent_cluster_t*)calloc(ctx->cluster_capacity, sizeof(persistent_cluster_t));
    if (!ctx->clusters) return -1;
    
    // Try to load existing clusters
    cluster_persistence_load(ctx);
    
    return 0;
}

// Cleanup cluster persistence
void cluster_persistence_cleanup(cluster_persistence_t* ctx) {
    if (!ctx) return;
    
    // Free cluster members
    for (uint32_t i = 0; i < ctx->cluster_count; i++) {
        if (ctx->clusters[i].member_node_ids) {
            free(ctx->clusters[i].member_node_ids);
        }
    }
    
    if (ctx->clusters) {
        free(ctx->clusters);
    }
    
    memset(ctx, 0, sizeof(cluster_persistence_t));
}

// Save clusters to file
int cluster_persistence_save(cluster_persistence_t* ctx) {
    if (!ctx || !ctx->enabled) return -1;
    
    int fd = open(ctx->cluster_file_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        printf("[CLUSTER-PERSIST] ERROR Failed to open cluster file: %s\n", ctx->cluster_file_path);
        return -1;
    }
    
    // Write header
    cluster_file_header_t header = {
        .magic = CLUSTER_MAGIC,
        .version = CLUSTER_VERSION,
        .cluster_count = ctx->cluster_count,
        .reserved = 0
    };
    
    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }
    
    // Write clusters
    for (uint32_t i = 0; i < ctx->cluster_count; i++) {
        persistent_cluster_t* cluster = &ctx->clusters[i];
        
        // Write cluster metadata
        if (write(fd, &cluster->cluster_id, sizeof(uint32_t)) != sizeof(uint32_t)) {
            close(fd);
            return -1;
        }
        if (write(fd, &cluster->member_count, sizeof(uint32_t)) != sizeof(uint32_t)) {
            close(fd);
            return -1;
        }
        if (write(fd, &cluster->centroid_dim, sizeof(uint32_t)) != sizeof(uint32_t)) {
            close(fd);
            return -1;
        }
        if (write(fd, &cluster->radius, sizeof(float)) != sizeof(float)) {
            close(fd);
            return -1;
        }
        if (write(fd, &cluster->created_timestamp, sizeof(uint64_t)) != sizeof(uint64_t)) {
            close(fd);
            return -1;
        }
        if (write(fd, &cluster->last_updated, sizeof(uint64_t)) != sizeof(uint64_t)) {
            close(fd);
            return -1;
        }
        
        // Write centroid
        if (write(fd, cluster->centroid, cluster->centroid_dim * sizeof(float)) != 
            (ssize_t)(cluster->centroid_dim * sizeof(float))) {
            close(fd);
            return -1;
        }
        
        // Write member IDs
        if (cluster->member_count > 0 && cluster->member_node_ids) {
            if (write(fd, cluster->member_node_ids, cluster->member_count * sizeof(uint64_t)) != 
                (ssize_t)(cluster->member_count * sizeof(uint64_t))) {
                close(fd);
                return -1;
            }
        }
    }
    
    fsync(fd);
    close(fd);
    
    printf("[CLUSTER-PERSIST] OK Saved %u clusters to %s\n", ctx->cluster_count, ctx->cluster_file_path);
    return 0;
}

// Load clusters from file
int cluster_persistence_load(cluster_persistence_t* ctx) {
    if (!ctx || !ctx->enabled) return -1;
    
    int fd = open(ctx->cluster_file_path, O_RDONLY);
    if (fd < 0) {
        // File doesn't exist yet - that's okay
        return 0;
    }
    
    // Read header
    cluster_file_header_t header;
    if (read(fd, &header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }
    
    if (header.magic != CLUSTER_MAGIC) {
        printf("[CLUSTER-PERSIST] ERROR Invalid cluster file magic\n");
        close(fd);
        return -1;
    }
    
    ctx->cluster_count = header.cluster_count;
    
    // Ensure capacity
    if (ctx->cluster_count > ctx->cluster_capacity) {
        uint32_t new_capacity = ctx->cluster_count * 2;
        persistent_cluster_t* new_clusters = (persistent_cluster_t*)realloc(ctx->clusters, 
                                                                             new_capacity * sizeof(persistent_cluster_t));
        if (!new_clusters) {
            close(fd);
            return -1;
        }
        ctx->clusters = new_clusters;
        ctx->cluster_capacity = new_capacity;
    }
    
    // Read clusters
    for (uint32_t i = 0; i < ctx->cluster_count; i++) {
        persistent_cluster_t* cluster = &ctx->clusters[i];
        memset(cluster, 0, sizeof(persistent_cluster_t));
        
        // Read cluster metadata
        if (read(fd, &cluster->cluster_id, sizeof(uint32_t)) != sizeof(uint32_t)) {
            close(fd);
            return -1;
        }
        if (read(fd, &cluster->member_count, sizeof(uint32_t)) != sizeof(uint32_t)) {
            close(fd);
            return -1;
        }
        if (read(fd, &cluster->centroid_dim, sizeof(uint32_t)) != sizeof(uint32_t)) {
            close(fd);
            return -1;
        }
        if (read(fd, &cluster->radius, sizeof(float)) != sizeof(float)) {
            close(fd);
            return -1;
        }
        if (read(fd, &cluster->created_timestamp, sizeof(uint64_t)) != sizeof(uint64_t)) {
            close(fd);
            return -1;
        }
        if (read(fd, &cluster->last_updated, sizeof(uint64_t)) != sizeof(uint64_t)) {
            close(fd);
            return -1;
        }
        
        // Read centroid
        if (cluster->centroid_dim > 128) {
            printf("[CLUSTER-PERSIST] ERROR Invalid centroid_dim: %u\n", cluster->centroid_dim);
            close(fd);
            return -1;
        }
        if (read(fd, cluster->centroid, cluster->centroid_dim * sizeof(float)) != 
            (ssize_t)(cluster->centroid_dim * sizeof(float))) {
            close(fd);
            return -1;
        }
        
        // Read member IDs
        if (cluster->member_count > 0) {
            cluster->member_node_ids = (uint64_t*)malloc(cluster->member_count * sizeof(uint64_t));
            if (!cluster->member_node_ids) {
                close(fd);
                return -1;
            }
            if (read(fd, cluster->member_node_ids, cluster->member_count * sizeof(uint64_t)) != 
                (ssize_t)(cluster->member_count * sizeof(uint64_t))) {
                free(cluster->member_node_ids);
                cluster->member_node_ids = NULL;
                close(fd);
                return -1;
            }
        }
    }
    
    close(fd);
    printf("[CLUSTER-PERSIST] OK Loaded %u clusters from %s\n", ctx->cluster_count, ctx->cluster_file_path);
    return 0;
}

// Add cluster
int cluster_persistence_add_cluster(cluster_persistence_t* ctx,
                                    const persistent_cluster_t* cluster) {
    if (!ctx || !cluster) return -1;
    
    // Ensure capacity
    if (ctx->cluster_count >= ctx->cluster_capacity) {
        uint32_t new_capacity = ctx->cluster_capacity * 2;
        persistent_cluster_t* new_clusters = (persistent_cluster_t*)realloc(ctx->clusters, 
                                                                             new_capacity * sizeof(persistent_cluster_t));
        if (!new_clusters) return -1;
        ctx->clusters = new_clusters;
        ctx->cluster_capacity = new_capacity;
    }
    
    // Copy cluster
    persistent_cluster_t* new_cluster = &ctx->clusters[ctx->cluster_count];
    memset(new_cluster, 0, sizeof(persistent_cluster_t));
    new_cluster->cluster_id = cluster->cluster_id;
    new_cluster->member_count = cluster->member_count;
    new_cluster->centroid_dim = cluster->centroid_dim;
    new_cluster->radius = cluster->radius;
    new_cluster->created_timestamp = cluster->created_timestamp;
    new_cluster->last_updated = cluster->last_updated;
    memcpy(new_cluster->centroid, cluster->centroid, cluster->centroid_dim * sizeof(float));
    
    // Copy member IDs
    if (cluster->member_count > 0 && cluster->member_node_ids) {
        new_cluster->member_node_ids = (uint64_t*)malloc(cluster->member_count * sizeof(uint64_t));
        if (!new_cluster->member_node_ids) return -1;
        memcpy(new_cluster->member_node_ids, cluster->member_node_ids, 
               cluster->member_count * sizeof(uint64_t));
    }
    
    ctx->cluster_count++;
    return 0;
}

// Get cluster by ID
persistent_cluster_t* cluster_persistence_get_cluster(cluster_persistence_t* ctx,
                                                      uint32_t cluster_id) {
    if (!ctx) return NULL;
    
    for (uint32_t i = 0; i < ctx->cluster_count; i++) {
        if (ctx->clusters[i].cluster_id == cluster_id) {
            return &ctx->clusters[i];
        }
    }
    
    return NULL;
}

// Get all clusters
uint32_t cluster_persistence_get_all_clusters(cluster_persistence_t* ctx,
                                             persistent_cluster_t** clusters) {
    if (!ctx || !clusters) return 0;
    
    *clusters = ctx->clusters;
    return ctx->cluster_count;
}

