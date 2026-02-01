#define _GNU_SOURCE
#include "integrity_validator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

// Internal validation functions
static bool validate_data_structure(const void* data, size_t size);
static bool validate_crypto_signature_internal(const void* sig_data, size_t size);
static bool validate_reference_integrity(const void* data, size_t size);
static bool validate_data_consistency(const void* data, size_t size);
static bool validate_performance_metrics(const void* data, size_t size);

// Utility functions
static uint64_t get_timestamp_ms(void);
static void add_error_message(integrity_validation_result_t* result, const char* message);
static void add_warning_message(integrity_validation_result_t* result, const char* message);

// Create integrity validator
integrity_validator_t* integrity_validator_create(integrity_level_t level) {
    integrity_validator_t* validator = malloc(sizeof(integrity_validator_t));
    if (!validator) return NULL;
    
    validator->level = level;
    validator->auto_repair = false;
    validator->strict_mode = false;
    validator->max_errors = 100;
    validator->max_warnings = 1000;
    validator->timeout_ms = 30000; // 30 seconds
    
    return validator;
}

void integrity_validator_destroy(integrity_validator_t* validator) {
    if (validator) {
        free(validator);
    }
}

// Get current timestamp in milliseconds
static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// Add error message to result
static void add_error_message(integrity_validation_result_t* result, const char* message) {
    if (!result || result->error_count >= 32) return;
    
    result->error_messages[result->error_count] = malloc(strlen(message) + 1);
    if (result->error_messages[result->error_count]) {
        strcpy(result->error_messages[result->error_count], message);
        result->error_count++;
        result->errors++;
    }
}

// Add warning message to result
static void add_warning_message(integrity_validation_result_t* result, const char* message) {
    if (!result || result->warning_count >= 32) return;
    
    result->warning_messages[result->warning_count] = malloc(strlen(message) + 1);
    if (result->warning_messages[result->warning_count]) {
        strcpy(result->warning_messages[result->warning_count], message);
        result->warning_count++;
        result->warnings++;
    }
}

// Validate data structure integrity
static bool validate_data_structure(const void* data, size_t size) {
    if (!data || size == 0) return false;
    
    // Check for null pointer
    if (data == NULL) return false;
    
    // Check for reasonable size limits
    if (size > 1024 * 1024 * 1024) return false; // 1GB limit
    
    // Check for alignment (basic check)
    if ((uintptr_t)data % 4 != 0) return false;
    
    return true;
}

// Validate crypto signature
static bool validate_crypto_signature_internal(const void* sig_data, size_t size) {
    if (!sig_data || size < 64) return false; // Minimum signature size
    
    // Check for valid signature structure
    const uint8_t* sig = (const uint8_t*)sig_data;
    
    // Check for non-zero signature (basic validation)
    bool has_non_zero = false;
    for (size_t i = 0; i < 64 && i < size; i++) {
        if (sig[i] != 0) {
            has_non_zero = true;
            break;
        }
    }
    
    return has_non_zero;
}

// Validate reference integrity
static bool validate_reference_integrity(const void* data, size_t size) {
    if (!data || size == 0) return false;
    
    // Basic reference validation
    // In a real implementation, this would check for valid pointers,
    // circular references, orphaned references, etc.
    
    return true;
}

// Validate data consistency
static bool validate_data_consistency(const void* data, size_t size) {
    if (!data || size == 0) return false;
    
    // Basic consistency checks
    // In a real implementation, this would check for:
    // - Valid enum values
    // - Consistent field relationships
    // - Valid string terminators
    // - Reasonable value ranges
    
    return true;
}

// Validate performance metrics
static bool validate_performance_metrics(const void* data, size_t size) {
    if (!data || size == 0) return false;
    
    // Basic performance validation
    // In a real implementation, this would check for:
    // - Reasonable performance scores
    // - Valid timing values
    // - Consistent metrics
    
    return true;
}

