/*
 * SYMBOLIC COMPRESSION INTEGRATION
 * =================================
 * 
 * Integration layer for symbolic compression with persistent_lattice.
 * Handles compression flag in binary mode header and transparent compression/decompression.
 */

#include "symbolic_compression.h"
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// BINARY MODE HEADER FORMAT
// ============================================================================
// 
// Uncompressed: [length:2][data...]
// Compressed:  [length:2 (bit 15 = 1)][compression_type:1][compressed_data...]
//
// Bit 15 of length header indicates compression:
// - 0 = uncompressed
// - 1 = compressed
//
// Compression type byte (only present if compressed):
// - 0 = None (shouldn't happen if bit 15 is set)
// - 1 = Global dictionary (Node 0)
// - 2 = Local dictionary (future)

/**
 * Pack compressed data into binary mode format
 * @param compressed_data Compressed payload (without compression_type byte)
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
                               size_t* output_len) {
    if (!compressed_data || !output || !output_len) return -1;
    
    // Total size: 2 bytes (length) + 1 byte (compression_type) + compressed_len
    // Length field stores: 1 (compression_type) + compressed_len
    size_t payload_size = 1 + compressed_len;
    
    if (payload_size > 0x7FFF) {
        return -1; // Payload too large for 15-bit length field
    }
    
    size_t total_size = 2 + payload_size;
    
    if (output_size < total_size) {
        return -1; // Output buffer too small
    }
    
    // Write length header with compression bit set (bit 15)
    // Length field = payload_size (1 byte compression_type + compressed_len)
    uint16_t length_header = (uint16_t)payload_size | 0x8000; // Set bit 15
    memcpy(output, &length_header, 2);
    
    // Write compression type
    output[2] = compression_type;
    
    // Write compressed data
    memcpy(output + 3, compressed_data, compressed_len);
    
    *output_len = total_size;
    return 0;
}

/**
 * Unpack compressed data from binary mode format
 * @param input Input buffer (binary mode format)
 * @param input_len Input length
 * @param compression_type Output: compression type
 * @param compressed_data Output: compressed payload (pointer into input)
 * @param compressed_len Output: compressed payload length
 * @return 0 on success, -1 on error
 */
int symbolic_unpack_binary_header(const uint8_t* input,
                                 size_t input_len,
                                 uint8_t* compression_type,
                                 const uint8_t** compressed_data,
                                 size_t* compressed_len) {
    if (!input || input_len < 3) return -1;
    
    // Read length header
    uint16_t length_header;
    memcpy(&length_header, input, 2);
    
    // Check compression bit
    if (!(length_header & 0x8000)) {
        return -1; // Not compressed
    }
    
    // Extract length (bits 0-14) - this is payload size (1 byte compression_type + compressed_data)
    size_t payload_len = length_header & 0x7FFF;
    
    if (input_len < 2 + payload_len) {
        return -1; // Truncated
    }
    
    if (payload_len < 1) {
        return -1; // Must have at least compression_type byte
    }
    
    // Read compression type
    if (compression_type) {
        *compression_type = input[2];
    }
    
    // Set compressed data pointer and length (payload_len - 1, since payload includes compression_type)
    if (compressed_data) {
        *compressed_data = input + 3;
    }
    if (compressed_len) {
        *compressed_len = payload_len - 1; // Subtract compression_type byte
    }
    
    return 0;
}

/**
 * Compress and pack node data for binary mode storage
 * @param ctx Compression context
 * @param input Input data (uncompressed)
 * @param input_len Input length
 * @param output Output buffer (binary mode format with compression)
 * @param output_size Output buffer size
 * @param output_len Output length (written)
 * @return 0 on success, -1 on error
 */
int symbolic_compress_for_binary(symbolic_compression_context_t* ctx,
                                const uint8_t* input,
                                size_t input_len,
                                uint8_t* output,
                                size_t output_size,
                                size_t* output_len) {
    if (!ctx || !input || !output || !output_len) return -1;
    
    // Compress data
    uint8_t compressed_buffer[512];
    size_t compressed_len = 0;
    
    if (symbolic_compress(ctx, input, input_len, compressed_buffer, sizeof(compressed_buffer), &compressed_len) != 0) {
        return -1;
    }
    
    // Pack into binary mode format (adds compression_type byte)
    return symbolic_pack_binary_header(compressed_buffer,
                                      compressed_len,
                                      1,  // Global dictionary
                                      output,
                                      output_size,
                                      output_len);
}

/**
 * Decompress node data from binary mode format
 * @param ctx Compression context
 * @param input Input buffer (binary mode format)
 * @param input_len Input length
 * @param output Output buffer (uncompressed)
 * @param output_size Output buffer size
 * @param output_len Output length (written)
 * @return 0 on success, -1 on error
 */
int symbolic_decompress_from_binary(symbolic_compression_context_t* ctx,
                                   const uint8_t* input,
                                   size_t input_len,
                                   uint8_t* output,
                                   size_t output_size,
                                   size_t* output_len) {
    if (!ctx || !input || !output || !output_len) return -1;
    
    // Check if compressed
    if (!symbolic_is_compressed(input, input_len)) {
        // Not compressed - extract as normal binary data
        if (input_len < 2) return -1;
        uint16_t length;
        memcpy(&length, input, 2);
        size_t data_len = length;
        
        if (input_len < 2 + data_len) return -1;
        if (output_size < data_len) return -1;
        
        memcpy(output, input + 2, data_len);
        *output_len = data_len;
        return 0;
    }
    
    // Unpack compressed data
    uint8_t compression_type;
    const uint8_t* compressed_data;
    size_t compressed_len;
    
    if (symbolic_unpack_binary_header(input, input_len, &compression_type,
                                     &compressed_data, &compressed_len) != 0) {
        printf("[SYMBOLIC-INTEGRATION] ❌ Failed to unpack binary header\n");
        return -1;
    }
    
    // Decompress (compressed_data is the payload, no compression_type byte)
    // The decompress function expects compression_type byte at start, so we add it
    uint8_t compressed_buffer[512];
    if (compressed_len + 1 > sizeof(compressed_buffer)) {
        printf("[SYMBOLIC-INTEGRATION] ❌ Compressed data too large: %zu bytes\n", compressed_len);
        return -1;
    }
    
    compressed_buffer[0] = compression_type;
    memcpy(compressed_buffer + 1, compressed_data, compressed_len);
    
    // Decompress
    int result = symbolic_decompress(ctx, compressed_buffer, compressed_len + 1,
                                    output, output_size, output_len);
    
    if (result != 0) {
        printf("[SYMBOLIC-INTEGRATION] ❌ Decompression failed: compressed_len=%zu, output_size=%zu\n",
               compressed_len, output_size);
    }
    
    return result;
}

