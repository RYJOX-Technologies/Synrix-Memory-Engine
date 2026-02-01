#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* type_name(int t) {
    switch (t) {
        case LATTICE_NODE_PRIMITIVE: return "PRIMITIVE";
        case LATTICE_NODE_KERNEL: return "KERNEL";
        case LATTICE_NODE_PATTERN: return "PATTERN";
        case LATTICE_NODE_PERFORMANCE: return "PERFORMANCE";
        case LATTICE_NODE_LEARNING: return "LEARNING";
        case LATTICE_NODE_ANTI_PATTERN: return "ANTI_PATTERN";
        case LATTICE_NODE_SIDECAR_MAPPING: return "SIDECAR_MAPPING";
        case LATTICE_NODE_SIDECAR_EVENT: return "SIDECAR_EVENT";
        case LATTICE_NODE_SIDECAR_SUGGESTION: return "SIDECAR_SUGGESTION";
        case LATTICE_NODE_SIDECAR_STATE: return "SIDECAR_STATE";
        default: return "OTHER";
    }
}

static void format_time_human(uint64_t ts, char* out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    time_t t = (time_t)ts;
    struct tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    strftime(out, out_sz, "%Y-%m-%d %H:%M:%S", &tmv);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <lattice_file> [prefix] [--recent] [--limit N]\n", argv[0]);
        fprintf(stderr, "  prefix: Query prefix (e.g., PROJECT_, WORK_, TASK_, WINDOWS_)\n");
        fprintf(stderr, "  --recent: Show only recent nodes (last 30 days)\n");
        fprintf(stderr, "  --limit N: Limit output to N nodes (default: 50)\n");
        return 1;
    }
    
    const char* path = argv[1];
    const char* query_prefix = NULL;
    int show_recent = 0;
    int limit = 50;
    
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--recent") == 0) {
            show_recent = 1;
        } else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc) {
            limit = atoi(argv[++i]);
        } else if (argv[i][0] != '-') {
            query_prefix = argv[i];
        }
    }
    
    persistent_lattice_t lattice;
    if (lattice_init(&lattice, path) != 0) {
        fprintf(stderr, "Failed to open lattice: %s\n", path);
        return 2;
    }
    
    printf("=== SYNRIX LATTICE WORK QUERY ===\n");
    printf("Lattice: %s\n", path);
    printf("Nodes: %u (total: %u)\n", lattice.node_count, lattice.total_nodes);
    printf("\n");
    
    // Calculate time threshold for recent (30 days ago)
    uint64_t recent_threshold = 0;
    if (show_recent) {
        time_t now = time(NULL);
        recent_threshold = (uint64_t)(now - (30 * 24 * 3600));
    }
    
    // Common prefixes to search for work-related nodes
    const char* prefixes[] = {
        "PROJECT_", "WORK_", "TASK_", "WINDOWS_", "BUILD_", 
        "FEATURE_", "FIX_", "IMPROVEMENT_", "SYNRIX_", "AION_"
    };
    int prefix_count = sizeof(prefixes) / sizeof(prefixes[0]);
    
    // If specific prefix provided, use it; otherwise search all common prefixes
    const char** search_prefixes = NULL;
    int search_count = 0;
    
    if (query_prefix) {
        search_prefixes = &query_prefix;
        search_count = 1;
    } else {
        search_prefixes = prefixes;
        search_count = prefix_count;
    }
    
    uint32_t total_found = 0;
    
    for (int p = 0; p < search_count; p++) {
        const char* prefix = search_prefixes[p];
        uint64_t node_ids[1000];
        uint64_t min_ts = show_recent ? recent_threshold : 0;
        uint32_t found = lattice_find_nodes_by_name_filtered(&lattice, prefix, node_ids, 1000, 0.0, min_ts, 0);
        
        if (found == 0) continue;
        
        printf("--- Prefix: %s (%u nodes) ---\n", prefix, found);
        
        int printed = 0;
        for (uint32_t i = 0; i < found && printed < limit; i++) {
            lattice_node_t node;
            if (lattice_get_node_data(&lattice, node_ids[i], &node) != 0) {
                continue;
            }
            
            // Filter by recent if requested
            if (show_recent && node.timestamp < recent_threshold) {
                continue;
            }
            
            char tbuf[32];
            format_time_human(node.timestamp, tbuf, sizeof(tbuf));
            
            printf("[%5llu] %-15s conf=%.2f  %s\n", 
                   (unsigned long long)node.id, type_name(node.type), 
                   node.confidence, tbuf);
            printf("        name: %s\n", node.name);
            
            if (node.data[0] && !lattice_is_node_binary(&lattice, node.id)) {
                char buf[257];
                strncpy(buf, node.data, 256);
                buf[256] = '\0';
                // Truncate if too long
                if (strlen(buf) > 200) {
                    buf[200] = '\0';
                    strcat(buf, "...");
                }
                printf("        data: %s\n", buf);
            }
            
            // Show payload for specific types
            if (node.type == LATTICE_NODE_SIDECAR_EVENT) {
                printf("        event: %s, outcome: %s\n", 
                       node.payload.sidecar_event.event_type,
                       node.payload.sidecar_event.outcome);
            } else if (node.type == LATTICE_NODE_SIDECAR_SUGGESTION) {
                printf("        suggestion: %s -> %s (approved: %s)\n",
                       node.payload.sidecar_suggestion.intent_name,
                       node.payload.sidecar_suggestion.capability_name,
                       node.payload.sidecar_suggestion.is_approved ? "yes" : "no");
            }
            
            printf("\n");
            printed++;
            total_found++;
            
            // Free children if allocated
            if (node.children) {
                free(node.children);
            }
        }
        
        if (found > limit) {
            printf("... (%u more nodes with this prefix)\n", found - limit);
        }
        printf("\n");
    }
    
    printf("=== Total nodes found: %u ===\n", total_found);
    
    lattice_cleanup(&lattice);
    return 0;
}
