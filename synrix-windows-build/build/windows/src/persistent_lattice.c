#define _GNU_SOURCE
#include "persistent_lattice.h"
#include "license_verify.h"
#include "license_global.h"
#include "wal.h"
#include "isolation.h"
#include "lattice_constraints.h"
#include "dynamic_prefix_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>  // For _get_osfhandle
#endif

static int synrix_verbose_enabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char* env = getenv("SYNRIX_VERBOSE");
        cached = (env && env[0] && strcmp(env, "0") != 0) ? 1 : 0;
    }
    return cached;
}

#define SYNRIX_LOG_INFO(...) do { if (synrix_verbose_enabled()) printf(__VA_ARGS__); } while(0)

// Get current timestamp in microseconds
static uint64_t get_current_timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Forward declaration
void lattice_build_prefix_index(persistent_lattice_t* lattice);

// Forward declaration for prefix extraction
static size_t extract_prefix_from_name(const char* name, char* prefix_out, size_t prefix_max);

// Initialize persistent lattice
int lattice_init(persistent_lattice_t* lattice, const char* storage_path, uint32_t max_nodes, uint32_t device_id) {
    if (!lattice || !storage_path) return -1;
    
    memset(lattice, 0, sizeof(persistent_lattice_t));
    
    // Initialize error tracking
    lattice->last_error = LATTICE_ERROR_NONE;
    
    // Initialize evaluation mode (free tier) - enabled by default
    // Override from signed license (SYNRIX_LICENSE_KEY) if valid
    lattice->evaluation_mode = true;
    #ifdef SYNRIX_FREE_TIER_LIMIT
        lattice->free_tier_limit = SYNRIX_FREE_TIER_LIMIT;  // Use compile-time limit
    #else
        lattice->free_tier_limit = 25000;  // Default 25k node limit for free tier
    #endif
    {
        lattice->license_verified_unlimited = false;
        uint32_t license_limit = 0;
        int license_unlimited = 0;
        if (synrix_license_parse(NULL, &license_limit, &license_unlimited) == 0) {
            if (license_unlimited) {
                lattice->evaluation_mode = false;
                lattice->free_tier_limit = 0;  /* unlimited */
                lattice->license_verified_unlimited = true;
            } else {
                lattice->evaluation_mode = true;
                lattice->free_tier_limit = license_limit;
            }
        }
    }

    // Set device ID (0 = single device, auto-assigns unique ID based on timestamp)
    if (device_id == 0) {
        // Auto-assign device ID based on timestamp (ensures uniqueness)
        lattice->device_id = (uint32_t)(get_current_timestamp() & 0xFFFFFFFF);
    } else {
        lattice->device_id = device_id;
    }
    
    // Set storage path
    strncpy(lattice->storage_path, storage_path, sizeof(lattice->storage_path) - 1);
    lattice->storage_path[sizeof(lattice->storage_path) - 1] = '\0'; // Ensure null termination
    
    // Initialize RAM cache
    // Configurable max_nodes: 0 = use default (10k), otherwise use specified value
    // Default: 10k nodes (~9 MB RAM) for embedded systems
    // Minimal: 1k nodes (~1 MB RAM) for microcontrollers
    // Large: 1.5M nodes (~1.30 GB RAM) for full RAM loading
    if (max_nodes == 0) {
        lattice->max_nodes = 10000; // Default for demo/embedded
    } else {
        lattice->max_nodes = max_nodes;
    }
    lattice->nodes = (lattice_node_t*)malloc(lattice->max_nodes * sizeof(lattice_node_t));
    if (!lattice->nodes) return -1;
    
    // Initialize node ID mapping (64-bit IDs)
    lattice->node_id_map = (uint64_t*)malloc(lattice->max_nodes * sizeof(uint64_t));
    if (!lattice->node_id_map) {
        free(lattice->nodes);
        return -1;
    }

    // Initialize access tracking - NOW DYNAMIC!
    // Use malloc + memset for large allocations (faster than calloc on some systems)
    lattice->access_count = (uint32_t*)malloc(lattice->max_nodes * sizeof(uint32_t));
    if (!lattice->access_count) {
        free(lattice->node_id_map);
        free(lattice->nodes);
        return -1;
    }
    memset(lattice->access_count, 0, lattice->max_nodes * sizeof(uint32_t));

    lattice->last_access = (uint32_t*)malloc(lattice->max_nodes * sizeof(uint32_t));
    if (!lattice->last_access) {
        free(lattice->access_count);
        free(lattice->node_id_map);
        free(lattice->nodes);
        return -1;
    }
    memset(lattice->last_access, 0, lattice->max_nodes * sizeof(uint32_t));

    // Initialize reverse index map (node_id -> array_index) for O(1) lookups
    // Size: max_nodes (or 10k default if max_nodes not specified)
    // Allocate reverse index: need max_nodes + 1 because node IDs go from 1 to max_nodes
    uint32_t initial_index_size = (lattice->max_nodes > 10000) ? (lattice->max_nodes + 1) : 10001;
    lattice->id_to_index_map = (uint32_t*)malloc(initial_index_size * sizeof(uint32_t));
    if (!lattice->id_to_index_map) {
        free(lattice->last_access);
        free(lattice->access_count);
        free(lattice->node_id_map);
        free(lattice->nodes);
        return -1;
    }
    memset(lattice->id_to_index_map, 0, initial_index_size * sizeof(uint32_t));

    // Initialize semantic prefix index
    memset(&lattice->prefix_index, 0, sizeof(lattice->prefix_index));
    lattice->prefix_index.built = false;
    lattice->prefix_index.use_dynamic_index = true;  // Default: use dynamic index (no hardcoding)
    dynamic_prefix_index_init(&lattice->prefix_index.dynamic_index);
    
    // Initialize thread-safety mode (default: single-threaded for performance)
    lattice->thread_safe_mode = false;
    
    // Initialize prefetching (default: enabled for performance)
    lattice->prefetch_enabled = true;
    
    // Initialize persistence configuration (production defaults)
    lattice->persistence.auto_save_enabled = true;
    lattice->persistence.auto_save_interval_nodes = 5000;  // Save every 5k nodes (good balance for indexing)
    lattice->persistence.auto_save_interval_seconds = 300; // Save every 5 minutes
    lattice->persistence.save_on_memory_pressure = true;    // Save when RAM fills
    lattice->persistence.nodes_since_last_save = 0;
    lattice->persistence.last_save_timestamp = get_current_timestamp();
    
    // Enable WAL by default for crash recovery (production requirement)
    if (lattice_enable_wal(lattice) != 0) {
        printf("[LATTICE] WARN Failed to enable WAL (continuing without crash recovery)\n");
    }
    
    // Try to load existing lattice
    // Note: lattice_load() will respect max_nodes and only load up to that limit
    int load_result = lattice_load(lattice);
    if (load_result != 0) {
        // Create new lattice file
        lattice->next_id = 1;  // Local ID starts at 1 (will be combined with device_id)
        lattice->total_nodes = 0;
        lattice->dirty = false;
    }

    /* Global license usage: register this lattice's node count (one cap per license per machine) */
    if (lattice->evaluation_mode && lattice->free_tier_limit > 0 && lattice->total_nodes > 0) {
        (void)license_global_register(lattice->total_nodes, lattice->free_tier_limit);
    }

    // Build semantic prefix index after loading (O(n) once, then O(k) queries)
    // Only build if we have nodes to index
    if (lattice->node_count > 0) {
        lattice_build_prefix_index(lattice);
    }
    
    // Initialize mmap for streaming access (optional, can be done later)
    // For now, mmap is initialized lazily on first streaming access
    lattice->storage_fd = -1;
    lattice->mmap_ptr = NULL;
    lattice->mmap_size = 0;
    
    // Open persistent file descriptor for reads (eliminates open/close overhead)
    // This significantly improves p99 latency for prefix searches on cache misses
    if (lattice->storage_path[0] != '\0') {
        lattice->storage_fd = open(lattice->storage_path, O_RDONLY);
        // If file doesn't exist yet, that's OK - it will be created on first save
        // We'll reopen it when needed (after first save)
    }
    
    return 0;
}

// Initialize lattice in disk mode (file-backed memory with MAP_SHARED)
// This enables kernel-managed dirty page flushing (Leaky Bucket strategy)
int lattice_init_disk_mode(persistent_lattice_t* lattice, const char* storage_path, 
                           uint32_t max_nodes, uint32_t total_file_nodes, uint32_t device_id) {
    if (!lattice || !storage_path || total_file_nodes == 0) return -1;
    
    memset(lattice, 0, sizeof(persistent_lattice_t));
    
    // Set device ID (0 = single device, auto-assigns unique ID)
    if (device_id == 0) {
        lattice->device_id = (uint32_t)(get_current_timestamp() & 0xFFFFFFFF);
    } else {
        lattice->device_id = device_id;
    }
    
    // Set storage path
    strncpy(lattice->storage_path, storage_path, sizeof(lattice->storage_path) - 1);
    lattice->storage_path[sizeof(lattice->storage_path) - 1] = '\0';
    printf("[LATTICE-INIT-DISK] Storage path: %s\n", lattice->storage_path);
    printf("[LATTICE-INIT-DISK] Pre-allocating %u nodes (%.2f GB) in file-backed memory\n",
           total_file_nodes, (total_file_nodes * sizeof(lattice_node_t)) / (1024.0 * 1024.0 * 1024.0));
    printf("[LATTICE-INIT-DISK] RAM cache limit: %u nodes (%.2f GB) for metadata\n",
           max_nodes, (max_nodes * sizeof(lattice_node_t)) / (1024.0 * 1024.0 * 1024.0));
    fflush(stdout);
    
    // Set disk mode flag
    lattice->disk_mode = true;
    lattice->total_file_nodes = total_file_nodes;
    
    // Set RAM cache size (for metadata arrays)
    if (max_nodes == 0) {
        lattice->max_nodes = 10000; // Default
    } else {
        lattice->max_nodes = max_nodes;
    }
    
    // Calculate file size (header + nodes)
    // Header: 4 uint32_t = 16 bytes
    size_t header_size = 4 * sizeof(uint32_t);
    size_t file_size = header_size + (total_file_nodes * sizeof(lattice_node_t));
    
    // 1. Check if file exists and get its size
    struct stat st;
    bool file_exists = (stat(storage_path, &st) == 0);
    size_t existing_size = file_exists ? st.st_size : 0;
    
    // 2. Open/create the file (don't truncate if it exists and is the right size)
    int fd;
    if (file_exists && existing_size == file_size) {
        // File exists and is correct size - open without truncating
        fd = open(storage_path, O_RDWR, 0644);
        printf("[LATTICE-INIT-DISK] OK Using existing file (%.2f GB)\n", file_size / (1024.0 * 1024.0 * 1024.0));
    } else {
        // New file or wrong size - create/truncate and pre-allocate
        fd = open(storage_path, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("[LATTICE-INIT-DISK] Failed to open file");
            return -1;
        }
        
        // STRETCH the file to full size (CRITICAL: reserves disk space)
        // ROOT CAUSE OF CORRUPTION: ftruncate() fills the file with zeros (uninitialized memory).
        // This creates slots with id=0 that appear as "corruption" when loading.
        // FIX: We now skip id=0 slots during indexing and ensure header[3] matches total_nodes.
        printf("[LATTICE-INIT-DISK] Pre-allocating %.2f GB on disk...\n", file_size / (1024.0 * 1024.0 * 1024.0));
        fflush(stdout);
        if (ftruncate(fd, file_size) == -1) {
            perror("[LATTICE-INIT-DISK] ftruncate failed");
            close(fd);
            return -1;
        }
        printf("[LATTICE-INIT-DISK] OK File pre-allocated (uninitialized slots will be skipped during indexing)\n");
        fflush(stdout);
    }
    
    if (fd < 0) {
        perror("[LATTICE-INIT-DISK] Failed to open file");
        return -1;
    }
    
    // 3. Map the file with MAP_SHARED (enables kernel-managed dirty page flushing)
    // mmap requires page-aligned offset, so we'll map from the start and adjust the pointer
    (void)sysconf(_SC_PAGESIZE); // Page size info (kept for documentation)
    size_t nodes_size = total_file_nodes * sizeof(lattice_node_t);
    
    // Map the entire file (header + nodes) to ensure page alignment
    void* map_addr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map_addr == MAP_FAILED) {
        perror("[LATTICE-INIT-DISK] mmap failed");
        close(fd);
        return -1;
    }
    
    printf("[LATTICE-INIT-DISK] OK Memory-mapped %.2f GB with MAP_SHARED\n", file_size / (1024.0 * 1024.0 * 1024.0));
    fflush(stdout);
    
    // 4. Point nodes array to mmap'd memory (skip header)
    lattice->nodes = (lattice_node_t*)((char*)map_addr + header_size);
    lattice->mmap_ptr = map_addr;
    lattice->mmap_size = file_size;  // Store full file size for cleanup
    
    // 5. Advise kernel about access pattern (sequential writes)
    // This helps the kernel optimize page allocation and flushing
    // For large files, use MADV_SEQUENTIAL to hint sequential access
    // For smaller files (<1GB), we can prefetch aggressively
    if (file_size < (1024ULL * 1024 * 1024)) {
        // Small file: prefetch first 100MB to reduce initial page faults
        size_t prefetch_size = (file_size < (100 * 1024 * 1024)) ? file_size : (100 * 1024 * 1024);
        madvise(map_addr, prefetch_size, MADV_WILLNEED);
    } else {
        // Large file: use sequential hint (kernel will prefetch as we write)
        madvise(map_addr, file_size, MADV_SEQUENTIAL);
    }
    lattice->storage_fd = fd;
    
    // 5. Allocate metadata arrays in RAM (these stay in RAM)
    printf("[LATTICE-INIT-DISK] Allocating metadata arrays in RAM...\n");
    fflush(stdout);
    
    lattice->node_id_map = (uint64_t*)malloc(lattice->max_nodes * sizeof(uint64_t));
    if (!lattice->node_id_map) {
        munmap(map_addr, nodes_size);
        close(fd);
        return -1;
    }
    
    lattice->access_count = (uint32_t*)calloc(lattice->max_nodes, sizeof(uint32_t));
    if (!lattice->access_count) {
        free(lattice->node_id_map);
        munmap(map_addr, nodes_size);
        close(fd);
        return -1;
    }
    
    lattice->last_access = (uint32_t*)calloc(lattice->max_nodes, sizeof(uint32_t));
    if (!lattice->last_access) {
        free(lattice->access_count);
        free(lattice->node_id_map);
        munmap(map_addr, nodes_size);
        close(fd);
        return -1;
    }
    
    // Allocate reverse index (growable, but start with reasonable size)
    uint32_t initial_index_size = (lattice->max_nodes > 10000) ? (lattice->max_nodes + 1) : 10001;
    lattice->id_to_index_map = (uint32_t*)calloc(initial_index_size, sizeof(uint32_t));
    if (!lattice->id_to_index_map) {
        free(lattice->last_access);
        free(lattice->access_count);
        free(lattice->node_id_map);
        munmap(map_addr, nodes_size);
        close(fd);
        return -1;
    }
    
    printf("[LATTICE-INIT-DISK] OK Metadata arrays allocated\n");
    fflush(stdout);
    
    // Initialize semantic prefix index
    memset(&lattice->prefix_index, 0, sizeof(lattice->prefix_index));
    lattice->prefix_index.built = false;
    lattice->prefix_index.use_dynamic_index = true;  // Default: use dynamic index (no hardcoding)
    dynamic_prefix_index_init(&lattice->prefix_index.dynamic_index);
    
    // Initialize thread-safety and prefetching
    lattice->thread_safe_mode = false;
    lattice->prefetch_enabled = true;
    
    // Initialize persistence configuration (production defaults)
    lattice->persistence.auto_save_enabled = true;
    lattice->persistence.auto_save_interval_nodes = 5000;  // Save every 5k nodes (good balance for indexing)
    lattice->persistence.auto_save_interval_seconds = 300; // Save every 5 minutes
    lattice->persistence.save_on_memory_pressure = true;    // Save when RAM fills
    lattice->persistence.nodes_since_last_save = 0;
    lattice->persistence.last_save_timestamp = get_current_timestamp();
    
    // Enable WAL by default for crash recovery (production requirement)
    if (lattice_enable_wal(lattice) != 0) {
        printf("[LATTICE-INIT-DISK] WARN Failed to enable WAL (continuing without crash recovery)\n");
    }
    
    // Check if file already exists and load header
    // In disk mode, nodes are already accessible via mmap - we just need to read the header
    // (file_exists was already checked above)
    
    if (file_exists && existing_size == file_size) {
        // File exists and is correct size - read header to get existing state
        uint32_t header[4];
        ssize_t read_bytes = pread(fd, header, sizeof(header), 0);
        if (read_bytes == sizeof(header)) {
            if (header[0] == 0x4C415454) {
                // Valid header found - restore state
                lattice->total_nodes = header[1];
                lattice->next_id = header[2];
                uint32_t nodes_in_file = header[3];
                printf("[LATTICE-INIT-DISK] OK Found existing file: %u nodes, next_id=%lu\n",
                       lattice->total_nodes, lattice->next_id);
            
            // Build RAM cache index for first max_nodes (for fast lookups)
            // Nodes are already in mmap'd memory, we just need to index them
            uint32_t nodes_to_index = (nodes_in_file < lattice->max_nodes) ? nodes_in_file : lattice->max_nodes;
            lattice->node_count = 0;
            
            // Header size is 4 * sizeof(uint32_t) = 16 bytes (documented for reference)
            // CRITICAL FIX: Only index initialized nodes (skip id=0 slots from pre-allocation)
            for (uint32_t i = 0; i < nodes_to_index; i++) {
                // Access node directly from mmap'd memory
                lattice_node_t* node = &lattice->nodes[i];
                
                // Skip uninitialized nodes (id=0) - these are from file pre-allocation
                // File is pre-allocated with zeros, but only nodes up to total_nodes are actually written
                if (node->id == 0) {
                    // Uninitialized slot - skip it (file was pre-allocated but this slot wasn't used)
                    continue;
                }
                
                // Validate node before indexing (prevent indexing corrupted nodes)
                uint32_t local_id = (uint32_t)(node->id & 0xFFFFFFFF);
                uint32_t max_safe_id = lattice->max_nodes * 10;
                if (local_id > max_safe_id) {
                    // Check if it's a valid chunked node (uses "C:" prefix)
                    if (strncmp(node->name, "C:", 2) != 0 && 
                        strncmp(node->name, "CHUNK:", 6) != 0) {
                        // Invalid large ID, skip
                        continue;
                    }
                }
                
                // Node is valid - index it
                lattice->node_id_map[lattice->node_count] = node->id;
                if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
                    lattice->id_to_index_map[local_id] = lattice->node_count;
                }
                
                lattice->node_count++;
            }
            
                printf("[LATTICE-INIT-DISK] OK Indexed %u nodes in RAM cache (of %u total in file)\n",
                       lattice->node_count, lattice->total_nodes);
            } else {
                // Invalid magic - treat as new
                printf("[LATTICE-INIT-DISK] WARN File exists but invalid magic (0x%08X), initializing as new\n", header[0]);
                lattice->node_count = 0;
                lattice->next_id = 1;
                lattice->total_nodes = 0;
                
                // Write initial header
                uint32_t new_header[4] = {
                    0x4C415454, // "LATT" magic
                    0,          // total_nodes
                    1,          // next_id
                    0           // node_count
                };
                (void)pwrite(fd, new_header, sizeof(new_header), 0);
            }
        } else {
            // Failed to read header
            printf("[LATTICE-INIT-DISK] WARN Failed to read header (read %zd bytes), initializing as new\n", read_bytes);
            lattice->node_count = 0;
            lattice->next_id = 1;
            lattice->total_nodes = 0;
        }
    } else {
        // New file - initialize state
        lattice->node_count = 0;
        lattice->next_id = 1;
        lattice->total_nodes = 0;
        
        // Write initial header to file
        uint32_t header[4] = {
            0x4C415454, // "LATT" magic
            0,          // total_nodes (will be updated on save)
            1,          // next_id
            0           // node_count (will be updated on save)
        };
        
        if (pwrite(fd, header, sizeof(header), 0) != sizeof(header)) {
            perror("[LATTICE-INIT-DISK] Failed to write header");
            // Continue anyway - header will be written on first save
        }
    }
    
    lattice->dirty = false;
    
    printf("[LATTICE-INIT-DISK] OK Disk mode initialized (kernel will manage dirty page flushing)\n");
    fflush(stdout);
    
    return 0;
}

// Cleanup persistent lattice
void lattice_cleanup(persistent_lattice_t* lattice) {
    if (!lattice) return;
    
    // Cleanup WAL FIRST (before unmapping memory, as WAL might access lattice)
    // This ensures WAL background thread stops before we unmap memory
    if (lattice->wal) {
        wal_cleanup(lattice->wal);
        free(lattice->wal);
        lattice->wal = NULL;
    }
    
    // Save if dirty (after WAL cleanup, so WAL doesn't try to write to unmapped memory)
    if (lattice->dirty) {
        lattice_save(lattice);
    }
    
    // In disk mode: unmap mmap'd memory (don't free, it's file-backed)
    // In RAM mode: free malloc'd memory
    if (lattice->disk_mode) {
        // Unmap mmap'd memory
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
            // Sync dirty pages before unmapping
            msync(lattice->mmap_ptr, lattice->mmap_size, MS_SYNC);
            munmap(lattice->mmap_ptr, lattice->mmap_size);
            lattice->mmap_ptr = NULL;
            lattice->mmap_size = 0;
        }
        // Close file descriptor
        if (lattice->storage_fd >= 0) {
            close(lattice->storage_fd);
            lattice->storage_fd = -1;
        }
        // nodes pointer points to mmap'd memory, don't free it
        lattice->nodes = NULL;
    } else {
        // RAM mode: free malloc'd memory
        if (lattice->nodes) {
            for (uint32_t i = 0; i < lattice->node_count; i++) {
                if (lattice->nodes[i].children) {
                    free(lattice->nodes[i].children);
                }
            }
            free(lattice->nodes);
        }
        // Unmap memory (if any)
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
            munmap(lattice->mmap_ptr, lattice->mmap_size);
        }
        // Close file descriptor
        if (lattice->storage_fd >= 0) {
            close(lattice->storage_fd);
        }
    }
    
    // Free semantic prefix index
    if (lattice->prefix_index.isa_ids) free(lattice->prefix_index.isa_ids);
    if (lattice->prefix_index.material_ids) free(lattice->prefix_index.material_ids);
    if (lattice->prefix_index.learning_ids) free(lattice->prefix_index.learning_ids);
    if (lattice->prefix_index.performance_ids) free(lattice->prefix_index.performance_ids);
    
    // Cleanup dynamic prefix index
    dynamic_prefix_index_cleanup(&lattice->prefix_index.dynamic_index);

    // Free dynamic arrays
    if (lattice->access_count) {
        free(lattice->access_count);
    }
    if (lattice->last_access) {
        free(lattice->last_access);
    }
    if (lattice->id_to_index_map) {
        free(lattice->id_to_index_map);
    }
    
    if (lattice->node_id_map) {
        free(lattice->node_id_map);
        lattice->node_id_map = NULL;
    }
    
    // Cleanup isolation if enabled
    if (lattice->isolation) {
        isolation_cleanup(lattice->isolation);
        free(lattice->isolation);
        lattice->isolation = NULL;
    }
    
    memset(lattice, 0, sizeof(persistent_lattice_t));
}

// Save lattice to disk
int lattice_save(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    // In disk mode: nodes are already in file via mmap, just update header and sync
    if (lattice->disk_mode) {
        if (lattice->storage_fd < 0) return -1;
        
        // Update header
        // ROOT CAUSE OF CORRUPTION: File is pre-allocated with ftruncate() which fills it with zeros.
        // When we write nodes sequentially, we only write up to total_nodes, leaving slots beyond
        // that as zeros (id=0). The header[3] (nodes_to_load) must match total_nodes to prevent
        // loading uninitialized slots. This is now correctly set to total_nodes.
        uint32_t header[4] = {
            0x4C415454, // "LATT" magic
            lattice->total_nodes,  // total_nodes = actual nodes written
            lattice->next_id,
            lattice->total_nodes  // nodes_to_load = same as total_nodes (prevents loading uninitialized slots)
        };
        
        if (pwrite(lattice->storage_fd, header, sizeof(header), 0) != sizeof(header)) {
            return -1;
        }
        
        // WINDOWS FIX: Follow the "Commit-Before-Close" rule
        // Order is critical: FlushViewOfFile -> UnmapViewOfFile -> FlushFileBuffers -> CloseHandle
        #ifdef _WIN32
            // Step 1: Flush the memory-mapped view to ensure dirty pages are written
            if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && 
                lattice->mmap_size > 0 && lattice->total_nodes > 0) {
                // Calculate the size of the written portion (header + nodes written so far)
                size_t header_size = 4 * sizeof(uint32_t);
                size_t written_nodes_size = lattice->total_nodes * sizeof(lattice_node_t);
                size_t written_size = header_size + written_nodes_size;
                
                // Don't sync more than we mapped
                if (written_size > lattice->mmap_size) {
                    written_size = lattice->mmap_size;
                }
                
                // FlushViewOfFile forces dirty pages to be written to disk
                if (!FlushViewOfFile(lattice->mmap_ptr, written_size)) {
                    fprintf(stderr, "[LATTICE-SAVE] Failed to flush memory view: Windows error %lu\n", GetLastError());
                    return -1;
                }
            }
            
            // Step 2: Unmap the view (Windows requires this before closing)
            if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
                UnmapViewOfFile(lattice->mmap_ptr);
                lattice->mmap_ptr = MAP_FAILED;
                lattice->mmap_size = 0;
            }
            
            // Step 3: Flush file buffers (forces OS cache to commit to disk)
            HANDLE hFile = (HANDLE)_get_osfhandle(lattice->storage_fd);
            if (hFile != INVALID_HANDLE_VALUE) {
                if (!FlushFileBuffers(hFile)) {
                    fprintf(stderr, "[LATTICE-SAVE] Failed to flush file buffers: Windows error %lu\n", GetLastError());
                    return -1;
                }
            }
        #else
            // POSIX: Sync the mmap'd region
            if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && 
                lattice->mmap_size > 0 && lattice->total_nodes > 0) {
                // Calculate the size of the written portion (header + nodes written so far)
                size_t header_size = 4 * sizeof(uint32_t);
                size_t written_nodes_size = lattice->total_nodes * sizeof(lattice_node_t);
                size_t written_size = header_size + written_nodes_size;
                
                // Don't sync more than we mapped
                if (written_size > lattice->mmap_size) {
                    written_size = lattice->mmap_size;
                }
                
                // Only sync the portion we've written (kernel will only sync dirty pages)
                msync(lattice->mmap_ptr, written_size, MS_SYNC);
            }
            
            // Force header to disk
            fsync(lattice->storage_fd);
        #endif
        
        lattice->dirty = false;
        
        return 0;
    }
    
    // RAM mode: write nodes to file (ATOMIC: write to temp, then rename)
    // Production requirement: Atomic saves prevent corruption from partial writes
    
    // Create temp file path
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", lattice->storage_path);
    
    // Open temp file for writing (O_RDWR needed on Windows for read-back verification)
    // CRITICAL: O_BINARY prevents Windows from converting \n to \r\n (which adds 2 bytes!)
    #ifdef _WIN32
        int fd = open(temp_path, O_CREAT | O_RDWR | O_TRUNC | O_BINARY, 0644);
    #else
        int fd = open(temp_path, O_CREAT | O_RDWR | O_TRUNC, 0644);
    #endif
    if (fd < 0) {
        perror("[LATTICE-SAVE] Failed to create temp file");
        return -1;
    }
    
    // Write header
    // CRITICAL: In RAM mode, we write exactly node_count nodes, so header[1] and header[3] must both be node_count
    // This ensures the file is a clean snapshot with no uninitialized slots
    // DEBUG: Always print values to diagnose corruption
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Writing header - node_count=%u, next_id=%llu, total_nodes=%u\n", 
    //         lattice->node_count, (unsigned long long)lattice->next_id, lattice->total_nodes);
    // fflush(stderr);
    
    if (lattice->node_count > 1000000 || lattice->next_id > 1000000000ULL) {
        fprintf(stderr, "[LATTICE-SAVE] WARN Suspicious values: node_count=%u, next_id=%llu\n", 
                lattice->node_count, (unsigned long long)lattice->next_id);
        fflush(stderr);
        // Don't save with corrupted values - return error
        return -1;
    }
    uint32_t header[4] = {
        0x4C415454, // "LATT" magic
        lattice->node_count,  // total_nodes = node_count (we write exactly this many)
        (uint32_t)lattice->next_id,  // Cast to uint32_t (next_id is uint64_t, but header only has 32 bits)
        lattice->node_count  // nodes_to_load = node_count (we wrote exactly this many)
    };
    
    ssize_t written = write(fd, header, sizeof(header));
    if (written != sizeof(header)) {
        fprintf(stderr, "[LATTICE-SAVE] ERROR Failed to write header: wrote %zd bytes, expected %zu\n", written, sizeof(header));
        fflush(stderr);
        close(fd);
        unlink(temp_path);
        return -1;
    }
    
    // CRITICAL: On Windows, we must flush before reading back
    #ifdef _WIN32
        HANDLE hFile = (HANDLE)_get_osfhandle(fd);
        if (hFile != INVALID_HANDLE_VALUE) {
            FlushFileBuffers(hFile);
        }
    #else
        fsync(fd);
    #endif
    
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Wrote header successfully: magic=0x%08X, total_nodes=%u, next_id=%u, nodes_to_load=%u\n",
    //         header[0], header[1], header[2], header[3]);
    // fflush(stderr);
    
    // CRITICAL: Verify header immediately after write (before closing fd)
    // Check current file position
    off_t current_pos = lseek(fd, 0, SEEK_CUR);
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Current file position after write: %lld\n", (long long)current_pos);
    // fflush(stderr);
    
    // Reset file position to beginning
    off_t seek_result = lseek(fd, 0, SEEK_SET);
    if (seek_result != 0) {
        fprintf(stderr, "[LATTICE-SAVE] ERROR Failed to seek to beginning: seek_result=%lld, errno=%d\n",
                (long long)seek_result, errno);
        fflush(stderr);
        close(fd);
        unlink(temp_path);
        return -1;
    }
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Seek to beginning successful, position now: %lld\n", (long long)seek_result);
    // fflush(stderr);
    
    // Check file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: File size after write: %lld bytes (expected at least %zu)\n",
    //         (long long)file_size, sizeof(header));
    // fflush(stderr);
    lseek(fd, 0, SEEK_SET);  // Reset to beginning again
    
    uint32_t immediate_verify_header[4];
    memset(immediate_verify_header, 0, sizeof(immediate_verify_header));  // Zero out first
    ssize_t verify_bytes = read(fd, immediate_verify_header, sizeof(immediate_verify_header));
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Read attempt: %zd bytes, errno=%d\n", verify_bytes, errno);
    // fflush(stderr);
    if (verify_bytes != sizeof(immediate_verify_header)) {
        fprintf(stderr, "[LATTICE-SAVE] ERROR Header verification failed: read %zd bytes (expected %zu), errno=%d\n",
                verify_bytes, sizeof(immediate_verify_header), errno);
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Partial read content (hex): ");
        for (int i = 0; i < (verify_bytes > 0 ? verify_bytes : 0); i++) {
            fprintf(stderr, "%02X ", ((uint8_t*)immediate_verify_header)[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        close(fd);
        unlink(temp_path);
        return -1;
    }
    if (immediate_verify_header[0] != header[0] || immediate_verify_header[1] != header[1] || 
        immediate_verify_header[2] != header[2] || immediate_verify_header[3] != header[3]) {
        fprintf(stderr, "[LATTICE-SAVE] ERROR Header mismatch! Written: [0x%08X, %u, %u, %u], Read: [0x%08X, %u, %u, %u]\n",
                header[0], header[1], header[2], header[3],
                immediate_verify_header[0], immediate_verify_header[1], immediate_verify_header[2], immediate_verify_header[3]);
        fflush(stderr);
        close(fd);
        unlink(temp_path);
        return -1;
    }
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Header verified immediately after write\n");
    // fflush(stderr);
    
    // Seek back to end for node writes
    lseek(fd, sizeof(header), SEEK_SET);
    
    // Write nodes - only write valid nodes (id != 0)
    // This ensures we don't write uninitialized slots, preventing corruption warnings on load
    uint32_t valid_nodes_written = 0;
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip uninitialized nodes (id == 0)
        if (node->id == 0) {
            continue;
        }
        
        // Write node structure
        ssize_t node_bytes = write(fd, node, sizeof(lattice_node_t));
        if (node_bytes != sizeof(lattice_node_t)) {
            fprintf(stderr, "[LATTICE-SAVE] ERROR Failed to write node %u: wrote %zd bytes (expected %zu), errno=%d\n",
                    i, node_bytes, sizeof(lattice_node_t), errno);
            fflush(stderr);
            close(fd);
            unlink(temp_path);
            return -1;
        }
        valid_nodes_written++;
    }
    
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Wrote %u nodes, file position now: %lld\n",
    //         valid_nodes_written, (long long)lseek(fd, 0, SEEK_CUR));
    // fflush(stderr);
    
    // Update header to reflect actual number of valid nodes written
    // This prevents loading uninitialized slots and eliminates false corruption warnings
    if (valid_nodes_written != lattice->node_count) {
        // Rewrite header with correct count
        lseek(fd, 0, SEEK_SET);
        uint32_t corrected_header[4] = {
            0x4C415454, // "LATT" magic
            valid_nodes_written,  // total_nodes = actual valid nodes written
            lattice->next_id,
            valid_nodes_written  // nodes_to_load = same as valid_nodes_written
        };
        if (write(fd, corrected_header, sizeof(corrected_header)) != sizeof(corrected_header)) {
            close(fd);
            unlink(temp_path);
            return -1;
        }
    }
    
    // Force data to disk before renaming (critical for crash safety)
    fsync(fd);
    
    // DEBUG: Verify temp file header before atomic replace
    lseek(fd, 0, SEEK_SET);
    uint32_t verify_header[4];
    if (read(fd, verify_header, sizeof(verify_header)) == sizeof(verify_header)) {
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Temp file header before replace: magic=0x%08X, total_nodes=%u, next_id=%u, nodes_to_load=%u\n",
    //             verify_header[0], verify_header[1], verify_header[2], verify_header[3]);
    //     fflush(stderr);
    }
    close(fd);
    
    // ATOMIC RENAME: Replace old file with new one (atomic on most filesystems)
    // This is a full snapshot save - writes all nodes to temp file, then atomically replaces main file
    // Note: This is NOT deleting and recreating - it's an atomic file replacement
    #ifdef _WIN32
        // WINDOWS FIX: Follow the "Commit-Before-Close" rule
        // Order is critical: FlushViewOfFile -> UnmapViewOfFile -> FlushFileBuffers -> CloseHandle -> Verify -> Replace
        
        // Step 1: Flush the memory-mapped view if it exists (before unmapping)
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && lattice->mmap_size > 0) {
            if (!FlushViewOfFile(lattice->mmap_ptr, lattice->mmap_size)) {
                fprintf(stderr, "[LATTICE-SAVE] Failed to flush memory view: Windows error %lu\n", GetLastError());
                unlink(temp_path);
                return -1;
            }
        }
        
        // Step 2: Unmap the memory-mapped region if it exists
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
            UnmapViewOfFile(lattice->mmap_ptr);
            lattice->mmap_ptr = MAP_FAILED;
            lattice->mmap_size = 0;
        }
        
        // Step 3: Flush file buffers for the main file if it's open
        if (lattice->storage_fd >= 0) {
            HANDLE hFile = (HANDLE)_get_osfhandle(lattice->storage_fd);
            if (hFile != INVALID_HANDLE_VALUE) {
                FlushFileBuffers(hFile);
            }
        }
        
        // Step 4: Close the file descriptor if it's open
        if (lattice->storage_fd >= 0) {
            close(lattice->storage_fd);
            lattice->storage_fd = -1;
        }
        
        // Step 5: Verify temp file before replace
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Attempting to verify temp file: %s\n", temp_path);
    // fflush(stderr);
        int verify_fd = open(temp_path, O_RDONLY);
        if (verify_fd >= 0) {
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Opened temp file for verification\n");
    // fflush(stderr);
            uint32_t verify_header[4];
            ssize_t bytes_read = read(verify_fd, verify_header, sizeof(verify_header));
            if (bytes_read == sizeof(verify_header)) {
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Temp file header before replace: magic=0x%08X, total_nodes=%u, next_id=%u, nodes_to_load=%u\n",
    //                     verify_header[0], verify_header[1], verify_header[2], verify_header[3]);
    //             fflush(stderr);
            } else {
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Failed to read temp file header: read %zd bytes (expected %zu)\n", bytes_read, sizeof(verify_header));
    // fflush(stderr);
            }
            close(verify_fd);
        } else {
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Failed to open temp file for verification: errno=%d\n", errno);
    // fflush(stderr);
        }
        
        // Step 6: Now we can safely replace the file
        wchar_t temp_path_w[512];
        wchar_t storage_path_w[512];
        MultiByteToWideChar(CP_UTF8, 0, temp_path, -1, temp_path_w, 512);
        MultiByteToWideChar(CP_UTF8, 0, lattice->storage_path, -1, storage_path_w, 512);
        
        // MoveFileEx with MOVEFILE_REPLACE_EXISTING atomically replaces the file
        // This now works because we've flushed, unmapped, and closed the file
        BOOL replace_result = MoveFileExW(temp_path_w, storage_path_w, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if (!replace_result) {
            DWORD error = GetLastError();
            fprintf(stderr, "[LATTICE-SAVE] ERROR Failed to replace file: Windows error %lu\n", error);
            fflush(stderr);
            unlink(temp_path);
            return -1;
        }
    // fprintf(stderr, "[LATTICE-SAVE] DEBUG: Atomic replace succeeded\n");
    // fflush(stderr);
        
        // Step 6: VERIFY the file was actually replaced (check file size)
        // If the replace failed silently, the file will be 0 bytes or wrong size
        struct stat st;
        if (stat(lattice->storage_path, &st) == 0) {
            // Calculate expected file size: header (16 bytes) + nodes
            size_t expected_size = sizeof(uint32_t) * 4 + (valid_nodes_written * sizeof(lattice_node_t));
            if (st.st_size == 0 || (st.st_size < expected_size && st.st_size < sizeof(uint32_t) * 4)) {
                fprintf(stderr, "[LATTICE-SAVE] WARN File replace verification failed: file size is %lld, expected at least %zu\n", 
                        (long long)st.st_size, expected_size);
                // Don't return error - file might be valid but smaller (if nodes were compacted)
                // But log the warning so we know something might be wrong
            }
        }
    #else
        // POSIX: rename() is atomic and replaces the file atomically
        if (rename(temp_path, lattice->storage_path) != 0) {
            perror("[LATTICE-SAVE] Failed to rename temp file");
            unlink(temp_path);
            return -1;
        }
    #endif
    
    // Reopen read FD if it wasn't open (file now exists after first save)
    if (lattice->storage_fd < 0) {
        lattice->storage_fd = open(lattice->storage_path, O_RDONLY);
    }
    
    // Remap if file grew (mmap needs to cover new size)
    if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
        struct stat st;
        if (stat(lattice->storage_path, &st) == 0 && st.st_size > lattice->mmap_size) {
            // File grew - remap to new size
            munmap(lattice->mmap_ptr, lattice->mmap_size);
            lattice->mmap_size = st.st_size;
            int fd = (lattice->storage_fd >= 0) ? lattice->storage_fd : open(lattice->storage_path, O_RDONLY);
            if (fd >= 0) {
                lattice->mmap_ptr = mmap(NULL, lattice->mmap_size, PROT_READ, MAP_SHARED, fd, 0);
                if (lattice->mmap_ptr == MAP_FAILED) {
                    lattice->mmap_ptr = NULL;
                    lattice->mmap_size = 0;
                } else {
                    if (lattice->storage_fd < 0) {
                        lattice->storage_fd = fd;
                    } else {
                        close(fd);
                    }
                }
            }
        }
    }
    
    lattice->dirty = false;
    lattice->persistence.nodes_since_last_save = 0;
    lattice->persistence.last_save_timestamp = get_current_timestamp();
    
    return 0;
}

