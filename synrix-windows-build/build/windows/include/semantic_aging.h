/*
 * SEMANTIC AGING MODEL
 * ====================
 * 
 * Handles semantic drift over long time horizons:
 * - Concept versioning (track evolution)
 * - Historical anchoring (mark outdated, preserve history)
 * - Drift detection (monitor prefix ambiguity, confidence anomalies)
 * - Semantic half-life (concepts decay in relevance over time)
 * 
 * This is NOT data aging—it's semantic aging. Concepts can become
 * outdated, ambiguous, or superseded without being deleted.
 * 
 * Design Philosophy:
 * - Don't delete, mark as historical
 * - Preserve lineage for context
 * - Enable rollback to previous semantic states
 * - Detect drift before it becomes a problem
 */

#ifndef SEMANTIC_AGING_H
#define SEMANTIC_AGING_H

#include "persistent_lattice.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// SEMANTIC AGING DATA STRUCTURES
// ============================================================================

// Concept version information
typedef struct {
    uint64_t node_id;              // Current node ID
    uint64_t previous_version_id;  // Previous version (0 if first version)
    uint64_t superseded_by_id;      // Superseded by this node (0 if current)
    uint64_t created_timestamp;     // When this version was created
    uint64_t superseded_timestamp;  // When this version was superseded (0 if current)
    char reason[128];               // Why this version was created/superseded
    uint32_t version_number;        // Version number (1, 2, 3, ...)
    bool is_historical;            // True if this is a historical version
} concept_version_t;

// Semantic drift detection result
typedef struct {
    char prefix[64];               // Prefix being analyzed
    uint32_t ambiguity_count;      // Number of distinct meanings
    double confidence_variance;     // Variance in confidence scores
    double age_variance;            // Variance in node ages
    bool has_drift;                 // True if drift detected
    char drift_type[32];           // "ambiguity", "confidence", "age", "combined"
    double drift_severity;          // 0.0 (none) to 1.0 (severe)
} drift_detection_t;

// Semantic half-life calculation result
typedef struct {
    char prefix[64];               // Prefix being analyzed
    double half_life_days;         // Days until confidence decays to 50%
    double current_confidence;      // Current average confidence
    double decay_rate;              // Confidence decay per day
    uint32_t node_count;            // Number of nodes with this prefix
    uint64_t oldest_timestamp;      // Oldest node timestamp
    uint64_t newest_timestamp;      // Newest node timestamp
} semantic_half_life_t;

// ============================================================================
// CONCEPT VERSIONING
// ============================================================================

/**
 * Mark a concept as historical (superseded by a newer version).
 * 
 * This preserves the old concept in the lattice but marks it as historical,
 * allowing queries to prefer newer versions while maintaining lineage.
 * 
 * @param lattice The lattice to operate on
 * @param node_id The node to mark as historical
 * @param superseded_by_id The node that supersedes this one (0 if just marking as outdated)
 * @param reason Why this concept was superseded
 * @return 0 on success, -1 on error
 */
int lattice_mark_concept_historical(persistent_lattice_t* lattice, 
                                     uint64_t node_id, 
                                     uint64_t superseded_by_id,
                                     const char* reason);

/**
 * Get the version history (lineage) for a concept.
 * 
 * Returns an array of version information, starting with the current version
 * and tracing back through previous versions.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to get lineage for
 * @param versions Output array for version information
 * @param max_versions Maximum number of versions to return
 * @return Number of versions found
 */
uint32_t lattice_get_concept_lineage(persistent_lattice_t* lattice,
                                     uint64_t node_id,
                                     concept_version_t* versions,
                                     uint32_t max_versions);

/**
 * Create a new version of a concept (supersedes previous version).
 * 
 * This creates a new node with the same semantic prefix but updated content,
 * and marks the previous version as historical.
 * 
 * @param lattice The lattice to operate on
 * @param previous_node_id The previous version to supersede
 * @param type Node type for new version
 * @param name Node name (should have same prefix as previous)
 * @param data Node data (updated content)
 * @param reason Why this new version was created
 * @return New node ID on success, 0 on error
 */
uint64_t lattice_create_concept_version(persistent_lattice_t* lattice,
                                        uint64_t previous_node_id,
                                        lattice_node_type_t type,
                                        const char* name,
                                        const char* data,
                                        const char* reason);

