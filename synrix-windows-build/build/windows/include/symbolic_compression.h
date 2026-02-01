/*
 * SYMBOLIC COMPRESSION SYSTEM
 * ===========================
 * 
 * Domain-specific entropy coding for 512-byte graph nodes.
 * Uses SIMD-accelerated FSST-style compression with domain-adaptive dictionaries.
 * 
 * Key Features:
 * - 2-5× compression ratio (domain-dependent)
 * - Sub-μs decompression (hot path optimized)
 * - Backward compatible (auto-detect compressed/uncompressed)
 * - WAL-safe (versioned dictionaries)
 * 
 * Architecture:
 * - Dictionary stored in Node 0 (reserved meta-node)
 * - 128 symbols (0x80-0xFF) map to common domain strings
 * - Compression flag in binary mode header (bit 15)
 * - RAM lookup table for O(1) decompression
 */

#ifndef SYMBOLIC_COMPRESSION_H
#define SYMBOLIC_COMPRESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONSTANTS
// ============================================================================

#define SYMBOLIC_COMPRESSION_MAGIC 0x53594E52  // "SYNR" in ASCII
#define SYMBOLIC_COMPRESSION_VERSION 1
#define SYMBOLIC_MAX_SYMBOLS 128              // 0x80-0xFF
#define SYMBOLIC_MAX_STRING_LEN 64            // Max string length per symbol
#define SYMBOLIC_DICTIONARY_NODE_ID 0         // Reserved Node 0 for dictionary

// Compression types
typedef enum {
    SYMBOLIC_COMPRESSION_NONE = 0,      // No compression
    SYMBOLIC_COMPRESSION_GLOBAL_DICT = 1, // Global dictionary (Node 0)
    SYMBOLIC_COMPRESSION_LOCAL_DICT = 2   // Local dictionary (future: per-type)
} symbolic_compression_type_t;

// ============================================================================
// DICTIONARY STRUCTURE
// ============================================================================

// Dictionary entry: maps code (0x80-0xFF) to string
typedef struct {
    uint8_t code;                    // Symbol code (0x80-0xFF)
    uint16_t string_len;             // Length of string (max 64)
    char string[SYMBOLIC_MAX_STRING_LEN]; // Null-terminated string
} symbolic_dict_entry_t;

// Dictionary header (stored in Node 0)
typedef struct {
    uint32_t magic;                  // "SYNR" magic number
    uint32_t version;                // Dictionary version
    uint16_t entry_count;            // Number of entries (max 128)
    uint16_t reserved;               // Reserved for alignment
    uint64_t checksum;               // Checksum of dictionary entries
    // Followed by: symbolic_dict_entry_t entries[entry_count]
} symbolic_dictionary_header_t;

// In-memory dictionary (hot lookup table)
typedef struct {
    bool loaded;                     // Dictionary loaded flag
    uint32_t version;                // Dictionary version
    uint16_t entry_count;            // Number of entries
    
    // Fast lookup: code - 0x80 = array index
    // Array size: 128 entries (0x80-0xFF)
    const char* strings[SYMBOLIC_MAX_SYMBOLS];      // String pointers
    uint16_t lengths[SYMBOLIC_MAX_SYMBOLS];          // String lengths
    
    // Storage for actual strings (allocated once)
    char* string_storage;             // All strings concatenated
    size_t string_storage_size;      // Total size of string storage
} symbolic_dictionary_t;

// ============================================================================
// COMPRESSION CONTEXT
// ============================================================================

// Compression context (per-lattice)
typedef struct {
    symbolic_dictionary_t dictionary; // Loaded dictionary
    bool compression_enabled;         // Compression enabled flag
    bool simd_available;              // SIMD (NEON) available
} symbolic_compression_context_t;

// ============================================================================
// DICTIONARY MANAGEMENT
// ============================================================================

/**
 * Initialize compression context
 * @param ctx Compression context to initialize
 * @return 0 on success, -1 on error
 */
int symbolic_compression_init(symbolic_compression_context_t* ctx);

/**
 * Load dictionary from Node 0
 * @param ctx Compression context
 * @param lattice Lattice instance (for node access)
 * @return 0 on success, -1 on error
 */
int symbolic_dictionary_load(symbolic_compression_context_t* ctx, void* lattice);

/**
 * Save dictionary to Node 0
 * @param ctx Compression context
 * @param lattice Lattice instance (for node access)
 * @return 0 on success, -1 on error
 */
int symbolic_dictionary_save(symbolic_compression_context_t* ctx, void* lattice);