// Load lattice from disk
int lattice_load(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    // Open file for reading (O_BINARY needed on Windows to prevent text mode conversion)
    #ifdef _WIN32
        int fd = open(lattice->storage_path, O_RDONLY | O_BINARY);
    #else
        int fd = open(lattice->storage_path, O_RDONLY);
    #endif
    if (fd < 0) {
        // File doesn't exist yet - this is normal for new lattices, not an error
        if (errno == ENOENT) {
            // Silent return for new files (expected case)
            return -1;
        }
        printf("[LATTICE-LOAD] ERROR Failed to open file: %s (errno=%d)\n", lattice->storage_path, errno);
        return -1;
    }
    SYNRIX_LOG_INFO("[LATTICE-LOAD] OK Opened file: %s\n", lattice->storage_path);
    
    // Read header
    uint32_t header[4];
    if (read(fd, header, sizeof(header)) != sizeof(header)) {
        close(fd);
        return -1;
    }

    // Verify magic
    if (header[0] != 0x4C415454) {
        printf("[LATTICE-LOAD] ERROR Invalid magic: expected 0x4C415454, got 0x%08X\n", header[0]);
        close(fd);
        return -1;
    }
    
    lattice->total_nodes = header[1];
    lattice->next_id = header[2];
    uint32_t nodes_to_load = header[3];
    
    // Load nodes into RAM cache
    lattice->node_count = 0;
    // CRITICAL FIX: Add safety check to prevent infinite loop from corrupted data
    // Respect max_nodes (RAM limit) - this allows testing streaming mode
    uint32_t max_safe_nodes = (nodes_to_load < lattice->max_nodes) ? nodes_to_load : lattice->max_nodes;
    // Remove hard 100k limit - use max_nodes instead (allows up to 1.5M+ nodes)
    // if (max_safe_nodes > 100000) max_safe_nodes = 100000;  // Hard limit - REMOVED
    
    uint32_t corrupted_nodes_skipped = 0;
    uint32_t consecutive_uninitialized = 0;
    uint32_t consecutive_invalid = 0;  // Track consecutive invalid nodes (uninitialized or misaligned)
    const uint32_t MAX_CONSECUTIVE_UNINIT = 10;  // Stop reading after 10 consecutive uninitialized/invalid nodes
    bool corruption_already_counted = false;  // Track if we've already incremented corrupted_nodes_skipped for this node
    
    // CRITICAL: Only read up to nodes_to_load (from header), not max_safe_nodes
    // This prevents reading past the end of valid nodes
    uint32_t nodes_to_read = (nodes_to_load < max_safe_nodes) ? nodes_to_load : max_safe_nodes;
    
    for (uint32_t i = 0; i < nodes_to_read; i++) {
        // Check if we've hit end of file before reading
        off_t current_pos = lseek(fd, 0, SEEK_CUR);
        off_t file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, current_pos, SEEK_SET);
        
        if (current_pos >= file_size) {
            // Past end of file - stop reading
            break;
        }
        
        lattice_node_t node;
        
        if (read(fd, &node, sizeof(lattice_node_t)) != sizeof(lattice_node_t)) {
            break;
        }
        
        // CORRUPTION DETECTION: Validate node integrity before loading (fast path, no printf unless corrupted)
        bool is_corrupted = false;
        corruption_already_counted = false;  // Reset for each node
        
        // Validate node ID: must be non-zero and within reasonable bounds
        // Better validation: Account for chunked storage (IDs can be sparse)
        uint32_t local_id = (uint32_t)(node.id & 0xFFFFFFFF);
        uint32_t max_safe_id = lattice->max_nodes * 10;  // Increased from *2 to *10 for chunked storage
        if (node.id == 0) {
            // Uninitialized node - skip silently (not corruption, just uninitialized slot)
            // This happens when file has fewer nodes than header says (expected in sparse files)
            consecutive_uninitialized++;
            consecutive_invalid++;
            if (consecutive_invalid >= MAX_CONSECUTIVE_UNINIT) {
                // Hit end of written data - stop reading
                break;
            }
            is_corrupted = true;
        } else {
            bool is_valid_node = true;
            if (local_id > max_safe_id) {
                // Large ID - might be valid chunked node, check name
                // Chunked nodes have "C:" or "CHUNK:" prefix
                if (strncmp(node.name, "C:", 2) != 0 && 
                    strncmp(node.name, "CHUNK:", 6) != 0) {
                    // Not a chunked node, but ID is too large - likely misaligned read or corruption
                    // If name is empty or looks like data, this is likely past end of written nodes
                    bool looks_like_data = (node.name[0] == '\0' || 
                                           node.type == 0 || 
                                           node.type > LATTICE_NODE_CPT_METADATA);
                    if (looks_like_data) {
                        // This looks like we're reading past written nodes - count as invalid
                        consecutive_invalid++;
                        if (consecutive_invalid >= MAX_CONSECUTIVE_UNINIT) {
                            break;
                        }
                        is_valid_node = false;
                    } else {
                        // Might be actual corruption - warn but continue
                        // Increment counter first, then check for warning
                        corrupted_nodes_skipped++;
                        corruption_already_counted = true;  // Mark that we've counted this corruption
                        // Only warn about first few corrupted nodes, then summarize
                        if (corrupted_nodes_skipped <= 3) {
                            printf("[LATTICE-LOAD] WARN CORRUPTION: Invalid node ID %lu (local_id=%u) at position %u - SKIPPING\n",
                                   node.id, local_id, i);
                        } else if (corrupted_nodes_skipped == 4) {
                            printf("[LATTICE-LOAD] WARN (Additional corrupted nodes will be skipped silently...)\n");
                        }
                    }
                    is_corrupted = true;
                }
                // If it's a chunked node, allow it (chunked storage uses sparse IDs)
            }
            
            if (is_valid_node) {
                consecutive_uninitialized = 0;  // Reset counter on valid node
                consecutive_invalid = 0;  // Reset invalid counter too
            }
        }
        
        // Validate node type: must be a valid enum value
        // Allow chunked node types (LATTICE_NODE_CHUNK_HEADER=200, LATTICE_NODE_CHUNK_DATA=201)
        // Also allow chunked nodes by name even if type is wrong (they use user's type but are chunked by name)
        // Handle truncated names: check for "C:", "CHUNK:", or truncated versions
        bool is_chunked_by_name = (strncmp(node.name, "C:", 2) == 0 || 
                                   strncmp(node.name, "CHUNK:", 6) == 0 ||
                                   strncmp(node.name, "KED:", 4) == 0 ||
                                   strncmp(node.name, "K:", 2) == 0);
        // Also check by type - if type is CHUNK_HEADER or CHUNK_DATA, it's a chunk node
        bool is_chunked_by_type = (node.type == LATTICE_NODE_CHUNK_HEADER || 
                                   node.type == LATTICE_NODE_CHUNK_DATA);
        bool is_valid_type = (node.type >= LATTICE_NODE_PRIMITIVE && node.type <= LATTICE_NODE_CPT_METADATA) ||
                             (node.type == LATTICE_NODE_CHUNK_HEADER) ||
                             (node.type == LATTICE_NODE_CHUNK_DATA);
        
        // Allow chunked nodes by type even if name check fails (name might be truncated)
        if (!is_corrupted && !is_valid_type && !is_chunked_by_name && !is_chunked_by_type) {
            // CRITICAL: If node type is way out of range (> 1000), this is likely reading past end of file
            // Stop reading immediately to prevent false corruption warnings
            if (node.type > 1000) {
                // This looks like garbage data from reading past end of file - stop reading
                consecutive_invalid++;
                if (consecutive_invalid >= MAX_CONSECUTIVE_UNINIT) {
                    break;
                }
                is_corrupted = true;
                continue;  // Skip this node and continue (will break if too many consecutive)
            }
            
            corrupted_nodes_skipped++;
            corruption_already_counted = true;
            if (corrupted_nodes_skipped <= 3) {
                printf("[LATTICE-LOAD] WARN CORRUPTION: Invalid node type %d for node %lu - SKIPPING\n",
                       node.type, node.id);
            } else if (corrupted_nodes_skipped == 4) {
                printf("[LATTICE-LOAD] WARN (Additional corrupted nodes will be skipped silently...)\n");
            }
            is_corrupted = true;
        }
        
        // Validate child_count: prevent huge allocations from corrupted data
        if (!is_corrupted && node.child_count > 1000) {
            corrupted_nodes_skipped++;
            corruption_already_counted = true;
            if (corrupted_nodes_skipped <= 3) {
                printf("[LATTICE-LOAD] WARN CORRUPTION: Invalid child_count %u for node %lu - SKIPPING\n",
                       node.child_count, node.id);
            } else if (corrupted_nodes_skipped == 4) {
                printf("[LATTICE-LOAD] WARN (Additional corrupted nodes will be skipped silently...)\n");
            }
            is_corrupted = true;
        }
        
        // Skip corrupted nodes entirely - don't load them into the lattice
        if (is_corrupted) {
            // Only increment if not already incremented above (for invalid large IDs)
            if (!corruption_already_counted) {
                corrupted_nodes_skipped++;
            }
            // CRITICAL FIX: Don't try to skip over children data - children pointer is part of the
            // node structure (8-byte pointer), not separate data in the file. The file format
            // is just the node structure (1216 bytes), so we've already read past it.
            // Trying to skip over children data would misalign the file pointer.
            continue;  // Skip this corrupted node (we've already read the full 1216-byte structure)
        }
        
        // Node is valid - load it into the lattice
        lattice_node_t* loaded_node = &lattice->nodes[lattice->node_count];
        *loaded_node = node;
        
        // CRITICAL FIX: Children are NOT stored separately in the file format
        // The save function writes only sizeof(lattice_node_t) (line 636), so children pointer
        // in the file is just garbage (a memory address). We must clear it and rebuild
        // children relationships from parent_id links if needed.
        loaded_node->children = NULL;
        loaded_node->child_count = 0;  // Will be rebuilt from parent_id relationships if needed
        
        // Update mappings
        lattice->node_id_map[lattice->node_count] = node.id;

        // Rebuild reverse index during load (use local ID - lower 32 bits)
        if (lattice->id_to_index_map) {
            // local_id is already validated above, so it's safe
            uint32_t estimated_current_size = (lattice->max_nodes > 10000) ? lattice->max_nodes : 10000;
            if (local_id >= estimated_current_size) {
                // CRITICAL FIX: Cap new_index_size to prevent huge allocations
                uint32_t new_index_size = (local_id + 10000 < max_safe_id) ? (local_id + 10000) : max_safe_id;
                uint32_t* new_id_to_index = (uint32_t*)realloc(lattice->id_to_index_map, new_index_size * sizeof(uint32_t));
                if (new_id_to_index) {
                    memset(new_id_to_index + estimated_current_size, 0, (new_index_size - estimated_current_size) * sizeof(uint32_t));
                    lattice->id_to_index_map = new_id_to_index;
                    estimated_current_size = new_index_size;
                } else {
                    printf("[LATTICE-LOAD] ERROR Failed to grow id_to_index_map to %u\n", new_index_size);
                }
            }
            if (local_id < estimated_current_size) {
                lattice->id_to_index_map[local_id] = lattice->node_count;
            }
        }

        lattice->node_count++;
    }
    
    if (corrupted_nodes_skipped > 0) {
        printf("[LATTICE-LOAD] WARN Loaded %u nodes, skipped %u corrupted nodes\n",
               lattice->node_count, corrupted_nodes_skipped);
        printf("[LATTICE-LOAD] INFO Run lattice_compact_file() to clean up the file\n");
    } else {
        SYNRIX_LOG_INFO("[LATTICE-LOAD] OK Loaded %u nodes (no corruption detected)\n", lattice->node_count);
    }

    close(fd);
    lattice->dirty = false;

    return 0;
}
    
// Internal function: Add node without limit check (for WAL recovery)
// CRITICAL: This function bypasses the free tier limit check - only use for recovery/restoration
static uint64_t lattice_add_node_internal(persistent_lattice_t* lattice, lattice_node_type_t type, 
                                         const char* name, const char* data, uint64_t parent_id) {
    // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
    // fprintf(stderr, "!!! DEBUG: ENTERED lattice_add_node_internal for name: %s\n", name ? name : "(null)");
    // fflush(stderr);
    
    if (!lattice) {
        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
        // fprintf(stderr, "!!! DEBUG: lattice_add_node_internal: lattice is NULL\n");
        // fflush(stderr);
        return 0;
    }
    
    // CONSTITUTIONAL CONSTRAINT VALIDATION
    // Validate prefix-based semantics (node name must have semantic prefix)
    if (!lattice_validate_prefix_semantics(name)) {
        // Note: validate_prefix_semantics returns true but warns if prefix is missing
        // We'll let it through with a warning, but log it
        fprintf(stderr, "[LATTICE-CONSTRAINTS] WARN Node name '%s' lacks semantic prefix - may degrade O(k) query performance\n", 
                name ? name : "(null)");
    }
    
    // Validate node size constraint (data must fit in fixed-size node)
    if (data) {
        size_t data_len = strlen(data);
        if (data_len > sizeof(((lattice_node_t*)0)->data) - 1) {
            fprintf(stderr, 
                    "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Data length (%zu) exceeds fixed-size node capacity (%zu)\n"
                    "This breaks the Lattice. Use chunked storage for large data.\n",
                    data_len, sizeof(((lattice_node_t*)0)->data) - 1);
            lattice->last_error = LATTICE_ERROR_INVALID_NODE;
            return 0; // Reject: violates fixed-size constraint
        }
    }
    
    // Validate single-writer constraint
    if (!lattice_validate_single_writer()) {
        fprintf(stderr, 
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Multiple writers detected\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n");
        lattice->last_error = LATTICE_ERROR_INVALID_NODE;
        return 0; // Reject: violates single-writer constraint
    }
    
    // DISK MODE: Check bounds (file is pre-allocated, no growth allowed)
    if (lattice->disk_mode) {
        if (lattice->total_nodes >= lattice->total_file_nodes) {
            printf("[LATTICE] ERROR Disk mode: Cannot add node, file is full (%u/%u nodes)\n",
                   lattice->total_nodes, lattice->total_file_nodes);
            return 0;
        }
        // In disk mode, write directly to mmap'd memory (kernel handles flushing)
        // node_count tracks RAM cache, total_nodes tracks file position
        // We can write beyond node_count because the file is pre-allocated
    }
    
    // INTELLIGENT MEMORY MANAGEMENT: Grow lattice if needed (only in RAM mode)
    if (!lattice->disk_mode && lattice->node_count >= lattice->max_nodes) {
        printf("[LATTICE] INFO Growing lattice from %u to %u nodes (intelligent memory management)\n", 
               lattice->max_nodes, lattice->max_nodes * 2);
        
        // Double the capacity
        uint32_t new_max = lattice->max_nodes * 2;
        lattice_node_t* new_nodes = (lattice_node_t*)realloc(lattice->nodes, new_max * sizeof(lattice_node_t));
        if (!new_nodes) {
            printf("[LATTICE] ERROR Failed to grow lattice - out of memory\n");
            return 0;
        }
        
        uint64_t* new_node_id_map = (uint64_t*)realloc(lattice->node_id_map, new_max * sizeof(uint64_t));
        if (!new_node_id_map) {
            printf("[LATTICE] ERROR Failed to grow node ID map - out of memory\n");
            free(new_nodes);
            return 0;
        }

        // Grow access tracking arrays - CRITICAL FOR FULL DYNAMIC SYSTEM
        uint32_t* new_access_count = (uint32_t*)realloc(lattice->access_count, new_max * sizeof(uint32_t));
        if (!new_access_count) {
            printf("[LATTICE] ERROR Failed to grow access_count - out of memory\n");
            free(new_node_id_map);
            free(new_nodes);
            return 0;
        }
        // Zero out new portion
        memset(new_access_count + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));

        uint32_t* new_last_access = (uint32_t*)realloc(lattice->last_access, new_max * sizeof(uint32_t));
        if (!new_last_access) {
            printf("[LATTICE] ERROR Failed to grow last_access - out of memory\n");
            free(new_access_count);
            free(new_node_id_map);
            free(new_nodes);
            return 0;
        }
        // Zero out new portion
        memset(new_last_access + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));

        // Grow reverse index map when lattice grows
        // CRITICAL: Always grow reverse index to match new_max to prevent out-of-bounds access
        uint32_t current_index_size = (lattice->max_nodes > 10000) ? (lattice->max_nodes + 1) : 10001;
        uint32_t new_index_size = new_max + 1; // Need space for IDs 1 to new_max
        // CRITICAL FIX: Cap new_index_size to prevent huge allocations from corrupted next_id
        uint32_t max_safe_index_size = new_max * 2;
        // Also account for sparse IDs (next_id might be larger than new_max)
        if (lattice->next_id > new_max && lattice->next_id <= max_safe_index_size) {
            uint32_t candidate_size = (lattice->next_id + 10000 < max_safe_index_size) ? (lattice->next_id + 10000) : max_safe_index_size;
            new_index_size = (candidate_size > new_max + 1) ? candidate_size : (new_max + 1);
        }
        
        // Always grow if new size is larger than current
        if (new_index_size > current_index_size && new_index_size <= max_safe_index_size) {
            uint32_t* new_id_to_index = (uint32_t*)realloc(lattice->id_to_index_map, new_index_size * sizeof(uint32_t));
            if (!new_id_to_index) {
                printf("[LATTICE] ERROR Failed to grow id_to_index_map - out of memory\n");
                free(new_last_access);
                free(new_access_count);
                free(new_node_id_map);
                free(new_nodes);
                return 0;
            }
            // Zero out the newly allocated portion
            memset(new_id_to_index + current_index_size, 0, (new_index_size - current_index_size) * sizeof(uint32_t));
            lattice->id_to_index_map = new_id_to_index;
        }

        // Update lattice - ALL ARRAYS GROWN TOGETHER
        lattice->nodes = new_nodes;
        lattice->node_id_map = new_node_id_map;
        lattice->access_count = new_access_count;
        lattice->last_access = new_last_access;
        lattice->max_nodes = new_max;

        printf("[LATTICE] OK Lattice grown to %u nodes (%.1f MB RAM, fully dynamic)\n",
               lattice->max_nodes, (lattice->max_nodes * sizeof(lattice_node_t)) / (1024.0 * 1024.0));
    }
    
    // In disk mode, write to file position (total_nodes), not RAM cache position (node_count)
    // In RAM mode, write to RAM cache position (node_count)
    uint32_t write_index = lattice->disk_mode ? lattice->total_nodes : lattice->node_count;
    
    // CRITICAL: In RAM mode, ensure we have space before writing
    // If write_index >= max_nodes, we need to grow the array FIRST
    if (!lattice->disk_mode && write_index >= lattice->max_nodes) {
        // Array growth should have happened above, but double-check
        // If we're still beyond bounds, grow now
        if (write_index >= lattice->max_nodes) {
            uint32_t new_max = lattice->max_nodes * 2;
            if (new_max < write_index + 1000) {
                new_max = write_index + 10000; // Ensure we have enough space
            }
            // Call growth logic (simplified - just realloc)
            lattice_node_t* new_nodes = (lattice_node_t*)realloc(lattice->nodes, new_max * sizeof(lattice_node_t));
            if (!new_nodes) {
                printf("[LATTICE] ERROR Failed to grow nodes array - out of memory\n");
                return 0;
            }
            lattice->nodes = new_nodes;
            lattice->max_nodes = new_max;
            printf("[LATTICE] OK Emergency grow: expanded to %u nodes\n", new_max);
        }
    }
    
    // Bounds check for disk mode
    if (lattice->disk_mode && write_index >= lattice->total_file_nodes) {
        printf("[LATTICE] ERROR Disk mode: write_index %u >= total_file_nodes %u\n",
               write_index, lattice->total_file_nodes);
        return 0;
    }
    
    // DISK MODE OPTIMIZATION: Write directly to mmap'd memory with minimal CPU overhead
    // This ensures data hits the memory controller (EMC) immediately, not stuck in CPU caches
    lattice_node_t* node = &lattice->nodes[write_index];
    
    // Initialize node - MINIMAL CPU WORK, MAXIMUM DATA MOVEMENT
    // Generate 64-bit ID: (device_id << 32) | local_id
    uint32_t local_id;
    if (lattice->thread_safe_mode) {
        // Atomic fetch-and-add for thread-safe local ID allocation
        local_id = (uint32_t)__atomic_fetch_add(&lattice->next_id, 1, __ATOMIC_SEQ_CST);
    } else {
        // Single-threaded: Direct increment (faster)
        local_id = (uint32_t)lattice->next_id++;
    }
    // Combine device_id and local_id into 64-bit ID
    node->id = ((uint64_t)lattice->device_id << 32) | local_id;
    node->type = type;
    
    // Optimize string copies: use memcpy for known lengths (faster than strncpy)
    // Add safety checks to prevent bus errors from NULL pointers or invalid lengths
    if (name) {
        size_t name_len = strlen(name);
        size_t copy_len = (name_len < sizeof(node->name) - 1) ? name_len : sizeof(node->name) - 1;
        memcpy(node->name, name, copy_len);
        node->name[copy_len] = '\0';
        // Ensure null termination
        node->name[sizeof(node->name) - 1] = '\0';
    } else {
        node->name[0] = '\0';
    }
    
    if (data) {
        size_t data_len = strlen(data);
        size_t copy_len = (data_len < sizeof(node->data) - 1) ? data_len : sizeof(node->data) - 1;
        memcpy(node->data, data, copy_len);
        node->data[copy_len] = '\0';
        // Ensure null termination
        node->data[sizeof(node->data) - 1] = '\0';
    } else {
        node->data[0] = '\0';
    }
    
    node->parent_id = parent_id;
    node->child_count = 0;
    node->children = NULL;
    node->confidence = 1.0;
    node->timestamp = get_current_timestamp();
    
    // Initialize payload - use single memset for entire node (more efficient)
    // This writes the entire node structure in one go, triggering page fault once
    memset(&node->payload, 0, sizeof(node->payload));
    
    // MEMORY BARRIER: Ensure write hits memory controller (EMC), not just CPU cache
    // This forces the data to actually move through the memory bus
    // Without this, writes might sit in L2/L3 cache and tegrastats won't see EMC activity
    __sync_synchronize(); // Full memory barrier - ensures all writes are visible to memory subsystem
    
    // DISK MODE: Skip expensive operations that don't affect the write
    // Only do minimal metadata updates for nodes in RAM cache
    if (!lattice->disk_mode || write_index < lattice->max_nodes) {
        // Update forward mapping (array_index -> node_id) - only for RAM cache
        if (write_index < lattice->max_nodes) {
            lattice->node_id_map[write_index] = node->id;
        }
        
        // Update reverse mapping (local_id -> array_index) for O(1) lookup
        // Extract local ID (lower 32 bits) for reverse index
        uint32_t local_id = (uint32_t)(node->id & 0xFFFFFFFF);
        // CRITICAL: Cap local_id to prevent huge allocations from corrupted data
        uint32_t max_safe_id = lattice->max_nodes * 2;
        if (local_id <= max_safe_id) {
            // Only grow if needed and only for nodes in RAM cache
            uint32_t current_index_size = (lattice->max_nodes > 10000) ? lattice->max_nodes : 10000;
            if (local_id >= current_index_size && lattice->id_to_index_map) {
                // CRITICAL FIX: Cap new_index_size to prevent huge allocations
                uint32_t new_index_size = (local_id + 10000 < max_safe_id) ? (local_id + 10000) : max_safe_id;
                uint32_t* new_id_to_index = (uint32_t*)realloc(lattice->id_to_index_map, new_index_size * sizeof(uint32_t));
                if (new_id_to_index) {
                    memset(new_id_to_index + current_index_size, 0, (new_index_size - current_index_size) * sizeof(uint32_t));
                    lattice->id_to_index_map = new_id_to_index;
                }
            }
            // Safety bound: 10x max_nodes (allows for sparse IDs)
            if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
                lattice->id_to_index_map[local_id] = write_index;
            }
        }
        
        // Add to parent if specified (only for RAM cache nodes to avoid expensive lookups)
        if (parent_id > 0) {
            lattice_add_child(lattice, parent_id, node->id);
        }
    }
    // DISK MODE: For nodes beyond RAM cache, skip parent/child operations
    // The graph structure is preserved in the file, we just skip the in-memory index updates

    // In disk mode: only increment total_nodes (file position)
    // In RAM mode: increment both node_count (RAM cache) and total_nodes
    if (lattice->disk_mode) {
        lattice->total_nodes++;
        // Keep a small RAM cache for frequently accessed nodes (up to max_nodes)
        // Note: node_count is limited to max_nodes (RAM cache), but total_nodes can go up to total_file_nodes
        if (write_index < lattice->max_nodes) {
            lattice->node_count++;
        }
        
        // Prefault next chunk of pages to reduce page fault latency
        // Aggressive prefetching for NVMe: prefetch 32MB ahead every 5000 nodes
        // This keeps the kernel ahead of our writes, reducing page fault stalls
        // For NVMe drives, larger prefetch windows (32MB) are more efficient than small ones (8MB)
        // More frequent prefetching (every 5K nodes) keeps write pipeline full
        if (lattice->total_nodes % 5000 == 0 && lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
            size_t header_size = 4 * sizeof(uint32_t);
            size_t current_offset = header_size + (lattice->total_nodes * sizeof(lattice_node_t));
            size_t prefetch_size = 32 * 1024 * 1024; // Prefetch next 32MB (8192 pages) - NVMe optimized
            size_t prefetch_offset = current_offset;
            
            // Make sure we don't go beyond the file
            if (prefetch_offset + prefetch_size <= lattice->mmap_size) {
                void* prefetch_addr = (char*)lattice->mmap_ptr + prefetch_offset;
                // MADV_WILLNEED: Hint kernel to prefault these pages (non-blocking)
                // Larger prefetch window (32MB) keeps kernel ahead of writes on fast NVMe
                // This reduces page fault latency and prevents write stalls
                madvise(prefetch_addr, prefetch_size, MADV_WILLNEED);
            }
        }
    } else {
        lattice->node_count++;
        lattice->total_nodes++;
    }
    lattice->dirty = true;
    
    // PRODUCTION PERSISTENCE: Auto-save logic (works in both RAM and disk mode)
    if (lattice->persistence.auto_save_enabled) {
        lattice->persistence.nodes_since_last_save++;
        
        // Check if we need to save based on node count
        bool should_save_nodes = (lattice->persistence.auto_save_interval_nodes > 0) &&
                                 (lattice->persistence.nodes_since_last_save >= lattice->persistence.auto_save_interval_nodes);
        
        // Check if we need to save based on time
        uint64_t current_time = get_current_timestamp();
        uint64_t time_since_save = current_time - lattice->persistence.last_save_timestamp;
        bool should_save_time = (lattice->persistence.auto_save_interval_seconds > 0) &&
                                (time_since_save >= (uint64_t)lattice->persistence.auto_save_interval_seconds * 1000000);
        
        // Check if we need to save on memory pressure (when RAM fills)
        // Only trigger when crossing 90% threshold AND we've added significant nodes since last save
        // Use same interval as auto-save to avoid excessive saves (aligns with 5000 node interval)
        // This prevents saving on every single node after 90%
        // CRITICAL: Only check memory pressure if auto-save interval is > 0 (otherwise interval_nodes=0 makes this always true)
        bool should_save_pressure = lattice->persistence.save_on_memory_pressure &&
                                    (lattice->persistence.auto_save_interval_nodes > 0) && // Must have valid interval
                                    (lattice->node_count >= lattice->max_nodes * 0.9) && // At 90% capacity
                                    (lattice->persistence.nodes_since_last_save >= lattice->persistence.auto_save_interval_nodes); // Use same interval as auto-save
        
        if (should_save_nodes || should_save_time || should_save_pressure) {
            if (should_save_nodes) {
                printf("[LATTICE-AUTO-SAVE] Saving snapshot (node count: %u >= %u)\n",
                       lattice->persistence.nodes_since_last_save, lattice->persistence.auto_save_interval_nodes);
            } else if (should_save_time) {
                printf("[LATTICE-AUTO-SAVE] Saving snapshot (time: %lu seconds >= %u)\n",
                       time_since_save / 1000000, lattice->persistence.auto_save_interval_seconds);
            } else if (should_save_pressure) {
                printf("[LATTICE-AUTO-SAVE] Saving snapshot (memory pressure: %u/%u nodes, %.1f%%)\n",
                       lattice->node_count, lattice->max_nodes, (lattice->node_count * 100.0) / lattice->max_nodes);
            }
            fflush(stdout);
            
            // Perform auto-save (non-blocking, but synchronous for data safety)
            if (lattice_save(lattice) == 0) {
                // Also checkpoint WAL if enabled (apply WAL entries to main file)
                if (lattice->wal && lattice->wal_enabled) {
                    lattice_wal_checkpoint(lattice);
                }
                printf("[LATTICE-AUTO-SAVE] OK Snapshot saved and checkpointed\n");
                lattice->persistence.nodes_since_last_save = 0; // Reset counter after save
                lattice->persistence.last_save_timestamp = get_current_timestamp();
                fflush(stdout);
            } else {
                printf("[LATTICE-AUTO-SAVE] WARN Failed to save snapshot (will retry on next interval)\n");
                fflush(stdout);
            }
        }
    }

    // OPTIMIZATION: Use incremental index updates instead of invalidating
    // Only invalidate for large lattices where incremental update is expensive
    // For small lattices, use incremental update (faster)
    char prefix[64];
    if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
        if (lattice->prefix_index.built) {
            // For large lattices, defer index updates (rebuild later)
            // For small lattices, use incremental update
            if (lattice->node_count < 10000) {
                // Incremental update (fast)
                lattice_prefix_index_add_node(lattice, node->id, name);
            } else {
                // Mark as invalid (will rebuild lazily when needed, but defer during active operations)
                lattice->prefix_index.built = false;
            }
        }
    }
    
    // WAL logging - CRITICAL: This must be before the return statement
    // Format: type(1) | name_len(4) | name | data_len(4) | data | parent_id(8)
    // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
    // fprintf(stderr, "!!! DEBUG: Reached end of lattice_add_node_internal, about to check WAL: wal_enabled=%d, wal_ptr=%p, node_id=%llu\n", 
    //        lattice->wal_enabled, (void*)lattice->wal, (unsigned long long)node->id);
    // fflush(stderr);
    
    if (lattice->wal_enabled && lattice->wal) {
        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
        // fprintf(stderr, "[LATTICE-DEBUG] Entering WAL write block in lattice_add_node_internal\n");
        // fflush(stderr);
        
        // Debug: Check WAL state
        if (lattice->wal->enabled == 0 || lattice->wal->wal_fd < 0) {
            // Only warn on actual errors, not in hot path
            // fprintf(stderr, "[LATTICE] WARN WARNING: WAL not properly enabled (enabled=%d, fd=%d)\n",
            //        lattice->wal->enabled, lattice->wal->wal_fd);
            // fflush(stderr);
        } else {
            // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
            // fprintf(stderr, "[LATTICE-DEBUG] Attempting WAL append: node_id=%llu, wal->enabled=%d, wal->wal_fd=%d, batch_size=%u, batch_count=%u\n",
            //        (unsigned long long)node->id, lattice->wal->enabled, lattice->wal->wal_fd, 
            //        lattice->wal->batch_size, lattice->wal->batch_count);
            // fflush(stderr);
            
            // Pack binary data for WAL
            // Format: type(1) | name_len(4) | name | data_len(4) | data | parent_id(8)
            // This matches the format expected by WAL recovery
            size_t name_len = name ? strlen(name) : 0;
            size_t data_len = data ? strlen(data) : 0;
            
            // Bounds check: ensure name_len and data_len fit in uint32_t
            if (name_len > UINT32_MAX || data_len > UINT32_MAX) {
                fprintf(stderr, "[LATTICE] WARN WARNING: name_len (%zu) or data_len (%zu) exceeds uint32_t max, skipping WAL entry\n", name_len, data_len);
                fflush(stderr);
            } else {
                uint32_t name_len_32 = (uint32_t)name_len;
                uint32_t data_len_32 = (uint32_t)data_len;
                size_t packed_size = 1 + 4 + name_len_32 + 4 + data_len_32 + 8; // type + name_len + name + data_len + data + parent_id
                
                // Bounds check: ensure packed_size fits in uint32_t for wal_append
                if (packed_size > UINT32_MAX) {
                    fprintf(stderr, "[LATTICE] WARN WARNING: packed_size (%zu) exceeds uint32_t max, skipping WAL entry\n", packed_size);
                    fflush(stderr);
                } else {
                    uint32_t packed_size_32 = (uint32_t)packed_size;
                    char* packed_data = (char*)malloc(packed_size);
                    if (packed_data) {
                        size_t offset = 0;
                        // Type byte (required for WAL recovery)
                        packed_data[offset++] = (char)type;
                        // Name length and name (cast to uint32_t for 4-byte write)
                        memcpy(packed_data + offset, &name_len_32, 4);
                        offset += 4;
                        if (name_len_32 > 0) {
                            memcpy(packed_data + offset, name, name_len_32);
                            offset += name_len_32;
                        }
                        // Data length and data (cast to uint32_t for 4-byte write)
                        memcpy(packed_data + offset, &data_len_32, 4);
                        offset += 4;
                        if (data_len_32 > 0) {
                            memcpy(packed_data + offset, data, data_len_32);
                            offset += data_len_32;
                        }
                        // Parent ID
                        memcpy(packed_data + offset, &parent_id, 8);
                        
                        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
                        // fprintf(stderr, "[LATTICE-DEBUG] Calling wal_append...\n");
                        // fflush(stderr);
                        uint64_t wal_seq = wal_append(lattice->wal, WAL_OP_ADD_NODE, node->id, packed_data, packed_size_32);
                        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
                        // fprintf(stderr, "[LATTICE-DEBUG] wal_append returned: %llu\n", (unsigned long long)wal_seq);
                        // fflush(stderr);
                        if (wal_seq == 0) {
                            // Only log actual errors, not in hot path
                            // fprintf(stderr, "[LATTICE] WARN WARNING: wal_append returned 0 (WAL write may have failed)\n");
                            // fprintf(stderr, "[LATTICE]    WAL enabled: %d, WAL ptr: %p, WAL fd: %d, wal->enabled: %d\n", 
                            //        lattice->wal_enabled, (void*)lattice->wal, 
                            //        lattice->wal ? lattice->wal->wal_fd : -1,
                            //        lattice->wal ? lattice->wal->enabled : 0);
                            // fflush(stderr);
                        }
                        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
                        // else {
                        //     fprintf(stderr, "[LATTICE-DEBUG] WAL append successful: sequence=%llu, batch_count=%u\n",
                        //            (unsigned long long)wal_seq, lattice->wal->batch_count);
                        //     fflush(stderr);
                        // }
                        free(packed_data);
                    } else {
                        fprintf(stderr, "[LATTICE] WARN WARNING: Failed to allocate memory for WAL packed data\n");
                        fflush(stderr);
                    }
                }
            }
        }
    }
    // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
    // else {
    //     fprintf(stderr, "[LATTICE-DEBUG] WAL write block skipped: wal_enabled=%d, wal_ptr=%p\n",
    //            lattice->wal_enabled, (void*)lattice->wal);
    //     fflush(stderr);
    // }
    
    return node->id;
}

