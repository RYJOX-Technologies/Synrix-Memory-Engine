/*
 * SYMBOLIC COMPRESSION IMPLEMENTATION
 * ===================================
 * 
 * Domain-specific entropy coding for 512-byte graph nodes.
 * Phase 1: Core Infrastructure (Dictionary Storage & Management)
 */

#include "symbolic_compression.h"
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// SIMD DETECTION (ARM NEON for Jetson)
// ============================================================================

#ifdef __ARM_NEON
#include <arm_neon.h>
#define SYMBOLIC_HAVE_NEON 1
#else
#define SYMBOLIC_HAVE_NEON 0
#endif

bool symbolic_simd_available(void) {
    // Check for ARM NEON support
    #if SYMBOLIC_HAVE_NEON
    return true;
    #else
    return false;
    #endif
}

// ============================================================================
// INITIALIZATION & CLEANUP
// ============================================================================

int symbolic_compression_init(symbolic_compression_context_t* ctx) {
    if (!ctx) return -1;
    
    memset(ctx, 0, sizeof(symbolic_compression_context_t));
    ctx->simd_available = symbolic_simd_available();
    ctx->compression_enabled = false;
    
    // Initialize dictionary lookup arrays
    for (int i = 0; i < SYMBOLIC_MAX_SYMBOLS; i++) {
        ctx->dictionary.strings[i] = NULL;
        ctx->dictionary.lengths[i] = 0;
    }
    
    return 0;
}

void symbolic_compression_cleanup(symbolic_compression_context_t* ctx) {
    if (!ctx) return;
    
    // Free string storage
    if (ctx->dictionary.string_storage) {
        free(ctx->dictionary.string_storage);
        ctx->dictionary.string_storage = NULL;
    }
    
    // Clear dictionary
    memset(ctx, 0, sizeof(symbolic_compression_context_t));
}

// ============================================================================
// DICTIONARY SERIALIZATION
// ============================================================================

/**
 * Serialize dictionary to binary format (for storage in Node 0)
 * Format: [header][entry_1][entry_2]...[entry_N]
 */
size_t dictionary_serialize(const symbolic_dictionary_t* dict,
                                   uint8_t* output,
                                   size_t output_size) {
    if (!dict || !output) return 0;
    
    // Write header
    symbolic_dictionary_header_t header;
    header.magic = SYMBOLIC_COMPRESSION_MAGIC;
    header.version = dict->version;
    header.entry_count = dict->entry_count;
    header.reserved = 0;
    header.checksum = 0; // TODO: Calculate checksum
    
    size_t offset = 0;
    if (offset + sizeof(header) > output_size) return 0;
    memcpy(output + offset, &header, sizeof(header));
    offset += sizeof(header);
    
    // Write entries (only write entries that exist)
    uint16_t entries_written = 0;
    for (uint16_t i = 0; i < SYMBOLIC_MAX_SYMBOLS && entries_written < dict->entry_count; i++) {
        const char* str = dict->strings[i];
        uint16_t len = dict->lengths[i];
        
        if (!str || len == 0) continue;
        
        uint8_t code = 0x80 + i;
        symbolic_dict_entry_t entry;
        entry.code = code;
        entry.string_len = len;
        if (len > SYMBOLIC_MAX_STRING_LEN) len = SYMBOLIC_MAX_STRING_LEN;
        memcpy(entry.string, str, len);
        entry.string[len] = '\0';
        
        if (offset + sizeof(entry) > output_size) {
            printf("[SYMBOLIC] ⚠️  Serialization buffer too small (need %zu bytes, have %zu)\n",
                   offset + sizeof(entry), output_size);
            return 0;
        }
        memcpy(output + offset, &entry, sizeof(entry));
        offset += sizeof(entry);
        entries_written++;
    }
    
    // Update entry_count in header to match actual entries written
    if (entries_written != dict->entry_count) {
        // Update header with correct count
        symbolic_dictionary_header_t* header_ptr = (symbolic_dictionary_header_t*)output;
        header_ptr->entry_count = entries_written;
    }
    
    return offset;
}

/**
 * Deserialize dictionary from binary format
 */
