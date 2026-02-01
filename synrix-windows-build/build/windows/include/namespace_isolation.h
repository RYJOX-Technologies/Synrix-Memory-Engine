/*
 * NAMESPACE ISOLATION & RIGIDITY FAILURE MODES
 * ===========================================
 * 
 * Handles failure modes of rigidity:
 * - Namespace isolation (prefix-based namespaces)
 * - Trust boundaries for noisy domains
 * - Noisy-domain quarantine (prevent garbage semantics from fossilizing)
 * - Prefix stability validation
 * 
 * Design Philosophy:
 * - Rigidity is the innovation, but it has edges
 * - Some domains have unstable prefix semantics
 * - Some domains have high polymorphism
 * - Some domains have adversarial naming (human input, noisy sensors)
 * - We need to protect the lattice from garbage semantics
 */

#ifndef NAMESPACE_ISOLATION_H
#define NAMESPACE_ISOLATION_H

#include "persistent_lattice.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// NAMESPACE DATA STRUCTURES
// ============================================================================

// Namespace definition
typedef struct {
    char namespace_id[64];        // Unique namespace identifier
    char prefix[64];              // Prefix for this namespace (e.g., "USER_INPUT_", "SENSOR_")
    double trust_level;           // Default trust level for this namespace
    bool is_quarantined;          // True if namespace is quarantined
    bool requires_validation;     // True if nodes require explicit validation
    uint32_t node_count;          // Number of nodes in this namespace
    uint32_t validation_failures; // Number of validation failures
    uint64_t created_timestamp;   // When namespace was created
    uint64_t last_quarantine_check; // Last time quarantine status was checked
} namespace_t;

// Prefix stability metrics
typedef struct {
    char prefix[64];              // Prefix being analyzed
    double stability_score;        // 0.0 (unstable) to 1.0 (stable)
    uint32_t distinct_meanings;    // Number of distinct semantic meanings
    double confidence_variance;    // Variance in confidence scores
    double age_variance;          // Variance in node ages
    bool is_stable;               // True if prefix is considered stable
    uint32_t recommendation;     // 0=ok, 1=monitor, 2=quarantine
} prefix_stability_t;

// Quarantine record
typedef struct {
    char namespace_id[64];        // Namespace being quarantined
    char reason[256];             // Why it was quarantined
    uint64_t quarantined_timestamp; // When it was quarantined
    double failure_rate;          // Failure rate that triggered quarantine
    bool is_active;               // True if quarantine is still active
} quarantine_record_t;

// ============================================================================
// NAMESPACE MANAGEMENT
// ============================================================================

/**
 * Create a new namespace.
 * 
 * Namespaces provide isolation for different domains, preventing
 * garbage semantics from contaminating the main lattice.
 * 
 * @param lattice The lattice to operate on
 * @param namespace_id Unique identifier for namespace
 * @param prefix Prefix for nodes in this namespace
 * @param default_trust Default trust level (0.0 to 1.0)
 * @return 0 on success, -1 on error
 */
int lattice_create_namespace(persistent_lattice_t* lattice,
                              const char* namespace_id,
                              const char* prefix,
                              double default_trust);

/**
 * Get namespace information.
 * 
 * @param lattice The lattice to query
 * @param namespace_id Namespace identifier
 * @param namespace Output structure for namespace info
 * @return 0 on success, -1 on error
 */
int lattice_get_namespace(persistent_lattice_t* lattice,
                          const char* namespace_id,
                          namespace_t* namespace);

/**
 * Check if a node belongs to a namespace.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to check
 * @param namespace_id Output buffer for namespace ID (if found)
 * @param namespace_id_size Size of output buffer
 * @return True if node belongs to a namespace
 */
bool lattice_node_belongs_to_namespace(persistent_lattice_t* lattice,
                                       uint64_t node_id,
                                       char* namespace_id,
                                       size_t namespace_id_size);

// ============================================================================
// TRUST BOUNDARIES FOR NOISY DOMAINS
// ============================================================================

/**
 * Set trust boundary for a namespace.
 * 
 * Noisy domains (user input, sensors) get lower trust boundaries
 * to prevent garbage semantics from fossilizing.
 * 
 * @param lattice The lattice to update
 * @param namespace_id Namespace identifier
 * @param min_trust_level Minimum trust level required
 * @param require_validation True if nodes require explicit validation
 * @return 0 on success, -1 on error
 */