// Public API: Add node with free tier limit enforcement
uint64_t lattice_add_node(persistent_lattice_t* lattice, lattice_node_type_t type, 
                         const char* name, const char* data, uint64_t parent_id) {
    // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
    // fprintf(stderr, "!!! DEBUG: ENTERED lattice_add_node for name: %s\n", name ? name : "(null)");
    // fflush(stderr);
    
    if (!lattice) {
        // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
        // fprintf(stderr, "!!! DEBUG: lattice is NULL, returning 0\n");
        // fflush(stderr);
        return 0;
    }
    
    // DEBUG PRINTS DISABLED FOR PERFORMANCE BENCHMARK
    // fprintf(stderr, "[LATTICE-ADD] WAL state: wal_enabled=%d, wal_ptr=%p\n", 
    //        lattice->wal_enabled, (void*)lattice->wal);
    // fflush(stderr);
    // if (lattice->wal) {
    //     printf("[LATTICE-ADD] WAL details: enabled=%d, fd=%d, sequence=%llu\n",
    //            lattice->wal->enabled, lattice->wal->wal_fd, 
    //            (unsigned long long)lattice->wal->sequence);
    // }

    /* Global license usage: one cap per license per machine (deny add if over limit) */
    if (lattice->evaluation_mode && lattice->free_tier_limit > 0) {
        if (license_global_add_one(lattice->free_tier_limit) != 0) {
            lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
            fprintf(stderr,
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached (global)\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes across all lattices.\n"
                "  No new nodes can be added.\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit);
            fflush(stderr);
            return 0;
        }
    }

    // FREE TIER LIMIT ENFORCEMENT (Phase 1: Evaluation Mode)
    // Check BEFORE memory allocation to prevent OOM on small devices
    // Check if adding THIS node would exceed the limit (total_nodes will be incremented AFTER this check)
    // We check (total_nodes + 1) > limit to ensure we stop at exactly the limit (node limit+1 is rejected)
    if (lattice->evaluation_mode && (lattice->total_nodes + 1) > lattice->free_tier_limit) {
        lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
        fprintf(stderr, 
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes.\n"
                "  Current usage: %u nodes.\n"
                "\n"
                "  No new nodes can be added to this lattice.\n"
                "\n"
                "  Options:\n"
                "  - Delete existing nodes to free up space\n"
                "  - Upgrade to Pro tier for unlimited nodes (synrix.io)\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit, lattice->total_nodes);
        fflush(stderr);  // Ensure error message is flushed immediately
        return 0; // Return 0 to indicate failure (Python SDK will catch this)
    }
    
    // Call internal function (bypasses limit check - already done above)
    return lattice_add_node_internal(lattice, type, name, data, parent_id);
}

// Thread-safe ID reservation: Reserve a block of IDs atomically
// This allows multiple threads to create nodes in parallel without contention
uint32_t lattice_reserve_id_block(persistent_lattice_t* lattice, uint32_t count) {
    if (!lattice || count == 0) return 0;
    
    // Atomic fetch-and-add: Reserve 'count' IDs and return the base ID
    // This is lock-free and allows parallel node creation
    uint32_t base_id = __atomic_fetch_add(&lattice->next_id, count, __ATOMIC_SEQ_CST);
    
    return base_id;
}

// Thread-safe node addition with pre-reserved ID
// Use this when you've already reserved an ID block via lattice_reserve_id_block()
uint64_t lattice_add_node_with_id(persistent_lattice_t* lattice, uint32_t reserved_local_id,
                                  lattice_node_type_t type, const char* name, 
                                  const char* data, uint64_t parent_id) {
    if (!lattice) return 0;


    /* Global license usage: one cap per license per machine (deny add if over limit) */
    if (lattice->evaluation_mode && lattice->free_tier_limit > 0) {
        if (license_global_add_one(lattice->free_tier_limit) != 0) {
            lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
            fprintf(stderr,
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached (global)\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes across all lattices.\n"
                "  No new nodes can be added.\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit);
            fflush(stderr);
            return 0;
        }
    }

    // FREE TIER LIMIT ENFORCEMENT (Phase 1: Evaluation Mode)
    // Check if adding THIS node would exceed the limit
    if (lattice->evaluation_mode && (lattice->total_nodes + 1) > lattice->free_tier_limit) {
        lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
        fprintf(stderr, 
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes.\n"
                "  Current usage: %u nodes.\n"
                "\n"
                "  No new nodes can be added to this lattice.\n"
                "\n"
                "  Options:\n"
                "  - Delete existing nodes to free up space\n"
                "  - Upgrade to Pro tier for unlimited nodes (synrix.io)\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit, lattice->total_nodes);
        return 0;
    }
    
    // CONSTITUTIONAL CONSTRAINT VALIDATION
    // Validate prefix-based semantics (node name must have semantic prefix)
    if (!lattice_validate_prefix_semantics(name)) {
        fprintf(stderr, "[LATTICE-CONSTRAINTS] WARN Node name '%s' lacks semantic prefix - may degrade O(k) query performance\n", 
                name ? name : "(null)");
    }
    
    // Validate node size constraint (data must fit in fixed-size node)
    if (data) {
        size_t data_len = strlen(data);
        if (data_len > sizeof(((lattice_node_t*)0)->data) - 1) {
            fprintf(stderr, 
                    "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Data length (%zu) exceeds fixed-size node capacity (%zu)\n"
                    "This breaks the Lattice. Use chunked storage for large data.\n",
                    data_len, sizeof(((lattice_node_t*)0)->data) - 1);
            return 0; // Reject: violates fixed-size constraint
        }
    }
    
    // Validate single-writer constraint
    if (!lattice_validate_single_writer()) {
        fprintf(stderr, 
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Multiple writers detected\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n");
        return 0; // Reject: violates single-writer constraint
    }
    
    // Ensure we have space (same growth logic as lattice_add_node)
    if (lattice->node_count >= lattice->max_nodes) {
        printf("[LATTICE] INFO Growing lattice from %u to %u nodes (thread-safe mode)\n", 
               lattice->max_nodes, lattice->max_nodes * 2);
        
        uint32_t new_max = lattice->max_nodes * 2;
        lattice_node_t* new_nodes = (lattice_node_t*)realloc(lattice->nodes, new_max * sizeof(lattice_node_t));
        if (!new_nodes) {
            printf("[LATTICE] ERROR Failed to grow lattice - out of memory\n");
            return 0;
        }
        
        uint64_t* new_node_id_map = (uint64_t*)realloc(lattice->node_id_map, new_max * sizeof(uint64_t));
        if (!new_node_id_map) {
            free(new_nodes);
            return 0;
        }
        
        uint32_t* new_access_count = (uint32_t*)realloc(lattice->access_count, new_max * sizeof(uint32_t));
        if (!new_access_count) {
            free(new_node_id_map);
            free(new_nodes);
            return 0;
        }
        memset(new_access_count + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));
        
        uint32_t* new_last_access = (uint32_t*)realloc(lattice->last_access, new_max * sizeof(uint32_t));
        if (!new_last_access) {
            free(new_access_count);
            free(new_node_id_map);
            free(new_nodes);
            return 0;
        }
        memset(new_last_access + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));
        
        // Grow reverse index if needed
        // CRITICAL FIX: Cap new_index_size to prevent huge allocations
        uint32_t max_safe_id = new_max * 2;
        if (reserved_local_id >= new_max && reserved_local_id <= max_safe_id) {
            uint32_t new_index_size = (reserved_local_id + 10000 < max_safe_id) ? (reserved_local_id + 10000) : max_safe_id;
            uint32_t* new_id_to_index = (uint32_t*)realloc(lattice->id_to_index_map, new_index_size * sizeof(uint32_t));
            if (new_id_to_index) {
                memset(new_id_to_index + lattice->next_id, 0, (new_index_size - lattice->next_id) * sizeof(uint32_t));
                lattice->id_to_index_map = new_id_to_index;
            }
        }
        
        lattice->nodes = new_nodes;
        lattice->node_id_map = new_node_id_map;
        lattice->access_count = new_access_count;
        lattice->last_access = new_last_access;
        lattice->max_nodes = new_max;
    }
    
    lattice_node_t* node = &lattice->nodes[lattice->node_count];
    
    // Use the pre-reserved local ID and combine with device_id to form 64-bit ID
    node->id = ((uint64_t)lattice->device_id << 32) | reserved_local_id;
    node->type = type;
    strncpy(node->name, name, sizeof(node->name) - 1);
    strncpy(node->data, data, sizeof(node->data) - 1);
    node->parent_id = parent_id;
    node->child_count = 0;
    node->children = NULL;
    node->confidence = 1.0;
    node->timestamp = get_current_timestamp();
    
    // Initialize payload
    memset(&node->payload, 0, sizeof(node->payload));
    
    // Add to parent if specified
    if (parent_id > 0) {
        lattice_add_child(lattice, parent_id, node->id);
    }
    
    // Update mappings
    lattice->node_id_map[lattice->node_count] = node->id;
    
    // Update reverse index (use local ID - lower 32 bits)
    uint32_t local_id = (uint32_t)(node->id & 0xFFFFFFFF);
    // Safety bound: 10x max_nodes (allows for sparse IDs)
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        lattice->id_to_index_map[local_id] = lattice->node_count;
    }
    
    lattice->node_count++;
    lattice->total_nodes++;
    lattice->dirty = true;
    
    // Invalidate prefix index if needed
    if (lattice->prefix_index.built && 
        (strncmp(name, "ISA_", 4) == 0 || strncmp(name, "MATERIAL_", 9) == 0 ||
         strncmp(name, "LEARNING_", 9) == 0 || strncmp(name, "PERFORMANCE_", 12) == 0)) {
        lattice->prefix_index.built = false;
    }
    
    return node->id;
}

// Add node with compressed data (preserves compression header)
uint64_t lattice_add_node_compressed(persistent_lattice_t* lattice, lattice_node_type_t type,
                                     const char* name, const void* compressed_data, size_t compressed_data_len,
                                     uint64_t parent_id) {
    if (!lattice || !name || !compressed_data || compressed_data_len == 0) return 0;
    

    /* Global license usage: one cap per license per machine (deny add if over limit) */
    if (lattice->evaluation_mode && lattice->free_tier_limit > 0) {
        if (license_global_add_one(lattice->free_tier_limit) != 0) {
            lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
            fprintf(stderr,
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached (global)\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes across all lattices.\n"
                "  No new nodes can be added.\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit);
            fflush(stderr);
            return 0;
        }
    }
    // FREE TIER LIMIT ENFORCEMENT (Phase 1: Evaluation Mode)
    // Check if adding THIS node would exceed the limit
    if (lattice->evaluation_mode && (lattice->total_nodes + 1) > lattice->free_tier_limit) {
        lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
        fprintf(stderr, 
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes.\n"
                "  Current usage: %u nodes.\n"
                "\n"
                "  No new nodes can be added to this lattice.\n"
                "\n"
                "  Options:\n"
                "  - Delete existing nodes to free up space\n"
                "  - Upgrade to Pro tier for unlimited nodes (synrix.io)\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit, lattice->total_nodes);
        return 0;
    }
    
    // CONSTITUTIONAL CONSTRAINT VALIDATION
    // Validate prefix-based semantics (node name must have semantic prefix)
    if (!lattice_validate_prefix_semantics(name)) {
        fprintf(stderr, "[LATTICE-CONSTRAINTS] WARN Node name '%s' lacks semantic prefix - may degrade O(k) query performance\n", name);
    }
    
    // Validate node size constraint (compressed data must fit in fixed-size node)
    size_t max_data_size = sizeof(((lattice_node_t*)0)->data);
    if (compressed_data_len > max_data_size) {
        fprintf(stderr, 
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Compressed data length (%zu) exceeds fixed-size node capacity (%zu)\n"
                "This breaks the Lattice. Use chunked storage for large data.\n",
                compressed_data_len, max_data_size);
        return 0; // Reject: violates fixed-size constraint
    }
    
    // Validate single-writer constraint
    if (!lattice_validate_single_writer()) {
        fprintf(stderr, 
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Multiple writers detected\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n");
        return 0; // Reject: violates single-writer constraint
    }
    
    // Compressed data format: [length:2 (bit 15 set)][compression_type:1][payload...]
    // We preserve this format in the node's data field
    
    // Reuse add_node_binary logic but preserve compression header
    // Get write index
    uint32_t write_index = lattice->disk_mode ? lattice->total_nodes : lattice->node_count;
    
    if (lattice->disk_mode && write_index >= lattice->total_file_nodes) {
        printf("[LATTICE] ERROR Disk mode: write_index %u >= total_file_nodes %u\n",
               write_index, lattice->total_file_nodes);
        return 0;
    }
    
    // Grow if needed (non-disk mode)
    if (!lattice->disk_mode && lattice->node_count >= lattice->max_nodes) {
        uint32_t new_max = lattice->max_nodes * 2;
        lattice_node_t* new_nodes = (lattice_node_t*)realloc(lattice->nodes, new_max * sizeof(lattice_node_t));
        if (!new_nodes) return 0;
        uint64_t* new_node_id_map = (uint64_t*)realloc(lattice->node_id_map, new_max * sizeof(uint64_t));
        if (!new_node_id_map) {
            free(new_nodes);
            return 0;
        }
        uint32_t* new_access_count = (uint32_t*)realloc(lattice->access_count, new_max * sizeof(uint32_t));
        if (!new_access_count) {
            free(new_nodes);
            free(new_node_id_map);
            return 0;
        }
        memset(new_access_count + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));
        uint32_t* new_last_access = (uint32_t*)realloc(lattice->last_access, new_max * sizeof(uint32_t));
        if (!new_last_access) {
            free(new_nodes);
            free(new_node_id_map);
            free(new_access_count);
            return 0;
        }
        memset(new_last_access + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));
        lattice->nodes = new_nodes;
        lattice->node_id_map = new_node_id_map;
        lattice->access_count = new_access_count;
        lattice->last_access = new_last_access;
        lattice->max_nodes = new_max;
    }
    
    lattice_node_t* node = &lattice->nodes[write_index];
    
    // Generate ID
    uint32_t local_id;
    if (lattice->thread_safe_mode) {
        local_id = (uint32_t)__atomic_fetch_add(&lattice->next_id, 1, __ATOMIC_SEQ_CST);
    } else {
        local_id = (uint32_t)lattice->next_id++;
    }
    node->id = ((uint64_t)lattice->device_id << 32) | local_id;
    node->type = type;
    
    // Copy name
    size_t name_len = strlen(name);
    size_t copy_len = (name_len < sizeof(node->name) - 1) ? name_len : sizeof(node->name) - 1;
    memcpy(node->name, name, copy_len);
    node->name[copy_len] = '\0';
    node->name[sizeof(node->name) - 1] = '\0';
    
    // Copy compressed data directly (preserves compression header)
    memcpy(node->data, compressed_data, compressed_data_len);
    // Zero-fill remainder
    if (compressed_data_len < sizeof(node->data)) {
        memset(node->data + compressed_data_len, 0, sizeof(node->data) - compressed_data_len);
    }
    
    node->parent_id = parent_id;
    node->child_count = 0;
    node->children = NULL;
    node->confidence = 1.0;
    node->timestamp = get_current_timestamp();
    memset(&node->payload, 0, sizeof(node->payload));
    
    __sync_synchronize();
    
    // Update mappings
    if (!lattice->disk_mode || write_index < lattice->max_nodes) {
        if (write_index < lattice->max_nodes) {
            lattice->node_id_map[write_index] = node->id;
        }
        if (lattice->id_to_index_map) {
            uint32_t local_id_32 = (uint32_t)(node->id & 0xFFFFFFFF);
            if (local_id_32 < lattice->max_nodes * 2) {
                lattice->id_to_index_map[local_id_32] = write_index;
            }
        }
    }
    
    if (!lattice->disk_mode) {
        lattice->node_count++;
    }
    lattice->total_nodes++;
    lattice->dirty = true;
    
    // WAL logging (use ADD_NODE format compatible with recovery)
    // Format: type(1) | name_len(4) | name | data_len(4) | data | parent_id(8)
    if (lattice->wal_enabled && lattice->wal) {
        size_t name_len = strlen(name);
        
        // Bounds check: ensure name_len and compressed_data_len fit in uint32_t
        if (name_len > UINT32_MAX || compressed_data_len > UINT32_MAX) {
            printf("[LATTICE] WARN WARNING: name_len (%zu) or compressed_data_len (%zu) exceeds uint32_t max, skipping WAL entry\n", name_len, compressed_data_len);
        } else {
            uint32_t name_len_32 = (uint32_t)name_len;
            uint32_t data_len_32 = (uint32_t)compressed_data_len;
            size_t total_size = 1 + 4 + name_len_32 + 4 + data_len_32 + 8;
            
            // Bounds check: ensure total_size fits in uint32_t for wal_append
            if (total_size > UINT32_MAX) {
                printf("[LATTICE] WARN WARNING: total_size (%zu) exceeds uint32_t max, skipping WAL entry\n", total_size);
            } else {
                uint32_t total_size_32 = (uint32_t)total_size;
                char* packed = (char*)malloc(total_size);
                if (packed) {
                    size_t offset = 0;
                    packed[offset++] = (char)type;
                    
                    memcpy(packed + offset, &name_len_32, 4);
                    offset += 4;
                    memcpy(packed + offset, name, name_len_32);
                    offset += name_len_32;
                    
                    memcpy(packed + offset, &data_len_32, 4);
                    offset += 4;
                    memcpy(packed + offset, compressed_data, data_len_32);
                    offset += data_len_32;
                    
                    memcpy(packed + offset, &parent_id, 8);
                    offset += 8;
                    
                    wal_append(lattice->wal, WAL_OP_ADD_NODE, node->id, packed, total_size_32);
                    free(packed);
                }
            }
        }
    }
    
    return node->id;
}

// Binary-safe node addition (handles arbitrary binary data including null bytes)
uint64_t lattice_add_node_binary(persistent_lattice_t* lattice, lattice_node_type_t type,
                                 const char* name, const void* data, size_t data_len, uint64_t parent_id) {
    // Validate input
    if (!lattice) return 0;


    /* Global license usage: one cap per license per machine (deny add if over limit) */
    if (lattice->evaluation_mode && lattice->free_tier_limit > 0) {
        if (license_global_add_one(lattice->free_tier_limit) != 0) {
            lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
            fprintf(stderr,
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached (global)\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes across all lattices.\n"
                "  No new nodes can be added.\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit);
            fflush(stderr);
            return 0;
        }
    }

    // FREE TIER LIMIT ENFORCEMENT (Phase 1: Evaluation Mode)
    // Check if adding THIS node would exceed the limit
    if (lattice->evaluation_mode && (lattice->total_nodes + 1) > lattice->free_tier_limit) {
        lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
        fprintf(stderr, 
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached\n"
                "====================================================================\n"
                "\n"
                "  You've reached the free tier limit of %u nodes.\n"
                "  Current usage: %u nodes.\n"
                "\n"
                "  No new nodes can be added to this lattice.\n"
                "\n"
                "  Options:\n"
                "  - Delete existing nodes to free up space\n"
                "  - Upgrade to Pro tier for unlimited nodes (synrix.io)\n"
                "\n"
                "====================================================================\n\n",
                lattice->free_tier_limit, lattice->total_nodes);
        return 0;
    }
    
    // CONSTITUTIONAL CONSTRAINT VALIDATION
    // Validate prefix-based semantics (node name must have semantic prefix)
    if (!lattice_validate_prefix_semantics(name)) {
        fprintf(stderr, "[LATTICE-CONSTRAINTS] WARN Node name '%s' lacks semantic prefix - may degrade O(k) query performance\n",
                name ? name : "(null)");
    }
    
    // Validate node size constraint (data must fit in fixed-size node)
    // Max usable size is 510 bytes (512 - 2 for length header)
    size_t max_binary_size = sizeof(((lattice_node_t*)0)->data) - 2;
    if (data_len > max_binary_size) {
        fprintf(stderr, 
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Binary data length (%zu) exceeds fixed-size node capacity (%zu)\n"
                "This breaks the Lattice. Use chunked storage for large data.\n",
                data_len, max_binary_size);
        return 0; // Reject: violates fixed-size constraint
    }
    
    // Validate single-writer constraint
    if (!lattice_validate_single_writer()) {
        fprintf(stderr, 
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Multiple writers detected\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n");
        return 0; // Reject: violates single-writer constraint
    }
    
    // Reuse the core add_node logic but with binary-safe data handling
    // Get write index
    uint32_t write_index = lattice->disk_mode ? lattice->total_nodes : lattice->node_count;
    
    // Bounds check for disk mode
    if (lattice->disk_mode && write_index >= lattice->total_file_nodes) {
        printf("[LATTICE] ERROR Disk mode: write_index %u >= total_file_nodes %u\n",
               write_index, lattice->total_file_nodes);
        return 0;
    }
    
    // Grow RAM cache if needed (non-disk mode only)
    if (!lattice->disk_mode && lattice->node_count >= lattice->max_nodes) {
        uint32_t new_max = lattice->max_nodes * 2;
        lattice_node_t* new_nodes = (lattice_node_t*)realloc(lattice->nodes, new_max * sizeof(lattice_node_t));
        if (!new_nodes) return 0;
        uint64_t* new_node_id_map = (uint64_t*)realloc(lattice->node_id_map, new_max * sizeof(uint64_t));
        if (!new_node_id_map) {
            free(new_nodes);
            return 0;
        }
        uint32_t* new_access_count = (uint32_t*)realloc(lattice->access_count, new_max * sizeof(uint32_t));
        if (!new_access_count) {
            free(new_nodes);
            free(new_node_id_map);
            return 0;
        }
        memset(new_access_count + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));
        uint32_t* new_last_access = (uint32_t*)realloc(lattice->last_access, new_max * sizeof(uint32_t));
        if (!new_last_access) {
            free(new_nodes);
            free(new_node_id_map);
            free(new_access_count);
            return 0;
        }
        memset(new_last_access + lattice->max_nodes, 0, (new_max - lattice->max_nodes) * sizeof(uint32_t));
        lattice->nodes = new_nodes;
        lattice->node_id_map = new_node_id_map;
        lattice->access_count = new_access_count;
        lattice->last_access = new_last_access;
        lattice->max_nodes = new_max;
    }
    
    lattice_node_t* node = &lattice->nodes[write_index];
    
    // CRITICAL: Zero out node structure to prevent corruption from uninitialized memory
    // This ensures all fields start clean, especially important for chunk nodes
    memset(node, 0, sizeof(lattice_node_t));
    
    // Generate 64-bit ID
    uint32_t local_id;
    if (lattice->thread_safe_mode) {
        local_id = (uint32_t)__atomic_fetch_add(&lattice->next_id, 1, __ATOMIC_SEQ_CST);
    } else {
        local_id = (uint32_t)lattice->next_id++;
    }
    node->id = ((uint64_t)lattice->device_id << 32) | local_id;
    node->type = type;
    
    // Copy name (string)
    if (name) {
        size_t name_len = strlen(name);
        size_t copy_len = (name_len < sizeof(node->name) - 1) ? name_len : sizeof(node->name) - 1;
        memcpy(node->name, name, copy_len);
        node->name[copy_len] = '\0';
        node->name[sizeof(node->name) - 1] = '\0';
        
        // Validate name was copied correctly (for chunk nodes - detect corruption)
        if (strncmp(name, "CHUNK:", 6) == 0 && strncmp(node->name, "CHUNK:", 6) != 0) {
            printf("[LATTICE-BINARY] ERROR Name corruption detected! Input: '%s', Stored: '%s'\n", name, node->name);
        }
    } else {
        node->name[0] = '\0';
    }
    
    // Copy binary data (binary-safe, handles null bytes)
    // Store length in first 2 bytes (uint16_t) for binary data retrieval
    // This reduces usable space to 510 bytes but enables length tracking
    if (data && data_len > 0) {
        // Max usable data size is 510 bytes (512 - 2 for length header)
        size_t max_data_size = sizeof(node->data) - 2;
        size_t copy_len = (data_len < max_data_size) ? data_len : max_data_size;
        
        // Store length in first 2 bytes (little-endian)
        uint16_t stored_len = (uint16_t)copy_len;
        memcpy(node->data, &stored_len, 2);
        
        // Copy actual data starting at offset 2
        memcpy(node->data + 2, data, copy_len);
        
        // Zero-fill remainder
        if (copy_len < max_data_size) {
            memset(node->data + 2 + copy_len, 0, max_data_size - copy_len);
        }
    } else {
        // Empty binary data: length = 0
        uint16_t zero_len = 0;
        memcpy(node->data, &zero_len, 2);
        memset(node->data + 2, 0, sizeof(node->data) - 2);
    }
    
    node->parent_id = parent_id;
    node->child_count = 0;
    node->children = NULL;
    node->confidence = 1.0;
    node->timestamp = get_current_timestamp();
    
    // Initialize payload
    memset(&node->payload, 0, sizeof(node->payload));
    
    // Memory barrier
    __sync_synchronize();
    
    // Update mappings (same as lattice_add_node)
    if (!lattice->disk_mode || write_index < lattice->max_nodes) {
        if (write_index < lattice->max_nodes) {
            lattice->node_id_map[write_index] = node->id;
        }
        
        uint32_t local_id_for_index = (uint32_t)(node->id & 0xFFFFFFFF);
        // CRITICAL: Cap local_id_for_index to prevent huge allocations from corrupted data
        uint32_t max_safe_id = lattice->max_nodes * 2;
        if (local_id_for_index <= max_safe_id) {
            uint32_t current_index_size = (lattice->max_nodes > 10000) ? lattice->max_nodes : 10000;
            if (local_id_for_index >= current_index_size && lattice->id_to_index_map) {
                // CRITICAL FIX: Cap new_index_size to prevent huge allocations
                uint32_t new_index_size = (local_id_for_index + 10000 < max_safe_id) ? (local_id_for_index + 10000) : max_safe_id;
                uint32_t* new_id_to_index = (uint32_t*)realloc(lattice->id_to_index_map, new_index_size * sizeof(uint32_t));
                if (new_id_to_index) {
                    memset(new_id_to_index + current_index_size, 0, (new_index_size - current_index_size) * sizeof(uint32_t));
                    lattice->id_to_index_map = new_id_to_index;
                }
            }
            if (lattice->id_to_index_map && local_id_for_index < lattice->max_nodes * 10) {
                lattice->id_to_index_map[local_id_for_index] = write_index;
            }
        }
        
        // Update prefix index
        lattice_prefix_index_add_node(lattice, node->id, node->name);
    }
    
    // Update counters
    // CRITICAL: In RAM mode, always increment both node_count (RAM cache) and total_nodes
    // This ensures chunk nodes are included when saving in RAM mode
    if (!lattice->disk_mode) {
        lattice->node_count++;
    }
    lattice->total_nodes++;
    
    lattice->dirty = true;
    
    // Add to parent if specified
    if (parent_id > 0) {
        lattice_add_child(lattice, parent_id, node->id);
    }
    
    // NUCLEAR DEBUG: Right before WAL write block
    // fprintf(stderr, "!!! DEBUG: Reached end of node creation, about to check WAL: wal_enabled=%d, wal_ptr=%p, node_id=%llu\n", 
    //        lattice->wal_enabled, (void*)lattice->wal, (unsigned long long)node->id);
    // fflush(stderr);
    
    // WAL logging
    // fprintf(stderr, "!!! DEBUG: About to check WAL in lattice_add_node_internal: wal_enabled=%d, wal_ptr=%p\n", 
    //        lattice->wal_enabled, (void*)lattice->wal);
    // fflush(stderr);
    // fprintf(stderr, "[LATTICE-DEBUG] WAL check: wal_enabled=%d, wal_ptr=%p\n", 
    //        lattice->wal_enabled, (void*)lattice->wal);
    // fflush(stderr);
    if (lattice->wal_enabled && lattice->wal) {
    // fprintf(stderr, "[LATTICE-DEBUG] Entering WAL write block\n");
    // fflush(stderr);
        // Debug: Check WAL state
        if (lattice->wal->enabled == 0 || lattice->wal->wal_fd < 0) {
            fprintf(stderr, "[LATTICE] WARN WARNING: WAL not properly enabled (enabled=%d, fd=%d)\n",
                   lattice->wal->enabled, lattice->wal->wal_fd);
            fflush(stderr);
        } else {
            // Debug: Log WAL append attempt
    // fprintf(stderr, "[LATTICE-DEBUG] Attempting WAL append: node_id=%llu, wal->enabled=%d, wal->wal_fd=%d, batch_size=%u, batch_count=%u\n",
    //                (unsigned long long)node->id, lattice->wal->enabled, lattice->wal->wal_fd, 
    //                lattice->wal->batch_size, lattice->wal->batch_count);
    //         fflush(stderr);
        }
        
            // Pack binary data for WAL
            // Format: type(1) | name_len(4) | name | data_len(4) | data | parent_id(8)
            // This matches the format expected by WAL recovery
            size_t name_len = name ? strlen(name) : 0;
            size_t data_len = data ? strlen(data) : 0;
            
            // Bounds check: ensure name_len and data_len fit in uint32_t
            if (name_len > UINT32_MAX || data_len > UINT32_MAX) {
            printf("[LATTICE] WARN WARNING: name_len (%zu) or data_len (%zu) exceeds uint32_t max, skipping WAL entry\n", name_len, data_len);
        } else {
            uint32_t name_len_32 = (uint32_t)name_len;
            uint32_t data_len_32 = (uint32_t)data_len;
            size_t packed_size = 1 + 4 + name_len_32 + 4 + data_len_32 + 8; // type + name_len + name + data_len + data + parent_id
            
            // Bounds check: ensure packed_size fits in uint32_t for wal_append
            if (packed_size > UINT32_MAX) {
                printf("[LATTICE] WARN WARNING: packed_size (%zu) exceeds uint32_t max, skipping WAL entry\n", packed_size);
            } else {
                uint32_t packed_size_32 = (uint32_t)packed_size;
                char* packed_data = (char*)malloc(packed_size);
                if (packed_data) {
                    size_t offset = 0;
                    // Type byte (required for WAL recovery)
                    packed_data[offset++] = (char)type;
                    // Name length and name (cast to uint32_t for 4-byte write)
                    memcpy(packed_data + offset, &name_len_32, 4);
                    offset += 4;
                    if (name_len_32 > 0) {
                        memcpy(packed_data + offset, name, name_len_32);
                        offset += name_len_32;
                    }
                    // Data length and data (cast to uint32_t for 4-byte write)
                    memcpy(packed_data + offset, &data_len_32, 4);
                    offset += 4;
                    if (data_len_32 > 0) {
                        memcpy(packed_data + offset, data, data_len_32);
                        offset += data_len_32;
                    }
                    // Parent ID
                    memcpy(packed_data + offset, &parent_id, 8);
                    uint64_t wal_seq = wal_append(lattice->wal, WAL_OP_ADD_NODE, node->id, packed_data, packed_size_32);
                    if (wal_seq == 0) {
                        printf("[LATTICE] WARN WARNING: wal_append returned 0 (WAL write may have failed)\n");
                        printf("[LATTICE]    WAL enabled: %d, WAL ptr: %p, WAL fd: %d, wal->enabled: %d\n", 
                               lattice->wal_enabled, (void*)lattice->wal, 
                               lattice->wal ? lattice->wal->wal_fd : -1,
                               lattice->wal ? lattice->wal->enabled : 0);
                    } else {
                        printf("[LATTICE-DEBUG] WAL append successful: sequence=%lu, batch_count=%u\n",
                               wal_seq, lattice->wal->batch_count);
                    }
                    free(packed_data);
                }
            }
        }
    }
    
    return node->id;
}

// Add node with deduplication
uint64_t lattice_add_node_deduplicated(persistent_lattice_t* lattice, lattice_node_type_t type, 
                                      const char* name, const char* data, uint64_t parent_id) {
    if (!lattice) return 0;
    
    // Check if node already exists (by type and name)
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (node->type == type && strcmp(node->name, name) == 0) {
            // Node already exists - deduplicated!
            node->confidence = fmin(1.0, node->confidence + 0.1);
            node->timestamp = get_current_timestamp();
            lattice->dirty = true;
            return node->id;
        }
    }
    
    // Node doesn't exist - add new node
    return lattice_add_node(lattice, type, name, data, parent_id);
}

// WARNING: DEPRECATED: Returns pointer that becomes invalid if lattice_add_node() triggers realloc
// WARNING: Do not hold onto the returned pointer across calls to lattice_add_node()
// Use lattice_get_node_data() or lattice_get_node_copy() for safe access
lattice_node_t* lattice_get_node(persistent_lattice_t* lattice, uint64_t id) {
    if (!lattice) return NULL;
    
    // O(1) LOOKUP using reverse index (local_id -> array_index)
    // Extract local ID (lower 32 bits) for reverse index lookup
    uint32_t local_id = (uint32_t)(id & 0xFFFFFFFF);
    // Safety bound: 10x max_nodes (allows for sparse IDs)
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        uint32_t index = lattice->id_to_index_map[local_id];

        // Validate index is in range and node_id matches (sanity check)
        if (index < lattice->node_count && lattice->node_id_map[index] == id) {
            lattice->access_count[index]++;
            lattice->last_access[index] = get_current_timestamp();
            return &lattice->nodes[index];  // WARNING: UNSAFE: Pointer invalidated by realloc
        }
    }

    // Fallback to O(n) search if reverse index unavailable or out of bounds
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->node_id_map[i] == id) {
            lattice->access_count[i]++;
            lattice->last_access[i] = get_current_timestamp();
            return &lattice->nodes[i];  // WARNING: UNSAFE: Pointer invalidated by realloc
        }
    }

    // Node not in cache - load from disk on-demand
    // NOTE: In disk mode, nodes are already accessible via mmap (kernel-managed leaky bucket)
    // This path is only for RAM mode where we need to load from file
    if (lattice->storage_path[0] != '\0' && !lattice->disk_mode) {
        // OPTIMIZATION: Use direct file offset for sequential IDs (O(1) instead of O(n))
        // Node IDs are sequential: local_id = next_id++, so file_index = local_id - 1
        uint32_t local_id = (uint32_t)(id & 0xFFFFFFFF);
        
        // Try direct file offset first (fast path for sequential IDs)
        if (local_id > 0 && local_id <= lattice->total_nodes) {
            // Calculate file offset directly
            size_t header_size = 4 * sizeof(uint32_t);
            uint32_t file_index = local_id - 1; // local_id is 1-indexed, file_index is 0-indexed
            off_t node_offset = header_size + (file_index * sizeof(lattice_node_t));
            
            // Open storage file
            int fd = open(lattice->storage_path, O_RDONLY);
            if (fd >= 0) {
                // Read header to verify magic
                uint32_t header[4];
                if (read(fd, header, sizeof(header)) == sizeof(header) && header[0] == 0x4C415454) {
                    uint32_t total_nodes = header[1];
                    uint32_t nodes_in_file = header[3];
                    
                    // Bounds check
                    if (file_index < nodes_in_file && file_index < total_nodes) {
                        lattice_node_t node;
                        
                        // Read node directly at calculated offset (O(1))
                        if (lseek(fd, node_offset, SEEK_SET) == node_offset) {
                            if (read(fd, &node, sizeof(lattice_node_t)) == sizeof(lattice_node_t)) {
                                uint32_t node_local_id = (uint32_t)(node.id & 0xFFFFFFFF);
                                
                                // Verify node ID matches (safety check for sparse IDs)
                                // CRITICAL FIX: Compare local_id only, not full device-prefixed ID
                                // Device ID can change between saves/loads, but local_id is stable
                                if (node.id == id || node_local_id == local_id) {
                                    // Found the node - load into cache if space available
                                    // Use kernel-managed eviction if cache is full (leaky bucket)
                                    if (lattice->node_count >= lattice->max_nodes) {
                                        // Evict oldest node to make space (kernel handles mmap pages, we handle RAM cache)
                                        lattice_evict_oldest_nodes(lattice, 1);
                                    }
                                    
                                    if (lattice->node_count < lattice->max_nodes) {
                                        uint32_t idx = lattice->node_count;
                                        lattice->nodes[idx] = node;
                                        lattice->nodes[idx].children = NULL; // Children not stored on disk
                                        
                                        // Update mappings
                                        // CRITICAL: Store the node's actual ID (from file), not the search ID
                                        // This ensures consistency - node.id matches node_id_map[idx]
                                        lattice->node_id_map[idx] = node.id;
                                        uint32_t max_safe_id = lattice->max_nodes * 2;
                                        if (lattice->id_to_index_map && local_id < max_safe_id) {
                                            // Grow reverse index if needed
                                            uint32_t current_index_size = (lattice->max_nodes > 10000) ? lattice->max_nodes : 10000;
                                            if (local_id >= current_index_size) {
                                                uint32_t new_size = (local_id + 10000 < max_safe_id) ? (local_id + 10000) : max_safe_id;
                                                uint32_t* new_map = (uint32_t*)realloc(lattice->id_to_index_map, new_size * sizeof(uint32_t));
                                                if (new_map) {
                                                    memset(new_map + current_index_size, 0, (new_size - current_index_size) * sizeof(uint32_t));
                                                    lattice->id_to_index_map = new_map;
                                                    current_index_size = new_size;
                                                }
                                            }
                                            if (lattice->id_to_index_map && local_id < current_index_size) {
                                                lattice->id_to_index_map[local_id] = idx;
                                            }
                                        }
                                        
                                        lattice->access_count[idx] = 1;
                                        lattice->last_access[idx] = get_current_timestamp();
                                        lattice->node_count++;
                                        
                                        close(fd);
                                        return &lattice->nodes[idx]; // WARNING: UNSAFE: Pointer invalidated by realloc
                                    }
                                    close(fd);
                                    return NULL; // Cache full (shouldn't happen after eviction)
                                }
                                // ID mismatch - fall through to linear scan for sparse IDs
                            }
                        }
                    }
                }
                close(fd);
            }
        }
        
        // Fallback: Linear scan for sparse/non-sequential IDs (chunked nodes, etc.)
        // This is slower but handles cases where IDs are not sequential
        int fd = open(lattice->storage_path, O_RDONLY);
        if (fd >= 0) {
            uint32_t header[4];
            if (read(fd, header, sizeof(header)) == sizeof(header) && header[0] == 0x4C415454) {
                uint32_t total_nodes = header[1];
                uint32_t nodes_in_file = header[3];
                
                // Limit scan to reasonable range (don't scan entire 50M node file)
                uint32_t max_scan = (nodes_in_file < 100000) ? nodes_in_file : 100000;
                for (uint32_t i = 0; i < max_scan && i < total_nodes; i++) {
                    lattice_node_t node;
                    size_t header_size = 4 * sizeof(uint32_t);
                    off_t node_offset = header_size + (i * sizeof(lattice_node_t));
                    
                    if (lseek(fd, node_offset, SEEK_SET) == node_offset) {
                        if (read(fd, &node, sizeof(lattice_node_t)) == sizeof(lattice_node_t)) {
                            // CRITICAL FIX: Compare local_id only for sparse ID fallback
                            uint32_t node_local_id = (uint32_t)(node.id & 0xFFFFFFFF);
                            if (node.id == id || node_local_id == local_id) {
                                // Found the node - load into cache if space available
                                // Use eviction if cache is full
                                if (lattice->node_count >= lattice->max_nodes) {
                                    lattice_evict_oldest_nodes(lattice, 1);
                                }
                                
                                if (lattice->node_count < lattice->max_nodes) {
                                    uint32_t idx = lattice->node_count;
                                    lattice->nodes[idx] = node;
                                    lattice->nodes[idx].children = NULL;
                                    
                                    // CRITICAL: Store the node's actual ID (from file), not the search ID
                                    lattice->node_id_map[idx] = node.id;
                                    uint32_t local_id_found = (uint32_t)(node.id & 0xFFFFFFFF);
                                    uint32_t max_safe_id = lattice->max_nodes * 2;
                                    if (lattice->id_to_index_map && local_id_found < max_safe_id) {
                                        uint32_t current_index_size = (lattice->max_nodes > 10000) ? lattice->max_nodes : 10000;
                                        if (local_id_found >= current_index_size) {
                                            uint32_t new_size = (local_id_found + 10000 < max_safe_id) ? (local_id_found + 10000) : max_safe_id;
                                            uint32_t* new_map = (uint32_t*)realloc(lattice->id_to_index_map, new_size * sizeof(uint32_t));
                                            if (new_map) {
                                                memset(new_map + current_index_size, 0, (new_size - current_index_size) * sizeof(uint32_t));
                                                lattice->id_to_index_map = new_map;
                                                current_index_size = new_size;
                                            }
                                        }
                                        if (lattice->id_to_index_map && local_id_found < current_index_size) {
                                            lattice->id_to_index_map[local_id_found] = idx;
                                        }
                                    }
                                    
                                    lattice->access_count[idx] = 1;
                                    lattice->last_access[idx] = get_current_timestamp();
                                    lattice->node_count++;
                                    
                                    close(fd);
                                    return &lattice->nodes[idx];
                                }
                                close(fd);
                                return NULL; // Cache full
                            }
                        }
                    }
                }
            }
            close(fd);
        }
    }
    
    return NULL;
}