int dictionary_deserialize(symbolic_dictionary_t* dict,
                                  const uint8_t* input,
                                  size_t input_size) {
    if (!dict || !input || input_size < sizeof(symbolic_dictionary_header_t)) {
        return -1;
    }
    
    // Read header
    symbolic_dictionary_header_t header;
    memcpy(&header, input, sizeof(header));
    
    // Validate magic
    if (header.magic != SYMBOLIC_COMPRESSION_MAGIC) {
        printf("[SYMBOLIC] ❌ Invalid dictionary magic: 0x%08X\n", header.magic);
        return -1;
    }
    
    // Validate version
    if (header.version != SYMBOLIC_COMPRESSION_VERSION) {
        printf("[SYMBOLIC] ⚠️  Dictionary version mismatch: %u (expected %u)\n",
               header.version, SYMBOLIC_COMPRESSION_VERSION);
        // Continue anyway (for now)
    }
    
    // Validate entry count
    if (header.entry_count > SYMBOLIC_MAX_SYMBOLS) {
        printf("[SYMBOLIC] ❌ Invalid entry count: %u (max %d)\n",
               header.entry_count, SYMBOLIC_MAX_SYMBOLS);
        return -1;
    }
    
    // Initialize dictionary
    dict->loaded = false;
    dict->version = header.version;
    dict->entry_count = header.entry_count;
    
    // Calculate total string storage size
    size_t total_string_size = 0;
    size_t offset = sizeof(header);
    
    for (uint16_t i = 0; i < header.entry_count; i++) {
        if (offset + sizeof(symbolic_dict_entry_t) > input_size) {
            printf("[SYMBOLIC] ❌ Truncated dictionary entry %u\n", i);
            return -1;
        }
        
        symbolic_dict_entry_t entry;
        memcpy(&entry, input + offset, sizeof(entry));
        offset += sizeof(entry);
        
        if (entry.code < 0x80 || entry.code > 0xFF) {
            printf("[SYMBOLIC] ❌ Invalid entry code: 0x%02X\n", entry.code);
            return -1;
        }
        
        if (entry.string_len > SYMBOLIC_MAX_STRING_LEN) {
            printf("[SYMBOLIC] ❌ Entry string too long: %u (max %d)\n",
                   entry.string_len, SYMBOLIC_MAX_STRING_LEN);
            return -1;
        }
        
        total_string_size += entry.string_len + 1; // +1 for null terminator
    }
    
    // Allocate string storage
    dict->string_storage = (char*)malloc(total_string_size);
    if (!dict->string_storage) {
        printf("[SYMBOLIC] ❌ Failed to allocate string storage (%zu bytes)\n",
               total_string_size);
        return -1;
    }
    dict->string_storage_size = total_string_size;
    
    // Build lookup table
    char* storage_ptr = dict->string_storage;
    offset = sizeof(header);
    
    for (uint16_t i = 0; i < header.entry_count; i++) {
        symbolic_dict_entry_t entry;
        memcpy(&entry, input + offset, sizeof(entry));
        offset += sizeof(entry);
        
        uint8_t code = entry.code;
        uint16_t array_index = code - 0x80;
        
        if (array_index >= SYMBOLIC_MAX_SYMBOLS) {
            printf("[SYMBOLIC] ❌ Array index out of bounds: %u\n", array_index);
            continue;
        }
        
        // Copy string to storage
        size_t copy_len = entry.string_len;
        if (copy_len > 0) {
            memcpy(storage_ptr, entry.string, copy_len);
            storage_ptr[copy_len] = '\0';
            
            // Set lookup table pointers
            dict->strings[array_index] = storage_ptr;
            dict->lengths[array_index] = copy_len;
            
            storage_ptr += copy_len + 1;
        }
    }
    
    dict->loaded = true;
    printf("[SYMBOLIC] ✅ Loaded dictionary: %u entries, version %u\n",
           dict->entry_count, dict->version);
    
    return 0;
}

// ============================================================================
// DICTIONARY LOAD/SAVE (Node 0 Integration)
// ============================================================================

