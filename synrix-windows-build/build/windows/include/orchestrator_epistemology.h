/*
 * ORCHESTRATOR EPISTEMOLOGY
 * =========================
 * 
 * Handles trust, provenance, and conflict resolution for the orchestrator:
 * - Provenance tracking (where did this pattern come from? when? under what conditions?)
 * - Confidence aggregation (how to combine conflicting patterns?)
 * - Disagreement preservation (don't resolve conflicts—preserve them)
 * - Trust boundaries (some patterns are more trustworthy than others)
 * 
 * Design Philosophy:
 * - "Federated learning, but for symbolic laws"
 * - Preserve local truths alongside global patterns
 * - Track which patterns work in which contexts
 * - Don't force resolution of conflicts—let application layer decide
 */

#ifndef ORCHESTRATOR_EPISTEMOLOGY_H
#define ORCHESTRATOR_EPISTEMOLOGY_H

#include "persistent_lattice.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// PROVENANCE DATA STRUCTURES
// ============================================================================

// Source context for a pattern
typedef struct {
    char source_id[64];           // Unique identifier for source (e.g., "discovery_cell_1", "user_input", "simulation")
    char source_type[32];         // Type: "discovery", "user", "simulation", "synthesis", "import"
    uint64_t created_timestamp;   // When this pattern was created
    char context[128];            // Context where pattern was discovered (e.g., "Jetson_Orin_Nano", "x86_64")
    char conditions[256];        // Conditions under which pattern is valid (e.g., "ARM64 only", "temperature < 50C")
    double trust_level;           // Trust level for this source (0.0 to 1.0)
    uint32_t validation_count;    // Number of times this pattern was validated
    uint32_t failure_count;       // Number of times this pattern failed validation
} provenance_t;

// Conflict/disagreement record
typedef struct {
    uint64_t node_id_1;           // First conflicting node
    uint64_t node_id_2;           // Second conflicting node
    char conflict_type[32];       // "contradiction", "incompatible", "context_mismatch"
    char description[256];        // Description of the conflict
    double severity;              // 0.0 (minor) to 1.0 (severe)
    uint64_t detected_timestamp;  // When conflict was detected
    bool resolved;                // True if conflict was explicitly resolved
    char resolution_method[64];   // How conflict was resolved (if resolved)
} disagreement_t;

// Aggregated confidence result
typedef struct {
    double aggregated_confidence; // Combined confidence from multiple sources
    uint32_t source_count;        // Number of sources contributing
    double trust_weighted_sum;    // Sum of confidence * trust_level
    double trust_sum;             // Sum of trust levels
    bool has_conflicts;            // True if conflicting patterns exist
    uint32_t conflict_count;      // Number of conflicts detected
} aggregated_confidence_t;

// Trust boundary filter
typedef struct {
    double min_trust_level;        // Minimum trust level (0.0 to 1.0)
    char* context_filter;          // Context filter (NULL = all contexts)
    char* source_type_filter;      // Source type filter (NULL = all types)
    bool exclude_conflicts;        // Exclude nodes with active conflicts
    uint64_t min_validation_count; // Minimum validation count
} trust_boundary_t;

// ============================================================================
// PROVENANCE TRACKING
// ============================================================================

/**
 * Add a pattern with provenance information.
 * 
 * This stores not just the pattern, but where it came from, when,
 * and under what conditions it's valid.
 * 
 * @param lattice The lattice to add to
 * @param type Node type
 * @param name Node name
 * @param data Node data
 * @param provenance Provenance information
 * @return Node ID on success, 0 on error
 */
uint64_t lattice_add_pattern_with_provenance(persistent_lattice_t* lattice,
                                              lattice_node_type_t type,
                                              const char* name,
                                              const char* data,
                                              const provenance_t* provenance);

/**
 * Get provenance information for a node.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to get provenance for
 * @param provenance Output structure for provenance
 * @return 0 on success, -1 on error
 */
int lattice_get_provenance(persistent_lattice_t* lattice,
                           uint64_t node_id,
                           provenance_t* provenance);

/**
 * Update provenance trust level.
 * 
 * Adjusts trust level based on validation results.
 * 
 * @param lattice The lattice to update
 * @param node_id The node to update
 * @param validation_success True if validation succeeded
 * @return New trust level
 */
double lattice_update_provenance_trust(persistent_lattice_t* lattice,
                                       uint64_t node_id,
                                       bool validation_success);

// ============================================================================
// CONFIDENCE AGGREGATION
// ============================================================================

/**
 * Aggregate confidence from multiple conflicting patterns.
 * 
 * Combines confidence scores from multiple sources using trust-weighted
 * aggregation. Preserves conflicts rather than resolving them.
 * 
 * @param lattice The lattice to query
 * @param node_ids Array of node IDs to aggregate
 * @param node_count Number of nodes
 * @param result Output structure for aggregated confidence
 * @return 0 on success, -1 on error
 */
