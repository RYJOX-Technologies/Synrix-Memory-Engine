#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char* type_name(int t) {
    switch (t) {
        case LATTICE_NODE_PRIMITIVE: return "PRIMITIVE";
        case LATTICE_NODE_PERFORMANCE: return "PERFORMANCE";
        case LATTICE_NODE_LEARNING: return "LEARNING";
        default: return "OTHER";
    }
}

static void format_time_human(unsigned long ts, char* out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    time_t t = (time_t)ts;
    struct tm tmv; localtime_r(&t, &tmv);
    strftime(out, out_sz, "%Y-%m-%d %H:%M:%S", &tmv);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <lattice_file> [limit] [--pretty]\n", argv[0]);
        return 1;
    }
    const char* path = argv[1];
    int limit = 20;
    int pretty = 1; // default pretty on
    if (argc >= 3 && argv[2][0] != '-') limit = atoi(argv[2]);
    for (int i = 2; i < argc; i++) if (strcmp(argv[i], "--pretty") == 0) pretty = 1;

    persistent_lattice_t lattice;
    if (lattice_init(&lattice, path) != 0) {
        fprintf(stderr, "Failed to open lattice: %s\n", path);
        return 2;
    }

    printf("LATTICE DUMP: %s (nodes: %u, total: %u)\n", path, lattice.node_count, lattice.total_nodes);
    // Coverage summary
    unsigned int prim_count = 0, prim_validated = 0;
    for (uint32_t i = 0; i < lattice.node_count; i++) {
        lattice_node_t* n = &lattice.nodes[i];
        if (n->type == LATTICE_NODE_PRIMITIVE && strncmp(n->name, "ISA_", 4) == 0) {
            prim_count++;
            if (strstr(n->data, "validated:true")) prim_validated++;
        }
    }
    if (prim_count > 0) {
        printf("Coverage: ISA mnemonics=%u, validated=%u\n", prim_count, prim_validated);
    }

    int printed = 0;
    for (uint32_t i = 0; i < lattice.node_count && printed < limit; i++) {
        lattice_node_t* n = &lattice.nodes[i];
        if (!pretty) {
            printf("[%5u] %-12s conf=%.2f updated=%lu name=%s\n",
                   n->id, type_name(n->type), n->confidence, (unsigned long)n->timestamp, n->name);
            if (n->type == LATTICE_NODE_PERFORMANCE) {
                printf("         perf: time=%.3fns ipc=%.3f thr=%.3f eff=%.3f\n",
                       n->payload.performance.execution_time_ns,
                       n->payload.performance.instructions_per_cycle,
                       n->payload.performance.throughput_mb_s,
                       n->payload.performance.efficiency_score);
            }
            if (n->data[0]) {
                char buf[121];
                strncpy(buf, n->data, 120); buf[120] = '\0';
                printf("         data: %s\n", buf);
            }
        } else {
            char tbuf[32];
            format_time_human((unsigned long)n->timestamp, tbuf, sizeof tbuf);
            printf("id: %u\n", n->id);
            printf("type: %s\n", type_name(n->type));
            printf("name: %s\n", n->name);
            printf("confidence: %.2f\n", n->confidence);
            printf("updated: %s\n", tbuf);
            if (n->type == LATTICE_NODE_PERFORMANCE) {
                printf("perf.time_ns: %.3f\n", n->payload.performance.execution_time_ns);
                printf("perf.ipc: %.3f\n", n->payload.performance.instructions_per_cycle);
                printf("perf.throughput: %.3f\n", n->payload.performance.throughput_mb_s);
                printf("perf.efficiency: %.3f\n", n->payload.performance.efficiency_score);
            }
            if (n->data[0]) {
                // Split data key/value pairs delimited by '|'
                char buf[513];
                strncpy(buf, n->data, 512); buf[512] = '\0';
                printf("data:\n");
                char* tok = strtok(buf, "|");
                while (tok) { printf("  - %s\n", tok); tok = strtok(NULL, "|"); }
            }
            printf("---\n");
        }
        printed++;
    }

    lattice_cleanup(&lattice);
    return 0;
}