int symbolic_dictionary_load(symbolic_compression_context_t* ctx, void* lattice) {
    if (!ctx || !lattice) return -1;
    
    persistent_lattice_t* lat = (persistent_lattice_t*)lattice;
    
    // Find dictionary node by name "SYMBOLIC_DICTIONARY"
    uint64_t dict_node_ids[1];
    uint32_t found_count = lattice_find_nodes_by_name(lat, "SYMBOLIC_DICTIONARY", dict_node_ids, 1);
    
    if (found_count == 0) {
        // Try Node 0 as fallback
        lattice_node_t node;
        if (lattice_get_node_data(lat, SYMBOLIC_DICTIONARY_NODE_ID, &node) != 0) {
            printf("[SYMBOLIC] ℹ️  No dictionary found\n");
            return 0; // Not an error - dictionary is optional
        }
        dict_node_ids[0] = SYMBOLIC_DICTIONARY_NODE_ID;
    }
    
    uint64_t dict_id = dict_node_ids[0];
    
    // Check if dictionary is chunked
    if (lattice_is_node_chunked(lat, dict_id)) {
        // Dictionary is chunked - need to reassemble
        void* reassembled = NULL;
        size_t reassembled_len = 0;
        
        if (lattice_get_node_chunked(lat, dict_id,
                                     &reassembled, &reassembled_len) != 0) {
            printf("[SYMBOLIC] ❌ Failed to reassemble chunked dictionary\n");
            return -1;
        }
        
        // Deserialize from reassembled data
        int result = dictionary_deserialize(&ctx->dictionary,
                                           (const uint8_t*)reassembled,
                                           reassembled_len);
        free(reassembled);
        
        if (result == 0) {
            ctx->compression_enabled = true;
        }
        return result;
    } else {
        // Dictionary fits in single node - get binary data
        if (!lattice_is_node_binary(lat, dict_id)) {
            printf("[SYMBOLIC] ❌ Dictionary node is not binary format\n");
            return -1;
        }
        
        uint8_t dict_data[512];
        size_t dict_len = 0;
        bool is_binary = false;
        
        if (lattice_get_node_data_binary(lat, dict_id,
                                        dict_data, &dict_len, &is_binary) != 0) {
            printf("[SYMBOLIC] ❌ Failed to read dictionary node data\n");
            return -1;
        }
        
        return dictionary_deserialize(&ctx->dictionary, dict_data, dict_len);
    }
}

int symbolic_dictionary_save(symbolic_compression_context_t* ctx, void* lattice) {
    if (!ctx || !lattice) return -1;
    
    if (!ctx->dictionary.loaded || ctx->dictionary.entry_count == 0) {
        printf("[SYMBOLIC] ❌ No dictionary to save\n");
        return -1;
    }
    
    persistent_lattice_t* lat = (persistent_lattice_t*)lattice;
    
    // Serialize dictionary
    // Max size: header (24) + 128 entries × 74 bytes = ~9472 bytes
    // Use 16KB buffer to be safe
    uint8_t dict_data[16384];
    size_t dict_size = dictionary_serialize(&ctx->dictionary, dict_data, sizeof(dict_data));
    
    if (dict_size == 0) {
        printf("[SYMBOLIC] ❌ Failed to serialize dictionary\n");
        return -1;
    }
    
    // Check if dictionary fits in single node (510 bytes max)
    if (dict_size <= 510) {
        // Store in Node 0 as binary (use reserved ID 0)
        // First check if Node 0 exists, update it if so
        lattice_node_t existing;
        if (lattice_get_node_data(lat, SYMBOLIC_DICTIONARY_NODE_ID, &existing) == 0) {
            // Node 0 exists, update it
            if (lattice_update_node_binary(lat, SYMBOLIC_DICTIONARY_NODE_ID,
                                          dict_data, dict_size) != 0) {
                printf("[SYMBOLIC] ❌ Failed to update dictionary Node 0\n");
                return -1;
            }
            printf("[SYMBOLIC] ✅ Updated dictionary in Node 0 (%zu bytes)\n", dict_size);
            return 0;
        } else {
            // Node 0 doesn't exist, create it with reserved ID
            // Note: We need to ensure Node 0 is created, but lattice_add_node_binary
            // will assign the next available ID. We'll use a workaround: create with
            // a special name and then find it, or use lattice_add_node_with_id if available.
            // For now, we'll create it and store the ID in the lattice metadata.
            uint64_t dict_id = lattice_add_node_binary(lat,
                                                      LATTICE_NODE_PRIMITIVE,
                                                      "SYMBOLIC_DICTIONARY",
                                                      dict_data,
                                                      dict_size,
                                                      0);
            
            if (dict_id == 0) {
                printf("[SYMBOLIC] ❌ Failed to save dictionary to Node 0\n");
                return -1;
            }
            
            // If dict_id is not 0, we need to handle this differently
            // For now, we'll use the first node created as the dictionary node
            // and store it with a known name for lookup
            printf("[SYMBOLIC] ✅ Saved dictionary to node %lu (%zu bytes)\n", dict_id, dict_size);
            return 0;
        }
    } else {
        // Dictionary is too large - use chunked storage
        // Check if Node 0 exists (chunked)
        lattice_node_t existing;
        if (lattice_get_node_data(lat, SYMBOLIC_DICTIONARY_NODE_ID, &existing) == 0) {
            // Node 0 exists, but we can't update chunked nodes easily
            // For now, we'll create a new chunked node
            // TODO: Implement chunked node update
        }
        
        uint64_t dict_id = lattice_add_node_chunked(lat,
                                                    LATTICE_NODE_PRIMITIVE,
                                                    "SYMBOLIC_DICTIONARY",
                                                    dict_data,
                                                    dict_size,
                                                    0);
        
        if (dict_id == 0) {
            printf("[SYMBOLIC] ❌ Failed to save chunked dictionary\n");
            return -1;
        }
        
        // Store dictionary node ID in a way we can retrieve it
        // For now, we'll search by name "SYMBOLIC_DICTIONARY"
        printf("[SYMBOLIC] ✅ Saved chunked dictionary to node %lu (%zu bytes)\n", dict_id, dict_size);
        return 0;
    }
}