// SAFE API: Copy node data to user-provided buffer (pointer-safe, realloc-safe)
int lattice_get_node_data(persistent_lattice_t* lattice, uint64_t id, lattice_node_t* out_node) {
    if (!lattice || !out_node) return -1;
    
    // Safety: Ensure lattice structure is valid
    if (!lattice->nodes || !lattice->node_id_map) {
        return -1; // Lattice not properly initialized
    }
    
    // O(1) LOOKUP using reverse index
    // Safety bound: Check that id is within allocated bounds before accessing array
    // CRITICAL: The reverse index can be grown dynamically during load, but we use
    // a conservative bound check: if id > max_nodes * 10, fall back to O(n) search
    // This prevents out-of-bounds access while still allowing sparse IDs
    if (lattice->id_to_index_map != NULL && id > 0) {
        // Conservative safety bound: 10x max_nodes (allows for sparse IDs but prevents huge allocations)
        // The reverse index might have been grown during load, but we don't track its exact size
        // So we use a conservative bound that's safe for typical use cases
        uint32_t safe_bound = lattice->max_nodes * 10;
        if (id < safe_bound) {
            // Additional safety: Validate pointer is still valid (not corrupted)
            // Use volatile to prevent compiler optimizations
            volatile uint32_t* safe_map = (volatile uint32_t*)lattice->id_to_index_map;
            if (safe_map == NULL) {
                // Pointer became NULL - fall through to O(n) search
            } else {
                uint32_t index = safe_map[id];
                
                // Validate index is valid and node_id matches
                // Note: index can be 0 (first node), so check < node_count, not > 0
                if (index < lattice->node_count && lattice->node_id_map && 
                    index < lattice->max_nodes && lattice->node_id_map[index] == id) {
                    lattice->access_count[index]++;
                    lattice->last_access[index] = get_current_timestamp();
                    // SAFE: Copy data instead of returning pointer
                    *out_node = lattice->nodes[index];
                    
                    // Async prefetch: Predict and preload likely next nodes (non-blocking)
                    if (lattice->prefetch_enabled) {
                        lattice_prefetch_related_nodes(lattice, id);
                    }
                    // Deep copy children array if present
                    if (lattice->nodes[index].child_count > 0 && lattice->nodes[index].children) {
                        out_node->children = (uint64_t*)malloc(lattice->nodes[index].child_count * sizeof(uint64_t));
                        if (out_node->children) {
                            memcpy(out_node->children, lattice->nodes[index].children, 
                                   lattice->nodes[index].child_count * sizeof(uint64_t));  // FIX: was uint32_t, should be uint64_t
                        } else {
                            out_node->child_count = 0;  // Failed to allocate, mark as empty
                        }
                    } else {
                        out_node->children = NULL;
                    }
                    return 0;
                }
            }
        }
        // If id >= safe_bound, fall through to O(n) search (safer than risking out-of-bounds)
    }
    
    // Fallback to O(n) search in RAM cache
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->node_id_map[i] == id) {
            lattice->access_count[i]++;
            lattice->last_access[i] = get_current_timestamp();
            // SAFE: Copy data instead of returning pointer
            *out_node = lattice->nodes[i];
            // Deep copy children array if present
            if (lattice->nodes[i].child_count > 0 && lattice->nodes[i].children) {
                out_node->children = (uint64_t*)malloc(lattice->nodes[i].child_count * sizeof(uint64_t));
                if (out_node->children) {
                    memcpy(out_node->children, lattice->nodes[i].children, 
                           lattice->nodes[i].child_count * sizeof(uint64_t));  // FIX: was uint32_t, should be uint64_t
                } else {
                    out_node->child_count = 0;
                }
            } else {
                out_node->children = NULL;
            }
            return 0;
        }
    }
    
    // ON-DEMAND LOADING: If node not in RAM cache, load from disk (RAM mode only)
    // NOTE: In disk mode, nodes are already accessible via mmap (kernel-managed leaky bucket)
    // This path is only for RAM mode where we need to load from file
    if (!lattice->disk_mode && lattice->storage_path[0] != '\0') {
        // Try to load from disk using optimized O(1) direct file offset
        lattice_node_t* loaded_node = lattice_get_node(lattice, id);
        if (loaded_node) {
            // Node was loaded into cache - copy to output
            *out_node = *loaded_node;
            // Deep copy children array if present
            if (loaded_node->child_count > 0 && loaded_node->children) {
                out_node->children = (uint64_t*)malloc(loaded_node->child_count * sizeof(uint64_t));
                if (out_node->children) {
                    memcpy(out_node->children, loaded_node->children, 
                           loaded_node->child_count * sizeof(uint64_t));
                } else {
                    out_node->child_count = 0;
                }
            } else {
                out_node->children = NULL;
            }
            return 0;
        }
    }
    
    // DISK MODE: If node not in RAM cache, access directly from mmap'd file
    // In disk mode, node IDs are 64-bit device-prefixed: (device_id << 32) | local_id
    // Extract local_id (lower 32 bits) to get the file index
    if (lattice->disk_mode && id > 0) {
        // Extract local_id from 64-bit device-prefixed ID
        uint32_t local_id = (uint32_t)(id & 0xFFFFFFFF);
        
        // In disk mode, nodes are written sequentially, so local_id corresponds to file position
        // Note: local_id is 1-indexed (first node has local_id=1), so file_index = local_id - 1
        uint32_t file_index = (local_id > 0) ? (local_id - 1) : 0;
        
        // Bounds check: ensure we don't access beyond the file
        // In disk mode, lattice->nodes points to the full mmap'd file (after header)
        // So we can access any node in the file via lattice->nodes[file_index]
        if (file_index < lattice->total_file_nodes && local_id <= lattice->total_nodes &&
            lattice->nodes && lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
            // Access node directly from mmap'd memory
            lattice_node_t* file_node = &lattice->nodes[file_index];
            
            // In disk mode, nodes are written sequentially, so file_index corresponds to local_id-1
            // The node ID might be device-prefixed (64-bit), so we compare local_id portion
            uint32_t node_local_id = (uint32_t)(file_node->id & 0xFFFFFFFF);
            if (file_node->id == id || node_local_id == local_id || file_node->id == 0) {
                // Accept if:
                // 1. Full IDs match (64-bit device-prefixed)
                // 2. Local IDs match (32-bit portion)
                // 3. Node is uninitialized (id==0) - use file_index as truth
                // Copy node data
                *out_node = *file_node;
                
                // Deep copy children array if present
                if (file_node->child_count > 0 && file_node->children) {
                    out_node->children = (uint64_t*)malloc(file_node->child_count * sizeof(uint64_t));
                    if (out_node->children) {
                        memcpy(out_node->children, file_node->children, 
                               file_node->child_count * sizeof(uint64_t));  // FIX: was uint32_t, should be uint64_t
                    } else {
                        out_node->child_count = 0;
                    }
                } else {
                    out_node->children = NULL;
                }
                return 0;
            }
        }
    }
    
    return -1;  // Node not found
}

// SAFE API: Allocate and copy node data (caller must free with lattice_free_node_copy())
lattice_node_t* lattice_get_node_copy(persistent_lattice_t* lattice, uint64_t id) {
    if (!lattice) return NULL;
    
    lattice_node_t* copy = (lattice_node_t*)malloc(sizeof(lattice_node_t));
    if (!copy) return NULL;
    
    if (lattice_get_node_data(lattice, id, copy) == 0) {
        return copy;
    }
    
    free(copy);
    return NULL;
}

// Free a node copy allocated by lattice_get_node_copy()
void lattice_free_node_copy(lattice_node_t* node) {
    if (node) {
        if (node->children) {
            free(node->children);
        }
        free(node);
    }
}

// Check if a node contains binary data
bool lattice_is_node_binary(persistent_lattice_t* lattice, uint64_t id) {
    if (!lattice) return false;
    
    lattice_node_t node;
    if (lattice_get_node_data(lattice, id, &node) != 0) {
        return false;
    }
    
    // Check if data is binary (has length header in first 2 bytes)
    // Binary data format: first 2 bytes = uint16_t length, data starts at offset 2
    uint16_t potential_len;
    memcpy(&potential_len, node.data, 2);
    
    // If length is reasonable (0-510), check if it's actually binary
    // Binary data: length header + data that may contain nulls
    // Text data: null-terminated string starting at offset 0
    if (potential_len > 0 && potential_len <= 510) {
        // Check if data at offset 2 is null-terminated within the length
        // If the string length (from offset 2) matches the stored length, it might be text
        // But if there are nulls in the middle or the string is longer, it's binary
        size_t string_len_from_offset_2 = strnlen(node.data + 2, potential_len);
        
        // If the string length from offset 2 is less than the stored length,
        // it means there are null bytes in the middle -> binary
        // Also check if offset 0 itself is null-terminated (text would be)
        size_t string_len_from_offset_0 = strnlen(node.data, sizeof(node.data));
        
        // Binary if: stored length > 0 AND (string from offset 2 has nulls in middle OR offset 0 is not null-terminated)
        bool has_nulls_in_middle = (string_len_from_offset_2 < potential_len);
        bool offset_0_not_text = (string_len_from_offset_0 >= sizeof(node.data) || string_len_from_offset_0 == 0);
        
        // Most reliable: if offset 0 is not a valid string AND we have a length header, it's binary
        if (offset_0_not_text || has_nulls_in_middle) {
            return true;
        }
    }
    
    return false;
}

// Validate node data access (warns if using wrong API for binary/text)
int lattice_validate_data_access(persistent_lattice_t* lattice, uint64_t id, bool expecting_text) {
    if (!lattice) return -1;
    
    bool is_binary = lattice_is_node_binary(lattice, id);
    
    if (expecting_text && is_binary) {
        fprintf(stderr, "[LATTICE] WARN WARNING: Node %lu contains BINARY data, but text API was used.\n", id);
        fprintf(stderr, "[LATTICE]    Use lattice_get_node_data_binary() or lattice_update_node_binary() instead.\n");
        fprintf(stderr, "[LATTICE]    Calling strlen() or strncpy() on binary data will corrupt it.\n");
        return -1; // Warning issued
    }
    
    if (!expecting_text && !is_binary) {
        // This is fine - binary API can handle text data
        return 0;
    }
    
    return 0; // Safe
}

// Update node data
int lattice_update_node(persistent_lattice_t* lattice, uint64_t id, const char* data) {
    if (!lattice || !data) return -1;
    
    // Runtime safety check: warn if updating binary node with text API
    lattice_validate_data_access(lattice, id, true);
    
    // Find node index using same logic as lattice_get_node_data
    uint32_t index = UINT32_MAX;
    if (lattice->id_to_index_map != NULL && id > 0) {
        uint32_t safe_bound = lattice->max_nodes * 10;
        if (id < safe_bound) {
            uint32_t idx = lattice->id_to_index_map[id];
            if (idx < lattice->node_count && lattice->node_id_map && 
                idx < lattice->max_nodes && lattice->node_id_map[idx] == id) {
                index = idx;
            }
        }
    }
    
    // Fallback to O(n) search if not found via index
    if (index == UINT32_MAX) {
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            if (lattice->node_id_map[i] == id) {
                index = i;
                break;
            }
        }
    }
    
    if (index == UINT32_MAX || index >= lattice->node_count) return -1;
    
    // Update node data directly
    strncpy(lattice->nodes[index].data, data, sizeof(lattice->nodes[index].data) - 1);
    lattice->nodes[index].data[sizeof(lattice->nodes[index].data) - 1] = '\0';
    lattice->nodes[index].timestamp = get_current_timestamp();
    lattice->dirty = true;
    
    return 0;
}

// Binary-safe node update (handles arbitrary binary data including null bytes)
int lattice_update_node_binary(persistent_lattice_t* lattice, uint64_t id, const void* data, size_t data_len) {
    lattice_node_t* node = NULL;
    
    // O(1) lookup: Try RAM cache first
    uint32_t local_id = (uint32_t)(id & 0xFFFFFFFF);
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        uint32_t index = lattice->id_to_index_map[local_id];
        if (index < lattice->node_count && lattice->node_id_map[index] == id) {
            node = &lattice->nodes[index];
        }
    }
    
    // O(1) disk mode: Direct file index access (nodes are sequential)
    if (!node && lattice->disk_mode && id > 0) {
        uint32_t file_index = (local_id > 0) ? (local_id - 1) : 0;
        if (file_index < lattice->total_file_nodes && local_id <= lattice->total_nodes) {
            lattice_node_t* file_node = &lattice->nodes[file_index];
            if (file_node->id == id) {
                node = file_node;
            }
        }
    }
    
    // Fallback: O(n) search in RAM cache (only if above methods fail)
    if (!node) {
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            if (lattice->node_id_map[i] == id) {
                node = &lattice->nodes[i];
                break;
            }
        }
    }
    
    if (!node) return -1;
    
    // Validate input
    if (data_len > sizeof(node->data)) {
        printf("[LATTICE] ERROR Binary data too large: %zu bytes (max %zu)\n", 
               data_len, sizeof(node->data));
        return -1;
    }
    
    // Copy binary data (binary-safe, handles null bytes)
    // Store length in first 2 bytes (uint16_t) for binary data retrieval
    if (data && data_len > 0) {
        // Max usable data size is 510 bytes (512 - 2 for length header)
        size_t max_data_size = sizeof(node->data) - 2;
        size_t copy_len = (data_len < max_data_size) ? data_len : max_data_size;
        
        // Store length in first 2 bytes (little-endian)
        uint16_t stored_len = (uint16_t)copy_len;
        memcpy(node->data, &stored_len, 2);
        
        // Copy actual data starting at offset 2
        memcpy(node->data + 2, data, copy_len);
        
        // Zero-fill remainder
        if (copy_len < max_data_size) {
            memset(node->data + 2 + copy_len, 0, max_data_size - copy_len);
        }
    } else {
        // Empty binary data: length = 0
        uint16_t zero_len = 0;
        memcpy(node->data, &zero_len, 2);
        memset(node->data + 2, 0, sizeof(node->data) - 2);
    }
    
    node->timestamp = get_current_timestamp();
    lattice->dirty = true;
    
    // WAL logging (pack with length header for binary data)
    // Note: If data is compressed, the compression flag (bit 15) is preserved in the length header
    if (lattice->wal_enabled && lattice->wal) {
        // Check if node data has compression flag set (read from node, not input)
        // This preserves compression state during WAL logging
        uint16_t stored_length_header;
        memcpy(&stored_length_header, node->data, 2);
        
        // Pack: 2 bytes length (with compression flag if set) + data
        // Bounds check: ensure data_len fits in uint32_t
        if (data_len > UINT32_MAX - 2) {
            printf("[LATTICE] WARN WARNING: data_len (%zu) exceeds uint32_t max - 2, skipping WAL entry\n", data_len);
        } else {
            size_t packed_size = 2 + data_len;
            // Bounds check: ensure packed_size fits in uint32_t for wal_append
            if (packed_size > UINT32_MAX) {
                printf("[LATTICE] WARN WARNING: packed_size (%zu) exceeds uint32_t max, skipping WAL entry\n", packed_size);
            } else {
                uint32_t packed_size_32 = (uint32_t)packed_size;
                char* packed_data = (char*)malloc(packed_size);
                if (packed_data) {
                    // Use stored length header (preserves compression flag)
                    memcpy(packed_data, &stored_length_header, 2);
                    if (data_len > 0) {
                        memcpy(packed_data + 2, data, data_len);
                    }
                    wal_append(lattice->wal, WAL_OP_UPDATE_NODE, id, packed_data, packed_size_32);
                    free(packed_data);
                }
            }
        }
    }
    
    return 0;
}