// ============================================================================
// SEMANTIC HALF-LIFE
// ============================================================================

/**
 * Calculate semantic half-life for a prefix.
 * 
 * Semantic half-life is the time (in days) until the average confidence
 * of nodes with this prefix decays to 50% of its current value.
 * 
 * This helps identify prefixes that are becoming less relevant over time.
 * 
 * @param lattice The lattice to analyze
 * @param prefix The prefix to analyze (e.g., "ISA_", "PATTERN_")
 * @param result Output structure for half-life calculation
 * @return 0 on success, -1 on error
 */
int lattice_calculate_semantic_half_life(persistent_lattice_t* lattice,
                                          const char* prefix,
                                          semantic_half_life_t* result);

/**
 * Apply semantic decay to a node's confidence based on age.
 * 
 * Older nodes naturally decay in confidence unless they're actively used.
 * This prevents "survivorship bias" where old successful patterns appear
 * more confident than they should be.
 * 
 * @param lattice The lattice to operate on
 * @param node_id The node to apply decay to
 * @param decay_rate Confidence decay per day (e.g., 0.001 = 0.1% per day)
 * @return New confidence score after decay
 */
double lattice_apply_semantic_decay(persistent_lattice_t* lattice,
                                    uint64_t node_id,
                                    double decay_rate);

// ============================================================================
// DRIFT DETECTION
// ============================================================================

/**
 * Detect semantic drift for a prefix.
 * 
 * Drift can manifest as:
 * - Prefix ambiguity: Same prefix, diverging meanings
 * - Confidence anomalies: Unexpected confidence patterns
 * - Age variance: Nodes created at very different times
 * 
 * @param lattice The lattice to analyze
 * @param prefix The prefix to analyze
 * @param time_window_days Time window for analysis (0 = all time)
 * @param result Output structure for drift detection
 * @return 0 on success, -1 on error
 */
int lattice_detect_semantic_drift(persistent_lattice_t* lattice,
                                   const char* prefix,
                                   uint32_t time_window_days,
                                   drift_detection_t* result);

/**
 * Detect prefix ambiguity (same prefix, multiple distinct meanings).
 * 
 * This is a specific type of drift where a prefix is being used for
 * semantically different concepts.
 * 
 * @param lattice The lattice to analyze
 * @param prefix The prefix to check
 * @param ambiguity_threshold Minimum number of distinct meanings to flag
 * @return Number of distinct meanings found
 */
uint32_t lattice_detect_prefix_ambiguity(persistent_lattice_t* lattice,
                                         const char* prefix,
                                         uint32_t ambiguity_threshold);

/**
 * Detect confidence score anomalies.
 * 
 * Flags nodes with confidence scores that are unexpectedly high or low
 * given their age and usage patterns.
 * 
 * @param lattice The lattice to analyze
 * @param prefix The prefix to analyze (NULL for all prefixes)
 * @param anomaly_node_ids Output array for anomalous node IDs
 * @param max_anomalies Maximum number of anomalies to return
 * @return Number of anomalies found
 */
uint32_t lattice_detect_confidence_anomalies(persistent_lattice_t* lattice,
                                             const char* prefix,
                                             uint64_t* anomaly_node_ids,
                                             uint32_t max_anomalies);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Check if a node is historical (superseded).
 * 
 * @param lattice The lattice to query
 * @param node_id The node to check
 * @return True if historical, false if current
 */
bool lattice_is_node_historical(persistent_lattice_t* lattice, uint64_t node_id);

/**
 * Get the current (non-historical) version of a concept.
 * 
 * If a node has been superseded, this returns the latest version.
 * 
 * @param lattice The lattice to query
 * @param node_id The node to get current version for
 * @return Current node ID, or 0 if not found
 */
uint64_t lattice_get_current_version(persistent_lattice_t* lattice, uint64_t node_id);

/**
 * Get all historical versions of a concept.
 * 
 * @param lattice The lattice to query
 * @param current_node_id The current version node ID
 * @param historical_ids Output array for historical node IDs
 * @param max_historical Maximum number of historical IDs to return
 * @return Number of historical versions found
 */
uint32_t lattice_get_historical_versions(persistent_lattice_t* lattice,
                                         uint64_t current_node_id,
                                         uint64_t* historical_ids,
                                         uint32_t max_historical);

#ifdef __cplusplus
}
#endif

#endif // SEMANTIC_AGING_H