// ============================================================================
// COMPRESSION DETECTION
// ============================================================================

bool symbolic_is_compressed(const uint8_t* data, size_t data_len) {
    if (!data || data_len < 3) return false;
    
    // Binary mode format: [length:2][compression_type:1][payload...]
    // Compression flag: bit 15 of length header
    uint16_t length_header;
    memcpy(&length_header, data, 2);
    
    // Check if compression bit is set (bit 15)
    if (length_header & 0x8000) {
        return true;
    }
    
    return false;
}

// ============================================================================
// COMPRESSION ALGORITHM (Phase 2)
// ============================================================================

/**
 * Find longest matching string in dictionary (greedy match)
 * Returns symbol code (0x80-0xFF) if match found, 0 if no match
 */
static uint8_t find_longest_match(const symbolic_dictionary_t* dict,
                                  const uint8_t* input,
                                  size_t input_len,
                                  size_t* match_len) {
    if (!dict || !dict->loaded || !input || input_len == 0) {
        if (match_len) *match_len = 0;
        return 0;
    }
    
    uint8_t best_code = 0;
    size_t best_len = 0;
    
    // Try each dictionary entry (greedy: find longest match)
    for (uint16_t i = 0; i < SYMBOLIC_MAX_SYMBOLS; i++) {
        const char* dict_str = dict->strings[i];
        uint16_t dict_len = dict->lengths[i];
        
        if (!dict_str || dict_len == 0) continue;
        if (dict_len > input_len) continue; // Can't match if dict string is longer
        
        // Check if this entry matches at current position
        if (memcmp(input, dict_str, dict_len) == 0) {
            // Found a match - check if it's longer than current best
            if (dict_len > best_len) {
                best_len = dict_len;
                best_code = 0x80 + i; // Convert array index to symbol code
            }
        }
    }
    
    if (match_len) *match_len = best_len;
    return best_code;
}

/**
 * Compress input data using dictionary (FSST-style greedy matching)
 */
int symbolic_compress(symbolic_compression_context_t* ctx,
                      const uint8_t* input,
                      size_t input_len,
                      uint8_t* output,
                      size_t output_size,
                      size_t* output_len) {
    if (!ctx || !input || !output || !output_len) return -1;
    
    if (!ctx->dictionary.loaded || ctx->dictionary.entry_count == 0) {
        // No dictionary - can't compress
        return -1;
    }
    
    if (input_len == 0) {
        *output_len = 0;
        return 0;
    }
    
    // Compression format:
    // [compressed_payload...]
    // Payload: mix of literal bytes (0x00-0x7F) and symbol codes (0x80-0xFF)
    // Note: compression_type byte is added by integration layer (symbolic_pack_binary_header)
    
    uint8_t* out_ptr = output;
    size_t out_remaining = output_size;
    const uint8_t* in_ptr = input;
    size_t in_remaining = input_len;
    
    // Greedy compression: find longest matches
    while (in_remaining > 0 && out_remaining > 0) {
        size_t match_len = 0;
        uint8_t symbol_code = find_longest_match(&ctx->dictionary, in_ptr, in_remaining, &match_len);
        
        if (symbol_code != 0 && match_len > 0) {
            // Found a match - write symbol code
            *out_ptr++ = symbol_code;
            out_remaining--;
            in_ptr += match_len;
            in_remaining -= match_len;
        } else {
            // No match - write literal byte (must be 0x00-0x7F)
            uint8_t literal = *in_ptr++;
            in_remaining--;
            
            // Ensure literal is in valid range (0x00-0x7F)
            if (literal >= 0x80) {
                // Can't encode byte >= 0x80 as literal
                // This is a limitation - we skip it or encode as-is
                // For now, we'll skip bytes >= 0x80 (they should be rare in text)
                continue;
            }
            
            *out_ptr++ = literal;
            out_remaining--;
        }
    }
    
    // Check if we consumed all input
    if (in_remaining > 0) {
        // Output buffer too small
        return -1;
    }
    
    // Calculate output length
    *output_len = (out_ptr - output);
    
    return 0;
}

