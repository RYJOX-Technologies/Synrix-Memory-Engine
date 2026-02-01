#ifndef INTEGRITY_VALIDATOR_H
#define INTEGRITY_VALIDATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Integrity validation levels
typedef enum {
    INTEGRITY_LEVEL_BASIC = 1,        // Basic structure validation
    INTEGRITY_LEVEL_STRUCTURAL = 2,   // Structural consistency checks
    INTEGRITY_LEVEL_SEMANTIC = 3,     // Semantic consistency checks
    INTEGRITY_LEVEL_CRYPTO = 4,       // Cryptographic signature validation
    INTEGRITY_LEVEL_COMPLETE = 5      // All validation levels
} integrity_level_t;

// Integrity check types
typedef enum {
    INTEGRITY_CHECK_STRUCTURE = 1,    // Data structure integrity
    INTEGRITY_CHECK_REFERENCES = 2,   // Reference integrity
    INTEGRITY_CHECK_CRYPTO = 3,       // Cryptographic integrity
    INTEGRITY_CHECK_CONSISTENCY = 4,  // Data consistency
    INTEGRITY_CHECK_PERFORMANCE = 5   // Performance integrity
} integrity_check_type_t;

// Integrity validation result
typedef struct {
    bool passed;                      // Overall validation result
    uint32_t checks_performed;        // Number of checks performed
    uint32_t checks_failed;           // Number of checks failed
    uint32_t warnings;                // Number of warnings
    uint32_t errors;                  // Number of errors
    uint32_t validation_time_ms;      // Validation time in milliseconds
    char* error_messages[32];         // Error message array
    char* warning_messages[32];       // Warning message array
    uint32_t error_count;             // Number of error messages
    uint32_t warning_count;           // Number of warning messages
} integrity_validation_result_t;

// Integrity validator context
typedef struct {
    integrity_level_t level;          // Validation level
    bool auto_repair;                 // Auto-repair enabled
    bool strict_mode;                 // Strict validation mode
    uint32_t max_errors;              // Maximum errors before stopping
    uint32_t max_warnings;            // Maximum warnings before stopping
    uint32_t timeout_ms;              // Validation timeout
} integrity_validator_t;

// Function declarations
integrity_validator_t* integrity_validator_create(integrity_level_t level);
void integrity_validator_destroy(integrity_validator_t* validator);

// Validation functions
integrity_validation_result_t* integrity_validate_lattice(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size);

integrity_validation_result_t* integrity_validate_element(
    integrity_validator_t* validator,
    void* element_data,
    size_t data_size);

integrity_validation_result_t* integrity_validate_pattern(
    integrity_validator_t* validator,
    void* pattern_data,
    size_t data_size);

integrity_validation_result_t* integrity_validate_crypto_signature(
    integrity_validator_t* validator,
    void* signature_data,
    size_t data_size);

// Repair functions
int integrity_repair_lattice(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size);

int integrity_repair_element(
    integrity_validator_t* validator,
    void* element_data,
    size_t data_size);

int integrity_repair_pattern(
    integrity_validator_t* validator,
    void* pattern_data,
    size_t data_size);

// Result management
void integrity_validation_result_destroy(integrity_validation_result_t* result);
void integrity_print_validation_result(const integrity_validation_result_t* result);
bool integrity_validation_passed(const integrity_validation_result_t* result);

// Advanced validation
int integrity_validate_references(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size);

int integrity_validate_consistency(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size);

int integrity_validate_performance(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size);

// Statistics
void integrity_get_validation_stats(
    integrity_validator_t* validator,
    uint32_t* total_validations,
    uint32_t* passed_validations,
    uint32_t* failed_validations,
    uint32_t* repair_attempts,
    uint32_t* successful_repairs);

#endif // INTEGRITY_VALIDATOR_H