// Binary-safe data retrieval (returns actual data length for binary nodes)
int lattice_get_node_data_binary(persistent_lattice_t* lattice, uint64_t id, 
                                 void* out_data, size_t* out_data_len, bool* is_binary) {
    if (!lattice || !out_data || !out_data_len || !is_binary) return -1;
    
    lattice_node_t node;
    if (lattice_get_node_data(lattice, id, &node) != 0) {
        return -1;
    }
    
    // Check if data is binary (has length header in first 2 bytes)
    // Binary data format: first 2 bytes = uint16_t length, data starts at offset 2
    uint16_t potential_len;
    memcpy(&potential_len, node.data, 2);
    
    // Use the same detection logic as lattice_is_node_binary
    bool is_binary_data = false;
    if (potential_len > 0 && potential_len <= 510) {
        size_t string_len_from_offset_2 = strnlen(node.data + 2, potential_len);
        size_t string_len_from_offset_0 = strnlen(node.data, sizeof(node.data));
        
        bool has_nulls_in_middle = (string_len_from_offset_2 < potential_len);
        bool offset_0_not_text = (string_len_from_offset_0 >= sizeof(node.data) || string_len_from_offset_0 == 0);
        
        if (offset_0_not_text || has_nulls_in_middle) {
            is_binary_data = true;
        }
    }
    
    if (is_binary_data) {
        // Binary data: extract length and data
        *is_binary = true;
        *out_data_len = potential_len;
        
        // Copy data (skip first 2 bytes which contain length)
        size_t copy_len = (potential_len < sizeof(node.data) - 2) ? potential_len : sizeof(node.data) - 2;
        memcpy(out_data, node.data + 2, copy_len);
    } else {
        // Text data: null-terminated string
        *is_binary = false;
        size_t text_len = strlen(node.data);
        *out_data_len = text_len;
        memcpy(out_data, node.data, text_len + 1); // Include null terminator
    }
    
    return 0;
}

// ============================================================================
// CHUNKED DATA STORAGE (Hybrid Strategy)
// ============================================================================

// Check if node is chunked data header
bool lattice_is_node_chunked(persistent_lattice_t* lattice, uint64_t id) {
    if (!lattice) return false;
    
    lattice_node_t node;
    if (lattice_get_node_data(lattice, id, &node) != 0) {
        return false;
    }
    
    // Check if name starts with "C:" (chunked node prefix)
    // Also check if name is at least 8 characters (safety check)
    if (strlen(node.name) < 8) {
        return false;
    }
    
    return (strncmp(node.name, "C:", 2) == 0);
}

// Hash embedding versioning support
// Store embedding metadata in node's data[512] field (uses binary API)
int lattice_store_embedding_metadata(persistent_lattice_t* lattice, uint64_t node_id,
                                     const embedding_metadata_t* metadata) {
    if (!lattice || !metadata) return -1;
    
    // Store metadata using binary API (fits in 510 bytes)
    size_t metadata_size = sizeof(embedding_metadata_t);
    if (metadata_size > 510) {
        printf("[LATTICE-EMBEDDING] ERROR Metadata too large (%zu bytes, max 510)\n", metadata_size);
        return -1;
    }
    
    // Use binary update API to store metadata
    return lattice_update_node_binary(lattice, node_id, metadata, metadata_size);
}

// Retrieve embedding metadata from node
int lattice_get_embedding_metadata(persistent_lattice_t* lattice, uint64_t node_id,
                                   embedding_metadata_t* metadata) {
    if (!lattice || !metadata) return -1;
    
    // Allocate buffer for binary data retrieval
    uint8_t data_buffer[512];
    size_t data_len = 0;
    bool is_binary = false;
    
    if (lattice_get_node_data_binary(lattice, node_id, data_buffer, &data_len, &is_binary) != 0) {
        return -1;
    }
    
    // Check if data is binary and has correct size
    if (!is_binary || data_len != sizeof(embedding_metadata_t)) {
        return -1;
    }
    
    // Copy metadata
    memcpy(metadata, data_buffer, sizeof(embedding_metadata_t));
    
    return 0;
}

// Check if node has embedding metadata
bool lattice_has_embedding_metadata(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return false;
    
    embedding_metadata_t metadata;
    return (lattice_get_embedding_metadata(lattice, node_id, &metadata) == 0);
}

// Chunked data storage - splits large data across multiple nodes
// Parent: name="C:original_name", data=metadata (total_size, chunk_count, chunk_ids)
// Children: name="CHUNK:parent_id:index:total", data=chunk payload with index header
uint64_t lattice_add_node_chunked(persistent_lattice_t* lattice,
                                  lattice_node_type_t type,
                                  const char* name,
                                  const void* data,
                                  size_t data_len,
                                  uint64_t parent_id) {
    if (!lattice || !name || !data || data_len == 0) return 0;
    
    // Chunk size: 500 bytes payload + 10 bytes header (index:8 + length:2)
    const size_t CHUNK_PAYLOAD_SIZE = 500;
    const size_t CHUNK_HEADER_SIZE = 10;  // chunk_index:8 + chunk_length:2
    // CHUNK_TOTAL_SIZE = CHUNK_HEADER_SIZE + CHUNK_PAYLOAD_SIZE = 510 bytes (documented)
    
    // Calculate number of chunks needed
    uint32_t chunk_count = (uint32_t)((data_len + CHUNK_PAYLOAD_SIZE - 1) / CHUNK_PAYLOAD_SIZE);
    
    if (chunk_count == 0) {
        printf("[LATTICE-CHUNK] ERROR Invalid chunk count\n");
        return 0;
    }
    
    // FREE TIER LIMIT ENFORCEMENT (Phase 1: Evaluation Mode)
    // Check if adding all chunks (1 parent + chunk_count children) would exceed limit
    uint32_t total_nodes_needed = 1 + chunk_count; // 1 parent + N chunks
    if (lattice->evaluation_mode && (lattice->total_nodes + total_nodes_needed) > lattice->free_tier_limit) {
        lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
        fprintf(stderr, 
                "\n"
                "====================================================================\n"
                "  SYNRIX: Free Tier Limit Reached\n"
                "====================================================================\n"
                "  Cannot add chunked data: would exceed free tier limit of %u nodes.\n"
                "  Current usage: %u nodes\n"
                "  Required: %u nodes (1 parent + %u chunks)\n\n"
                "  No new nodes can be added to the lattice.\n\n"
                "  Options:\n"
                "  - Delete existing nodes to free up space\n"
                "  - Upgrade to Pro tier for unlimited nodes (synrix.io)\n"
                "  - Contact support for assistance\n"
                "====================================================================\n\n",
                lattice->free_tier_limit, lattice->total_nodes, total_nodes_needed, chunk_count);
        return 0;
    }
    
    // Allocate chunk IDs array
    uint64_t* chunk_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
    if (!chunk_ids) {
        printf("[LATTICE-CHUNK] ERROR Failed to allocate chunk IDs array\n");
        return 0;
    }
    
    // Create parent node (chunk header)
    // Use "C:" prefix (2 bytes) instead of "CHUNKED:" (8 bytes) to save 6 bytes
    char parent_name[64];
    snprintf(parent_name, sizeof(parent_name), "C:%s", name);
    
    // Parent metadata format (binary) - optimized for O(k) reassembly:
    // Small metadata: [total_size:8][chunk_count:4][checksum:8][first_chunk_local_id:4] = 24 bytes
    // Large metadata: [total_size:8][chunk_count:4][checksum:8][first_chunk_local_id:4][chunk_id_1:8]...[chunk_id_N:8]
    // Strategy: Always store first_chunk_local_id for O(k) sequential read, optionally store all IDs if space allows
    size_t small_metadata_size = 24;  // total_size:8 + chunk_count:4 + checksum:8 + first_chunk_local_id:4
    size_t full_metadata_size = 24 + (chunk_count * 8);  // + chunk IDs array
    
    // Use full metadata if it fits, otherwise use small metadata (O(k) sequential read)
    size_t metadata_size = (full_metadata_size <= 510) ? full_metadata_size : small_metadata_size;
    
    uint8_t* metadata = (uint8_t*)malloc(metadata_size);
    if (!metadata) {
        free(chunk_ids);
        return 0;
    }
    
    // Write metadata header (first_chunk_local_id will be filled after first chunk is created)
    memcpy(metadata, &data_len, 8);  // total_size
    memcpy(metadata + 8, &chunk_count, 4);  // chunk_count
    uint64_t checksum = 0;  // TODO: Calculate CRC32 checksum
    memcpy(metadata + 12, &checksum, 8);  // checksum
    uint32_t first_chunk_local_id = 0;  // Will be set after first chunk creation
    memcpy(metadata + 20, &first_chunk_local_id, 4);  // first_chunk_local_id (placeholder)
    
    // Create parent node (metadata will be updated after chunks are created)
    // Use LATTICE_NODE_CHUNK_HEADER for parent chunked node
    uint64_t parent_node_id = lattice_add_node_binary(lattice, LATTICE_NODE_CHUNK_HEADER, parent_name,
                                                     metadata, metadata_size, parent_id);
    if (parent_node_id == 0) {
        free(chunk_ids);
        free(metadata);
        return 0;
    }
    
    // Create chunk nodes (sequential creation - chunks are written in order)
    const uint8_t* data_ptr = (const uint8_t*)data;
    uint32_t progress_interval = (chunk_count > 1000) ? (chunk_count / 100) : 1;  // 1% progress updates
    
    for (uint32_t i = 0; i < chunk_count; i++) {
        // Progress indicator for large chunking operations
        if (i > 0 && (i % progress_interval == 0 || i == chunk_count - 1)) {
            // Progress reporting disabled for maximum throughput test
            // printf("[LATTICE-CHUNK] Progress: %u/%u chunks (%.1f%%)\n", 
            //        i + 1, chunk_count, ((i + 1) * 100.0) / chunk_count);
        }
        // Calculate chunk size (last chunk may be smaller)
        size_t chunk_payload_size = (i == chunk_count - 1) 
            ? (data_len - (i * CHUNK_PAYLOAD_SIZE))
            : CHUNK_PAYLOAD_SIZE;
        
        if (chunk_payload_size > CHUNK_PAYLOAD_SIZE) {
            chunk_payload_size = CHUNK_PAYLOAD_SIZE;
        }
        
        // Create chunk name: "C:parent_id:index:total" (using "C:" prefix to save space)
        char chunk_name[64];
        int name_len = snprintf(chunk_name, sizeof(chunk_name), "C:%lu:%u:%u", 
                parent_node_id, i, chunk_count);
        
        // Validate name was created correctly
        if (name_len < 0 || name_len >= (int)sizeof(chunk_name)) {
            printf("[LATTICE-CHUNK] ERROR Failed to create chunk name (snprintf returned %d)\n", name_len);
            free(chunk_ids);
            free(metadata);
            return 0;
        }
        
        // Verify name starts with "C:" (chunked prefix)
        if (strncmp(chunk_name, "C:", 2) != 0) {
            printf("[LATTICE-CHUNK] ERROR Chunk name missing 'C:' prefix: '%s'\n", chunk_name);
            free(chunk_ids);
            free(metadata);
            return 0;
        }
        
        // Chunk data format: [chunk_index:8][chunk_length:2][chunk_payload:500]
        size_t chunk_data_size = CHUNK_HEADER_SIZE + chunk_payload_size;
        uint8_t* chunk_data = (uint8_t*)malloc(chunk_data_size);
        if (!chunk_data) {
            // Cleanup: delete parent and already-created chunks
            // TODO: Implement cleanup
            free(chunk_ids);
            free(metadata);
            return 0;
        }
        
        // Write chunk header
        uint64_t chunk_index = i;
        uint16_t chunk_length = (uint16_t)chunk_payload_size;
        memcpy(chunk_data, &chunk_index, 8);  // chunk_index
        memcpy(chunk_data + 8, &chunk_length, 2);  // chunk_length
        
        // Write chunk payload
        memcpy(chunk_data + CHUNK_HEADER_SIZE, data_ptr + (i * CHUNK_PAYLOAD_SIZE), 
               chunk_payload_size);
        
        // Create chunk node
        // Use LATTICE_NODE_CHUNK_DATA for chunk data nodes
        uint64_t chunk_id = lattice_add_node_binary(lattice, LATTICE_NODE_CHUNK_DATA, chunk_name,
                                                   chunk_data, chunk_data_size, 
                                                   parent_node_id);
        if (chunk_id == 0) {
            free(chunk_data);
            free(chunk_ids);
            free(metadata);
            return 0;
        }
        
        chunk_ids[i] = chunk_id;
        free(chunk_data);
        
        // Store first chunk's local_id for O(k) sequential read
        if (i == 0) {
            first_chunk_local_id = (uint32_t)(chunk_id & 0xFFFFFFFF);
            memcpy(metadata + 20, &first_chunk_local_id, 4);
        }
    }
    
    // Update parent metadata:
    // - Always update first_chunk_local_id (for O(k) sequential read)
    // - Optionally update full chunk IDs array if space allows (for O(1) direct access)
    
    if (metadata_size >= 24 + (chunk_count * 8)) {
        // Full metadata: update chunk IDs array (only if it fits in 510 bytes)
        for (uint32_t i = 0; i < chunk_count; i++) {
            memcpy(metadata + 24 + (i * 8), &chunk_ids[i], 8);
        }
    }
    // else: Using small metadata (first_chunk_local_id only) - already set in loop
    
    // Always update parent with at least first_chunk_local_id
    // O(1) lookup: Parent was created first, so it's at a low file index
    // In disk mode, we can access it directly by local_id
    int update_result = lattice_update_node_binary(lattice, parent_node_id, metadata, metadata_size);
    if (update_result != 0) {
        printf("[LATTICE-CHUNK] WARN Warning: Parent metadata update returned %d\n", update_result);
    }
    
    free(chunk_ids);
    free(metadata);
    
    // Verbose chunk logging removed for performance (4k+ messages slow down indexing)
    // Uncomment for debugging: printf("[LATTICE-CHUNK] OK Created chunked data: %zu bytes in %u chunks (parent: %lu)\n", data_len, chunk_count, parent_node_id);
    
    return parent_node_id;
}

// Reassemble chunked data
int lattice_get_node_chunked(persistent_lattice_t* lattice,
                             uint64_t parent_id,
                             void** out_data,
                             size_t* out_data_len) {
    if (!lattice || !out_data || !out_data_len) return -1;
    
    // Get parent node
    lattice_node_t parent;
    if (lattice_get_node_data(lattice, parent_id, &parent) != 0) {
        printf("[LATTICE-CHUNK] ERROR Parent node %lu not found\n", parent_id);
        return -1;
    }
    
    // Verify it's a chunked node
        if (strncmp(parent.name, "C:", 2) != 0) {
        printf("[LATTICE-CHUNK] ERROR Node %lu is not a chunked data header\n", parent_id);
        return -1;
    }
    
    // Parse parent metadata
    size_t total_size = 0;
    uint32_t chunk_count = 0;
    uint32_t first_chunk_local_id = 0;
    bool is_binary = false;
    size_t metadata_len = 0;
    uint8_t parent_data[512];
    
    if (lattice_get_node_data_binary(lattice, parent_id, parent_data, &metadata_len, &is_binary) == 0) {
        // Binary data: parent_data already has length header stripped
        if (metadata_len >= 24) {
            memcpy(&total_size, parent_data, 8);
            memcpy(&chunk_count, parent_data + 8, 4);
            memcpy(&first_chunk_local_id, parent_data + 20, 4);  // first_chunk_local_id
        } else if (metadata_len >= 12) {
            // Legacy format (old metadata without first_chunk_local_id)
            memcpy(&total_size, parent_data, 8);
            memcpy(&chunk_count, parent_data + 8, 4);
        }
    } else {
        // Fallback: read from parent node struct directly
        if (lattice_is_node_binary(lattice, parent_id)) {
            // Binary format: first 2 bytes = length, data starts at offset 2
            uint16_t len;
            memcpy(&len, parent.data, 2);
            if (len >= 24) {
                memcpy(&total_size, parent.data + 2, 8);
                memcpy(&chunk_count, parent.data + 10, 4);
                memcpy(&first_chunk_local_id, parent.data + 22, 4);  // first_chunk_local_id
            } else if (len >= 12) {
                // Legacy format
                memcpy(&total_size, parent.data + 2, 8);
                memcpy(&chunk_count, parent.data + 10, 4);
            }
        }
    }
    
    if (chunk_count == 0 || total_size == 0) {
        printf("[LATTICE-CHUNK] ERROR Invalid metadata: size=%zu, chunks=%u\n", 
               total_size, chunk_count);
        return -1;
    }
    
    // Allocate output buffer
    void* reassembled = malloc(total_size);
    if (!reassembled) {
        printf("[LATTICE-CHUNK] ERROR Failed to allocate %zu bytes\n", total_size);
        return -1;
    }
    
    // Discovery strategy: O(k) sequential read using first_chunk_local_id
    // This maintains O(1) parent lookup + O(k) reassembly (k = chunk count, necessary to read all chunks)
    uint64_t* chunk_ids = NULL;
    bool use_sequential_path = false;
    
    // O(k) Sequential Path: Use first_chunk_local_id to read chunks directly by file index
    // Chunks are created sequentially, so we can read them directly without scanning
    if (first_chunk_local_id > 0 && lattice->disk_mode) {
        chunk_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
        if (chunk_ids) {
            // Convert first_chunk_local_id to file index (local_id is 1-indexed)
            uint32_t first_chunk_file_index = first_chunk_local_id - 1;
            
            // Verify bounds
            if (first_chunk_file_index + chunk_count <= lattice->total_file_nodes) {
                // Read chunks sequentially from file (O(k) - we must read k chunks anyway)
                for (uint32_t i = 0; i < chunk_count; i++) {
                    lattice_node_t* chunk_node = &lattice->nodes[first_chunk_file_index + i];
                    chunk_ids[i] = chunk_node->id;
                }
                use_sequential_path = true;
            }
        }
    }
    
    // Fast path: Read chunk IDs from parent metadata (if available and fits)
    // This is O(1) per chunk lookup, but only works if metadata fits in 510 bytes
    if (!use_sequential_path && metadata_len >= 24 + (chunk_count * 8)) {
        chunk_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
        if (chunk_ids) {
            // parent_data already has length header stripped
            // Chunk IDs start at offset 24 (after total_size:8 + chunk_count:4 + checksum:8 + first_chunk_local_id:4)
            for (uint32_t i = 0; i < chunk_count; i++) {
                memcpy(&chunk_ids[i], parent_data + 24 + (i * 8), 8);
            }
            use_sequential_path = true;
        }
    } else if (!use_sequential_path && lattice_is_node_binary(lattice, parent_id)) {
        // Try reading from parent.data directly (binary format with 2-byte header)
        uint16_t len;
        memcpy(&len, parent.data, 2);
        if (len >= 24 + (chunk_count * 8)) {
            chunk_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
            if (chunk_ids) {
                // Skip 2-byte length header, chunk IDs at offset 26 (2 + 24)
                for (uint32_t i = 0; i < chunk_count; i++) {
                    memcpy(&chunk_ids[i], parent.data + 26 + (i * 8), 8);
                }
                use_sequential_path = true;
            }
        }
    }
    
    // Fallback path: Name-based discovery (O(n) scan - only if sequential path fails)
    if (!use_sequential_path) {
        char prefix[64];
        snprintf(prefix, sizeof(prefix), "CHUNK:%lu:", parent_id);
        size_t prefix_len = strlen(prefix);
        
        // For large chunk counts, we need to search the entire file (disk mode)
        // Allocate dynamic array for candidate IDs
        uint64_t* candidate_ids = NULL;
        uint32_t candidate_count = 0;
        uint32_t candidate_capacity = chunk_count;  // Pre-allocate for expected count
        
        if (lattice->disk_mode && first_chunk_local_id > 0) {
            // O(k) Sequential read: Use first_chunk_local_id (already extracted from metadata)
            candidate_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
            if (candidate_ids) {
                uint32_t first_chunk_file_index = first_chunk_local_id - 1;
                
                // Read chunks sequentially from file (O(k) - necessary to read all chunks)
                if (first_chunk_file_index + chunk_count <= lattice->total_file_nodes) {
                    for (uint32_t i = 0; i < chunk_count; i++) {
                        lattice_node_t* chunk_node = &lattice->nodes[first_chunk_file_index + i];
                        candidate_ids[i] = chunk_node->id;
                    }
                    candidate_count = chunk_count;
                } else {
                    free(candidate_ids);
                    candidate_ids = NULL;
                }
            }
        }
        
        // Last resort: Name-based scan (O(n) - only if sequential read fails)
        if (!candidate_ids && lattice->disk_mode) {
            // Disk mode: Chunks are created sequentially after parent
            // Extract parent's local_id to find its file position
            uint32_t parent_local_id = (uint32_t)(parent_id & 0xFFFFFFFF);
            uint32_t parent_file_index = (parent_local_id > 0) ? (parent_local_id - 1) : 0;
            
            candidate_ids = (uint64_t*)malloc(candidate_capacity * sizeof(uint64_t));
            if (!candidate_ids) {
                free(reassembled);
                return -1;
            }
            
            // Chunks start immediately after parent, scan forward only
            uint32_t scan_start = parent_file_index + 1;
            uint32_t scan_end = scan_start + chunk_count + 1000;
            if (scan_end > lattice->total_file_nodes) {
                scan_end = lattice->total_file_nodes;
            }
            
            printf("[LATTICE-CHUNK] WARN Fallback: Scanning file indices %u-%u for chunks...\n",
                   scan_start, scan_end);
            
            // Scan forward from parent position (chunks are sequential)
            for (uint32_t file_index = scan_start; 
                 file_index < scan_end && candidate_count < chunk_count; 
                 file_index++) {
                lattice_node_t* file_node = &lattice->nodes[file_index];
                
                // Check if name matches prefix
                if (strncmp(file_node->name, prefix, prefix_len) == 0) {
                    if (candidate_count >= candidate_capacity) {
                        candidate_capacity *= 2;
                        uint64_t* new_ids = (uint64_t*)realloc(candidate_ids, 
                                                               candidate_capacity * sizeof(uint64_t));
                        if (!new_ids) break;
                        candidate_ids = new_ids;
                    }
                    candidate_ids[candidate_count++] = file_node->id;
                }
            }
        } else {
            // RAM mode: Use existing search function (limited to RAM cache)
            uint64_t temp_ids[1000];
            uint32_t temp_count = lattice_find_nodes_by_name(lattice, prefix, temp_ids, 1000);
            
            candidate_ids = (uint64_t*)malloc(temp_count * sizeof(uint64_t));
            if (candidate_ids) {
                memcpy(candidate_ids, temp_ids, temp_count * sizeof(uint64_t));
                candidate_count = temp_count;
            }
        }
        
        if (candidate_count < chunk_count) {
            printf("[LATTICE-CHUNK] WARN Found %u chunks, expected %u\n", 
                   candidate_count, chunk_count);
            // Continue with what we found
        }
        
        // Parse index from names and sort
        typedef struct {
            uint64_t id;
            uint32_t index;
        } chunk_entry_t;
        
        chunk_entry_t* entries = (chunk_entry_t*)malloc(candidate_count * sizeof(chunk_entry_t));
        if (!entries) {
            free(reassembled);
            return -1;
        }
        
        for (uint32_t i = 0; i < candidate_count; i++) {
            lattice_node_t chunk_node;
            if (lattice_get_node_data(lattice, candidate_ids[i], &chunk_node) == 0) {
                // Parse "CHUNK:parent_id:index:total"
                uint32_t parsed_index = 0;
                uint64_t dummy_parent_id;
                uint32_t dummy_total;
                if (sscanf(chunk_node.name, "CHUNK:%lu:%u:%u", &dummy_parent_id, &parsed_index, &dummy_total) == 3) {
                    entries[i].id = candidate_ids[i];
                    entries[i].index = parsed_index;
                }
            }
        }
        
        // Sort by index
        for (uint32_t i = 0; i < candidate_count; i++) {
            for (uint32_t j = i + 1; j < candidate_count; j++) {
                if (entries[i].index > entries[j].index) {
                    chunk_entry_t tmp = entries[i];
                    entries[i] = entries[j];
                    entries[j] = tmp;
                }
            }
        }
        
        chunk_ids = (uint64_t*)malloc(candidate_count * sizeof(uint64_t));
        if (chunk_ids) {
            for (uint32_t i = 0; i < candidate_count; i++) {
                chunk_ids[i] = entries[i].id;
            }
            chunk_count = candidate_count;  // Use actual count found
        }
        
        free(entries);
        if (candidate_ids) free(candidate_ids);  // Free temporary candidate array
    }
    
    if (!chunk_ids) {
        free(reassembled);
        return -1;
    }
    
    // Reassemble chunks
    uint8_t* output_ptr = (uint8_t*)reassembled;
    size_t remaining = total_size;
    
    for (uint32_t i = 0; i < chunk_count && remaining > 0; i++) {
        lattice_node_t chunk;
        if (lattice_get_node_data(lattice, chunk_ids[i], &chunk) != 0) {
            printf("[LATTICE-CHUNK] WARN Chunk %lu not found, skipping\n", chunk_ids[i]);
            continue;
        }
        
        // Parse chunk data: [chunk_index:8][chunk_length:2][chunk_payload:500]
        uint64_t chunk_index = 0;
        uint16_t chunk_length = 0;
        uint8_t chunk_data[512];
        size_t chunk_data_len = 0;
        bool chunk_is_binary = false;
        
        if (lattice_get_node_data_binary(lattice, chunk_ids[i], chunk_data, 
                                        &chunk_data_len, &chunk_is_binary) == 0 && chunk_is_binary) {
            if (chunk_data_len >= 10) {
                memcpy(&chunk_index, chunk_data, 8);
                memcpy(&chunk_length, chunk_data + 8, 2);
                
                // Validate chunk index
                if (chunk_index != i) {
                    printf("[LATTICE-CHUNK] WARN Chunk index mismatch: expected %u, got %lu\n",
                           i, chunk_index);
                }
                
                // Copy chunk payload (skip 10-byte header)
                size_t copy_size = (chunk_length < remaining) ? chunk_length : remaining;
                memcpy(output_ptr, chunk_data + 10, copy_size);
                output_ptr += copy_size;
                remaining -= copy_size;
            }
        }
    }
    
    free(chunk_ids);
    
    *out_data = reassembled;
    *out_data_len = total_size - remaining;
    
    printf("[LATTICE-CHUNK] OK Reassembled %zu bytes from %u chunks\n", 
           *out_data_len, chunk_count);
    
    return 0;
}

// Python-friendly wrapper: Get size of chunked data
// Returns: size on success, -1 on error
ssize_t lattice_get_node_chunked_size(persistent_lattice_t* lattice, uint64_t parent_id) {
    if (!lattice) return -1;
    
    // Get parent node
    lattice_node_t parent;
    if (lattice_get_node_data(lattice, parent_id, &parent) != 0) {
        return -1;
    }
    
    // Verify it's a chunked node
    if (strncmp(parent.name, "C:", 2) != 0) {
        return -1;
    }
    
    // Parse parent metadata to get total_size
    size_t total_size = 0;
    uint32_t chunk_count = 0;
    bool is_binary = false;
    size_t metadata_len = 0;
    uint8_t parent_data[512];
    
    if (lattice_get_node_data_binary(lattice, parent_id, parent_data, &metadata_len, &is_binary) == 0) {
        if (metadata_len >= 12) {
            memcpy(&total_size, parent_data, 8);
            memcpy(&chunk_count, parent_data + 8, 4);
        }
    } else if (lattice_is_node_binary(lattice, parent_id)) {
        uint16_t len;
        memcpy(&len, parent.data, 2);
        if (len >= 12) {
            memcpy(&total_size, parent.data + 2, 8);
            memcpy(&chunk_count, parent.data + 10, 4);
        }
    }
    
    if (chunk_count == 0 || total_size == 0) {
        return -1;
    }
    
    return (ssize_t)total_size;
}

// Python-friendly wrapper: Copy chunked data to pre-allocated buffer
// Returns: actual size written on success, -1 on error, -2 if buffer too small
// This function duplicates the logic from lattice_get_node_chunked but writes directly to buffer
ssize_t lattice_get_node_chunked_to_buffer(persistent_lattice_t* lattice,
                                           uint64_t parent_id,
                                           void* buffer,
                                           size_t buffer_size) {
    if (!lattice || !buffer) return -1;
    
    // Get parent node
    lattice_node_t parent;
    if (lattice_get_node_data(lattice, parent_id, &parent) != 0) {
        return -1;
    }
    
    // Verify it's a chunked node
    if (strncmp(parent.name, "C:", 2) != 0) {
        return -1;
    }
    
    // Parse parent metadata to get total_size and chunk_count
    size_t total_size = 0;
    uint32_t chunk_count = 0;
    uint32_t first_chunk_local_id = 0;
    bool is_binary = false;
    size_t metadata_len = 0;
    uint8_t parent_data[512];
    
    // Try binary API first (strips length header, returns clean data)
    if (lattice_get_node_data_binary(lattice, parent_id, parent_data, &metadata_len, &is_binary) == 0) {
        if (metadata_len >= 24) {
            memcpy(&total_size, parent_data, 8);
            memcpy(&chunk_count, parent_data + 8, 4);
            memcpy(&first_chunk_local_id, parent_data + 20, 4);
        } else if (metadata_len >= 12) {
            memcpy(&total_size, parent_data, 8);
            memcpy(&chunk_count, parent_data + 8, 4);
        }
    } else {
        // Fallback: read from parent node struct directly (parent is already loaded)
        // Check if parent.data is valid (has at least 2 bytes for length header)
        if (parent.data && lattice_is_node_binary(lattice, parent_id)) {
            uint16_t len;
            // Safety: ensure we can read 2 bytes
            if (sizeof(parent.data) >= 2) {
                memcpy(&len, parent.data, 2);
                // Validate length is reasonable
                if (len > 0 && len <= 510 && len + 2 <= sizeof(parent.data)) {
                    if (len >= 24) {
                        memcpy(&total_size, parent.data + 2, 8);
                        memcpy(&chunk_count, parent.data + 10, 4);
                        memcpy(&first_chunk_local_id, parent.data + 22, 4);
                    } else if (len >= 12) {
                        memcpy(&total_size, parent.data + 2, 8);
                        memcpy(&chunk_count, parent.data + 10, 4);
                    }
                }
            }
        }
    }
    
    if (chunk_count == 0 || total_size == 0) {
        return -1;
    }
    
    // Check if buffer is large enough
    if (total_size > buffer_size) {
        return -2;  // Buffer too small
    }
    
    // Get chunk IDs (simplified - use sequential path if available)
    uint64_t* chunk_ids = NULL;
    bool use_sequential_path = false;
    
    if (first_chunk_local_id > 0 && lattice->disk_mode && lattice->nodes) {
        chunk_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
        if (chunk_ids) {
            uint32_t first_chunk_file_index = first_chunk_local_id - 1;
            // Safety check: ensure we don't access out of bounds
            if (first_chunk_file_index < lattice->total_file_nodes &&
                first_chunk_file_index + chunk_count <= lattice->total_file_nodes &&
                first_chunk_file_index + chunk_count <= lattice->node_count) {
                for (uint32_t i = 0; i < chunk_count; i++) {
                    uint32_t idx = first_chunk_file_index + i;
                    if (idx < lattice->node_count && idx < lattice->max_nodes) {
                        lattice_node_t* chunk_node = &lattice->nodes[idx];
                        chunk_ids[i] = chunk_node->id;
                    } else {
                        free(chunk_ids);
                        chunk_ids = NULL;
                        break;
                    }
                }
                if (chunk_ids) {
                    use_sequential_path = true;
                }
            } else {
                free(chunk_ids);
                chunk_ids = NULL;
            }
        }
    }
    
    // Fallback: metadata path
    if (!use_sequential_path && metadata_len >= 24 + (chunk_count * 8)) {
        chunk_ids = (uint64_t*)malloc(chunk_count * sizeof(uint64_t));
        if (chunk_ids) {
            for (uint32_t i = 0; i < chunk_count; i++) {
                memcpy(&chunk_ids[i], parent_data + 24 + (i * 8), 8);
            }
            use_sequential_path = true;
        }
    }
    
    if (!chunk_ids) {
        return -1;
    }
    
    // Reassemble chunks directly into buffer
    uint8_t* output_ptr = (uint8_t*)buffer;
    size_t remaining = total_size;
    
    for (uint32_t i = 0; i < chunk_count && remaining > 0; i++) {
        uint8_t chunk_data[512];
        size_t chunk_data_len = 0;
        bool chunk_is_binary = false;
        
        if (lattice_get_node_data_binary(lattice, chunk_ids[i], chunk_data, 
                                        &chunk_data_len, &chunk_is_binary) == 0 && chunk_is_binary) {
            if (chunk_data_len >= 10) {
                uint64_t chunk_index = 0;
                uint16_t chunk_length = 0;
                memcpy(&chunk_index, chunk_data, 8);
                memcpy(&chunk_length, chunk_data + 8, 2);
                
                // Copy chunk payload (skip 10-byte header)
                size_t copy_size = (chunk_length < remaining) ? chunk_length : remaining;
                memcpy(output_ptr, chunk_data + 10, copy_size);
                output_ptr += copy_size;
                remaining -= copy_size;
            }
        }
    }
    
    free(chunk_ids);
    
    return (ssize_t)(total_size - remaining);
}