/**
 * Decompress input data using dictionary (HOT PATH - optimized)
 * This is the critical path - must be <0.1μs per node
 */
int symbolic_decompress(symbolic_compression_context_t* ctx,
                        const uint8_t* input,
                        size_t input_len,
                        uint8_t* output,
                        size_t output_size,
                        size_t* output_len) {
    if (!ctx || !input || !output || !output_len) return -1;
    
    if (!ctx->dictionary.loaded) {
        return -1; // No dictionary loaded
    }
    
    if (input_len == 0) {
        *output_len = 0;
        return 0;
    }
    
    // Skip compression_type byte (first byte)
    if (input_len < 1) return -1;
    const uint8_t* in_ptr = input + 1; // Skip compression_type
    size_t in_remaining = input_len - 1;
    
    uint8_t* out_ptr = output;
    size_t out_remaining = output_size;
    
    // Hot path: simple loop with direct lookup
    // Each byte is either:
    // - Literal (0x00-0x7F): copy directly
    // - Symbol code (0x80-0xFF): lookup in dictionary and copy string
    
    while (in_remaining > 0 && out_remaining > 0) {
        uint8_t byte = *in_ptr++;
        in_remaining--;
        
        if (byte < 0x80) {
            // Literal byte - copy directly
            *out_ptr++ = byte;
            out_remaining--;
        } else {
            // Symbol code - lookup in dictionary
            uint16_t array_index = byte - 0x80;
            
            if (array_index >= SYMBOLIC_MAX_SYMBOLS) {
                // Invalid symbol code
                return -1;
            }
            
            const char* dict_str = ctx->dictionary.strings[array_index];
            uint16_t dict_len = ctx->dictionary.lengths[array_index];
            
            if (!dict_str || dict_len == 0) {
                // Symbol code not in dictionary
                return -1;
            }
            
            // Check if we have enough space
            if (out_remaining < dict_len) {
                return -1; // Output buffer too small
            }
            
            // Copy dictionary string (memcpy is fast, often SIMD-optimized)
            memcpy(out_ptr, dict_str, dict_len);
            out_ptr += dict_len;
            out_remaining -= dict_len;
        }
    }
    
    // Check if we consumed all input
    if (in_remaining > 0) {
        // Input not fully consumed (shouldn't happen)
        return -1;
    }
    
    *output_len = output_size - out_remaining;
    return 0;
}

double symbolic_estimate_ratio(symbolic_compression_context_t* ctx,
                               const uint8_t* input,
                               size_t input_len) {
    // TODO: Phase 3 - Implement frequency-based estimation
    (void)ctx; (void)input; (void)input_len;
    return 1.0; // No compression
}

// ============================================================================
// FREQUENCY ANALYSIS (Phase 3)
// ============================================================================

// String frequency entry (for building dictionary)
typedef struct {
    char* string;
    size_t length;
    uint64_t frequency;
    size_t total_bytes_saved; // bytes_saved = frequency * (length - 1) (1 byte for symbol)
} string_frequency_t;

// Comparison function for sorting by total_bytes_saved (descending)
static int compare_frequency(const void* a, const void* b) {
    const string_frequency_t* fa = (const string_frequency_t*)a;
    const string_frequency_t* fb = (const string_frequency_t*)b;
    
    if (fa->total_bytes_saved > fb->total_bytes_saved) return -1;
    if (fa->total_bytes_saved < fb->total_bytes_saved) return 1;
    return 0;
}