int lattice_aggregate_confidence(persistent_lattice_t* lattice,
                                 const uint64_t* node_ids,
                                 uint32_t node_count,
                                 aggregated_confidence_t* result);

/**
 * Aggregate confidence for a prefix query.
 * 
 * Finds all nodes with a given prefix and aggregates their confidence
 * scores, detecting conflicts in the process.
 * 
 * @param lattice The lattice to query
 * @param prefix The prefix to search for
 * @param result Output structure for aggregated confidence
 * @return 0 on success, -1 on error
 */
int lattice_aggregate_confidence_by_prefix(persistent_lattice_t* lattice,
                                           const char* prefix,
                                           aggregated_confidence_t* result);

// ============================================================================
// DISAGREEMENT PRESERVATION
// ============================================================================

/**
 * Detect disagreements between nodes.
 * 
 * Identifies conflicting patterns without resolving them.
 * 
 * @param lattice The lattice to analyze
 * @param node_id_1 First node to compare
 * @param node_id_2 Second node to compare
 * @param disagreement Output structure for disagreement
 * @return 0 if no disagreement, 1 if disagreement found, -1 on error
 */
int lattice_detect_disagreement(persistent_lattice_t* lattice,
                                uint64_t node_id_1,
                                uint64_t node_id_2,
                                disagreement_t* disagreement);

/**
 * Record a disagreement (preserve conflict).
 * 
 * Stores disagreement information without resolving it.
 * 
 * @param lattice The lattice to update
 * @param disagreement Disagreement to record
 * @return 0 on success, -1 on error
 */
int lattice_record_disagreement(persistent_lattice_t* lattice,
                                const disagreement_t* disagreement);

/**
 * Get all disagreements for a node.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to get disagreements for
 * @param disagreements Output array for disagreements
 * @param max_disagreements Maximum number to return
 * @return Number of disagreements found
 */
uint32_t lattice_get_disagreements(persistent_lattice_t* lattice,
                                   uint64_t node_id,
                                   disagreement_t* disagreements,
                                   uint32_t max_disagreements);

/**
 * Mark a disagreement as resolved (but preserve the record).
 * 
 * @param lattice The lattice to update
 * @param node_id_1 First node in disagreement
 * @param node_id_2 Second node in disagreement
 * @param resolution_method How the conflict was resolved
 * @return 0 on success, -1 on error
 */
int lattice_resolve_disagreement(persistent_lattice_t* lattice,
                                 uint64_t node_id_1,
                                 uint64_t node_id_2,
                                 const char* resolution_method);

// ============================================================================
// TRUST BOUNDARIES
// ============================================================================

/**
 * Query with trust boundary filter.
 * 
 * Only returns nodes that meet trust criteria.
 * 
 * @param lattice The lattice to query
 * @param prefix The prefix to search for
 * @param trust_boundary Trust boundary filter
 * @param node_ids Output array for node IDs
 * @param max_nodes Maximum number of nodes to return
 * @return Number of nodes found
 */
uint32_t lattice_query_with_trust_boundary(persistent_lattice_t* lattice,
                                           const char* prefix,
                                           const trust_boundary_t* trust_boundary,
                                           uint64_t* node_ids,
                                           uint32_t max_nodes);

/**
 * Get trust level for a node.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to check
 * @return Trust level (0.0 to 1.0), or -1.0 on error
 */
double lattice_get_trust_level(persistent_lattice_t* lattice, uint64_t node_id);

/**
 * Set trust level for a node.
 * 
 * @param lattice The lattice to update
 * @param node_id The node to update
 * @param trust_level New trust level (0.0 to 1.0)
 * @return 0 on success, -1 on error
 */
int lattice_set_trust_level(persistent_lattice_t* lattice,
                            uint64_t node_id,
                            double trust_level);

// ============================================================================
// CONTEXT-AWARE QUERIES
// ============================================================================

/**
 * Query patterns valid for a specific context.
 * 
 * Returns only patterns that are valid in the given context.
 * 
 * @param lattice The lattice to query
 * @param prefix The prefix to search for
 * @param context The context to filter by (e.g., "Jetson_Orin_Nano")
 * @param node_ids Output array for node IDs
 * @param max_nodes Maximum number of nodes to return
 * @return Number of nodes found
 */
uint32_t lattice_query_by_context(persistent_lattice_t* lattice,
                                  const char* prefix,
                                  const char* context,
                                  uint64_t* node_ids,
                                  uint32_t max_nodes);

/**
 * Check if a pattern is valid in a given context.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to check
 * @param context The context to validate against
 * @return True if valid, false otherwise
 */
bool lattice_is_valid_in_context(persistent_lattice_t* lattice,
                                 uint64_t node_id,
                                 const char* context);

#ifdef __cplusplus
}
#endif

#endif // ORCHESTRATOR_EPISTEMOLOGY_H

