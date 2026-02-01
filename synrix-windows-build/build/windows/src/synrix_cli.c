/**
 * SYNRIX CLI - Command Line Interface
 * 
 * Windows-native CLI executable that wraps libsynrix.dll
 * 
 * Usage:
 *   synrix.exe add <lattice> <name> <data>
 *   synrix.exe get <lattice> <node_id>
 *   synrix.exe query <lattice> <prefix> [limit]
 *   synrix.exe list <lattice> [prefix]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "persistent_lattice.h"

#define MAX_NAME_LEN 64
#define MAX_DATA_LEN 512

void print_usage(const char* prog_name) {
    fprintf(stderr, "SYNRIX CLI - Command Line Interface\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s add <lattice> <name> <data>\n", prog_name);
    fprintf(stderr, "  %s get <lattice> <node_id>\n", prog_name);
    fprintf(stderr, "  %s query <lattice> <prefix> [limit]\n", prog_name);
    fprintf(stderr, "  %s list <lattice> [prefix]\n", prog_name);
    fprintf(stderr, "  %s count <lattice>\n", prog_name);
    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "  %s add memory.lattice \"MEMORY:test\" \"This is test data\"\n", prog_name);
    fprintf(stderr, "  %s get memory.lattice 12345\n", prog_name);
    fprintf(stderr, "  %s query memory.lattice \"MEMORY:\" 10\n", prog_name);
    fprintf(stderr, "  %s list memory.lattice\n", prog_name);
}

int cmd_add(int argc, char** argv) {
    if (argc < 5) {
        fprintf(stderr, "Error: add requires <lattice> <name> <data>\n");
        return 1;
    }
    
    const char* lattice_path = argv[2];
    const char* name = argv[3];
    const char* data = argv[4];
    
    // Allocate lattice structure
    persistent_lattice_t* lattice = (persistent_lattice_t*)calloc(1, sizeof(persistent_lattice_t));
    if (!lattice) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    if (lattice_init(lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize lattice\n");
        free(lattice);
        return 1;
    }
    
    uint64_t node_id = lattice_add_node(lattice, LATTICE_NODE_LEARNING, name, data, 0);
    
    if (node_id == 0) {
        fprintf(stderr, "Error: Failed to add node\n");
        lattice_cleanup(lattice);
        free(lattice);
        return 1;
    }
    
    printf("{\"success\":true,\"node_id\":%llu}\n", (unsigned long long)node_id);
    
    lattice_save(lattice);
    lattice_cleanup(lattice);
    free(lattice);
    
    return 0;
}

int cmd_get(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Error: get requires <lattice> <node_id>\n");
        return 1;
    }
    
    const char* lattice_path = argv[2];
    uint64_t node_id = strtoull(argv[3], NULL, 10);
    
    persistent_lattice_t* lattice = (persistent_lattice_t*)calloc(1, sizeof(persistent_lattice_t));
    if (!lattice) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    if (lattice_init(lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize lattice\n");
        free(lattice);
        return 1;
    }
    
    lattice_node_t* node = lattice_get_node_copy(lattice, node_id);
    
    if (!node) {
        printf("{\"success\":false,\"error\":\"Node not found\"}\n");
        lattice_cleanup(lattice);
        free(lattice);
        return 1;
    }
    
    printf("{\"success\":true,\"id\":%llu,\"name\":\"%s\",\"data\":\"%s\"}\n",
           (unsigned long long)node->id, node->name, node->data);
    
    lattice_free_node_copy(node);
    lattice_cleanup(lattice);
    free(lattice);
    
    return 0;
}

int cmd_query(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Error: query requires <lattice> <prefix> [limit]\n");
        return 1;
    }
    
    const char* lattice_path = argv[2];
    const char* prefix = argv[3];
    uint32_t limit = (argc >= 5) ? (uint32_t)atoi(argv[4]) : 100;
    
    persistent_lattice_t* lattice = (persistent_lattice_t*)calloc(1, sizeof(persistent_lattice_t));
    if (!lattice) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    if (lattice_init(lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize lattice\n");
        free(lattice);
        return 1;
    }
    
    uint64_t* node_ids = (uint64_t*)malloc(limit * sizeof(uint64_t));
    if (!node_ids) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        lattice_cleanup(lattice);
        free(lattice);
        return 1;
    }
    
    uint32_t count = lattice_find_nodes_by_name(lattice, prefix, node_ids, limit);
    
    printf("{\"success\":true,\"count\":%u,\"nodes\":[", count);
    
    for (uint32_t i = 0; i < count; i++) {
        lattice_node_t* node = lattice_get_node_copy(lattice, node_ids[i]);
        if (node) {
            if (i > 0) printf(",");
            printf("{\"id\":%llu,\"name\":\"%s\",\"data\":\"%s\"}",
                   (unsigned long long)node->id, node->name, node->data);
            lattice_free_node_copy(node);
        }
    }
    
    printf("]}\n");
    
    free(node_ids);
    lattice_cleanup(lattice);
    free(lattice);
    
    return 0;
}

int cmd_list(int argc, char** argv) {
    const char* prefix = (argc >= 4) ? argv[3] : "";
    return cmd_query(argc, argv);
}

int cmd_count(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Error: count requires <lattice>\n");
        return 1;
    }
    
    const char* lattice_path = argv[2];
    
    persistent_lattice_t* lattice = (persistent_lattice_t*)calloc(1, sizeof(persistent_lattice_t));
    if (!lattice) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }
    
    if (lattice_init(lattice, lattice_path, 100000, 0) != 0) {
        fprintf(stderr, "Error: Failed to initialize lattice\n");
        free(lattice);
        return 1;
    }
    
    // Query all nodes with empty prefix
    uint64_t* node_ids = (uint64_t*)malloc(100000 * sizeof(uint64_t));
    if (!node_ids) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        lattice_cleanup(lattice);
        free(lattice);
        return 1;
    }
    
    uint32_t count = lattice_find_nodes_by_name(lattice, "", node_ids, 100000);
    
    printf("{\"success\":true,\"count\":%u}\n", count);
    
    free(node_ids);
    lattice_cleanup(lattice);
    free(lattice);
    
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* command = argv[1];
    
    if (strcmp(command, "add") == 0) {
        return cmd_add(argc, argv);
    } else if (strcmp(command, "get") == 0) {
        return cmd_get(argc, argv);
    } else if (strcmp(command, "query") == 0) {
        return cmd_query(argc, argv);
    } else if (strcmp(command, "list") == 0) {
        return cmd_list(argc, argv);
    } else if (strcmp(command, "count") == 0) {
        return cmd_count(argc, argv);
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }
}