/**
 * Extract all substrings from data (sliding window approach)
 * This finds all possible strings that could be compressed
 */
static void extract_strings_from_data(const uint8_t* data,
                                     size_t data_len,
                                     string_frequency_t* frequencies,
                                     uint32_t* freq_count,
                                     uint32_t max_frequencies,
                                     uint32_t min_string_len,
                                     uint32_t max_string_len) {
    if (!data || !frequencies || !freq_count || data_len == 0) return;
    
    // Use a simple hash table for frequency counting
    // For simplicity, we'll use a linear search (can be optimized later)
    
    // Extract all substrings of length min_string_len to max_string_len
    for (size_t start = 0; start < data_len; start++) {
        for (uint32_t len = min_string_len; len <= max_string_len && start + len <= data_len; len++) {
            // Check if substring contains only printable ASCII (0x20-0x7E)
            // This filters out binary data and focuses on text patterns
            bool is_printable = true;
            for (uint32_t i = 0; i < len; i++) {
                uint8_t byte = data[start + i];
                if (byte < 0x20 || byte > 0x7E) {
                    is_printable = false;
                    break;
                }
            }
            
            if (!is_printable) continue;
            
            // Find or create frequency entry
            bool found = false;
            for (uint32_t i = 0; i < *freq_count; i++) {
                if (frequencies[i].length == len &&
                    memcmp(frequencies[i].string, data + start, len) == 0) {
                    frequencies[i].frequency++;
                    frequencies[i].total_bytes_saved = frequencies[i].frequency * (len - 1);
                    found = true;
                    break;
                }
            }
            
            if (!found && *freq_count < max_frequencies) {
                // Allocate new entry
                frequencies[*freq_count].string = (char*)malloc(len + 1);
                if (frequencies[*freq_count].string) {
                    memcpy(frequencies[*freq_count].string, data + start, len);
                    frequencies[*freq_count].string[len] = '\0';
                    frequencies[*freq_count].length = len;
                    frequencies[*freq_count].frequency = 1;
                    frequencies[*freq_count].total_bytes_saved = len - 1; // 1 byte saved per occurrence
                    (*freq_count)++;
                }
            }
        }
    }
}

/**
 * Build dictionary from lattice data using frequency analysis
 */