// Helper: Find node index by ID (for safe updates)
static uint32_t find_node_index(persistent_lattice_t* lattice, uint64_t id) {
    if (!lattice || id == 0) return UINT32_MAX;
    
    // Try O(1) lookup via reverse index
    // Extract local ID (lower 32 bits) for reverse index lookup
    uint32_t local_id = (uint32_t)(id & 0xFFFFFFFF);
    if (lattice->id_to_index_map != NULL) {
        uint32_t safe_bound = lattice->max_nodes * 10;
        if (local_id < safe_bound) {
            uint32_t idx = lattice->id_to_index_map[local_id];
            if (idx < lattice->node_count && lattice->node_id_map && 
                idx < lattice->max_nodes && lattice->node_id_map[idx] == id) {
                return idx;
            }
        }
    }
    
    // Fallback to O(n) search
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->node_id_map[i] == id) {
            return i;
        }
    }
    
    return UINT32_MAX;
}

// Add child to parent node
int lattice_add_child(persistent_lattice_t* lattice, uint64_t parent_id, uint64_t child_id) {
    uint32_t parent_idx = find_node_index(lattice, parent_id);
    if (parent_idx == UINT32_MAX || parent_idx >= lattice->node_count) return -1;
    
    lattice_node_t* parent = &lattice->nodes[parent_idx];
    
    // Reallocate children array (64-bit IDs)
    uint64_t* new_children = (uint64_t*)realloc(parent->children, (parent->child_count + 1) * sizeof(uint64_t));
    if (!new_children) return -1;
    
    parent->children = new_children;
    parent->children[parent->child_count] = child_id;
    parent->child_count++;
    lattice->dirty = true;
    
    return 0;
}

// Add child with edge metadata (stores metadata in child node's data[512] field)
int lattice_add_child_with_metadata(persistent_lattice_t* lattice, 
                                    uint64_t parent_id, 
                                    uint64_t child_id,
                                    const edge_metadata_t* metadata) {
    if (!lattice || !metadata) return -1;
    
    // First add the child relationship
    if (lattice_add_child(lattice, parent_id, child_id) != 0) {
        return -1;
    }
    
    // Store metadata in child node's data[512] field using binary API
    size_t metadata_size = sizeof(edge_metadata_t);
    if (metadata_size > 510) {
        printf("[LATTICE-EDGE] ERROR Edge metadata too large (%zu bytes, max 510)\n", metadata_size);
        return -1;
    }
    
    // Use binary update API to store metadata in child node
    return lattice_update_node_binary(lattice, child_id, metadata, metadata_size);
}

// Delete node from lattice
int lattice_delete_node(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice || node_id == 0) return -1;
    
    // Find node index
    uint32_t node_idx = find_node_index(lattice, node_id);
    if (node_idx == UINT32_MAX || node_idx >= lattice->node_count) {
        // Node not in cache - might be on disk only
        // For now, we only delete from cache
        // TODO: Support deletion from disk storage
        return -1;
    }
    
    // Free children array if it exists
    if (lattice->nodes[node_idx].children) {
        free(lattice->nodes[node_idx].children);
        lattice->nodes[node_idx].children = NULL;
    }
    
    // Clear id_to_index_map entry
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        lattice->id_to_index_map[local_id] = 0;  // Clear mapping
    }
    
    // Shift remaining nodes (similar to eviction logic)
    for (uint32_t i = node_idx; i < lattice->node_count - 1; i++) {
        lattice->nodes[i] = lattice->nodes[i + 1];
        lattice->node_id_map[i] = lattice->node_id_map[i + 1];
        lattice->access_count[i] = lattice->access_count[i + 1];
        lattice->last_access[i] = lattice->last_access[i + 1];
        
        // Update id_to_index_map for shifted nodes
        uint64_t shifted_id = lattice->node_id_map[i];
        uint32_t shifted_local_id = (uint32_t)(shifted_id & 0xFFFFFFFF);
        if (lattice->id_to_index_map && shifted_local_id < lattice->max_nodes * 10) {
            lattice->id_to_index_map[shifted_local_id] = i;
        }
    }
    
    lattice->node_count--;
    lattice->total_nodes--;  // Decrement total count
    lattice->dirty = true;
    
    // WAL logging (only if WAL is enabled and not already being applied)
    // Note: WAL is disabled during recovery, so we check wal_enabled flag
    if (lattice->wal && lattice->wal_enabled) {
        wal_append_delete_node(lattice->wal, node_id);
    }
    
    return 0;
}

// Get edge metadata from child node
int lattice_get_edge_metadata(persistent_lattice_t* lattice, 
                              uint64_t parent_id,
                              uint64_t child_id,
                              edge_metadata_t* metadata) {
    if (!lattice || !metadata) return -1;
    
    // Verify parent-child relationship exists
    lattice_node_t parent;
    if (lattice_get_node_data(lattice, parent_id, &parent) != 0) return -1;
    
    bool is_child = false;
    for (uint32_t i = 0; i < parent.child_count; i++) {
        if (parent.children[i] == child_id) {
            is_child = true;
            break;
        }
    }
    if (!is_child) return -1;
    
    // Retrieve metadata from child node using binary API
    uint8_t data_buffer[512];
    size_t data_len = 0;
    bool is_binary = false;
    
    if (lattice_get_node_data_binary(lattice, child_id, data_buffer, &data_len, &is_binary) != 0) {
        return -1;
    }
    
    // Check if data is binary and has correct size
    if (!is_binary || data_len != sizeof(edge_metadata_t)) {
        return -1;
    }
    
    // Copy metadata
    memcpy(metadata, data_buffer, sizeof(edge_metadata_t));
    
    return 0;
}

// Check if edge has metadata
bool lattice_edge_has_metadata(persistent_lattice_t* lattice, 
                               uint64_t parent_id,
                               uint64_t child_id) {
    if (!lattice) return false;
    
    edge_metadata_t metadata;
    return (lattice_get_edge_metadata(lattice, parent_id, child_id, &metadata) == 0);
}

// Store performance data
int lattice_store_performance(persistent_lattice_t* lattice, const char* kernel_type,
                            uint32_t complexity, const lattice_performance_t* perf) {
    if (!lattice || !kernel_type || !perf) return -1;
    
    char name[64];
    snprintf(name, sizeof(name), "perf_%s_%u", kernel_type, complexity);
    
    char data[512];
    snprintf(data, sizeof(data), "cycles=%lu,instructions=%lu,time=%.2f,ipc=%.2f,throughput=%.2f,efficiency=%.2f",
             perf->cycles, perf->instructions, perf->execution_time_ns,
             perf->instructions_per_cycle, perf->throughput_mb_s, perf->efficiency_score);
    
    uint64_t node_id = lattice_add_node_deduplicated(lattice, LATTICE_NODE_PERFORMANCE, name, data, 0);
    if (node_id == 0) return -1;
    
    // Store performance data in payload
    uint32_t node_idx = find_node_index(lattice, node_id);
    if (node_idx != UINT32_MAX && node_idx < lattice->node_count) {
        lattice->nodes[node_idx].payload.performance = *perf;
        lattice->dirty = true;
    }
    
    return 0;
}

// Get best performance data
int lattice_get_best_performance(persistent_lattice_t* lattice, const char* kernel_type,
                                uint32_t complexity, lattice_performance_t* perf) {
    if (!lattice || !kernel_type || !perf) return -1;
    
    char name[64];
    snprintf(name, sizeof(name), "perf_%s_%u", kernel_type, complexity);
    
    lattice_node_t* node = NULL;
    double best_efficiency = 0.0;
    
    // Find best performance node
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* n = &lattice->nodes[i];
        if (n->type == LATTICE_NODE_PERFORMANCE && strcmp(n->name, name) == 0) {
            if (n->payload.performance.efficiency_score > best_efficiency) {
                best_efficiency = n->payload.performance.efficiency_score;
                node = n;
            }
        }
    }
    
    if (node) {
        *perf = node->payload.performance;
        return 0;
    }
    
    return -1;
}

// Store pattern data
int lattice_store_pattern(persistent_lattice_t* lattice, const char* pattern,
                         double success_rate, double performance_gain) {
    if (!lattice || !pattern) return -1;
    
    // Check if pattern already exists (by pattern_sequence, not name)
    lattice_node_t* existing_node = NULL;
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (node->type == LATTICE_NODE_LEARNING) {
            if (strcmp(node->payload.learning.pattern_sequence, pattern) == 0) {
                existing_node = node;
                break;
            }
        }
    }
    
    if (existing_node) {
        // Pattern exists - increment frequency and update metrics
        existing_node->payload.learning.frequency++;
        existing_node->payload.learning.last_used = get_current_timestamp();
        // Update success rate with weighted average (70% old, 30% new)
        existing_node->payload.learning.success_rate = 
            (existing_node->payload.learning.success_rate * 0.7) + (success_rate * 0.3);
        // Update performance gain (use latest)
        existing_node->payload.learning.performance_gain = performance_gain;
        return 0;
    }
    
    // New pattern - create node
    char name[64];
    snprintf(name, sizeof(name), "pattern_%lu", get_current_timestamp());
    
    char data[512];
    snprintf(data, sizeof(data), "pattern=%s,success=%.2f,gain=%.2f", pattern, success_rate, performance_gain);
    
    uint64_t node_id = lattice_add_node(lattice, LATTICE_NODE_LEARNING, name, data, 0);
    if (node_id == 0) return -1;
    
    // Store learning data in payload
    uint32_t node_idx = find_node_index(lattice, node_id);
    if (node_idx != UINT32_MAX && node_idx < lattice->node_count) {
        strncpy(lattice->nodes[node_idx].payload.learning.pattern_sequence, pattern, 
               sizeof(lattice->nodes[node_idx].payload.learning.pattern_sequence) - 1);
        lattice->nodes[node_idx].payload.learning.pattern_sequence[sizeof(lattice->nodes[node_idx].payload.learning.pattern_sequence) - 1] = '\0';
        lattice->nodes[node_idx].payload.learning.success_rate = success_rate;
        lattice->nodes[node_idx].payload.learning.performance_gain = performance_gain;
        lattice->nodes[node_idx].payload.learning.frequency = 1;
        lattice->nodes[node_idx].payload.learning.last_used = get_current_timestamp();
        lattice->nodes[node_idx].payload.learning.evolution_generation = 0;
        lattice->dirty = true;
    }
    
    return 0;
}

// Get evolved patterns
int lattice_get_evolved_patterns(persistent_lattice_t* lattice, const char* base_pattern,
                                lattice_learning_t* patterns, uint32_t max_patterns) {
    if (!lattice || !base_pattern || !patterns) return -1;
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && count < max_patterns; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (node->type == LATTICE_NODE_LEARNING) {
            if (strstr(node->payload.learning.pattern_sequence, base_pattern)) {
                patterns[count] = node->payload.learning;
                count++;
            }
        }
    }
    
    return count;
}

// Find nodes by type
uint32_t lattice_find_nodes_by_type(persistent_lattice_t* lattice, lattice_node_type_t type,
                                   uint64_t* node_ids, uint32_t max_ids) {
    if (!lattice || !node_ids) return 0;
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && count < max_ids; i++) {
        if (lattice->nodes[i].type == type) {
            node_ids[count] = lattice->nodes[i].id;
            count++;
        }
    }
    
    return count;
}

// Extract prefix from node name (e.g., "ISA_ADD" -> "ISA_", "QDRANT_COLLECTION:test" -> "QDRANT_COLLECTION:")
// Returns prefix length, or 0 if no valid prefix found
static size_t extract_prefix_from_name(const char* name, char* prefix_out, size_t prefix_max) {
    if (!name || !prefix_out || prefix_max == 0) return 0;
    
    // Find first delimiter: "_" or ":"
    const char* underscore = strchr(name, '_');
    const char* colon = strchr(name, ':');
    const char* delimiter = NULL;
    
    if (underscore && colon) {
        delimiter = (underscore < colon) ? underscore : colon;
    } else if (underscore) {
        delimiter = underscore;
    } else if (colon) {
        delimiter = colon;
    } else {
        return 0; // No delimiter found
    }
    
    // Extract prefix (everything up to and including delimiter)
    size_t prefix_len = (size_t)(delimiter - name + 1);
    if (prefix_len >= prefix_max) return 0;
    
    strncpy(prefix_out, name, prefix_len);
    prefix_out[prefix_len] = '\0';
    
    return prefix_len;
}

// Build semantic prefix index (O(n) once, enables O(k) queries where k << n)
// Automatically detects ALL prefixes using the system's naming pattern (PREFIX_ or PREFIX:)
// CRITICAL: Indexes ALL nodes (including disk nodes via mmap), not just RAM cache
void lattice_build_prefix_index(persistent_lattice_t* lattice) {
    if (!lattice || lattice->prefix_index.built) return;
    
    // Determine how many nodes to index: use total_nodes (all nodes including disk), not just node_count (RAM cache)
    uint32_t nodes_to_index = lattice->total_nodes;
    if (nodes_to_index == 0) {
        nodes_to_index = lattice->node_count;  // Fallback if total_nodes not set
    }
    
    if (lattice->prefix_index.use_dynamic_index) {
        // Build dynamic index from ALL nodes (including disk nodes via mmap)
        const char** node_names = (const char**)malloc(nodes_to_index * sizeof(const char*));
        uint64_t* node_ids = (uint64_t*)malloc(nodes_to_index * sizeof(uint64_t));
        if (node_names && node_ids) {
            uint32_t valid_count = 0;
            
            // Index nodes from RAM cache first
            for (uint32_t i = 0; i < lattice->node_count && i < nodes_to_index; i++) {
                if (lattice->nodes[i].id != 0) {
                    node_names[valid_count] = lattice->nodes[i].name;
                    node_ids[valid_count] = lattice->nodes[i].id;
                    valid_count++;
                }
            }
            
            // Index nodes from disk (via mmap) if available
            if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && nodes_to_index > lattice->node_count) {
                size_t header_size = 4 * sizeof(uint32_t);
                for (uint32_t i = lattice->node_count; i < nodes_to_index; i++) {
                    size_t node_offset = header_size + (i * sizeof(lattice_node_t));
                    if (node_offset + sizeof(lattice_node_t) <= lattice->mmap_size) {
                        lattice_node_t* disk_node = (lattice_node_t*)((char*)lattice->mmap_ptr + node_offset);
                        if (disk_node->id != 0) {
                            node_names[valid_count] = disk_node->name;
                            node_ids[valid_count] = disk_node->id;
                            valid_count++;
                        }
                    }
                }
            }
            
            if (valid_count > 0) {
                dynamic_prefix_index_build(&lattice->prefix_index.dynamic_index, 
                                          node_names, node_ids, valid_count);
            }
            free(node_names);
            free(node_ids);
        }
    } else {
        // Build hardcoded index (backward compatibility)
        // Free existing index if any
        if (lattice->prefix_index.isa_ids) free(lattice->prefix_index.isa_ids);
        if (lattice->prefix_index.material_ids) free(lattice->prefix_index.material_ids);
        if (lattice->prefix_index.learning_ids) free(lattice->prefix_index.learning_ids);
        if (lattice->prefix_index.performance_ids) free(lattice->prefix_index.performance_ids);
        
        // Reset counts
        lattice->prefix_index.isa_count = 0;
        lattice->prefix_index.material_count = 0;
        lattice->prefix_index.learning_count = 0;
        lattice->prefix_index.performance_count = 0;
        
        // Auto-detect prefixes: Count nodes per prefix (pass 1) - from RAM cache
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            if (lattice->nodes[i].id == 0) continue;  // Skip uninitialized nodes
            const char* name = lattice->nodes[i].name;
            char prefix[64];
            
            if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
                if (strcmp(prefix, "ISA_") == 0) {
                    lattice->prefix_index.isa_count++;
                } else if (strcmp(prefix, "MATERIAL_") == 0) {
                    lattice->prefix_index.material_count++;
                } else if (strcmp(prefix, "LEARNING_") == 0) {
                    lattice->prefix_index.learning_count++;
                } else if (strcmp(prefix, "PATTERN_") == 0) {
                    // PATTERN_ nodes are learning patterns - count them too
                    lattice->prefix_index.learning_count++;
                } else if (strcmp(prefix, "PERFORMANCE_") == 0) {
                    lattice->prefix_index.performance_count++;
                } else if (strcmp(prefix, "QDRANT_COLLECTION:") == 0 || strcmp(prefix, "QDRANT_POINT:") == 0) {
                    // Auto-detected new prefix - use existing structure for now
                    lattice->prefix_index.performance_count++; // Temporary: reuse performance slot
                }
            }
        }
        
        // Count from disk nodes (via mmap) if available
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && nodes_to_index > lattice->node_count) {
            size_t header_size = 4 * sizeof(uint32_t);
            for (uint32_t i = lattice->node_count; i < nodes_to_index; i++) {
                size_t node_offset = header_size + (i * sizeof(lattice_node_t));
                if (node_offset + sizeof(lattice_node_t) <= lattice->mmap_size) {
                    lattice_node_t* disk_node = (lattice_node_t*)((char*)lattice->mmap_ptr + node_offset);
                    if (disk_node->id == 0) continue;  // Skip uninitialized nodes
                    const char* name = disk_node->name;
                    char prefix[64];
                    
                    if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
                        if (strcmp(prefix, "ISA_") == 0) {
                            lattice->prefix_index.isa_count++;
                        } else if (strcmp(prefix, "MATERIAL_") == 0) {
                            lattice->prefix_index.material_count++;
                        } else if (strcmp(prefix, "LEARNING_") == 0) {
                            lattice->prefix_index.learning_count++;
                        } else if (strcmp(prefix, "PATTERN_") == 0) {
                            lattice->prefix_index.learning_count++;
                        } else if (strcmp(prefix, "PERFORMANCE_") == 0) {
                            lattice->prefix_index.performance_count++;
                        } else if (strcmp(prefix, "QDRANT_COLLECTION:") == 0 || strcmp(prefix, "QDRANT_POINT:") == 0) {
                            lattice->prefix_index.performance_count++;
                        }
                    }
                }
            }
        }
        
        // Allocate arrays (pass 2)
        if (lattice->prefix_index.isa_count > 0) {
            lattice->prefix_index.isa_ids = (uint64_t*)malloc(lattice->prefix_index.isa_count * sizeof(uint64_t));
            if (!lattice->prefix_index.isa_ids) return;
        }
        if (lattice->prefix_index.material_count > 0) {
            lattice->prefix_index.material_ids = (uint64_t*)malloc(lattice->prefix_index.material_count * sizeof(uint64_t));
            if (!lattice->prefix_index.material_ids) return;
        }
        if (lattice->prefix_index.learning_count > 0) {
            lattice->prefix_index.learning_ids = (uint64_t*)malloc(lattice->prefix_index.learning_count * sizeof(uint64_t));
            if (!lattice->prefix_index.learning_ids) return;
        }
        if (lattice->prefix_index.performance_count > 0) {
            lattice->prefix_index.performance_ids = (uint64_t*)malloc(lattice->prefix_index.performance_count * sizeof(uint64_t));
            if (!lattice->prefix_index.performance_ids) return;
        }
        
        // Populate arrays (pass 3) - from RAM cache
        uint32_t isa_idx = 0, material_idx = 0, learning_idx = 0, performance_idx = 0;
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            if (lattice->nodes[i].id == 0) continue;  // Skip uninitialized nodes
            const char* name = lattice->nodes[i].name;
            char prefix[64];
            
            if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
                if (strcmp(prefix, "ISA_") == 0 && lattice->prefix_index.isa_ids && isa_idx < lattice->prefix_index.isa_count) {
                    lattice->prefix_index.isa_ids[isa_idx++] = lattice->nodes[i].id;
                } else if (strcmp(prefix, "MATERIAL_") == 0 && lattice->prefix_index.material_ids && material_idx < lattice->prefix_index.material_count) {
                    lattice->prefix_index.material_ids[material_idx++] = lattice->nodes[i].id;
                } else if (strcmp(prefix, "LEARNING_") == 0 && lattice->prefix_index.learning_ids && learning_idx < lattice->prefix_index.learning_count) {
                    lattice->prefix_index.learning_ids[learning_idx++] = lattice->nodes[i].id;
                } else if (strcmp(prefix, "PATTERN_") == 0 && lattice->prefix_index.learning_ids && learning_idx < lattice->prefix_index.learning_count) {
                    // PATTERN_ nodes are learning patterns - use learning slot
                    lattice->prefix_index.learning_ids[learning_idx++] = lattice->nodes[i].id;
                } else if (strcmp(prefix, "PERFORMANCE_") == 0 && lattice->prefix_index.performance_ids && performance_idx < lattice->prefix_index.performance_count) {
                    lattice->prefix_index.performance_ids[performance_idx++] = lattice->nodes[i].id;
                } else if ((strcmp(prefix, "QDRANT_COLLECTION:") == 0 || strcmp(prefix, "QDRANT_POINT:") == 0) && 
                          lattice->prefix_index.performance_ids && performance_idx < lattice->prefix_index.performance_count) {
                    // Auto-detected: reuse performance slot temporarily
                    lattice->prefix_index.performance_ids[performance_idx++] = lattice->nodes[i].id;
                }
            }
        }
        
        // Populate arrays (pass 3 continued) - from disk nodes via mmap
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && nodes_to_index > lattice->node_count) {
            size_t header_size = 4 * sizeof(uint32_t);
            for (uint32_t i = lattice->node_count; i < nodes_to_index; i++) {
                size_t node_offset = header_size + (i * sizeof(lattice_node_t));
                if (node_offset + sizeof(lattice_node_t) <= lattice->mmap_size) {
                    lattice_node_t* disk_node = (lattice_node_t*)((char*)lattice->mmap_ptr + node_offset);
                    if (disk_node->id == 0) continue;  // Skip uninitialized nodes
                    const char* name = disk_node->name;
                    char prefix[64];
                    
                    if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
                        if (strcmp(prefix, "ISA_") == 0 && lattice->prefix_index.isa_ids && isa_idx < lattice->prefix_index.isa_count) {
                            lattice->prefix_index.isa_ids[isa_idx++] = disk_node->id;
                        } else if (strcmp(prefix, "MATERIAL_") == 0 && lattice->prefix_index.material_ids && material_idx < lattice->prefix_index.material_count) {
                            lattice->prefix_index.material_ids[material_idx++] = disk_node->id;
                        } else if (strcmp(prefix, "LEARNING_") == 0 && lattice->prefix_index.learning_ids && learning_idx < lattice->prefix_index.learning_count) {
                            lattice->prefix_index.learning_ids[learning_idx++] = disk_node->id;
                        } else if (strcmp(prefix, "PATTERN_") == 0 && lattice->prefix_index.learning_ids && learning_idx < lattice->prefix_index.learning_count) {
                            lattice->prefix_index.learning_ids[learning_idx++] = disk_node->id;
                        } else if (strcmp(prefix, "PERFORMANCE_") == 0 && lattice->prefix_index.performance_ids && performance_idx < lattice->prefix_index.performance_count) {
                            lattice->prefix_index.performance_ids[performance_idx++] = disk_node->id;
                        } else if ((strcmp(prefix, "QDRANT_COLLECTION:") == 0 || strcmp(prefix, "QDRANT_POINT:") == 0) && 
                                  lattice->prefix_index.performance_ids && performance_idx < lattice->prefix_index.performance_count) {
                            lattice->prefix_index.performance_ids[performance_idx++] = disk_node->id;
                        }
                    }
                }
            }
        }
    }
    
    lattice->prefix_index.built = true;
}

// Validate that hardcoded and dynamic prefix indexes match
bool lattice_validate_prefix_indexes(persistent_lattice_t* lattice) {
    if (!lattice) return false;
    
    if (!lattice->prefix_index.built) {
        printf("[VALIDATE] Prefix index not built, skipping validation\n");
        return false;
    }
    
    if (!lattice->prefix_index.dynamic_index.built) {
        printf("[VALIDATE] Dynamic prefix index not built, skipping validation\n");
        return false;
    }
    
    bool all_match = true;
    uint32_t discrepancies = 0;
    
    // Check known prefixes
    const char* known_prefixes[] = {
        "ISA_",
        "MATERIAL_",
        "LEARNING_",
        "PATTERN_",
        "PERFORMANCE_"
    };
    
    for (size_t i = 0; i < sizeof(known_prefixes) / sizeof(known_prefixes[0]); i++) {
        const char* prefix = known_prefixes[i];
        
        // Get hardcoded count
        uint32_t hardcoded_count = 0;
        uint64_t* hardcoded_ids = NULL;
        
        if (strcmp(prefix, "ISA_") == 0) {
            hardcoded_count = lattice->prefix_index.isa_count;
            hardcoded_ids = lattice->prefix_index.isa_ids;
        } else if (strcmp(prefix, "MATERIAL_") == 0) {
            hardcoded_count = lattice->prefix_index.material_count;
            hardcoded_ids = lattice->prefix_index.material_ids;
        } else if (strcmp(prefix, "LEARNING_") == 0) {
            // LEARNING_ in hardcoded includes PATTERN_ nodes (semantic grouping)
            // Dynamic system separates them - need to combine for comparison
            hardcoded_count = lattice->prefix_index.learning_count;
            hardcoded_ids = lattice->prefix_index.learning_ids;
            
            // Get PATTERN_ count from dynamic to add to LEARNING_ for comparison
            dynamic_prefix_entry_t* pattern_entry = dynamic_prefix_index_find(
                &lattice->prefix_index.dynamic_index, "PATTERN_");
            uint32_t pattern_count = pattern_entry ? pattern_entry->count : 0;
            
            // Compare: hardcoded LEARNING_ (includes PATTERN_) vs dynamic LEARNING_ + PATTERN_
            dynamic_prefix_entry_t* learning_entry = dynamic_prefix_index_find(
                &lattice->prefix_index.dynamic_index, "LEARNING_");
            uint32_t learning_dynamic_count = learning_entry ? learning_entry->count : 0;
            uint32_t combined_dynamic_count = learning_dynamic_count + pattern_count;
            
            if (hardcoded_count != combined_dynamic_count) {
                printf("[VALIDATE] ERROR Mismatch for prefix 'LEARNING_': hardcoded=%u (includes PATTERN_), dynamic=%u (LEARNING_=%u + PATTERN_=%u)\n",
                       hardcoded_count, combined_dynamic_count, learning_dynamic_count, pattern_count);
                all_match = false;
                discrepancies++;
            } else {
                printf("[VALIDATE] OK Prefix 'LEARNING_': %u nodes match (hardcoded includes PATTERN_, dynamic separates)\n", 
                       hardcoded_count);
            }
            continue;  // Skip normal comparison
        } else if (strcmp(prefix, "PATTERN_") == 0) {
            // PATTERN_ is grouped with LEARNING_ in hardcoded system
            // But dynamic system treats it separately (which is correct!)
            // So we need to combine LEARNING_ + PATTERN_ for comparison
            hardcoded_count = lattice->prefix_index.learning_count;
            hardcoded_ids = lattice->prefix_index.learning_ids;
            // Note: This will show PATTERN_ as separate in dynamic, which is expected
        } else if (strcmp(prefix, "PERFORMANCE_") == 0) {
            hardcoded_count = lattice->prefix_index.performance_count;
            hardcoded_ids = lattice->prefix_index.performance_ids;
        }
        
        // Get dynamic count
        dynamic_prefix_entry_t* dynamic_entry = dynamic_prefix_index_find(
            &lattice->prefix_index.dynamic_index, prefix);
        uint32_t dynamic_count = dynamic_entry ? dynamic_entry->count : 0;
        uint64_t* dynamic_ids = dynamic_entry ? dynamic_entry->node_ids : NULL;
        
        // Special case: PATTERN_ is grouped with LEARNING_ in hardcoded system
        // Dynamic system correctly treats them as separate prefixes
        if (strcmp(prefix, "PATTERN_") == 0) {
            // PATTERN_ nodes are in LEARNING_ slot in hardcoded system
            // Dynamic system correctly separates them - this is expected!
            printf("[VALIDATE] INFO Prefix 'PATTERN_': hardcoded groups with LEARNING_ (%u total), dynamic separates (%u PATTERN_ nodes)\n",
                   hardcoded_count, dynamic_count);
            // This is not a discrepancy - dynamic system is more precise
            continue;
        }
        
        // Compare counts
        if (hardcoded_count != dynamic_count) {
            printf("[VALIDATE] ERROR Mismatch for prefix '%s': hardcoded=%u, dynamic=%u\n",
                   prefix, hardcoded_count, dynamic_count);
            all_match = false;
            discrepancies++;
            continue;
        }
        
        // Compare IDs (if both have entries)
        if (hardcoded_count > 0 && hardcoded_ids && dynamic_ids) {
            // Sort both arrays for comparison (simple bubble sort for small arrays)
            // Actually, let's just check if all hardcoded IDs exist in dynamic
            for (uint32_t j = 0; j < hardcoded_count; j++) {
                bool found = false;
                for (uint32_t k = 0; k < dynamic_count; k++) {
                    if (hardcoded_ids[j] == dynamic_ids[k]) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    printf("[VALIDATE] ERROR ID %lu in hardcoded '%s' not found in dynamic\n",
                           (unsigned long)hardcoded_ids[j], prefix);
                    all_match = false;
                    discrepancies++;
                }
            }
        }
        
        if (hardcoded_count == dynamic_count && hardcoded_count > 0) {
            printf("[VALIDATE] OK Prefix '%s': %u nodes match\n", prefix, hardcoded_count);
        }
    }
    
    // Check for prefixes in dynamic index that aren't in hardcoded
    for (uint32_t i = 0; i < lattice->prefix_index.dynamic_index.entry_count; i++) {
        const char* prefix = lattice->prefix_index.dynamic_index.entries[i].prefix;
        bool is_known = false;
        for (size_t j = 0; j < sizeof(known_prefixes) / sizeof(known_prefixes[0]); j++) {
            if (strcmp(prefix, known_prefixes[j]) == 0) {
                is_known = true;
                break;
            }
        }
        if (!is_known) {
            printf("[VALIDATE] INFO Dynamic index found new prefix '%s' with %u nodes (not in hardcoded)\n",
                   prefix, lattice->prefix_index.dynamic_index.entries[i].count);
        }
    }
    
    if (all_match && discrepancies == 0) {
        printf("[VALIDATE] OK All prefix indexes match perfectly.\n");
    } else {
        printf("[VALIDATE] WARN Found %u discrepancies\n", discrepancies);
    }
    
    return all_match;
}

