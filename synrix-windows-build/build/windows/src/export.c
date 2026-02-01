#define _GNU_SOURCE
#include "export.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

// Export nodes to JSON
int lattice_export_to_json(persistent_lattice_t* lattice, 
                          const char* output_path,
                          const char* name_filter,
                          double min_confidence,
                          uint64_t min_timestamp,
                          uint64_t max_timestamp) {
    if (!lattice || !output_path) return -1;
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        printf("[EXPORT] ERROR Failed to open output file: %s\n", output_path);
        return -1;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": \"1.0\",\n");
    fprintf(fp, "  \"export_timestamp\": %lu,\n", (unsigned long)time(NULL));
    fprintf(fp, "  \"nodes\": [\n");
    
    uint32_t exported_count = 0;
    bool has_filters = (name_filter != NULL || min_confidence > 0.0 || 
                       min_timestamp > 0 || max_timestamp > 0);
    
    // Iterate through nodes
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Apply filters
        if (has_filters) {
            if (name_filter && strncmp(node->name, name_filter, strlen(name_filter)) != 0) {
                continue;
            }
            if (min_confidence > 0.0 && node->confidence < min_confidence) {
                continue;
            }
            if (min_timestamp > 0 && node->timestamp < min_timestamp) {
                continue;
            }
            if (max_timestamp > 0 && node->timestamp > max_timestamp) {
                continue;
            }
        }
        
        // Export node
        if (exported_count > 0) {
            fprintf(fp, ",\n");
        }
        
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": %lu,\n", (unsigned long)node->id);
        fprintf(fp, "      \"type\": %u,\n", node->type);
        fprintf(fp, "      \"name\": \"");
        
        // Escape JSON string
        for (const char* p = node->name; *p; p++) {
            if (*p == '"' || *p == '\\') {
                fprintf(fp, "\\%c", *p);
            } else if (*p == '\n') {
                fprintf(fp, "\\n");
            } else if (*p == '\r') {
                fprintf(fp, "\\r");
            } else if (*p == '\t') {
                fprintf(fp, "\\t");
            } else if (isprint((unsigned char)*p)) {
                fprintf(fp, "%c", *p);
            } else {
                fprintf(fp, "\\u%04x", (unsigned char)*p);
            }
        }
        fprintf(fp, "\",\n");
        
        // Export data (check if binary)
        bool is_binary = lattice_is_node_binary(lattice, node->id);
        if (is_binary) {
            void* data = NULL;
            size_t data_len = 0;
            bool is_bin = false;
            if (lattice_get_node_data_binary(lattice, node->id, &data, &data_len, &is_bin) == 0) {
                fprintf(fp, "      \"data_binary\": true,\n");
                fprintf(fp, "      \"data_length\": %zu,\n", data_len);
                fprintf(fp, "      \"data_base64\": \"");
                // Simple base64 encoding (for demo - use proper base64 in production)
                const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
                for (size_t j = 0; j < data_len && j < 100; j++) {  // Limit to 100 chars for demo
                    uint8_t byte = ((uint8_t*)data)[j];
                    fprintf(fp, "%c%c", base64_chars[(byte >> 2) & 0x3F], base64_chars[((byte << 4) & 0x30) | ((byte >> 4) & 0x0F)]);
                }
                if (data_len > 100) fprintf(fp, "...");
                fprintf(fp, "\",\n");
                free(data);
            } else {
                fprintf(fp, "      \"data\": null,\n");
            }
        } else {
            fprintf(fp, "      \"data\": \"");
            // Escape JSON string
            for (const char* p = node->data; *p && (p - node->data) < 511; p++) {
                if (*p == '"' || *p == '\\') {
                    fprintf(fp, "\\%c", *p);
                } else if (*p == '\n') {
                    fprintf(fp, "\\n");
                } else if (*p == '\r') {
                    fprintf(fp, "\\r");
                } else if (*p == '\t') {
                    fprintf(fp, "\\t");
                } else if (isprint((unsigned char)*p)) {
                    fprintf(fp, "%c", *p);
                } else {
                    fprintf(fp, "\\u%04x", (unsigned char)*p);
                }
            }
            fprintf(fp, "\",\n");
        }
        
        fprintf(fp, "      \"parent_id\": %lu,\n", (unsigned long)node->parent_id);
        fprintf(fp, "      \"child_count\": %u,\n", node->child_count);
        fprintf(fp, "      \"confidence\": %.6f,\n", node->confidence);
        fprintf(fp, "      \"timestamp\": %lu\n", (unsigned long)node->timestamp);
        fprintf(fp, "    }");
        
        exported_count++;
    }
    
    fprintf(fp, "\n  ],\n");
    fprintf(fp, "  \"total_nodes\": %u\n", exported_count);
    fprintf(fp, "}\n");
    
    fclose(fp);
    printf("[EXPORT] OK Exported %u nodes to %s\n", exported_count, output_path);
    return 0;
}

// Export with custom filter callback
int lattice_export_to_json_filtered(persistent_lattice_t* lattice,
                                   const char* output_path,
                                   export_node_filter_t filter,
                                   void* filter_ctx) {
    if (!lattice || !output_path || !filter) return -1;
    
    FILE* fp = fopen(output_path, "w");
    if (!fp) {
        printf("[EXPORT] ERROR Failed to open output file: %s\n", output_path);
        return -1;
    }
    
    fprintf(fp, "{\n");
    fprintf(fp, "  \"version\": \"1.0\",\n");
    fprintf(fp, "  \"export_timestamp\": %lu,\n", (unsigned long)time(NULL));
    fprintf(fp, "  \"nodes\": [\n");
    
    uint32_t exported_count = 0;
    
    // Iterate through nodes
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Apply custom filter
        if (!filter(lattice, node->id, node, filter_ctx)) {
            continue;
        }
        
        // Export node (same as above, but simplified for brevity)
        if (exported_count > 0) {
            fprintf(fp, ",\n");
        }
        
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"id\": %lu,\n", (unsigned long)node->id);
        fprintf(fp, "      \"type\": %u,\n", node->type);
        fprintf(fp, "      \"name\": \"%s\",\n", node->name);
        fprintf(fp, "      \"data\": \"%s\",\n", node->data);
        fprintf(fp, "      \"confidence\": %.6f,\n", node->confidence);
        fprintf(fp, "      \"timestamp\": %lu\n", (unsigned long)node->timestamp);
        fprintf(fp, "    }");
        
        exported_count++;
    }
    
    fprintf(fp, "\n  ],\n");
    fprintf(fp, "  \"total_nodes\": %u\n", exported_count);
    fprintf(fp, "}\n");
    
    fclose(fp);
    printf("[EXPORT] OK Exported %u nodes to %s\n", exported_count, output_path);
    return 0;
}

