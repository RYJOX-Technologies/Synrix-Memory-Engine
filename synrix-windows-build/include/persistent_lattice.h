#ifndef PERSISTENT_LATTICE_H
#define PERSISTENT_LATTICE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>  // for ssize_t

// Forward declarations
struct wal_context;
typedef struct wal_context wal_context_t;

// Forward declaration for isolation context (defined in isolation.h)
struct isolation_context;
typedef struct isolation_context isolation_context_t;

#ifdef __cplusplus
extern "C" {
#endif

// Include dynamic prefix index (needed for structure definition)
#include "dynamic_prefix_index.h"

// Lattice error codes
typedef enum {
    LATTICE_ERROR_NONE = 0,
    LATTICE_ERROR_NULL_POINTER = -1,
    LATTICE_ERROR_INVALID_PATH = -2,
    LATTICE_ERROR_MEMORY_ALLOC = -3,
    LATTICE_ERROR_FILE_IO = -4,
    LATTICE_ERROR_INVALID_NODE = -5,
    LATTICE_ERROR_FREE_TIER_LIMIT = -100,  // Free tier limit reached (25k nodes)
    LATTICE_ERROR_LICENSE_EXPIRED = -101,  // License expired (for future use)
    LATTICE_ERROR_LICENSE_INVALID = -102   // Invalid license (for future use)
} lattice_error_code_t;

// Lattice node types
typedef enum {
    LATTICE_NODE_PRIMITIVE = 1,
    LATTICE_NODE_KERNEL = 2,
    LATTICE_NODE_PATTERN = 3,
    LATTICE_NODE_PERFORMANCE = 4,
    LATTICE_NODE_LEARNING = 5,
    LATTICE_NODE_ANTI_PATTERN = 6,
    LATTICE_NODE_SIDECAR_MAPPING = 7,    // Intent->capability mappings with confidence
    LATTICE_NODE_SIDECAR_EVENT = 8,      // System events for learning
    LATTICE_NODE_SIDECAR_SUGGESTION = 9, // Approved suggestions
    LATTICE_NODE_SIDECAR_STATE = 10,     // Overall sidecar state
    LATTICE_NODE_CPT_ELEMENT = 100,      // CPT element node
    LATTICE_NODE_CPT_ADVANCED_PATTERN = 101, // CPT advanced pattern node
    LATTICE_NODE_CPT_METADATA = 106,     // CPT metadata node
    LATTICE_NODE_CHUNK_HEADER = 200,     // Chunked data header node
    LATTICE_NODE_CHUNK_DATA = 201        // Chunked data chunk node
} lattice_node_type_t;

// Performance metrics for learning
typedef struct {
    uint64_t cycles;
    uint64_t instructions;
    double execution_time_ns;
    double instructions_per_cycle;
    double throughput_mb_s;
    double efficiency_score;
    uint32_t complexity_level;
    char kernel_type[32];
    uint64_t timestamp;
} lattice_performance_t;

// Learning data for pattern evolution
typedef struct {
    char pattern_sequence[256];
    uint32_t frequency;
    double success_rate;
    double performance_gain;
    uint64_t last_used;
    uint32_t evolution_generation;
} lattice_learning_t;

// Sidecar mapping data - stores intent->capability mappings with confidence
typedef struct {
    char intent_name[64];
    char capability_name[64];
    float confidence;              // 0.0 - 1.0
    uint32_t successes;           // Success counter
    uint32_t failures;            // Failure counter
    uint64_t last_used;           // Last successful use timestamp
    uint64_t created;             // Creation timestamp
    uint8_t trust_zone;           // 0=DANGEROUS, 1=CAUTIOUS, 2=TRUSTED, 3=SAFE
    bool is_active;               // Can this mapping be used?
    float decay_rate;             // Confidence decay rate per day
} lattice_sidecar_mapping_t;

// Sidecar event data - stores system events for learning
typedef struct {
    char event_type[32];          // "primitive_discovered", "kernel_generated", etc.
    char context[256];            // JSON-like context data
    char outcome[16];             // "success", "failure", "partial"
    float confidence;             // System confidence in this event
    uint64_t timestamp;           // Event timestamp
    uint32_t system_phase;        // Which layer generated this
    uint32_t event_id;            // Unique event identifier
} lattice_sidecar_event_t;

// Sidecar suggestion data - stores approved suggestions
typedef struct {
    char intent_name[64];
    char capability_name[64];
    char reasoning[256];          // Why this suggestion was made
    float confidence;             // Confidence in this suggestion
    uint64_t created;             // Creation timestamp
    bool is_approved;             // Has this been approved?
    bool is_implemented;          // Has this been implemented?
    uint32_t suggestion_id;       // Unique suggestion identifier
} lattice_sidecar_suggestion_t;

// Sidecar state data - overall sidecar state
typedef struct {
    uint8_t current_mode;         // OBSERVE, LEARN, SUGGEST, INTEGRATE
    uint64_t last_learning_cycle; // Last learning cycle timestamp
    uint32_t total_observations;  // Total observations made
    uint32_t successful_predictions; // Successful predictions
    uint32_t failed_predictions;  // Failed predictions
    float learning_threshold;     // Minimum confidence to learn from
    float suggestion_threshold;   // Minimum confidence to suggest
    uint32_t max_event_age;       // Max event age in seconds
    uint32_t max_suggestions_per_cycle; // Rate limiting
    uint64_t state_version;       // State version for compatibility
} lattice_sidecar_state_t;

// Reserved Expansion Header - 128 bytes for OS-level features
// Enables quantum-resistant hashing, ownership labels, and temporal vectors
// Total node size: 1216 bytes (19 * 64, perfect alignment)
// Page geometry: 3 nodes per 4KB page, ~448 bytes wasted (10.9% fragmentation)
typedef struct {
    uint8_t quantum_hash[64];     // Quantum-resistant hash (SHA-512 or similar)
    uint32_t owner_uid;           // User ID (OS-level ownership)
    uint32_t owner_gid;           // Group ID (OS-level ownership)
    uint16_t permission_flags;     // Permission bits (rwx, etc.)
    uint16_t reserved_flags;      // Reserved for future OS flags
    double relevance_score;        // Temporal relevance/decay score
    double decay_rate;             // Decay rate per time unit
    uint64_t last_access_time;     // Last access timestamp
    uint64_t creation_time;        // Creation timestamp (may differ from timestamp)
    uint32_t access_count;         // Access frequency counter
    uint32_t reserved[4];         // Reserved for future expansion (16 bytes)
} __attribute__((packed)) lattice_expansion_header_t;

// Lattice node structure
// 
// DUAL-MODE DATA STORAGE:
// - Text mode: data[] is null-terminated string (max 511 bytes)
// - Binary mode: First 2 bytes = uint16_t length, data starts at offset 2 (max 510 bytes)
// 
// Use lattice_is_node_binary() to check mode before accessing data.
// Use lattice_get_node_data_binary() for safe binary retrieval.
// Use lattice_validate_data_access() to warn on API misuse.
//
typedef struct lattice_node {
    uint64_t id;  // 64-bit ID: (device_id << 32) | local_id for distributed systems
    lattice_node_type_t type;
    char name[64];
    char data[512];  // Dual-mode: text (null-terminated) or binary (2-byte length header)
    uint64_t parent_id;  // 64-bit for consistency
    uint32_t child_count;
    uint64_t* children;  // 64-bit child IDs
    double confidence;
    uint64_t timestamp;
    union {
        lattice_performance_t performance;
        lattice_learning_t learning;
        lattice_sidecar_mapping_t sidecar_mapping;
        lattice_sidecar_event_t sidecar_event;
        lattice_sidecar_suggestion_t sidecar_suggestion;
        lattice_sidecar_state_t sidecar_state;
    } payload;
    // Reserved Expansion Header - 128 bytes for OS-level features
    // Quantum-resistant hashing, ownership labels, temporal vectors
    lattice_expansion_header_t expansion;
} __attribute__((aligned(64))) lattice_node_t;  // 1216 bytes total (19 * 64), cache line aligned to prevent false sharing

// Lattice storage system - STREAMING ACCESS FOR MILLIONS OF NODES
typedef struct {
    // RAM cache - only essential nodes
    lattice_node_t* nodes;
    uint32_t node_count;        // Nodes currently in RAM
    uint32_t max_nodes;         // Max nodes in RAM (100k)
    
    // Storage metadata
    uint32_t total_nodes;       // Total nodes in storage
    uint64_t next_id;           // Next available local ID (use atomic operations for thread-safety)
    uint32_t device_id;         // Device identifier for distributed systems (0 = single device)
    char storage_path[256];
    bool dirty;
    
    // Thread-safety: Atomic ID reservation for multi-threaded writes
    // When multi-threading is enabled, use lattice_reserve_id_block() instead of direct next_id++
    bool thread_safe_mode;       // Enable atomic ID reservation
    
    // Async prefetching: Predict and preload likely next nodes
    bool prefetch_enabled;       // Enable automatic prefetching on node access
    
    // Disk mode: File-backed memory with MAP_SHARED (enables kernel-managed dirty page flushing)
    bool disk_mode;              // If true, nodes array is mmap'd with MAP_SHARED (no growth allowed)
    uint32_t total_file_nodes;   // Total nodes pre-allocated in file (when disk_mode=true)
    
    // Streaming access
    int storage_fd;             // File descriptor for direct access
    void* mmap_ptr;            // Memory-mapped file pointer
    size_t mmap_size;          // Size of memory-mapped region
    
    // Intelligent caching - ALL DYNAMIC (grows with max_nodes)
    uint64_t* node_id_map;     // Maps array index to node ID (dynamic, 64-bit)
    uint32_t* id_to_index_map; // Maps local ID (lower 32 bits) to array index for O(1) lookup (dynamic)
    uint32_t* access_count;    // Access frequency for LRU (dynamic, grows with max_nodes)
    uint32_t* last_access;     // Last access time for LRU (dynamic, grows with max_nodes)
    
    // Semantic prefix index - uses existing naming conventions for fast queries
    struct {
        uint32_t isa_count;           // Count of ISA_* nodes
        uint32_t material_count;      // Count of MATERIAL_* nodes
        uint32_t learning_count;       // Count of LEARNING_* nodes
        uint32_t performance_count;    // Count of PERFORMANCE_* nodes
        uint32_t qdrant_collection_count; // Count of QDRANT_COLLECTION_* nodes
        uint64_t* isa_ids;            // Array of ISA node IDs (O(1) lookup after prefix detection, 64-bit)
        uint64_t* material_ids;       // Array of MATERIAL node IDs (64-bit)
        uint64_t* learning_ids;       // Array of LEARNING node IDs (64-bit)
        uint64_t* performance_ids;    // Array of PERFORMANCE node IDs (64-bit)
        uint64_t* qdrant_collection_ids; // Array of QDRANT_COLLECTION node IDs (64-bit)
        bool built;                   // Whether index has been built
        
        // Dynamic prefix index (parallel system for testing)
        dynamic_prefix_index_t dynamic_index;  // Auto-discovers all prefixes
        bool use_dynamic_index;                // Feature flag (default: false)
    } prefix_index;
    
    // WAL (Write-Ahead Logging) for ACID durability
    wal_context_t* wal;               // WAL context (NULL if WAL disabled)
    bool wal_enabled;                 // WAL enabled flag
    
    // Isolation layer for concurrent read/write safety
    isolation_context_t* isolation;   // Isolation context (NULL if isolation disabled)
    bool isolation_enabled;           // Isolation enabled flag
    
    // Production persistence configuration
    struct {
        bool auto_save_enabled;       // Enable automatic periodic saves
        uint32_t auto_save_interval_nodes;  // Save every N nodes (0 = disabled)
        uint32_t auto_save_interval_seconds; // Save every T seconds (0 = disabled)
        bool save_on_memory_pressure; // Save snapshot when RAM fills
        uint32_t nodes_since_last_save; // Counter for node-based auto-save
        uint64_t last_save_timestamp;  // Timestamp of last save (for time-based auto-save)
    } persistence;
    
    // Error tracking
    lattice_error_code_t last_error;  // Last error code
    bool evaluation_mode;              // True if running in evaluation/free tier mode
    uint32_t free_tier_limit;         // Free tier node limit (default: 25000)
} persistent_lattice_t;

// Function declarations
// Initialize lattice with configurable RAM cache size
// max_nodes: Maximum nodes to keep in RAM cache (0 = use default 10k)
// device_id: Device identifier for distributed systems (0 = single device, auto-assigns unique ID)
int lattice_init(persistent_lattice_t* lattice, const char* storage_path, uint32_t max_nodes, uint32_t device_id);

// Hardware ID generation (for license tracking)
// Generates a stable, unique hardware identifier based on system characteristics
// Returns: 0 on success, -1 on error
// hwid_out: Buffer to store hardware ID (must be at least 65 bytes for null terminator)
int lattice_get_hardware_id(char* hwid_out, size_t hwid_size);

// Get last error code (for error handling)
lattice_error_code_t lattice_get_last_error(persistent_lattice_t* lattice);

// Initialize lattice in disk mode (file-backed memory with MAP_SHARED)
// max_nodes: Maximum nodes to keep in RAM cache (for metadata arrays)
// total_file_nodes: Total nodes to pre-allocate in file (file-backed memory)
// device_id: Device identifier for distributed systems (0 = single device)
// When total_file_nodes > max_nodes, nodes array is mmap'd with MAP_SHARED
// This enables kernel-managed dirty page flushing (Leaky Bucket strategy)
int lattice_init_disk_mode(persistent_lattice_t* lattice, const char* storage_path, 
                           uint32_t max_nodes, uint32_t total_file_nodes, uint32_t device_id);
void lattice_cleanup(persistent_lattice_t* lattice);
int lattice_save(persistent_lattice_t* lattice);
int lattice_load(persistent_lattice_t* lattice);

// Node management
uint64_t lattice_add_node(persistent_lattice_t* lattice, lattice_node_type_t type, 
                         const char* name, const char* data, uint64_t parent_id);
uint64_t lattice_add_node_deduplicated(persistent_lattice_t* lattice, lattice_node_type_t type, 
                                      const char* name, const char* data, uint64_t parent_id);

// Thread-safe ID reservation (for multi-threaded writes)
// Reserves a block of local IDs atomically, allowing parallel node creation
// Returns the base local ID (will be combined with device_id to form full 64-bit ID)
uint32_t lattice_reserve_id_block(persistent_lattice_t* lattice, uint32_t count);

// Thread-safe node addition with pre-reserved local ID
uint64_t lattice_add_node_with_id(persistent_lattice_t* lattice, uint32_t reserved_local_id,
                                  lattice_node_type_t type, const char* name, 
                                  const char* data, uint64_t parent_id);

// Binary-safe node addition (handles arbitrary binary data including null bytes)
// data: Pointer to binary data (can contain null bytes)
// data_len: Length of data in bytes (max 512)
uint64_t lattice_add_node_binary(persistent_lattice_t* lattice, lattice_node_type_t type,
                                 const char* name, const void* data, size_t data_len, uint64_t parent_id);

// Add node with compressed data (preserves compression header)
// compressed_data should be in binary mode format: [length:2 (bit 15 = compressed)][compression_type:1][payload...]
uint64_t lattice_add_node_compressed(persistent_lattice_t* lattice, lattice_node_type_t type,
                                     const char* name, const void* compressed_data, size_t compressed_data_len,
                                     uint64_t parent_id);

// ⚠️ DEPRECATED: lattice_get_node() returns pointers that become invalid if lattice_add_node() triggers realloc
// 
// EXTERNAL CALLERS: Use lattice_get_node_data() or lattice_get_node_copy() instead for safe access.
//                   Holding pointers returned by this function across any lattice modification operations
//                   (e.g., lattice_add_node) will cause undefined behavior (segfaults).
//
// INTERNAL USE: This function is still used internally for immediate, single-operation access patterns
//               where the pointer is consumed before any realloc can occur. These uses are safe.
//
// This function will be removed in a future version.
#ifdef __GNUC__
__attribute__((deprecated("Use lattice_get_node_data() or lattice_get_node_copy() instead. Pointers become invalid after realloc.")))
#elif defined(_MSC_VER)
__declspec(deprecated("Use lattice_get_node_data() or lattice_get_node_copy() instead. Pointers become invalid after realloc."))
#endif
lattice_node_t* lattice_get_node(persistent_lattice_t* lattice, uint64_t id);

// SAFE API: Copy node data to user-provided buffer (pointer-safe)
int lattice_get_node_data(persistent_lattice_t* lattice, uint64_t id, lattice_node_t* out_node);

// SAFE API: Allocate and copy node data (caller must free with lattice_free_node_copy())
lattice_node_t* lattice_get_node_copy(persistent_lattice_t* lattice, uint64_t id);

// Free a node copy allocated by lattice_get_node_copy()
void lattice_free_node_copy(lattice_node_t* node);

// Binary-safe data retrieval (returns actual data length for binary nodes)
// Returns: 0 on success, -1 on error
// data_len: Output parameter for actual data length (set to 0 if text/null-terminated)
// is_binary: Output parameter indicating if data is binary (true) or text (false)
int lattice_get_node_data_binary(persistent_lattice_t* lattice, uint64_t id, 
                                 void* out_data, size_t* out_data_len, bool* is_binary);

// Check if a node contains binary data (safe to call on any node)
// Returns: true if binary, false if text/null-terminated
bool lattice_is_node_binary(persistent_lattice_t* lattice, uint64_t id);

// Validate node data access (warns if using wrong API for binary/text)
// Returns: 0 if safe, -1 if warning issued (but still allows operation)
int lattice_validate_data_access(persistent_lattice_t* lattice, uint64_t id, bool expecting_text);

// Chunked data storage (for data exceeding 510 bytes)
// Splits large data across multiple nodes with triple redundancy:
// 1. Parent metadata array (fast, if parent in cache)
// 2. Name-based discovery (resilient, always works)
// 3. Chunk data header (self-describing, validation)
//
// Parent node: name="CHUNKED:original_name", data=metadata
// Child nodes: name="CHUNK:parent_id:index:total", data=chunk payload
//
// Returns: parent node ID (chunk header), or 0 on error
uint64_t lattice_add_node_chunked(persistent_lattice_t* lattice,
                                  lattice_node_type_t type,
                                  const char* name,
                                  const void* data,
                                  size_t data_len,
                                  uint64_t parent_id);

// Reassemble chunked data
// Returns: 0 on success, -1 on error
// out_data: allocated buffer (caller must free with free())
// out_data_len: actual data length
int lattice_get_node_chunked(persistent_lattice_t* lattice,
                             uint64_t parent_id,
                             void** out_data,
                             size_t* out_data_len);

// Check if node is chunked data header
// Returns: true if node is a chunked data parent, false otherwise
bool lattice_is_node_chunked(persistent_lattice_t* lattice, uint64_t id);

// Python-friendly wrappers for chunked data access (avoid void** issues)
// Get size of chunked data
// Returns: size on success, -1 on error
ssize_t lattice_get_node_chunked_size(persistent_lattice_t* lattice, uint64_t parent_id);

// Copy chunked data to pre-allocated buffer
// Returns: actual size written on success, -1 on error, -2 if buffer too small
ssize_t lattice_get_node_chunked_to_buffer(persistent_lattice_t* lattice,
                                           uint64_t parent_id,
                                           void* buffer,
                                           size_t buffer_size);

// Hash embedding versioning support
// Store embedding metadata (version, dimensions) in node's data[512] field
typedef struct {
    uint32_t hash_version;        // Hash function version (incremented on hash changes)
    uint32_t embedding_dim;       // Embedding dimensions (e.g., 128, 256)
    uint64_t created_timestamp;   // When embedding was created
    char hash_function_name[32];  // Name of hash function used (e.g., "simple_hash_v1")
} embedding_metadata_t;

// Store embedding metadata in node (uses binary API, stores in data[512] field)
// Returns: 0 on success, -1 on error
int lattice_store_embedding_metadata(persistent_lattice_t* lattice, uint64_t node_id,
                                     const embedding_metadata_t* metadata);

// Retrieve embedding metadata from node
// Returns: 0 on success, -1 if node doesn't have embedding metadata
int lattice_get_embedding_metadata(persistent_lattice_t* lattice, uint64_t node_id,
                                   embedding_metadata_t* metadata);

// Check if node has embedding metadata
// Returns: true if node has embedding metadata, false otherwise
bool lattice_has_embedding_metadata(persistent_lattice_t* lattice, uint64_t node_id);

// Edge metadata support
// Store relationship metadata (type, weight) in child node's data[512] field
typedef struct {
    char relationship_type[32];  // Relationship type (e.g., "IS_A", "HAS_PROPERTY", "RELATED_TO")
    double weight;                // Edge weight (similarity score, confidence, etc.)
    uint64_t timestamp;           // When relationship was created
    char description[128];        // Optional description of the relationship
} edge_metadata_t;

// Add child with edge metadata (stores metadata in child node's data[512] field)
// Returns: 0 on success, -1 on error
int lattice_add_child_with_metadata(persistent_lattice_t* lattice, 
                                    uint64_t parent_id, 
                                    uint64_t child_id,
                                    const edge_metadata_t* metadata);

// Get edge metadata from child node
// Returns: 0 on success, -1 if child doesn't have edge metadata
int lattice_get_edge_metadata(persistent_lattice_t* lattice, 
                              uint64_t parent_id,
                              uint64_t child_id,
                              edge_metadata_t* metadata);

// Check if edge has metadata
// Returns: true if edge has metadata, false otherwise
bool lattice_edge_has_metadata(persistent_lattice_t* lattice, 
                               uint64_t parent_id,
                               uint64_t child_id);

int lattice_update_node(persistent_lattice_t* lattice, uint64_t id, const char* data);

// Binary-safe node update (handles arbitrary binary data including null bytes)
// data: Pointer to binary data (can contain null bytes)
// data_len: Length of data in bytes (max 512)
int lattice_update_node_binary(persistent_lattice_t* lattice, uint64_t id, const void* data, size_t data_len);

int lattice_add_child(persistent_lattice_t* lattice, uint64_t parent_id, uint64_t child_id);

// Delete node from lattice
int lattice_delete_node(persistent_lattice_t* lattice, uint64_t node_id);

// Semantic prefix indexing - uses existing naming conventions for O(k) queries
void lattice_build_prefix_index(persistent_lattice_t* lattice);
void lattice_prefix_index_add_node(persistent_lattice_t* lattice, uint64_t node_id, const char* node_name);

// Dynamic prefix index validation and benchmarking (for parallel testing)
// Returns true if both indexes match, false if discrepancies found
// Reports discrepancies to stdout
bool lattice_validate_prefix_indexes(persistent_lattice_t* lattice);

// Benchmark both prefix index systems and report performance
// Returns 0 on success, -1 on error
int lattice_benchmark_prefix_indexes(persistent_lattice_t* lattice);

// Performance learning
int lattice_store_performance(persistent_lattice_t* lattice, const char* kernel_type,
                            uint32_t complexity, const lattice_performance_t* perf);
int lattice_get_best_performance(persistent_lattice_t* lattice, const char* kernel_type,
                                uint32_t complexity, lattice_performance_t* perf);

// Pattern learning
int lattice_store_pattern(persistent_lattice_t* lattice, const char* pattern,
                         double success_rate, double performance_gain);
int lattice_get_evolved_patterns(persistent_lattice_t* lattice, const char* base_pattern,
                                lattice_learning_t* patterns, uint32_t max_patterns);

// Update success_rate for a LEARNING node (Phase 3: Execution feedback)
// Uses exponential moving average: new_rate = (old_rate * 0.9) + (new_result * 0.1)
// Performance: <1μs (direct memory access)
int lattice_update_success_rate(persistent_lattice_t* lattice, uint64_t node_id, bool execution_success);

// Query functions
uint32_t lattice_find_nodes_by_type(persistent_lattice_t* lattice, lattice_node_type_t type,
                                   uint64_t* node_ids, uint32_t max_ids);
uint32_t lattice_find_nodes_by_name(persistent_lattice_t* lattice, const char* name,
                                   uint64_t* node_ids, uint32_t max_ids);

// Query with filtering (zero overhead - filtering in O(k) loop)
// min_confidence: 0.0 = no filter, > 0.0 = minimum confidence threshold
// min_timestamp: 0 = no filter, > 0 = minimum timestamp (microseconds)
// max_timestamp: 0 = no filter, > 0 = maximum timestamp (microseconds)
uint32_t lattice_find_nodes_by_name_filtered(persistent_lattice_t* lattice, 
                                             const char* name,
                                             uint64_t* node_ids, 
                                             uint32_t max_ids,
                                             double min_confidence,
                                             uint64_t min_timestamp,
                                             uint64_t max_timestamp);

// Learning and evolution
int lattice_evolve_patterns(persistent_lattice_t* lattice);
int lattice_update_confidence(persistent_lattice_t* lattice, uint64_t node_id, double confidence);

// Sidecar persistence functions
int lattice_save_sidecar_state(persistent_lattice_t* lattice, const char* sidecar_data);
int lattice_load_sidecar_state(persistent_lattice_t* lattice, char** sidecar_data);
int lattice_store_sidecar_mapping(persistent_lattice_t* lattice, const lattice_sidecar_mapping_t* mapping);
int lattice_load_sidecar_mappings(persistent_lattice_t* lattice, lattice_sidecar_mapping_t* mappings, uint32_t max_mappings, uint32_t* count);
int lattice_store_sidecar_event(persistent_lattice_t* lattice, const lattice_sidecar_event_t* event);
int lattice_load_recent_events(persistent_lattice_t* lattice, lattice_sidecar_event_t* events, uint32_t max_events, uint32_t* count);
int lattice_store_sidecar_suggestion(persistent_lattice_t* lattice, const lattice_sidecar_suggestion_t* suggestion);
int lattice_load_approved_suggestions(persistent_lattice_t* lattice, lattice_sidecar_suggestion_t* suggestions, uint32_t max_suggestions, uint32_t* count);

// STREAMING ACCESS FUNCTIONS - Handle millions of nodes efficiently
lattice_node_t* lattice_get_node_streaming(persistent_lattice_t* lattice, uint64_t node_id);
int lattice_add_node_streaming(persistent_lattice_t* lattice, lattice_node_type_t type, 
                              const char* name, const char* data, uint64_t parent_id);
uint32_t lattice_find_nodes_by_type_streaming(persistent_lattice_t* lattice, lattice_node_type_t type,
                                            uint64_t* node_ids, uint32_t max_ids);
int lattice_evict_oldest_nodes(persistent_lattice_t* lattice, uint32_t count);

// Async prefetcher: Predict and preload likely next nodes (non-blocking, uses madvise)
// Strategies:
//   1. Prefetch children (high probability)
//   2. Prefetch parent (often accessed after children)
//   3. Prefetch high-frequency nodes (based on access_count)
//   4. Prefetch semantically related nodes (same prefix, e.g., ISA_*)
int lattice_prefetch_related_nodes(persistent_lattice_t* lattice, uint64_t node_id);

// Prefetch specific node IDs (for explicit prefetching)
int lattice_prefetch_nodes(persistent_lattice_t* lattice, uint64_t* node_ids, uint32_t count);

// Enable/disable automatic prefetching on node access
void lattice_set_prefetch_enabled(persistent_lattice_t* lattice, bool enabled);

void lattice_print_streaming_stats(persistent_lattice_t* lattice);

// CORRUPTION DETECTION AND SURGICAL REPAIR FUNCTIONS
int lattice_scan_and_repair_corruption(persistent_lattice_t* lattice);

// Compact lattice file: Remove uninitialized/corrupted nodes and rebuild file
// Returns 0 on success, -1 on error
int lattice_compact_file(persistent_lattice_t* lattice);

// PRODUCTION PERSISTENCE CONFIGURATION
// Configure auto-save behavior (production requirement)
// auto_save_enabled: Enable/disable automatic periodic saves
// interval_nodes: Save every N nodes (0 = disabled)
// interval_seconds: Save every T seconds (0 = disabled)
// save_on_pressure: Save snapshot when RAM fills (90% capacity)
void lattice_configure_persistence(persistent_lattice_t* lattice,
                                   bool auto_save_enabled,
                                   uint32_t interval_nodes,
                                   uint32_t interval_seconds,
                                   bool save_on_pressure);

// WAL (Write-Ahead Logging) functions for ACID durability
// Enable WAL for the lattice
int lattice_enable_wal(persistent_lattice_t* lattice);

// Disable WAL for the lattice
void lattice_disable_wal(persistent_lattice_t* lattice);

// Flush WAL buffer to disk (manual flush for durability)
int lattice_wal_flush(persistent_lattice_t* lattice);

// Add node with WAL (guarantees durability)
uint64_t lattice_add_node_with_wal(persistent_lattice_t* lattice, lattice_node_type_t type,
                                   const char* name, const char* data, uint64_t parent_id);

// Update node with WAL
int lattice_update_node_with_wal(persistent_lattice_t* lattice, uint64_t id, const char* data);

// Add child with WAL
int lattice_add_child_with_wal(persistent_lattice_t* lattice, uint64_t parent_id, uint64_t child_id);

// Checkpoint WAL (mark entries as applied)
int lattice_wal_checkpoint(persistent_lattice_t* lattice);

// Recover from WAL on startup
int lattice_recover_from_wal(persistent_lattice_t* lattice);

// Isolation layer functions for concurrent read/write safety
// Enable isolation for the lattice
int lattice_enable_isolation(persistent_lattice_t* lattice);

// Disable isolation for the lattice
void lattice_disable_isolation(persistent_lattice_t* lattice);

// Get node with isolation (snapshot isolation for readers)
int lattice_get_node_data_with_isolation(persistent_lattice_t* lattice, uint64_t id,
                                        lattice_node_t* out_node, uint64_t* snapshot_version);

// Add node with isolation (exclusive write lock)
uint64_t lattice_add_node_with_isolation(persistent_lattice_t* lattice, lattice_node_type_t type,
                                        const char* name, const char* data, uint64_t parent_id);

// Update node with isolation (exclusive write lock)
int lattice_update_node_with_isolation(persistent_lattice_t* lattice, uint64_t id, const char* data);

// Add child with isolation (exclusive write lock)
int lattice_add_child_with_isolation(persistent_lattice_t* lattice, uint64_t parent_id, uint64_t child_id);

// ============================================================================
// DISK-RAM ARCHITECTURE: On-Demand Loading and Streaming
// ============================================================================

// Load nodes matching prefix from disk into RAM cache (selective loading)
// This allows agents to load only the datasets they need
// Returns: number of nodes loaded, or -1 on error
int lattice_load_prefix(persistent_lattice_t* lattice, const char* prefix, uint32_t max_nodes);

// Stream query results from disk without loading all into RAM
// callback: called for each matching node (return 0 to continue, non-zero to stop)
// user_data: passed to callback
// Returns: number of nodes processed, or -1 on error
typedef int (*lattice_stream_callback)(lattice_node_t* node, void* user_data);
int lattice_query_stream(persistent_lattice_t* lattice, const char* prefix,
                         lattice_stream_callback callback, void* user_data);

// Unload prefix from RAM cache (free up space for other datasets)
// Returns: number of nodes unloaded, or -1 on error
int lattice_unload_prefix(persistent_lattice_t* lattice, const char* prefix);

// Get cache statistics (for monitoring and optimization)
void lattice_get_cache_stats(persistent_lattice_t* lattice,
                            uint32_t* nodes_in_cache,
                            uint32_t* nodes_on_disk,
                            uint32_t* cache_hits,
                            uint32_t* cache_misses);

// Disable evaluation mode (unlimited nodes for developer/creator use)
int lattice_disable_evaluation_mode(persistent_lattice_t* lattice);

#ifdef __cplusplus
}
#endif

#endif // PERSISTENT_LATTICE_H