int lattice_set_namespace_trust_boundary(persistent_lattice_t* lattice,
                                         const char* namespace_id,
                                         double min_trust_level,
                                         bool require_validation);

/**
 * Check if a node passes trust boundary for its namespace.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to check
 * @return True if node passes trust boundary
 */
bool lattice_node_passes_trust_boundary(persistent_lattice_t* lattice,
                                        uint64_t node_id);

// ============================================================================
// NOISY-DOMAIN QUARANTINE
// ============================================================================

/**
 * Quarantine a namespace (prevent new nodes from being added).
 * 
 * Quarantined namespaces are isolated to prevent garbage semantics
 * from contaminating the main lattice.
 * 
 * @param lattice The lattice to update
 * @param namespace_id Namespace to quarantine
 * @param reason Why it's being quarantined
 * @return 0 on success, -1 on error
 */
int lattice_quarantine_namespace(persistent_lattice_t* lattice,
                                const char* namespace_id,
                                const char* reason);

/**
 * Check if a namespace is quarantined.
 * 
 * @param lattice The lattice to query
 * @param namespace_id Namespace identifier
 * @return True if quarantined
 */
bool lattice_is_namespace_quarantined(persistent_lattice_t* lattice,
                                     const char* namespace_id);

/**
 * Release a namespace from quarantine.
 * 
 * @param lattice The lattice to update
 * @param namespace_id Namespace to release
 * @return 0 on success, -1 on error
 */
int lattice_release_namespace_quarantine(persistent_lattice_t* lattice,
                                         const char* namespace_id);

/**
 * Get quarantine record for a namespace.
 * 
 * @param lattice The lattice to query
 * @param namespace_id Namespace identifier
 * @param quarantine Output structure for quarantine record
 * @return 0 on success, -1 on error
 */
int lattice_get_quarantine_record(persistent_lattice_t* lattice,
                                  const char* namespace_id,
                                  quarantine_record_t* quarantine);

// ============================================================================
// PREFIX STABILITY VALIDATION
// ============================================================================

/**
 * Analyze prefix stability.
 * 
 * Unstable prefixes indicate semantic drift or namespace contamination.
 * 
 * @param lattice The lattice to analyze
 * @param prefix The prefix to analyze
 * @param result Output structure for stability metrics
 * @return 0 on success, -1 on error
 */
int lattice_analyze_prefix_stability(persistent_lattice_t* lattice,
                                     const char* prefix,
                                     prefix_stability_t* result);

/**
 * Validate prefix stability before adding node.
 * 
 * Prevents adding nodes to unstable prefixes that might indicate
 * namespace contamination or semantic drift.
 * 
 * @param lattice The lattice to check
 * @param prefix The prefix to validate
 * @return True if prefix is stable enough for new nodes
 */
bool lattice_validate_prefix_stability(persistent_lattice_t* lattice,
                                      const char* prefix);

/**
 * Get stability recommendation for a prefix.
 * 
 * @param lattice The lattice to analyze
 * @param prefix The prefix to check
 * @return 0=ok, 1=monitor, 2=quarantine
 */
uint32_t lattice_get_prefix_stability_recommendation(persistent_lattice_t* lattice,
                                                     const char* prefix);

// ============================================================================
// NAMESPACE-AWARE NODE OPERATIONS
// ============================================================================

/**
 * Add node with namespace validation.
 * 
 * Validates that the node can be added to its namespace based on
 * trust boundaries, quarantine status, and prefix stability.
 * 
 * @param lattice The lattice to add to
 * @param type Node type
 * @param name Node name (must match namespace prefix)
 * @param data Node data
 * @param namespace_id Namespace identifier (NULL for default)
 * @return Node ID on success, 0 on error
 */
uint64_t lattice_add_node_with_namespace(persistent_lattice_t* lattice,
                                         lattice_node_type_t type,
                                         const char* name,
                                         const char* data,
                                         const char* namespace_id);

/**
 * Query nodes within a namespace.
 * 
 * @param lattice The lattice to query
 * @param namespace_id Namespace identifier
 * @param node_ids Output array for node IDs
 * @param max_nodes Maximum number of nodes to return
 * @return Number of nodes found
 */
uint32_t lattice_query_namespace(persistent_lattice_t* lattice,
                                 const char* namespace_id,
                                 uint64_t* node_ids,
                                 uint32_t max_nodes);

#ifdef __cplusplus
}
#endif

#endif // NAMESPACE_ISOLATION_H