/**
 * Build dictionary from lattice data (frequency analysis)
 * @param ctx Compression context
 * @param lattice Lattice instance
 * @param sample_rate Sampling rate (0.0-1.0, 1.0 = full scan)
 * @return 0 on success, -1 on error
 */
int symbolic_dictionary_build(symbolic_compression_context_t* ctx, 
                               void* lattice, 
                               double sample_rate);

/**
 * Check if node data is compressed
 * @param data Node data (binary mode format)
 * @param data_len Data length (from binary header)
 * @return true if compressed, false otherwise
 */
bool symbolic_is_compressed(const uint8_t* data, size_t data_len);

// ============================================================================
// COMPRESSION/DECOMPRESSION
// ============================================================================

/**
 * Compress node data using dictionary
 * @param ctx Compression context
 * @param input Input data (uncompressed)
 * @param input_len Input data length
 * @param output Output buffer (compressed)
 * @param output_size Output buffer size
 * @param output_len Output data length (written)
 * @return 0 on success, -1 on error
 */
int symbolic_compress(symbolic_compression_context_t* ctx,
                      const uint8_t* input,
                      size_t input_len,
                      uint8_t* output,
                      size_t output_size,
                      size_t* output_len);

/**
 * Decompress node data using dictionary (hot path - optimized)
 * @param ctx Compression context
 * @param input Input data (compressed)
 * @param input_len Input data length
 * @param output Output buffer (uncompressed)
 * @param output_size Output buffer size
 * @param output_len Output data length (written)
 * @return 0 on success, -1 on error
 */
int symbolic_decompress(symbolic_compression_context_t* ctx,
                        const uint8_t* input,
                        size_t input_len,
                        uint8_t* output,
                        size_t output_size,
                        size_t* output_len);

/**
 * Get compression ratio estimate (without actually compressing)
 * @param ctx Compression context
 * @param input Input data
 * @param input_len Input data length
 * @return Estimated compression ratio (1.0 = no compression, 2.0 = 2× compression)
 */
double symbolic_estimate_ratio(symbolic_compression_context_t* ctx,
                               const uint8_t* input,
                               size_t input_len);

// ============================================================================
// UTILITIES
// ============================================================================

/**
 * Cleanup compression context
 * @param ctx Compression context to cleanup
 */
void symbolic_compression_cleanup(symbolic_compression_context_t* ctx);

/**
 * Check if SIMD (NEON) is available
 * @return true if SIMD available, false otherwise
 */
bool symbolic_simd_available(void);

// ============================================================================
// INTEGRATION FUNCTIONS (Binary Mode)
// ============================================================================

/**
 * Pack compressed data into binary mode format
 * @param compressed_data Compressed payload
 * @param compressed_len Length of compressed payload
 * @param compression_type Compression type (1 = Global dict)
 * @param output Output buffer (binary mode format)
 * @param output_size Output buffer size
 * @param output_len Output length (written)
 * @return 0 on success, -1 on error
 */
int symbolic_pack_binary_header(const uint8_t* compressed_data,
                               size_t compressed_len,
                               uint8_t compression_type,
                               uint8_t* output,
                               size_t output_size,
                               size_t* output_len);

/**
 * Unpack compressed data from binary mode format
 * @param input Input buffer (binary mode format)
 * @param input_len Input length
 * @param compression_type Output: compression type
 * @param compressed_data Output: compressed payload
 * @param compressed_len Output: compressed payload length
 * @return 0 on success, -1 on error
 */
int symbolic_unpack_binary_header(const uint8_t* input,
                                 size_t input_len,
                                 uint8_t* compression_type,
                                 const uint8_t** compressed_data,
                                 size_t* compressed_len);

/**
 * Compress and pack node data for binary mode storage
 */
int symbolic_compress_for_binary(symbolic_compression_context_t* ctx,
                                const uint8_t* input,
                                size_t input_len,
                                uint8_t* output,
                                size_t output_size,
                                size_t* output_len);

/**
 * Decompress node data from binary mode format
 */
int symbolic_decompress_from_binary(symbolic_compression_context_t* ctx,
                                   const uint8_t* input,
                                   size_t input_len,
                                   uint8_t* output,
                                   size_t output_size,
                                   size_t* output_len);

// Internal functions (exposed for testing)
size_t dictionary_serialize(const symbolic_dictionary_t* dict,
                            uint8_t* output,
                            size_t output_size);
int dictionary_deserialize(symbolic_dictionary_t* dict,
                           const uint8_t* input,
                           size_t input_size);

#ifdef __cplusplus
}
#endif

#endif // SYMBOLIC_COMPRESSION_H