// Benchmark both prefix index systems
int lattice_benchmark_prefix_indexes(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    printf("\n");
    printf("===========================================================\n");
    printf("Prefix Index Performance Benchmark\n");
    printf("===========================================================\n");
    printf("\n");
    
    // Reset both indexes
    lattice->prefix_index.built = false;
    lattice->prefix_index.dynamic_index.built = false;
    
    // Benchmark hardcoded index build
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Build only hardcoded (temporarily disable dynamic)
    bool old_dynamic = lattice->prefix_index.use_dynamic_index;
    lattice->prefix_index.use_dynamic_index = false;
    
    // Manually build hardcoded only
    if (!lattice->prefix_index.built) {
        // Free existing
        if (lattice->prefix_index.isa_ids) free(lattice->prefix_index.isa_ids);
        if (lattice->prefix_index.material_ids) free(lattice->prefix_index.material_ids);
        if (lattice->prefix_index.learning_ids) free(lattice->prefix_index.learning_ids);
        if (lattice->prefix_index.performance_ids) free(lattice->prefix_index.performance_ids);
        
        // Reset counts
        lattice->prefix_index.isa_count = 0;
        lattice->prefix_index.material_count = 0;
        lattice->prefix_index.learning_count = 0;
        lattice->prefix_index.performance_count = 0;
        
        // Count (pass 1)
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            const char* name = lattice->nodes[i].name;
            char prefix[64];
            if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
                if (strcmp(prefix, "ISA_") == 0) {
                    lattice->prefix_index.isa_count++;
                } else if (strcmp(prefix, "MATERIAL_") == 0) {
                    lattice->prefix_index.material_count++;
                } else if (strcmp(prefix, "LEARNING_") == 0 || strcmp(prefix, "PATTERN_") == 0) {
                    lattice->prefix_index.learning_count++;
                } else if (strcmp(prefix, "PERFORMANCE_") == 0) {
                    lattice->prefix_index.performance_count++;
                }
            }
        }
        
        // Allocate (pass 2)
        if (lattice->prefix_index.isa_count > 0) {
            lattice->prefix_index.isa_ids = (uint64_t*)malloc(lattice->prefix_index.isa_count * sizeof(uint64_t));
        }
        if (lattice->prefix_index.material_count > 0) {
            lattice->prefix_index.material_ids = (uint64_t*)malloc(lattice->prefix_index.material_count * sizeof(uint64_t));
        }
        if (lattice->prefix_index.learning_count > 0) {
            lattice->prefix_index.learning_ids = (uint64_t*)malloc(lattice->prefix_index.learning_count * sizeof(uint64_t));
        }
        if (lattice->prefix_index.performance_count > 0) {
            lattice->prefix_index.performance_ids = (uint64_t*)malloc(lattice->prefix_index.performance_count * sizeof(uint64_t));
        }
        
        // Populate (pass 3)
        uint32_t isa_idx = 0, material_idx = 0, learning_idx = 0, performance_idx = 0;
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            const char* name = lattice->nodes[i].name;
            char prefix[64];
            if (extract_prefix_from_name(name, prefix, sizeof(prefix)) > 0) {
                if (strcmp(prefix, "ISA_") == 0 && lattice->prefix_index.isa_ids) {
                    lattice->prefix_index.isa_ids[isa_idx++] = lattice->nodes[i].id;
                } else if (strcmp(prefix, "MATERIAL_") == 0 && lattice->prefix_index.material_ids) {
                    lattice->prefix_index.material_ids[material_idx++] = lattice->nodes[i].id;
                } else if ((strcmp(prefix, "LEARNING_") == 0 || strcmp(prefix, "PATTERN_") == 0) && lattice->prefix_index.learning_ids) {
                    lattice->prefix_index.learning_ids[learning_idx++] = lattice->nodes[i].id;
                } else if (strcmp(prefix, "PERFORMANCE_") == 0 && lattice->prefix_index.performance_ids) {
                    lattice->prefix_index.performance_ids[performance_idx++] = lattice->nodes[i].id;
                }
            }
        }
        lattice->prefix_index.built = true;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double hardcoded_build_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    hardcoded_build_time /= 1e6; // Convert to milliseconds
    
    // Benchmark dynamic index build
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    const char** node_names = (const char**)malloc(lattice->node_count * sizeof(const char*));
    uint64_t* node_ids = (uint64_t*)malloc(lattice->node_count * sizeof(uint64_t));
    if (node_names && node_ids) {
        for (uint32_t i = 0; i < lattice->node_count; i++) {
            node_names[i] = lattice->nodes[i].name;
            node_ids[i] = lattice->nodes[i].id;
        }
        dynamic_prefix_index_build(&lattice->prefix_index.dynamic_index, 
                                  node_names, node_ids, lattice->node_count);
        free(node_names);
        free(node_ids);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double dynamic_build_time = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
    double dynamic_build_time_ms = dynamic_build_time / 1e6;
    
    // Restore dynamic flag
    lattice->prefix_index.use_dynamic_index = old_dynamic;
    
    // Report results
    printf("Build Performance:\n");
    printf("  Hardcoded index: %.3f ms\n", hardcoded_build_time);
    printf("  Dynamic index:   %.3f ms\n", dynamic_build_time_ms);
    if (hardcoded_build_time > 0) {
        double ratio = dynamic_build_time_ms / hardcoded_build_time;
        printf("  Ratio:           %.2fx %s\n", ratio, ratio > 1.0 ? "(slower)" : "(faster)");
    }
    printf("\n");
    
    // Memory usage
    size_t hardcoded_memory = (lattice->prefix_index.isa_count + 
                               lattice->prefix_index.material_count +
                               lattice->prefix_index.learning_count +
                               lattice->prefix_index.performance_count) * sizeof(uint64_t);
    
    size_t dynamic_memory = 0;
    for (uint32_t i = 0; i < lattice->prefix_index.dynamic_index.entry_count; i++) {
        dynamic_memory += lattice->prefix_index.dynamic_index.entries[i].count * sizeof(uint64_t);
        dynamic_memory += sizeof(dynamic_prefix_entry_t);
    }
    
    printf("Memory Usage:\n");
    printf("  Hardcoded index: %zu bytes (%.2f KB)\n", hardcoded_memory, hardcoded_memory / 1024.0);
    printf("  Dynamic index:   %zu bytes (%.2f KB)\n", dynamic_memory, dynamic_memory / 1024.0);
    if (hardcoded_memory > 0) {
        double ratio = (double)dynamic_memory / hardcoded_memory;
        printf("  Ratio:           %.2fx %s\n", ratio, ratio > 1.0 ? "(more)" : "(less)");
    }
    printf("\n");
    
    printf("Prefix Counts:\n");
    printf("  Hardcoded: %u known prefixes\n", 4); // ISA_, MATERIAL_, LEARNING_, PERFORMANCE_
    printf("  Dynamic:   %u discovered prefixes\n", lattice->prefix_index.dynamic_index.entry_count);
    printf("\n");
    
    printf("===========================================================\n");
    printf("\n");
    
    return 0;
}

// Incrementally update prefix index when a new node is added
// This avoids full rebuilds that cause memory corruption
void lattice_prefix_index_add_node(persistent_lattice_t* lattice, uint64_t node_id, const char* node_name) {
    if (!lattice || !node_name) return;
    
    // If index not built yet, do full build
    if (!lattice->prefix_index.built) {
        lattice_build_prefix_index(lattice);
        return;
    }
    
    // Update dynamic index if enabled
    if (lattice->prefix_index.use_dynamic_index) {
        dynamic_prefix_index_add_node(&lattice->prefix_index.dynamic_index, node_id, node_name);
        return;  // Dynamic index handles everything, no need for hardcoded
    }
    
    // Incremental update for hardcoded index (backward compatibility)
    uint64_t* new_array = NULL;
    uint32_t new_count = 0;
    
    if (strncmp(node_name, "ISA_", 4) == 0) {
        new_count = lattice->prefix_index.isa_count + 1;
        new_array = (uint64_t*)realloc(lattice->prefix_index.isa_ids, new_count * sizeof(uint64_t));
        if (new_array) {
            lattice->prefix_index.isa_ids = new_array;
            lattice->prefix_index.isa_ids[lattice->prefix_index.isa_count++] = node_id;
        }
    } else if (strncmp(node_name, "MATERIAL_", 9) == 0) {
        new_count = lattice->prefix_index.material_count + 1;
        new_array = (uint64_t*)realloc(lattice->prefix_index.material_ids, new_count * sizeof(uint64_t));
        if (new_array) {
            lattice->prefix_index.material_ids = new_array;
            lattice->prefix_index.material_ids[lattice->prefix_index.material_count++] = node_id;
        }
    } else if (strncmp(node_name, "LEARNING_", 9) == 0) {
        new_count = lattice->prefix_index.learning_count + 1;
        new_array = (uint64_t*)realloc(lattice->prefix_index.learning_ids, new_count * sizeof(uint64_t));
        if (new_array) {
            lattice->prefix_index.learning_ids = new_array;
            lattice->prefix_index.learning_ids[lattice->prefix_index.learning_count++] = node_id;
        }
    } else if (strncmp(node_name, "PERFORMANCE_", 12) == 0) {
        new_count = lattice->prefix_index.performance_count + 1;
        new_array = (uint64_t*)realloc(lattice->prefix_index.performance_ids, new_count * sizeof(uint64_t));
        if (new_array) {
            lattice->prefix_index.performance_ids = new_array;
            lattice->prefix_index.performance_ids[lattice->prefix_index.performance_count++] = node_id;
        }
    }
    // CONCEPT_* nodes don't need to be in prefix index (they're queried differently)
    // So we silently ignore them for incremental updates
}

// Helper function: Ensure mmap is initialized for fast file access
// Returns true if mmap is available, false otherwise
static bool ensure_mmap_initialized(persistent_lattice_t* lattice) {
    if (!lattice || lattice->storage_path[0] == '\0') return false;
    
    // Already mmap'd
    if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) return true;
    
    // Try to initialize mmap lazily
    struct stat st;
    if (stat(lattice->storage_path, &st) == 0 && st.st_size > 0) {
        int fd = open(lattice->storage_path, O_RDONLY);
        if (fd >= 0) {
            lattice->mmap_size = st.st_size;
            lattice->mmap_ptr = mmap(NULL, lattice->mmap_size, PROT_READ, MAP_SHARED, fd, 0);
            if (lattice->mmap_ptr == MAP_FAILED) {
                lattice->mmap_ptr = NULL;
                lattice->mmap_size = 0;
                close(fd);
                return false;
            } else {
                // Keep FD open (needed for MAP_SHARED to work properly)
                if (lattice->storage_fd < 0) {
                    lattice->storage_fd = fd;
                } else {
                    close(fd);  // Already have an FD
                }
                // Hint kernel to prefetch beginning of file
                size_t prefetch_size = (lattice->mmap_size < 1024*1024) ? lattice->mmap_size : 1024*1024;
                madvise(lattice->mmap_ptr, prefetch_size, MADV_WILLNEED);
                return true;
            }
        }
    }
    return false;
}

// Find nodes by name with semantic prefix optimization
uint32_t lattice_find_nodes_by_name(persistent_lattice_t* lattice, const char* name,
                                   uint64_t* node_ids, uint32_t max_ids) {
    // Delegate to filtered version with no filters
    return lattice_find_nodes_by_name_filtered(lattice, name, node_ids, max_ids, 0.0, 0, 0);
}

// Find nodes by name with optional filtering (zero overhead - filtering in O(k) loop)
uint32_t lattice_find_nodes_by_name_filtered(persistent_lattice_t* lattice, const char* name,
                                             uint64_t* node_ids, uint32_t max_ids,
                                             double min_confidence,
                                             uint64_t min_timestamp,
                                             uint64_t max_timestamp) {
    if (!lattice || !name || !node_ids) return 0;
    
    // Build prefix index if not already built
    // CRITICAL: Always build the index - it's O(n) once, then enables O(k) queries
    // The previous "optimization" to skip building for large lattices was wrong - it caused O(n) linear search
    if (!lattice->prefix_index.built) {
        // Always build the index, even for large lattices
        // The index build is O(n) once, but enables O(k) queries forever after
        lattice_build_prefix_index(lattice);
    }
    
    // Auto-detect prefix from query name using system's naming pattern
    char query_prefix[64];
    size_t prefix_len = extract_prefix_from_name(name, query_prefix, sizeof(query_prefix));
    
    // Check if query uses semantic prefix - if so, use prefix index for O(k) search
    uint64_t* candidate_ids = NULL;
    uint32_t candidate_count = 0;
    
    if (prefix_len > 0) {
        if (lattice->prefix_index.use_dynamic_index) {
            // Use dynamic index (no hardcoding, handles all prefixes)
            dynamic_prefix_entry_t* dynamic_entry = dynamic_prefix_index_find(
                &lattice->prefix_index.dynamic_index, query_prefix);
            if (dynamic_entry && dynamic_entry->node_ids) {
                candidate_ids = dynamic_entry->node_ids;
                candidate_count = dynamic_entry->count;
            }
        } else {
            // Fallback to hardcoded index (backward compatibility)
            if (strcmp(query_prefix, "ISA_") == 0 && lattice->prefix_index.isa_ids) {
                candidate_ids = lattice->prefix_index.isa_ids;
                candidate_count = lattice->prefix_index.isa_count;
            } else if (strcmp(query_prefix, "MATERIAL_") == 0 && lattice->prefix_index.material_ids) {
                candidate_ids = lattice->prefix_index.material_ids;
                candidate_count = lattice->prefix_index.material_count;
            } else if (strcmp(query_prefix, "LEARNING_") == 0 && lattice->prefix_index.learning_ids) {
                candidate_ids = lattice->prefix_index.learning_ids;
                candidate_count = lattice->prefix_index.learning_count;
            } else if (strcmp(query_prefix, "PERFORMANCE_") == 0 && lattice->prefix_index.performance_ids) {
                candidate_ids = lattice->prefix_index.performance_ids;
                candidate_count = lattice->prefix_index.performance_count;
            } else if (strcmp(query_prefix, "PATTERN_") == 0 && lattice->prefix_index.learning_ids) {
                // PATTERN_ nodes are semantically related - use learning slot (they're learning patterns)
                candidate_ids = lattice->prefix_index.learning_ids;
                candidate_count = lattice->prefix_index.learning_count;
            } else if (strcmp(query_prefix, "QDRANT_COLLECTION:") == 0 && lattice->prefix_index.performance_ids) {
                // Auto-detected new prefix - use performance slot temporarily
                candidate_ids = lattice->prefix_index.performance_ids;
                candidate_count = lattice->prefix_index.performance_count;
            }
        }
    }
    
    uint32_t count = 0;
    bool has_filters = (min_confidence > 0.0 || min_timestamp > 0 || max_timestamp > 0);
    
    if (candidate_ids) {
        // Semantic prefix optimization: O(k) where k << n
        // OPTIMIZATION: For pure prefix queries (query == prefix), skip node lookups
        // Just return the IDs directly - they all match by definition
        bool is_pure_prefix_query = (prefix_len > 0 && strcmp(query_prefix, name) == 0);
        
        if (is_pure_prefix_query && !has_filters) {
            // Fast path: Pure prefix query, no filters - just copy IDs
            uint32_t copy_count = (candidate_count < max_ids) ? candidate_count : max_ids;
            for (uint32_t i = 0; i < copy_count; i++) {
                node_ids[count++] = candidate_ids[i];
            }
        } else {
            // Slow path: Need to check node names or apply filters
            // OPTIMIZATION: Try RAM cache first, then mmap, then pread
            for (uint32_t i = 0; i < candidate_count && count < max_ids; i++) {
                uint64_t candidate_id = candidate_ids[i];
                lattice_node_t node;
                bool loaded = false;
                
                // PRIORITY 1: Try RAM cache first (fastest - no I/O at all)
                if (lattice_get_node_data(lattice, candidate_id, &node) == 0) {
                    loaded = true;
                } else {
                    // PRIORITY 2: Try mmap (fast - direct memory access, no syscalls)
                    // Only if node not in RAM cache
                    uint32_t local_id = (uint32_t)(candidate_id & 0xFFFFFFFF);
                    if (local_id > 0 && local_id <= lattice->total_nodes && 
                        lattice->storage_path[0] != '\0') {
                        // Calculate file offset directly (O(1))
                        size_t header_size = 4 * sizeof(uint32_t);
                        uint32_t file_index = local_id - 1;
                        off_t node_offset = header_size + (file_index * sizeof(lattice_node_t));
                        
                        if (ensure_mmap_initialized(lattice) && 
                            lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED &&
                            node_offset < lattice->mmap_size) {
                            // Direct memory access from mmap (zero syscall overhead)
                            lattice_node_t* node_ptr = (lattice_node_t*)((char*)lattice->mmap_ptr + node_offset);
                            if (node_ptr->id == candidate_id) {
                                node = *node_ptr;  // Copy to local
                                loaded = true;
                            }
                        } else {
                            // PRIORITY 3: Fallback to pread() (slower - syscall overhead)
                            // If FD not open yet, try to open it (file may have been created after init)
                            if (lattice->storage_fd < 0) {
                                lattice->storage_fd = open(lattice->storage_path, O_RDONLY);
                            }
                            
                            // Use pread() for atomic, thread-safe read (no lseek needed)
                            if (lattice->storage_fd >= 0) {
                                if (pread(lattice->storage_fd, &node, sizeof(lattice_node_t), node_offset) == sizeof(lattice_node_t)) {
                                    if (node.id == candidate_id) {
                                        loaded = true;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (!loaded) continue;  // Skip if we couldn't load the node
                
                // For pure prefix queries, skip strstr check (all match by definition)
                bool name_matches = is_pure_prefix_query || strstr(node.name, name);
                
                if (name_matches) {
                    // Apply filters if specified (zero overhead - just condition checks)
                    if (has_filters) {
                        if (min_confidence > 0.0 && node.confidence < min_confidence) continue;
                        if (min_timestamp > 0 && node.timestamp < min_timestamp) continue;
                        if (max_timestamp > 0 && node.timestamp > max_timestamp) continue;
                    }
                    node_ids[count] = node.id;
                    count++;
                }
                // Free children array if it was allocated by lattice_get_node_data
                if (node.children) {
                    free(node.children);
                }
            }
        }
    } else {
        // Fallback: Full scan (O(n)) for non-prefix queries
        for (uint32_t i = 0; i < lattice->node_count && count < max_ids; i++) {
            if (strstr(lattice->nodes[i].name, name)) {
                // Apply filters if specified
                if (has_filters) {
                    if (min_confidence > 0.0 && lattice->nodes[i].confidence < min_confidence) continue;
                    if (min_timestamp > 0 && lattice->nodes[i].timestamp < min_timestamp) continue;
                    if (max_timestamp > 0 && lattice->nodes[i].timestamp > max_timestamp) continue;
                }
                node_ids[count] = lattice->nodes[i].id;
                count++;
            }
        }
    }
    
    return count;
}

// Evolve patterns
int lattice_evolve_patterns(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    // Simple pattern evolution - combine successful patterns
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (node->type == LATTICE_NODE_LEARNING && node->payload.learning.success_rate > 0.8) {
            // Create evolved pattern
            char evolved_pattern[256];
            // Truncate pattern_sequence to ensure it fits
            char truncated_pattern[240];
            strncpy(truncated_pattern, node->payload.learning.pattern_sequence, sizeof(truncated_pattern) - 1);
            truncated_pattern[sizeof(truncated_pattern) - 1] = '\0';
            snprintf(evolved_pattern, sizeof(evolved_pattern), "evolved_%s", truncated_pattern);
            
            lattice_store_pattern(lattice, evolved_pattern, 
                                node->payload.learning.success_rate * 0.9, // Slightly lower success rate
                                node->payload.learning.performance_gain * 1.1); // Slightly higher performance
        }
    }
    
    return 0;
}

// Update confidence
int lattice_update_confidence(persistent_lattice_t* lattice, uint64_t node_id, double confidence) {
    uint32_t node_idx = find_node_index(lattice, node_id);
    if (node_idx == UINT32_MAX || node_idx >= lattice->node_count) return -1;
    
    lattice->nodes[node_idx].confidence = fmax(0.0, fmin(1.0, confidence));
    lattice->nodes[node_idx].timestamp = get_current_timestamp();
    lattice->dirty = true;
    
    return 0;
}

// Update success_rate for a LEARNING node (Phase 3: Execution feedback)
// Uses exponential moving average: new_rate = (old_rate * 0.9) + (new_result * 0.1)
// Performance: <1us (direct memory access, no I/O)
int lattice_update_success_rate(persistent_lattice_t* lattice, uint64_t node_id, bool execution_success) {
    if (!lattice) return -1;
    
    uint32_t node_idx = find_node_index(lattice, node_id);
    if (node_idx == UINT32_MAX || node_idx >= lattice->node_count) return -1;
    
    lattice_node_t* node = &lattice->nodes[node_idx];
    if (node->type != LATTICE_NODE_LEARNING) return -1;
    
    // Update success_rate using exponential moving average
    // 90% weight on old value, 10% weight on new result
    // This provides smooth learning while being responsive to recent results
    double new_result = execution_success ? 1.0 : 0.0;
    double old_rate = node->payload.learning.success_rate;
    
    // If success_rate is 0 (uninitialized), set it directly
    if (old_rate == 0.0) {
        node->payload.learning.success_rate = new_result;
    } else {
        // Exponential moving average: EMA = (old * 0.9) + (new * 0.1)
        node->payload.learning.success_rate = (old_rate * 0.9) + (new_result * 0.1);
    }
    
    // Update metadata
    node->payload.learning.frequency++;
    node->payload.learning.last_used = get_current_timestamp();
    lattice->dirty = true;
    
    return 0;
}

// Streaming access functions - PRODUCTION IMPLEMENTATION
// NOTE: This function intentionally returns a pointer for performance (streaming access)
// Callers must not hold the pointer across lattice modifications
__attribute__((deprecated("Use lattice_get_node_data() for safe access. This function is kept for streaming performance but returns unsafe pointers.")))
lattice_node_t* lattice_get_node_streaming(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return NULL;
    
    // First try cache (fast path) - use local ID (lower 32 bits)
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    // Safety bound: 10x max_nodes (allows for sparse IDs)
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        uint32_t index = lattice->id_to_index_map[local_id];
        if (index < lattice->node_count && lattice->node_id_map[index] == node_id) {
            lattice->access_count[index]++;
            lattice->last_access[index] = get_current_timestamp();
            return &lattice->nodes[index]; // WARNING: UNSAFE: Pointer invalidated by realloc
        }
    }
    
    // Fallback to linear search in cache
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->node_id_map[i] == node_id) {
            lattice->access_count[i]++;
            lattice->last_access[i] = get_current_timestamp();
            return &lattice->nodes[i]; // WARNING: UNSAFE: Pointer invalidated by realloc
        }
    }
    
    // Not in cache - initialize mmap if needed and use it
    if (!lattice->mmap_ptr || lattice->mmap_ptr == MAP_FAILED) {
        // Lazy mmap initialization for streaming access
        struct stat st;
        if (stat(lattice->storage_path, &st) == 0 && st.st_size > 0) {
            int fd = open(lattice->storage_path, O_RDONLY);
            if (fd >= 0) {
                lattice->mmap_size = st.st_size;
                lattice->mmap_ptr = mmap(NULL, lattice->mmap_size, PROT_READ, MAP_SHARED, fd, 0);
                if (lattice->mmap_ptr == MAP_FAILED) {
                    lattice->mmap_ptr = NULL;
                    lattice->mmap_size = 0;
                    close(fd);
                } else {
                    lattice->storage_fd = fd;
                    // Hint kernel to prefetch beginning of file
                    size_t prefetch_size = (lattice->mmap_size < 1024*1024) ? lattice->mmap_size : 1024*1024;
                    madvise(lattice->mmap_ptr, prefetch_size, MADV_WILLNEED);
                }
            }
        }
    }
    
    // Use mmap if available
    if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED && lattice->mmap_size > 0) {
        // Calculate node offset in mmap region
        // Format: header (16 bytes) + nodes (sizeof(lattice_node_t) each)
        size_t header_size = sizeof(uint32_t) * 4;
        if (node_id < lattice->total_nodes) {
            size_t node_offset = header_size + (node_id * sizeof(lattice_node_t));
            if (node_offset + sizeof(lattice_node_t) <= lattice->mmap_size) {
                lattice_node_t* mmap_node = (lattice_node_t*)((char*)lattice->mmap_ptr + node_offset);
                if (mmap_node->id == node_id) {
                    // Prefetch related nodes asynchronously (only if enabled)
                    if (lattice->prefetch_enabled) {
                        lattice_prefetch_related_nodes(lattice, node_id);
                    }
                    return mmap_node; // WARNING: UNSAFE: Pointer from mmap, but stable
                }
            }
        }
    }
    
    // Fallback to on-demand disk loading
    return lattice_get_node(lattice, node_id);
}

int lattice_add_node_streaming(persistent_lattice_t* lattice, lattice_node_type_t type, 
                              const char* name, const char* data, uint64_t parent_id) {
    if (!lattice) return -1;
    
    // Use regular add_node (mmap writes require special handling)
    // For writes, we use the regular path which handles cache and persistence
    int result = lattice_add_node(lattice, type, name, data, parent_id);
    
    // NOTE: Removed blocking msync(MS_SYNC) from hot path to maintain ~28us write latency.
    // - WAL provides durability guarantee via background flush thread
    // - MAP_SHARED automatically handles dirty page flushing by kernel
    // - Blocking msync would cause ~28ms latency (1000x slower)
    // If immediate flush needed, use MS_ASYNC (non-blocking) instead:
    //   msync(lattice->mmap_ptr, lattice->mmap_size, MS_ASYNC);
    
    return result;
}

uint32_t lattice_find_nodes_by_type_streaming(persistent_lattice_t* lattice, lattice_node_type_t type,
                                            uint64_t* node_ids, uint32_t max_ids) {
    if (!lattice || !node_ids || max_ids == 0) return 0;
    
    uint32_t found = 0;
    
    // First search cache
    for (uint32_t i = 0; i < lattice->node_count && found < max_ids; i++) {
        if (lattice->nodes[i].type == type) {
            node_ids[found++] = lattice->nodes[i].id;
        }
    }
    
    // If mmap is available and we need more results, search mmap region
    if (found < max_ids && lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
        size_t header_size = sizeof(uint32_t) * 4;
        size_t nodes_in_mmap = (lattice->mmap_size - header_size) / sizeof(lattice_node_t);
        
        for (size_t i = 0; i < nodes_in_mmap && found < max_ids; i++) {
            size_t node_offset = header_size + (i * sizeof(lattice_node_t));
            if (node_offset + sizeof(lattice_node_t) <= lattice->mmap_size) {
                lattice_node_t* mmap_node = (lattice_node_t*)((char*)lattice->mmap_ptr + node_offset);
                
                // Check if already found in cache
                bool already_found = false;
                for (uint32_t j = 0; j < found; j++) {
                    if (node_ids[j] == mmap_node->id) {
                        already_found = true;
                        break;
                    }
                }
                
                if (!already_found && mmap_node->type == type) {
                    node_ids[found++] = mmap_node->id;
                }
            }
        }
    }
    
    return found;
}

int lattice_evict_oldest_nodes(persistent_lattice_t* lattice, uint32_t count) {
    if (!lattice || count == 0) return 0;
    
    // Simple LRU eviction - remove oldest accessed nodes
    uint32_t evicted = 0;
    
    while (evicted < count && lattice->node_count > 0) {
        uint32_t oldest_idx = 0;
        uint64_t oldest_time = lattice->last_access[0];
        
        for (uint32_t i = 1; i < lattice->node_count; i++) {
            if (lattice->last_access[i] < oldest_time) {
                oldest_time = lattice->last_access[i];
                oldest_idx = i;
            }
        }
        
        // Remove node - clear reverse index entry first
        uint64_t evicted_id = lattice->node_id_map[oldest_idx];
        uint32_t evicted_local_id = (uint32_t)(evicted_id & 0xFFFFFFFF);
        if (lattice->id_to_index_map && evicted_local_id < lattice->max_nodes * 10) {
            lattice->id_to_index_map[evicted_local_id] = 0; // Clear reverse index
        }
        
        if (lattice->nodes[oldest_idx].children) {
            free(lattice->nodes[oldest_idx].children);
        }
        
        // Shift remaining nodes and update reverse index
        for (uint32_t i = oldest_idx; i < lattice->node_count - 1; i++) {
            lattice->nodes[i] = lattice->nodes[i + 1];
            lattice->node_id_map[i] = lattice->node_id_map[i + 1];
            lattice->access_count[i] = lattice->access_count[i + 1];
            lattice->last_access[i] = lattice->last_access[i + 1];
            
            // Update reverse index for shifted node (index changed from i+1 to i)
            uint64_t shifted_id = lattice->node_id_map[i];
            uint32_t shifted_local_id = (uint32_t)(shifted_id & 0xFFFFFFFF);
            if (lattice->id_to_index_map && shifted_local_id < lattice->max_nodes * 10) {
                lattice->id_to_index_map[shifted_local_id] = i;
            }
        }
        
        lattice->node_count--;
        evicted++;
    }
    
    return evicted;
}

// Async prefetcher: Predict and preload likely next nodes
// Uses access_count and last_access to predict access patterns
// Uses madvise() to hint kernel for page prefetching (non-blocking)
int lattice_prefetch_related_nodes(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return -1;
    
    // Safety: If prefetch is disabled, don't do anything
    if (!lattice->prefetch_enabled) {
        return 0;  // Success (no-op when disabled)
    }
    
    // Get the current node DIRECTLY (avoid recursion - don't call lattice_get_node_data)
    // Use O(1) lookup to get node without triggering prefetch again
    lattice_node_t* node = NULL;
    
    // O(1) LOOKUP using reverse index (same logic as lattice_get_node_data but without prefetch)
    // Use local ID (lower 32 bits) for reverse index lookup
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        uint32_t index = lattice->id_to_index_map[local_id];
        if (index < lattice->node_count) {
            node = &lattice->nodes[index];
        }
    }
    
    // If not in RAM cache, try disk mode
    if (!node && lattice->disk_mode && node_id > 0 && node_id <= lattice->total_nodes) {
        uint32_t file_index = node_id - 1;
        if (file_index < lattice->total_file_nodes) {
            node = &lattice->nodes[file_index];
            if (node->id != node_id) {
                node = NULL;  // Wrong node at this index
            }
        }
    }
    
    if (!node) {
        return -1;  // Node not found
    }
    
    // Strategy 1: Prefetch children (high probability of access)
    if (node->child_count > 0 && node->children) {
        // Hint kernel to prefetch pages containing children
        // Calculate approximate file offset for children nodes
        // For now, we'll use madvise on the mmap region if available
        if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
            // Estimate: each node is ~sizeof(lattice_node_t) bytes
            // Prefetch a window around likely child locations
            size_t prefetch_size = node->child_count * sizeof(lattice_node_t) * 2;  // 2x for safety
            if (prefetch_size > 0 && prefetch_size < lattice->mmap_size) {
                // Use madvise to hint kernel (non-blocking, async)
                madvise(lattice->mmap_ptr, prefetch_size, MADV_WILLNEED);
            }
        }
    }
    
    // Strategy 2: Prefetch parent (often accessed after children)
    if (node->parent_id > 0) {
        // Parent is likely to be accessed soon
        // Load parent into cache if not already present
        bool parent_in_cache = false;
        // Use same safety bound as lattice_get_node_data() for consistency
        if (lattice->id_to_index_map && node->parent_id > 0 && node->parent_id < lattice->max_nodes * 10) {
            uint32_t parent_idx = lattice->id_to_index_map[node->parent_id];
            if (parent_idx < lattice->node_count && lattice->node_id_map[parent_idx] == node->parent_id) {
                parent_in_cache = true;
            }
        }
        
        if (!parent_in_cache) {
            // Try to load parent from disk or mmap (direct access, no recursion)
            lattice_node_t* parent = NULL;
            // Direct access using same logic as above (avoid recursion)
            if (lattice->id_to_index_map && node->parent_id < lattice->max_nodes * 10) {
                uint32_t parent_idx = lattice->id_to_index_map[node->parent_id];
                if (parent_idx < lattice->node_count) {
                    parent = &lattice->nodes[parent_idx];
                }
            }
            if (!parent && lattice->disk_mode && node->parent_id > 0 && node->parent_id <= lattice->total_nodes) {
                uint32_t file_index = node->parent_id - 1;
                if (file_index < lattice->total_file_nodes) {
                    parent = &lattice->nodes[file_index];
                    if (parent->id != node->parent_id) {
                        parent = NULL;
                    }
                }
            }
            if (parent && lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
                // Hint kernel to prefetch parent's page
                size_t header_size = sizeof(uint32_t) * 4;
                size_t parent_offset = header_size + (node->parent_id * sizeof(lattice_node_t));
                size_t page_size = getpagesize();
                void* page_start = (void*)((char*)lattice->mmap_ptr + (parent_offset & ~(page_size - 1)));
                size_t prefetch_size = page_size;
                if ((char*)page_start + prefetch_size <= (char*)lattice->mmap_ptr + lattice->mmap_size) {
                    madvise(page_start, prefetch_size, MADV_WILLNEED);
                }
            }
        }
    }
    
    // Strategy 3: Prefetch high-frequency nodes (based on access_count)
    // Find top 5 most frequently accessed nodes and prefetch them
    uint32_t top_nodes[5] = {0};
    uint32_t top_counts[5] = {0};
    
    for (uint32_t i = 0; i < lattice->node_count && i < 1000; i++) {
        if (lattice->access_count[i] > top_counts[4]) {
            // Insert into top 5 (simple bubble-up)
            for (int j = 4; j >= 0; j--) {
                if (lattice->access_count[i] > top_counts[j]) {
                    if (j < 4) {
                        top_nodes[j + 1] = top_nodes[j];
                        top_counts[j + 1] = top_counts[j];
                    }
                    top_nodes[j] = lattice->nodes[i].id;
                    top_counts[j] = lattice->access_count[i];
                } else {
                    break;
                }
            }
        }
    }
    
    // Strategy 4: Prefetch semantically related nodes (same prefix)
    // If node is ISA_*, prefetch other ISA_* nodes
    if (strncmp(node->name, "ISA_", 4) == 0 && lattice->prefix_index.built) {
        // Prefetch window of ISA nodes (likely to be accessed together)
        if (lattice->prefix_index.isa_ids && lattice->prefix_index.isa_count > 0) {
            uint32_t prefetch_count = (lattice->prefix_index.isa_count < 10) ? 
                                     lattice->prefix_index.isa_count : 10;
            // Hint kernel to prefetch these pages
            if (lattice->mmap_ptr && lattice->mmap_ptr != MAP_FAILED) {
                size_t prefetch_size = prefetch_count * sizeof(lattice_node_t);
                if (prefetch_size < lattice->mmap_size) {
                    madvise(lattice->mmap_ptr, prefetch_size, MADV_WILLNEED);
                }
            }
        }
    }
    
    // Note: Don't free node->children - node is a pointer to the actual node in the lattice
    // We didn't allocate it, so we shouldn't free it
    
    return 0;
}

void lattice_print_streaming_stats(persistent_lattice_t* lattice) {
    if (!lattice) return;
    
    printf("Lattice Statistics:\n");
    printf("  Total nodes: %u\n", lattice->total_nodes);
    printf("  RAM nodes: %u\n", lattice->node_count);
    printf("  Max RAM nodes: %u\n", lattice->max_nodes);
    printf("  Next Local ID: %lu (Device ID: %u, Full ID format: device_id << 32 | local_id)\n", 
           (unsigned long)lattice->next_id, lattice->device_id);
    printf("  Dirty: %s\n", lattice->dirty ? "Yes" : "No");
    printf("  Storage path: %s\n", lattice->storage_path);
}