// Validate lattice data
integrity_validation_result_t* integrity_validate_lattice(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size) {
    
    if (!validator || !lattice_data) return NULL;
    
    integrity_validation_result_t* result = malloc(sizeof(integrity_validation_result_t));
    if (!result) return NULL;
    
    // Initialize result
    memset(result, 0, sizeof(integrity_validation_result_t));
    result->passed = true;
    
    uint64_t start_time = get_timestamp_ms();
    
    // Level 1: Basic structure validation
    if (validator->level >= INTEGRITY_LEVEL_BASIC) {
        result->checks_performed++;
        if (!validate_data_structure(lattice_data, data_size)) {
            add_error_message(result, "Basic data structure validation failed");
            result->passed = false;
        }
    }
    
    // Level 2: Structural consistency
    if (validator->level >= INTEGRITY_LEVEL_STRUCTURAL) {
        result->checks_performed++;
        if (!validate_reference_integrity(lattice_data, data_size)) {
            add_error_message(result, "Reference integrity validation failed");
            result->passed = false;
        }
    }
    
    // Level 3: Semantic consistency
    if (validator->level >= INTEGRITY_LEVEL_SEMANTIC) {
        result->checks_performed++;
        if (!validate_data_consistency(lattice_data, data_size)) {
            add_warning_message(result, "Data consistency validation failed");
        }
    }
    
    // Level 4: Cryptographic validation
    if (validator->level >= INTEGRITY_LEVEL_CRYPTO) {
        result->checks_performed++;
        if (!validate_crypto_signature_internal(lattice_data, data_size)) {
            add_warning_message(result, "Cryptographic signature validation failed");
        }
    }
    
    // Level 5: Performance validation
    if (validator->level >= INTEGRITY_LEVEL_COMPLETE) {
        result->checks_performed++;
        if (!validate_performance_metrics(lattice_data, data_size)) {
            add_warning_message(result, "Performance metrics validation failed");
        }
    }
    
    result->validation_time_ms = get_timestamp_ms() - start_time;
    
    return result;
}

// Validate element data
integrity_validation_result_t* integrity_validate_element(
    integrity_validator_t* validator,
    void* element_data,
    size_t data_size) {
    
    if (!validator || !element_data) return NULL;
    
    integrity_validation_result_t* result = malloc(sizeof(integrity_validation_result_t));
    if (!result) return NULL;
    
    // Initialize result
    memset(result, 0, sizeof(integrity_validation_result_t));
    result->passed = true;
    
    uint64_t start_time = get_timestamp_ms();
    
    // Basic structure validation
    result->checks_performed++;
    if (!validate_data_structure(element_data, data_size)) {
        add_error_message(result, "Element data structure validation failed");
        result->passed = false;
    }
    
    // Element-specific validation
    if (data_size >= sizeof(uint32_t)) {
        uint32_t* atomic_number = (uint32_t*)element_data;
        if (*atomic_number == 0 || *atomic_number > 1000) {
            add_error_message(result, "Invalid atomic number");
            result->passed = false;
        }
    }
    
    result->validation_time_ms = get_timestamp_ms() - start_time;
    
    return result;
}

// Validate pattern data
integrity_validation_result_t* integrity_validate_pattern(
    integrity_validator_t* validator,
    void* pattern_data,
    size_t data_size) {
    
    if (!validator || !pattern_data) return NULL;
    
    integrity_validation_result_t* result = malloc(sizeof(integrity_validation_result_t));
    if (!result) return NULL;
    
    // Initialize result
    memset(result, 0, sizeof(integrity_validation_result_t));
    result->passed = true;
    
    uint64_t start_time = get_timestamp_ms();
    
    // Basic structure validation
    result->checks_performed++;
    if (!validate_data_structure(pattern_data, data_size)) {
        add_error_message(result, "Pattern data structure validation failed");
        result->passed = false;
    }
    
    // Pattern-specific validation
    if (data_size >= sizeof(uint32_t)) {
        uint32_t* pattern_id = (uint32_t*)pattern_data;
        if (*pattern_id == 0) {
            add_error_message(result, "Invalid pattern ID");
            result->passed = false;
        }
    }
    
    result->validation_time_ms = get_timestamp_ms() - start_time;
    
    return result;
}