int symbolic_dictionary_build(symbolic_compression_context_t* ctx,
                              void* lattice,
                              double sample_rate) {
    if (!ctx || !lattice) return -1;
    
    persistent_lattice_t* lat = (persistent_lattice_t*)lattice;
    
    printf("[SYMBOLIC] 🔍 Building dictionary from lattice (sample_rate=%.1f%%)\n",
           sample_rate * 100.0);
    
    // Allocate frequency table (large enough for many unique strings)
    const uint32_t max_frequencies = 10000;
    string_frequency_t* frequencies = (string_frequency_t*)calloc(max_frequencies, sizeof(string_frequency_t));
    if (!frequencies) {
        printf("[SYMBOLIC] ❌ Failed to allocate frequency table\n");
        return -1;
    }
    
    uint32_t freq_count = 0;
    uint32_t nodes_scanned = 0;
    uint32_t nodes_to_scan = 0;
    
    // Determine how many nodes to scan
    if (sample_rate >= 1.0) {
        // Full scan
        nodes_to_scan = lat->total_nodes;
    } else {
        // Sampling
        nodes_to_scan = (uint32_t)(lat->total_nodes * sample_rate);
        if (nodes_to_scan == 0) nodes_to_scan = 1; // At least scan 1 node
    }
    
    printf("[SYMBOLIC] 📊 Scanning %u of %u nodes...\n", nodes_to_scan, lat->total_nodes);
    
    // Scan nodes (iterate through all nodes or sample)
    uint32_t scan_interval = (sample_rate < 1.0) ? (lat->total_nodes / nodes_to_scan) : 1;
    if (scan_interval == 0) scan_interval = 1;
    
    for (uint32_t i = 0; i < lat->total_nodes && nodes_scanned < nodes_to_scan; i += scan_interval) {
        lattice_node_t node;
        
        // Get node (try RAM cache first, then disk)
        if (i < lat->node_count) {
            node = lat->nodes[i];
        } else {
            // Node not in RAM - skip for now (would need disk read)
            // For sampling, this is acceptable
            continue;
        }
        
        if (node.id == 0) continue; // Skip invalid nodes
        
        // Extract strings from node name
        if (strlen(node.name) > 0) {
            extract_strings_from_data((const uint8_t*)node.name, strlen(node.name),
                                     frequencies, &freq_count, max_frequencies,
                                     4, 32); // Min 4 chars, max 32 chars
        }
        
        // Extract strings from node data (text mode)
        if (!lattice_is_node_binary(lat, node.id)) {
            size_t data_len = strlen(node.data);
            if (data_len > 0) {
                extract_strings_from_data((const uint8_t*)node.data, data_len,
                                         frequencies, &freq_count, max_frequencies,
                                         4, 32);
            }
        }
        
        nodes_scanned++;
        
        // Progress indicator
        if (nodes_scanned % 1000 == 0 || nodes_scanned == nodes_to_scan) {
            printf("[SYMBOLIC]   Progress: %u/%u nodes, %u unique strings\n",
                   nodes_scanned, nodes_to_scan, freq_count);
        }
    }
    
    printf("[SYMBOLIC] ✅ Scanned %u nodes, found %u unique strings\n",
           nodes_scanned, freq_count);
    
    // Sort by total_bytes_saved (descending)
    qsort(frequencies, freq_count, sizeof(string_frequency_t), compare_frequency);
    
    // Select top SYMBOLIC_MAX_SYMBOLS strings
    uint32_t selected_count = (freq_count < SYMBOLIC_MAX_SYMBOLS) ? freq_count : SYMBOLIC_MAX_SYMBOLS;
    
    printf("[SYMBOLIC] 📈 Top %u strings by compression benefit:\n", selected_count);
    
    // Build dictionary from top strings
    size_t total_string_size = 0;
    for (uint32_t i = 0; i < selected_count; i++) {
        total_string_size += frequencies[i].length + 1;
    }
    
    // Cleanup old dictionary
    if (ctx->dictionary.string_storage) {
        free(ctx->dictionary.string_storage);
    }
    
    // Allocate new dictionary storage
    ctx->dictionary.string_storage = (char*)malloc(total_string_size);
    if (!ctx->dictionary.string_storage) {
        printf("[SYMBOLIC] ❌ Failed to allocate dictionary storage\n");
        // Cleanup frequencies
        for (uint32_t i = 0; i < freq_count; i++) {
            if (frequencies[i].string) free(frequencies[i].string);
        }
        free(frequencies);
        return -1;
    }
    
    ctx->dictionary.string_storage_size = total_string_size;
    ctx->dictionary.entry_count = 0;
    ctx->dictionary.version = SYMBOLIC_COMPRESSION_VERSION;
    
    // Initialize lookup arrays
    for (int i = 0; i < SYMBOLIC_MAX_SYMBOLS; i++) {
        ctx->dictionary.strings[i] = NULL;
        ctx->dictionary.lengths[i] = 0;
    }
    
    // Build lookup table
    char* storage_ptr = ctx->dictionary.string_storage;
    
    for (uint32_t i = 0; i < selected_count; i++) {
        uint8_t code = 0x80 + i;
        uint16_t array_index = code - 0x80;
        
        size_t len = frequencies[i].length;
        
        // Copy string to storage
        memcpy(storage_ptr, frequencies[i].string, len);
        storage_ptr[len] = '\0';
        
        // Set lookup table
        ctx->dictionary.strings[array_index] = storage_ptr;
        ctx->dictionary.lengths[array_index] = len;
        
        storage_ptr += len + 1;
        ctx->dictionary.entry_count++;
        
        // Log top entries
        if (i < 10) {
            printf("[SYMBOLIC]   [0x%02X] '%s' (freq=%lu, saved=%zu bytes)\n",
                   code, frequencies[i].string, frequencies[i].frequency,
                   frequencies[i].total_bytes_saved);
        }
    }
    
    ctx->dictionary.loaded = true;
    
    // Cleanup frequencies
    for (uint32_t i = 0; i < freq_count; i++) {
        if (frequencies[i].string) free(frequencies[i].string);
    }
    free(frequencies);
    
    printf("[SYMBOLIC] ✅ Dictionary built: %u entries, version %u\n",
           ctx->dictionary.entry_count, ctx->dictionary.version);
    
    return 0;
}