// Corruption detection and repair
int lattice_scan_and_repair_corruption(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    uint32_t corrupted_count = 0;
    uint32_t repaired_count = 0;
    
    // Scan for corruption and attempt repair
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        bool is_corrupted = false;
        
        // Check for invalid node ID (uninitialized node)
        if (node->id == 0) {
            is_corrupted = true;
            // Repair: Mark for removal (will be compacted out)
            corrupted_count++;
        } else {
            uint32_t local_id = (uint32_t)(node->id & 0xFFFFFFFF);
            // Better validation: Account for chunked storage (IDs can be sparse)
            // Allow IDs up to 10x max_nodes for chunked data, but validate they're reasonable
            uint32_t max_safe_id = lattice->max_nodes * 10;
            if (local_id > max_safe_id && local_id < UINT32_MAX / 2) {
                // Very large IDs might be valid (chunked storage), but check bounds
                if (node->id >= lattice->next_id && node->id < (uint64_t)lattice->next_id + 1000000) {
                    // ID is beyond next_id but within reasonable range - might be valid chunked node
                    // Check if name suggests chunked storage
                    if (strncmp(node->name, "C:", 2) != 0 && 
                        strncmp(node->name, "CHUNK:", 6) != 0) {
                        is_corrupted = true;
                        corrupted_count++;
                    }
                } else {
                    is_corrupted = true;
                    corrupted_count++;
                }
            }
        }
        
        // Check for invalid type
        if (!is_corrupted && (node->type < LATTICE_NODE_PRIMITIVE || node->type > 106)) {
            is_corrupted = true;
            corrupted_count++;
        }
        
        // Attempt repair: Clear corrupted nodes (will be compacted)
        if (is_corrupted) {
            node->id = 0;  // Mark for removal
            node->type = LATTICE_NODE_PRIMITIVE;  // Reset to safe default
            node->name[0] = '\0';
            node->data[0] = '\0';
            node->child_count = 0;
            if (node->children) {
                free(node->children);
                node->children = NULL;
            }
            repaired_count++;
        }
    }
    
    if (corrupted_count > 0) {
        printf("[LATTICE-REPAIR] WARN Detected %u corrupted nodes, marked %u for removal\n",
               corrupted_count, repaired_count);
        // Suggest compaction
        printf("[LATTICE-REPAIR] INFO Run lattice_compact_file() to remove corrupted nodes\n");
        return 1;  // Corruption detected and marked
    }
    
    return 0;  // No corruption
}

// Compact lattice file: Remove uninitialized/corrupted nodes and rebuild file
// This removes nodes with id==0 and rebuilds the file with only valid nodes
int lattice_compact_file(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    printf("[LATTICE-COMPACT] INFO Starting file compaction...\n");
    
    // Collect valid nodes (skip nodes with id==0)
    lattice_node_t* valid_nodes = (lattice_node_t*)malloc(lattice->node_count * sizeof(lattice_node_t));
    uint64_t* valid_node_ids = (uint64_t*)malloc(lattice->node_count * sizeof(uint64_t));
    if (!valid_nodes || !valid_node_ids) {
        printf("[LATTICE-COMPACT] ERROR Failed to allocate memory\n");
        if (valid_nodes) free(valid_nodes);
        if (valid_node_ids) free(valid_node_ids);
        return -1;
    }
    
    uint32_t valid_count = 0;
    uint32_t removed_count = 0;
    
    // Scan for valid nodes
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip uninitialized/corrupted nodes (id == 0)
        if (node->id == 0) {
            removed_count++;
            continue;
        }
        
        // Validate node before including
        uint32_t local_id = (uint32_t)(node->id & 0xFFFFFFFF);
        uint32_t max_safe_id = lattice->max_nodes * 10;
        if (local_id > max_safe_id && local_id < UINT32_MAX / 2) {
            // Check if it's a valid chunked node
            if (strncmp(node->name, "CHUNKED:", 8) != 0 && 
                strncmp(node->name, "CHUNK:", 6) != 0) {
                // Invalid large ID, skip
                removed_count++;
                continue;
            }
        }
        
        // Copy valid node
        valid_nodes[valid_count] = *node;
        valid_node_ids[valid_count] = node->id;
        
        // Deep copy children array if present
        if (node->child_count > 0 && node->children) {
            valid_nodes[valid_count].children = (uint64_t*)malloc(node->child_count * sizeof(uint64_t));
            if (valid_nodes[valid_count].children) {
                memcpy(valid_nodes[valid_count].children, node->children,
                       node->child_count * sizeof(uint64_t));
            } else {
                valid_nodes[valid_count].child_count = 0;
            }
        } else {
            valid_nodes[valid_count].children = NULL;
        }
        
        valid_count++;
    }
    
    printf("[LATTICE-COMPACT] OK Found %u valid nodes, removing %u corrupted/uninitialized nodes\n",
           valid_count, removed_count);
    
    if (valid_count == 0) {
        printf("[LATTICE-COMPACT] WARN No valid nodes found, cannot compact\n");
        free(valid_nodes);
        free(valid_node_ids);
        return -1;
    }
    
    // Free old node arrays
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->nodes[i].children) {
            free(lattice->nodes[i].children);
        }
    }
    free(lattice->nodes);
    free(lattice->node_id_map);
    
    // Replace with compacted arrays
    lattice->nodes = valid_nodes;
    lattice->node_id_map = valid_node_ids;
    lattice->node_count = valid_count;
    lattice->total_nodes = valid_count;
    
    // Rebuild reverse index
    if (lattice->id_to_index_map) {
        free(lattice->id_to_index_map);
    }
    uint32_t index_size = lattice->max_nodes * 10;
    lattice->id_to_index_map = (uint32_t*)calloc(index_size, sizeof(uint32_t));
    if (lattice->id_to_index_map) {
        for (uint32_t i = 0; i < valid_count; i++) {
            uint32_t local_id = (uint32_t)(valid_node_ids[i] & 0xFFFFFFFF);
            if (local_id < index_size) {
                lattice->id_to_index_map[local_id] = i;
            }
        }
    }
    
    // Invalidate prefix index (will rebuild on next query)
    lattice->prefix_index.built = false;
    
    // Save compacted file
    if (lattice_save(lattice) != 0) {
    printf("[LATTICE-COMPACT] ERROR Failed to save compacted file\n");
        return -1;
    }
    
    printf("[LATTICE-COMPACT] OK Compaction complete: %u nodes remaining (removed %u)\n",
           valid_count, removed_count);
    
    return 0;
}

// Configure persistence settings (production requirement)
void lattice_configure_persistence(persistent_lattice_t* lattice,
                                   bool auto_save_enabled,
                                   uint32_t interval_nodes,
                                   uint32_t interval_seconds,
                                   bool save_on_pressure) {
    if (!lattice) return;
    
    lattice->persistence.auto_save_enabled = auto_save_enabled;
    lattice->persistence.auto_save_interval_nodes = interval_nodes;
    lattice->persistence.auto_save_interval_seconds = interval_seconds;
    lattice->persistence.save_on_memory_pressure = save_on_pressure;
    
    printf("[LATTICE-PERSISTENCE] OK Configured: auto_save=%s, interval_nodes=%u, interval_seconds=%u, save_on_pressure=%s\n",
           auto_save_enabled ? "enabled" : "disabled",
           interval_nodes, interval_seconds,
           save_on_pressure ? "enabled" : "disabled");
}

// Sidecar persistence functions - not used by DroneKit system
int lattice_save_sidecar_state(persistent_lattice_t* lattice, const char* sidecar_data) {
    (void)lattice;
    (void)sidecar_data;
    return 0;
}

int lattice_load_sidecar_state(persistent_lattice_t* lattice, char** sidecar_data) {
    (void)lattice;
    (void)sidecar_data;
    return 0;
}

int lattice_store_sidecar_mapping(persistent_lattice_t* lattice, const lattice_sidecar_mapping_t* mapping) {
    (void)lattice;
    (void)mapping;
    return 0;
}

int lattice_load_sidecar_mappings(persistent_lattice_t* lattice, lattice_sidecar_mapping_t* mappings, uint32_t max_mappings, uint32_t* count) {
    (void)lattice;
    (void)mappings;
    (void)max_mappings;
    (void)count;
    return 0;
}

int lattice_store_sidecar_event(persistent_lattice_t* lattice, const lattice_sidecar_event_t* event) {
    (void)lattice;
    (void)event;
    return 0;
}

int lattice_load_recent_events(persistent_lattice_t* lattice, lattice_sidecar_event_t* events, uint32_t max_events, uint32_t* count) {
    (void)lattice;
    (void)events;
    (void)max_events;
    (void)count;
    return 0;
}

int lattice_store_sidecar_suggestion(persistent_lattice_t* lattice, const lattice_sidecar_suggestion_t* suggestion) {
    (void)lattice;
    (void)suggestion;
    return 0;
}

int lattice_load_approved_suggestions(persistent_lattice_t* lattice, lattice_sidecar_suggestion_t* suggestions, uint32_t max_suggestions, uint32_t* count) {
    (void)lattice;
    (void)suggestions;
    (void)max_suggestions;
    (void)count;
    return 0;
}

// ============================================================================
// WAL (Write-Ahead Logging) Functions for ACID Durability
// ============================================================================

// Enable WAL for the lattice
int lattice_enable_wal(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    if (lattice->wal) {
        // WAL already enabled
        return 0;
    }
    
    // Allocate WAL context
    lattice->wal = (wal_context_t*)malloc(sizeof(wal_context_t));
    if (!lattice->wal) {
        printf("[LATTICE-WAL] ERROR Failed to allocate WAL context\n");
        return -1;
    }
    
    // Initialize WAL
    if (wal_init(lattice->wal, lattice->storage_path) != 0) {
        printf("[LATTICE-WAL] ERROR Failed to initialize WAL\n");
        free(lattice->wal);
        lattice->wal = NULL;
        return -1;
    }
    
    // Enable batching for performance - scale with tier limit
    // For free tier (25k nodes): batch size = 12.5k (half of limit, ensures checkpoint before limit)
    // For higher tiers: scale proportionally (50k for 100k limit, etc.)
    // This ensures WAL checkpoints happen before hitting the node limit
    uint32_t optimal_batch_size;
    if (lattice->evaluation_mode && lattice->free_tier_limit > 0) {
        // Free tier: use half of limit to ensure checkpoint before limit is reached
        optimal_batch_size = lattice->free_tier_limit / 2;
        // Minimum 1000, maximum 12.5k for free tier
        if (optimal_batch_size < 1000) optimal_batch_size = 1000;
        if (optimal_batch_size > 12500) optimal_batch_size = 12500;
    } else {
        // Production/Pro tier: use 50k (optimized for NVMe sequential writes)
        optimal_batch_size = 50000;
    }
    wal_set_batch_size(lattice->wal, optimal_batch_size);
    
    // CRITICAL: Disable adaptive batching or set limits high enough
    // Adaptive batching was capping at 500, killing performance
    // For free tier: max = tier limit, for production: max = 100K
    uint32_t max_batch = lattice->evaluation_mode && lattice->free_tier_limit > 0 
        ? lattice->free_tier_limit 
        : 100000;
    wal_enable_adaptive_batching(lattice->wal, 1000, max_batch);
    
    lattice->wal_enabled = true;
    SYNRIX_LOG_INFO("[LATTICE-WAL] OK WAL enabled for lattice (batching: %u entries, tier-scaled)\n", optimal_batch_size);
    
    // Recover from WAL if it exists (apply any uncheckpointed entries)
    lattice_recover_from_wal(lattice);
    
    return 0;
}

// Disable WAL for the lattice
void lattice_disable_wal(persistent_lattice_t* lattice) {
    if (!lattice || !lattice->wal) return;
    
    wal_cleanup(lattice->wal);
    free(lattice->wal);
    lattice->wal = NULL;
    lattice->wal_enabled = false;
    
    SYNRIX_LOG_INFO("[LATTICE-WAL] OK WAL disabled\n");
}

// Flush WAL buffer to disk (manual flush for durability)
int lattice_wal_flush(persistent_lattice_t* lattice) {
    if (!lattice || !lattice->wal || !lattice->wal_enabled) return -1;
    
    if (lattice->wal->batch_count == 0) {
        // Nothing to flush
        return 0;
    }
    
    SYNRIX_LOG_INFO("[LATTICE-WAL] INFO Flushing WAL buffer (%u entries) to disk...\n", lattice->wal->batch_count);
    
    // Flush buffer
    if (wal_flush(lattice->wal) != 0) {
        printf("[LATTICE-WAL] ERROR Failed to flush WAL buffer\n");
        return -1;
    }
    
    // Wait for flush to complete (synchronous wait for durability)
    if (lattice->wal->sequence > 0) {
        if (wal_flush_wait(lattice->wal, lattice->wal->sequence) != 0) {
            printf("[LATTICE-WAL] WARN Flush wait returned error (may be OK)\n");
        }
    }
    
    SYNRIX_LOG_INFO("[LATTICE-WAL] OK WAL buffer flushed to disk\n");
    return 0;
}

// Add node with WAL (guarantees durability)
uint64_t lattice_add_node_with_wal(persistent_lattice_t* lattice, lattice_node_type_t type,
                                  const char* name, const char* data, uint64_t parent_id) {
    if (!lattice) return 0;
    
    // Add node first (get the ID)
    uint64_t node_id = lattice_add_node(lattice, type, name, data, parent_id);
    if (node_id == 0) {
        return 0;
    }
    
    // Write to WAL if enabled
    if (lattice->wal && lattice->wal_enabled) {
        wal_append_add_node(lattice->wal, node_id, (uint8_t)type, name, data, parent_id);
        
        // Periodic checkpoint - scale with tier limit
        // For free tier: checkpoint at half of limit (ensures checkpoint before limit)
        // For production: checkpoint every 50k entries
        uint32_t checkpoint_interval = lattice->evaluation_mode && lattice->free_tier_limit > 0
            ? (lattice->free_tier_limit / 2)
            : 50000;
        if (lattice->wal->entries_since_checkpoint >= checkpoint_interval) {
            lattice_wal_checkpoint(lattice);
        }
    }
    
    return node_id;
}

// Update node with WAL
int lattice_update_node_with_wal(persistent_lattice_t* lattice, uint64_t id, const char* data) {
    if (!lattice) return -1;
    
    // Update node first
    int result = lattice_update_node(lattice, id, data);
    if (result != 0) {
        return result;
    }
    
    // Write to WAL if enabled
    if (lattice->wal && lattice->wal_enabled) {
        wal_append_update_node(lattice->wal, id, data);
        
        // Periodic checkpoint - scale with tier limit
        uint32_t checkpoint_interval = lattice->evaluation_mode && lattice->free_tier_limit > 0
            ? (lattice->free_tier_limit / 2)
            : 50000;
        if (lattice->wal->entries_since_checkpoint >= checkpoint_interval) {
            lattice_wal_checkpoint(lattice);
        }
    }
    
    return 0;
}

// Add child with WAL
int lattice_add_child_with_wal(persistent_lattice_t* lattice, uint64_t parent_id, uint64_t child_id) {
    if (!lattice) return -1;
    
    // Add child first
    int result = lattice_add_child(lattice, parent_id, child_id);
    if (result != 0) {
        return result;
    }
    
    // Write to WAL if enabled
    if (lattice->wal && lattice->wal_enabled) {
        wal_append_add_child(lattice->wal, parent_id, child_id);
        
        // Periodic checkpoint - scale with tier limit
        uint32_t checkpoint_interval = lattice->evaluation_mode && lattice->free_tier_limit > 0
            ? (lattice->free_tier_limit / 2)
            : 50000;
        if (lattice->wal->entries_since_checkpoint >= checkpoint_interval) {
            lattice_wal_checkpoint(lattice);
        }
    }
    
    return 0;
}

// Checkpoint WAL (apply entries to main file, then mark as applied)
int lattice_wal_checkpoint(persistent_lattice_t* lattice) {
    if (!lattice || !lattice->wal || !lattice->wal_enabled) return -1;
    
    // CRITICAL: Flush WAL buffer to disk BEFORE recovering
    // WAL entries are batched in memory - we must flush them to file first
    // so that recovery can read them
    if (lattice->wal->batch_count > 0) {
        SYNRIX_LOG_INFO("[LATTICE-WAL] INFO Flushing WAL buffer (%u entries) to disk...\n", lattice->wal->batch_count);
        wal_flush(lattice->wal);
        // Wait for flush to complete (synchronous wait for durability)
        if (lattice->wal->sequence > 0) {
            wal_flush_wait(lattice->wal, lattice->wal->sequence);
        }
    }
    
    // CRITICAL: Apply WAL entries to main file BEFORE checkpointing
    // The checkpoint just marks entries as applied, it doesn't write them to the file
    // So we need to recover/replay the WAL entries to apply them to the main file
    SYNRIX_LOG_INFO("[LATTICE-WAL] INFO Applying WAL entries to main file before checkpoint...\n");
    if (lattice_recover_from_wal(lattice) != 0) {
        printf("[LATTICE-WAL] WARN Failed to apply WAL entries (continuing anyway)\n");
    }
    
    // Save lattice (write applied nodes to file)
    if (lattice_save(lattice) != 0) {
        printf("[LATTICE-WAL] WARN Failed to save lattice before checkpoint\n");
        return -1;
    }
    
    // Checkpoint WAL (mark entries as applied)
    if (wal_checkpoint(lattice->wal) != 0) {
        printf("[LATTICE-WAL] ERROR Failed to checkpoint WAL\n");
        return -1;
    }
    
    SYNRIX_LOG_INFO("[LATTICE-WAL] OK WAL entries applied and checkpointed\n");
    return 0;
}

// Recover from WAL on startup
int lattice_recover_from_wal(persistent_lattice_t* lattice) {
    if (!lattice || !lattice->wal || !lattice->wal_enabled) {
        // WAL not enabled, nothing to recover
        return 0;
    }
    
    // Recovery callback functions
    int apply_add_node_cb(void* ctx, uint64_t node_id, uint8_t type,
                         const char* name, const char* data, uint64_t parent_id) {
        (void)node_id; // Parameter used for API consistency
        persistent_lattice_t* l = (persistent_lattice_t*)ctx;
        if (!l) return -1;
        
        // CRITICAL: Allow nodes with empty names during recovery (they might be chunk nodes)
        // But prefer to skip if name is NULL (invalid entry)
        if (!name) {
            return -1;  // Invalid entry, skip
        }
        
        // Check if node already exists by name (not by ID, since IDs might differ)
        // Only check if name is non-empty (chunk nodes might have empty names)
        bool node_exists = false;
        if (strlen(name) > 0) {
            for (uint32_t i = 0; i < l->node_count; i++) {
                if (l->nodes[i].type == (lattice_node_type_t)type &&
                    strlen(l->nodes[i].name) > 0 &&
                    strcmp(l->nodes[i].name, name) == 0) {
                    node_exists = true;
                    break;
                }
            }
        }
        
        if (node_exists) {
            // Node already exists, skip
            return 0;
        }
        
        // Check if data is compressed (binary format with compression flag)
        if (data && strlen(data) >= 2) {
            uint16_t length_header;
            memcpy(&length_header, data, 2);
            
            // Check if compressed (bit 15 set)
            if (length_header & 0x8000) {
                // Compressed data - use compressed-aware API
                bool wal_was_enabled = (l->wal && l->wal_enabled);
                if (l->wal) {
                    l->wal_enabled = false;  // Disable WAL during recovery
                }
                
                // Extract compressed data length
                size_t compressed_len = length_header & 0x7FFF;
                if (compressed_len > 0 && strlen(data) >= 2 + compressed_len) {
                    // CRITICAL: During WAL recovery, compressed nodes bypass limit check
                    // (lattice_add_node_compressed will need similar internal version if it has limit checks)
                    uint64_t added_id = lattice_add_node_compressed(l, (lattice_node_type_t)type, name,
                                                                   data, 2 + compressed_len, parent_id);
                    
                    if (l->wal) {
                        l->wal_enabled = wal_was_enabled;  // Restore WAL state
                    }
                    
                    if (added_id == 0) {
                        return -1;
                    }
                    return 0;
                }
                
                if (l->wal) {
                    l->wal_enabled = wal_was_enabled;
                }
            }
        }
        
        // Uncompressed data - use internal API (bypasses limit check - recovery restores state, doesn't add)
        bool wal_was_enabled = (l->wal && l->wal_enabled);
        if (l->wal) {
            l->wal_enabled = false;  // Disable WAL during recovery
        }
        
        // CRITICAL: Use internal function to bypass free tier limit check
        // WAL recovery restores existing state - it does not "add" new nodes
        uint64_t added_id = lattice_add_node_internal(l, (lattice_node_type_t)type, name, data, parent_id);
        
        if (l->wal) {
            l->wal_enabled = wal_was_enabled;  // Restore WAL state
        }
        
        if (added_id == 0) {
            return -1;
        }
        
        return 0;
    }
    
    int apply_update_node_cb(void* ctx, uint64_t node_id, const char* data) {
        persistent_lattice_t* l = (persistent_lattice_t*)ctx;
        if (!l || !data) return -1;
        
        // CRITICAL: Check if node exists before trying to update it
        // During WAL recovery, UPDATE_NODE operations may reference nodes that were never created
        // (e.g., if the main file was saved without those nodes, or if ADD_NODE failed)
        // In this case, we should skip the update rather than failing
        // Use direct lookup (same logic as lattice_update_node) to avoid overhead
        uint32_t index = UINT32_MAX;
        
        // Safety check: ensure maps are initialized
        if (!l->node_id_map || l->node_count == 0) {
            return 0;  // No nodes loaded yet - skip update
        }
        
        if (l->id_to_index_map != NULL && node_id > 0) {
            uint32_t safe_bound = l->max_nodes * 10;
            if (node_id < safe_bound && node_id < (uint64_t)(l->max_nodes * 10)) {
                uint32_t idx = l->id_to_index_map[node_id];
                if (idx < l->node_count && idx < l->max_nodes && 
                    l->node_id_map[idx] == node_id) {
                    index = idx;
                }
            }
        }
        
        // Fallback to O(n) search if not found via index
        if (index == UINT32_MAX && l->node_id_map) {
            for (uint32_t i = 0; i < l->node_count && i < l->max_nodes; i++) {
                if (l->node_id_map[i] == node_id) {
                    index = i;
                    break;
                }
            }
        }
        
        // If node doesn't exist, skip the update (return 0 to avoid error spam)
        if (index == UINT32_MAX || index >= l->node_count) {
            return 0;  // Node doesn't exist - skip update silently
        }
        
        // Get actual data size from WAL entry (stored in recovery context)
        // Since the callback doesn't receive data_size, we need to infer it
        // The data is null-terminated by WAL recovery, but for binary data we need the actual size
        // Strategy: Check if first 2 bytes look like a binary length header
        // If so, use binary API; otherwise use text API
        
        // Check if data is binary (has length header in first 2 bytes from WAL)
        // WAL packs binary data as: 2 bytes length + data
        // Compressed data: bit 15 of length header is set
        // Note: strlen() will stop at first null byte, so we can't use it for binary data size
        // Instead, we check if the first 2 bytes form a valid length header (0-510)
        uint16_t length_header;
        memcpy(&length_header, data, 2);
        
        // Check if this looks like a binary length header (0-510, or compressed with bit 15 set)
        bool looks_like_binary = (length_header <= 510) || ((length_header & 0x8000) != 0);
        
        if (looks_like_binary && length_header > 0) {
            // Check if compressed (bit 15 set)
            bool is_compressed = (length_header & 0x8000) != 0;
            
            if (is_compressed) {
                // Compressed data: need to decompress during recovery
                // Extract payload length (bits 0-14)
                size_t payload_len = length_header & 0x7FFF;
                
                // Use binary API for compressed data
                const void* compressed_data = data + 2; // Skip length header
                size_t compressed_data_len = payload_len;
                
                bool wal_was_enabled = (l->wal && l->wal_enabled);
                if (l->wal) {
                    l->wal_enabled = false;
                }
                
                // Store compressed data directly (decompression happens on read)
                int result = lattice_update_node_binary(l, node_id, compressed_data, compressed_data_len);
                
                if (l->wal) {
                    l->wal_enabled = wal_was_enabled;
                }
                return result;
            } else {
                // Uncompressed binary data
                size_t binary_data_len = length_header;
                
                if (binary_data_len > 0 && binary_data_len <= 510) {
                    const void* binary_data = data + 2; // Skip length header
                    
                    bool wal_was_enabled = (l->wal && l->wal_enabled);
                    if (l->wal) {
                        l->wal_enabled = false;
                    }
                    
                    int result = lattice_update_node_binary(l, node_id, binary_data, binary_data_len);
                    
                    if (l->wal) {
                        l->wal_enabled = wal_was_enabled;
                    }
                    return result;
                }
            }
        }
        
        // Text data: use string API
        bool wal_was_enabled = (l->wal && l->wal_enabled);
        if (l->wal) {
            l->wal_enabled = false;
        }
        
        int result = lattice_update_node(l, node_id, data);
        
        if (l->wal) {
            l->wal_enabled = wal_was_enabled;
        }
        
        return result;
    }
    
    int apply_delete_node_cb(void* ctx, uint64_t node_id) {
        persistent_lattice_t* l = (persistent_lattice_t*)ctx;
        if (!l) return -1;
        
        bool wal_was_enabled = (l->wal && l->wal_enabled);
        if (l->wal) {
            l->wal_enabled = false;  // Disable WAL to avoid recursion
        }
        
        int result = lattice_delete_node(l, node_id);
        
        if (l->wal) {
            l->wal_enabled = wal_was_enabled;
        }
        
        return result;
    }
    
    int apply_add_child_cb(void* ctx, uint64_t parent_id, uint64_t child_id) {
        persistent_lattice_t* l = (persistent_lattice_t*)ctx;
        if (!l) return -1;
        
        bool wal_was_enabled = (l->wal && l->wal_enabled);
        if (l->wal) {
            l->wal_enabled = false;
        }
        
        int result = lattice_add_child(l, parent_id, child_id);
        
        if (l->wal) {
            l->wal_enabled = wal_was_enabled;
        }
        
        return result;
    }
    
    // Recover from WAL
    SYNRIX_LOG_INFO("[LATTICE-WAL] INFO Recovering from WAL...\n");
    if (wal_recover(lattice->wal, apply_add_node_cb, apply_update_node_cb,
                   apply_delete_node_cb, apply_add_child_cb, lattice) != 0) {
        printf("[LATTICE-WAL] ERROR Failed to recover from WAL\n");
        return -1;
    }
    
    SYNRIX_LOG_INFO("[LATTICE-WAL] OK Recovery complete\n");
    
    return 0;
}

// ============================================================================
// Isolation Layer Functions for Concurrent Read/Write Safety
// ============================================================================

// Enable isolation for the lattice
int lattice_enable_isolation(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    
    if (lattice->isolation) {
        // Isolation already enabled
        return 0;
    }
    
    // Allocate isolation context
    lattice->isolation = (isolation_context_t*)malloc(sizeof(isolation_context_t));
    if (!lattice->isolation) {
        printf("[LATTICE-ISOLATION] ERROR Failed to allocate isolation context\n");
        return -1;
    }
    
    // Initialize isolation
    if (isolation_init(lattice->isolation) != 0) {
        printf("[LATTICE-ISOLATION] ERROR Failed to initialize isolation\n");
        free(lattice->isolation);
        lattice->isolation = NULL;
        return -1;
    }
    
    lattice->isolation_enabled = true;
    printf("[LATTICE-ISOLATION] OK Isolation enabled for lattice\n");
    
    return 0;
}

// Disable isolation for the lattice
void lattice_disable_isolation(persistent_lattice_t* lattice) {
    if (!lattice || !lattice->isolation) return;
    
    isolation_cleanup(lattice->isolation);
    free(lattice->isolation);
    lattice->isolation = NULL;
    lattice->isolation_enabled = false;
    
    printf("[LATTICE-ISOLATION] OK Isolation disabled\n");
}

// Get node with isolation (snapshot isolation for readers)
// Uses seqlock for lock-free reads with consistency validation
int lattice_get_node_data_with_isolation(persistent_lattice_t* lattice, uint64_t id,
                                         lattice_node_t* out_node, uint64_t* snapshot_version) {
    if (!lattice || !out_node) return -1;
    
    // If isolation not enabled, use regular read
    if (!lattice->isolation || !lattice->isolation_enabled) {
        return lattice_get_node_data(lattice, id, out_node);
    }
    
    uint64_t snapshot = 0;
    
    // Acquire read lock (lock-free, gets snapshot sequence)
    if (isolation_acquire_read_lock(lattice->isolation, &snapshot) != 0) {
        return -1;  // Failed to acquire read lock
    }
    
    // Perform read operation
    int result = lattice_get_node_data(lattice, id, out_node);
    
    // Store snapshot version
    if (snapshot_version) {
        *snapshot_version = snapshot;
    }
    
    // Release read lock (seqlock readers are lock-free, minimal work)
    isolation_release_read_lock(lattice->isolation);
    
    return result;
}

// Add node with isolation (exclusive write lock)
uint64_t lattice_add_node_with_isolation(persistent_lattice_t* lattice, lattice_node_type_t type,
                                        const char* name, const char* data, uint64_t parent_id) {
    if (!lattice) return 0;
    
    // Acquire write lock if isolation enabled
    if (lattice->isolation && lattice->isolation_enabled) {
        if (isolation_acquire_write_lock(lattice->isolation) != 0) {
            return 0;
        }
    }
    
    // Perform write (exclusive - no readers or writers can access)
    uint64_t node_id = 0;
    
    // Use WAL if enabled, otherwise use regular add
    if (lattice->wal && lattice->wal_enabled) {
        node_id = lattice_add_node_with_wal(lattice, type, name, data, parent_id);
    } else {
        node_id = lattice_add_node(lattice, type, name, data, parent_id);
    }
    
    // Release write lock and increment version if isolation enabled
    if (lattice->isolation && lattice->isolation_enabled) {
        isolation_release_write_lock(lattice->isolation);
    }
    
    return node_id;
}

// Update node with isolation (exclusive write lock)
int lattice_update_node_with_isolation(persistent_lattice_t* lattice, uint64_t id, const char* data) {
    if (!lattice) return -1;
    
    // Acquire write lock if isolation enabled
    if (lattice->isolation && lattice->isolation_enabled) {
        if (isolation_acquire_write_lock(lattice->isolation) != 0) {
            return -1;
        }
    }
    
    // Perform write (exclusive)
    int result = 0;
    
    // Use WAL if enabled, otherwise use regular update
    if (lattice->wal && lattice->wal_enabled) {
        result = lattice_update_node_with_wal(lattice, id, data);
    } else {
        result = lattice_update_node(lattice, id, data);
    }
    
    // Release write lock and increment version if isolation enabled
    if (lattice->isolation && lattice->isolation_enabled) {
        isolation_release_write_lock(lattice->isolation);
    }
    
    return result;
}

// Set license key (base64). Verifies signature and sets tier limit. Returns 0 on success, -1 if invalid.
int lattice_set_license_key(persistent_lattice_t* lattice, const char* license_key_base64) {
    if (!lattice || !license_key_base64) return -1;
    uint32_t limit = 0;
    int unlimited = 0;
    if (synrix_license_parse(license_key_base64, &limit, &unlimited) != 0) return -1;
    lattice->free_tier_limit = unlimited ? 0 : limit;
    lattice->evaluation_mode = (unlimited == 0);
    lattice->license_verified_unlimited = (unlimited != 0);
    return 0;
}

// Disable evaluation mode (unlimited nodes). Only succeeds if a valid unlimited (tier-4) key was set.
int lattice_disable_evaluation_mode(persistent_lattice_t* lattice) {
    if (!lattice) return -1;
    if (!lattice->license_verified_unlimited) return -1;  /* only allow when verified unlimited key */
    lattice->evaluation_mode = false;
    lattice->free_tier_limit = 0;
    return 0;
}

// Add child with isolation (exclusive write lock)
int lattice_add_child_with_isolation(persistent_lattice_t* lattice, uint64_t parent_id, uint64_t child_id) {
    if (!lattice) return -1;
    
    // Acquire write lock if isolation enabled
    if (lattice->isolation && lattice->isolation_enabled) {
        if (isolation_acquire_write_lock(lattice->isolation) != 0) {
            return -1;
        }
    }
    
    // Perform write (exclusive)
    int result = 0;
    
    // Use WAL if enabled, otherwise use regular add_child
    if (lattice->wal && lattice->wal_enabled) {
        result = lattice_add_child_with_wal(lattice, parent_id, child_id);
    } else {
        result = lattice_add_child(lattice, parent_id, child_id);
    }
    
    // Release write lock and increment version if isolation enabled
    if (lattice->isolation && lattice->isolation_enabled) {
        isolation_release_write_lock(lattice->isolation);
    }
    
    return result;
}