// Validate crypto signature
integrity_validation_result_t* integrity_validate_crypto_signature(
    integrity_validator_t* validator,
    void* signature_data,
    size_t data_size) {
    
    if (!validator || !signature_data) return NULL;
    
    integrity_validation_result_t* result = malloc(sizeof(integrity_validation_result_t));
    if (!result) return NULL;
    
    // Initialize result
    memset(result, 0, sizeof(integrity_validation_result_t));
    result->passed = true;
    
    uint64_t start_time = get_timestamp_ms();
    
    // Crypto signature validation
    result->checks_performed++;
    if (!validate_crypto_signature_internal(signature_data, data_size)) {
        add_error_message(result, "Cryptographic signature validation failed");
        result->passed = false;
    }
    
    result->validation_time_ms = get_timestamp_ms() - start_time;
    
    return result;
}

// Repair functions (stub implementations)
int integrity_repair_lattice(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size) {
    (void)validator;
    (void)lattice_data;
    (void)data_size;
    return 0;
}

int integrity_repair_element(
    integrity_validator_t* validator,
    void* element_data,
    size_t data_size) {
    (void)validator;
    (void)element_data;
    (void)data_size;
    return 0;
}

int integrity_repair_pattern(
    integrity_validator_t* validator,
    void* pattern_data,
    size_t data_size) {
    (void)validator;
    (void)pattern_data;
    (void)data_size;
    return 0;
}

// Result management
void integrity_validation_result_destroy(integrity_validation_result_t* result) {
    if (!result) return;
    
    // Free error messages
    for (uint32_t i = 0; i < result->error_count; i++) {
        free(result->error_messages[i]);
    }
    
    // Free warning messages
    for (uint32_t i = 0; i < result->warning_count; i++) {
        free(result->warning_messages[i]);
    }
    
    free(result);
}

void integrity_print_validation_result(const integrity_validation_result_t* result) {
    if (!result) return;
    
    printf("=== Integrity Validation Result ===\n");
    printf("Status: %s\n", result->passed ? "PASS" : "FAIL");
    printf("Checks Performed: %u\n", result->checks_performed);
    printf("Checks Failed: %u\n", result->checks_failed);
    printf("Warnings: %u\n", result->warnings);
    printf("Errors: %u\n", result->errors);
    printf("Validation Time: %u ms\n", result->validation_time_ms);
    
    if (result->error_count > 0) {
        printf("\nErrors:\n");
        for (uint32_t i = 0; i < result->error_count; i++) {
            printf("  %u: %s\n", i + 1, result->error_messages[i]);
        }
    }
    
    if (result->warning_count > 0) {
        printf("\nWarnings:\n");
        for (uint32_t i = 0; i < result->warning_count; i++) {
            printf("  %u: %s\n", i + 1, result->warning_messages[i]);
        }
    }
}

bool integrity_validation_passed(const integrity_validation_result_t* result) {
    return result ? result->passed : false;
}

// Advanced validation functions
int integrity_validate_references(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size) {
    (void)validator;
    (void)lattice_data;
    (void)data_size;
    return 0;
}

int integrity_validate_consistency(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size) {
    (void)validator;
    (void)lattice_data;
    (void)data_size;
    return 0;
}

int integrity_validate_performance(
    integrity_validator_t* validator,
    void* lattice_data,
    size_t data_size) {
    (void)validator;
    (void)lattice_data;
    (void)data_size;
    return 0;
}

// Statistics
void integrity_get_validation_stats(
    integrity_validator_t* validator,
    uint32_t* total_validations,
    uint32_t* passed_validations,
    uint32_t* failed_validations,
    uint32_t* repair_attempts,
    uint32_t* successful_repairs) {
    (void)validator;
    if (total_validations) *total_validations = 0;
    if (passed_validations) *passed_validations = 0;
    if (failed_validations) *failed_validations = 0;
    if (repair_attempts) *repair_attempts = 0;
    if (successful_repairs) *successful_repairs = 0;
}
