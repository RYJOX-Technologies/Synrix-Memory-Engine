#ifndef LATTICE_CORRUPTION_REPAIR_H
#define LATTICE_CORRUPTION_REPAIR_H

#include "persistent_lattice.h"

// Corruption detection and repair system for the universal lattice
// This enables NebulOS to self-heal from data corruption

typedef enum {
    CORRUPTION_TYPE_NONE = 0,
    CORRUPTION_TYPE_INVALID_POINTER = 1,
    CORRUPTION_TYPE_MEMORY_LEAK = 2,
    CORRUPTION_TYPE_DOUBLE_FREE = 3,
    CORRUPTION_TYPE_USE_AFTER_FREE = 4,
    CORRUPTION_TYPE_BUFFER_OVERFLOW = 5,
    CORRUPTION_TYPE_STRUCTURE_MISMATCH = 6
} corruption_type_t;

typedef struct {
    uint32_t node_id;
    corruption_type_t type;
    const char* description;
    uintptr_t invalid_pointer;
    float confidence;  // 0.0-1.0 confidence in corruption detection
    uint32_t repair_attempts;
    bool is_repaired;
} corruption_report_t;

typedef struct {
    uint32_t total_corruptions_detected;
    uint32_t total_corruptions_repaired;
    uint32_t corruption_types[7];  // Count per corruption type
    uint32_t repair_attempts;
    uint32_t failed_repairs;
    float repair_success_rate;
} corruption_statistics_t;

// Core corruption detection functions
int lattice_detect_corruption(persistent_lattice_t* lattice, corruption_report_t* reports, uint32_t max_reports);
bool lattice_validate_node(const lattice_node_t* node, uint32_t node_id);
bool lattice_validate_pointer(const void* ptr, const char* context);

// Surgical repair functions
int lattice_surgical_repair(persistent_lattice_t* lattice, const corruption_report_t* report);
int lattice_repair_invalid_pointer(persistent_lattice_t* lattice, uint32_t node_id, uintptr_t invalid_ptr);
int lattice_repair_memory_leak(persistent_lattice_t* lattice, uint32_t node_id);
int lattice_repair_structure_mismatch(persistent_lattice_t* lattice, uint32_t node_id);

// Learning integration
int lattice_corruption_learn_pattern(const corruption_report_t* report);
int lattice_corruption_apply_learned_fixes(persistent_lattice_t* lattice);

// Statistics and reporting
corruption_statistics_t lattice_get_corruption_statistics(void);
void lattice_print_corruption_report(const corruption_report_t* report);
void lattice_print_corruption_statistics(void);

// Self-healing orchestration
int lattice_self_heal(persistent_lattice_t* lattice);
int lattice_validate_and_repair(persistent_lattice_t* lattice);

#endif // LATTICE_CORRUPTION_REPAIR_H
