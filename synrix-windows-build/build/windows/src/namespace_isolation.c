/*
 * NAMESPACE ISOLATION - IMPLEMENTATION
 * ====================================
 * 
 * Implements failure modes of rigidity protection:
 * - Namespace isolation
 * - Trust boundaries
 * - Noisy-domain quarantine
 * - Prefix stability validation
 */

#include "namespace_isolation.h"
#include "semantic_aging.h"
#include "orchestrator_epistemology.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

// Get current timestamp in microseconds
static uint64_t get_current_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Find node by ID (helper)
static lattice_node_t* find_node_by_id(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return NULL;
    
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        uint32_t index = lattice->id_to_index_map[local_id];
        if (index < lattice->node_count && lattice->nodes[index].id == node_id) {
            return &lattice->nodes[index];
        }
    }
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->nodes[i].id == node_id) {
            return &lattice->nodes[i];
        }
    }
    
    return NULL;
}

// Serialize namespace to node data
static void serialize_namespace(const namespace_t* ns, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "NAMESPACE:id=%s:prefix=%s:trust=%.6f:quarantined=%d:requires_validation=%d:node_count=%u:failures=%u:created=%lu:last_check=%lu",
             ns->namespace_id, ns->prefix, ns->trust_level,
             ns->is_quarantined ? 1 : 0, ns->requires_validation ? 1 : 0,
             ns->node_count, ns->validation_failures,
             ns->created_timestamp, ns->last_quarantine_check);
}

// Deserialize namespace from node data
static bool deserialize_namespace(const char* data, namespace_t* ns) {
    if (!data || !ns) return false;
    
    memset(ns, 0, sizeof(namespace_t));
    
    if (strncmp(data, "NAMESPACE:", 10) != 0) {
        return false;
    }
    
    int quarantined_int = 0, requires_validation_int = 0;
    sscanf(data,
           "NAMESPACE:id=%63[^:]:prefix=%63[^:]:trust=%lf:quarantined=%d:requires_validation=%d:node_count=%u:failures=%u:created=%lu:last_check=%lu",
           ns->namespace_id, ns->prefix, &ns->trust_level,
           &quarantined_int, &requires_validation_int,
           &ns->node_count, &ns->validation_failures,
           &ns->created_timestamp, &ns->last_quarantine_check);
    
    ns->is_quarantined = (quarantined_int != 0);
    ns->requires_validation = (requires_validation_int != 0);
    
    return true;
}

// Serialize quarantine record
static void serialize_quarantine(const quarantine_record_t* q, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "QUARANTINE:namespace=%s:reason=%s:timestamp=%lu:failure_rate=%.6f:active=%d",
             q->namespace_id, q->reason, q->quarantined_timestamp,
             q->failure_rate, q->is_active ? 1 : 0);
}

// Deserialize quarantine record
static bool deserialize_quarantine(const char* data, quarantine_record_t* q) {
    if (!data || !q) return false;
    
    memset(q, 0, sizeof(quarantine_record_t));
    
    if (strncmp(data, "QUARANTINE:", 11) != 0) {
        return false;
    }
    
    int active_int = 0;
    sscanf(data,
           "QUARANTINE:namespace=%63[^:]:reason=%255[^:]:timestamp=%lu:failure_rate=%lf:active=%d",
           q->namespace_id, q->reason, &q->quarantined_timestamp,
           &q->failure_rate, &active_int);
    
    q->is_active = (active_int != 0);
    
    return true;
}

// Find namespace node by ID
static lattice_node_t* find_namespace_node(persistent_lattice_t* lattice, const char* namespace_id) {
    if (!lattice || !namespace_id) return NULL;
    
    char ns_name[128];
    snprintf(ns_name, sizeof(ns_name), "NAMESPACE:%s", namespace_id);
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (strcmp(node->name, ns_name) == 0) {
            return node;
        }
    }
    
    return NULL;
}

// ============================================================================
// NAMESPACE MANAGEMENT
// ============================================================================

int lattice_create_namespace(persistent_lattice_t* lattice,
                              const char* namespace_id,
                              const char* prefix,
                              double default_trust) {
    if (!lattice || !namespace_id || !prefix) return -1;
    
    if (default_trust < 0.0 || default_trust > 1.0) return -1;
    
    // Check if namespace already exists
    if (find_namespace_node(lattice, namespace_id) != NULL) {
        fprintf(stderr, "[NAMESPACE] WARN Namespace '%s' already exists\n", namespace_id);
        return -1;
    }
    
    // Create namespace node
    namespace_t ns;
    memset(&ns, 0, sizeof(namespace_t));
    strncpy(ns.namespace_id, namespace_id, sizeof(ns.namespace_id) - 1);
    strncpy(ns.prefix, prefix, sizeof(ns.prefix) - 1);
    ns.trust_level = default_trust;
    ns.is_quarantined = false;
    ns.requires_validation = (default_trust < 0.5); // Low trust = requires validation
    ns.node_count = 0;
    ns.validation_failures = 0;
    ns.created_timestamp = get_current_timestamp_us();
    ns.last_quarantine_check = ns.created_timestamp;
    
    char ns_data[512];
    serialize_namespace(&ns, ns_data, sizeof(ns_data));
    
    char ns_name[128];
    snprintf(ns_name, sizeof(ns_name), "NAMESPACE:%s", namespace_id);
    
    uint64_t ns_node_id = lattice_add_node(lattice, LATTICE_NODE_CPT_METADATA,
                                           ns_name, ns_data, 0);
    
    if (ns_node_id != 0) {
    printf("[NAMESPACE] OK Created namespace '%s' (prefix: %s, trust: %.3f)\n",
               namespace_id, prefix, default_trust);
        return 0;
    }
    
    return -1;
}

int lattice_get_namespace(persistent_lattice_t* lattice,
                          const char* namespace_id,
                          namespace_t* namespace) {
    if (!lattice || !namespace_id || !namespace) return -1;
    
    lattice_node_t* ns_node = find_namespace_node(lattice, namespace_id);
    if (!ns_node) return -1;
    
    return deserialize_namespace(ns_node->data, namespace) ? 0 : -1;
}

bool lattice_node_belongs_to_namespace(persistent_lattice_t* lattice,
                                       uint64_t node_id,
                                       char* namespace_id,
                                       size_t namespace_id_size) {
    if (!lattice || !namespace_id || namespace_id_size == 0) return false;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return false;
    
    // Check all namespaces to see if node's prefix matches
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* ns_node = &lattice->nodes[i];
        if (strncmp(ns_node->name, "NAMESPACE:", 10) != 0) continue;
        
        namespace_t ns;
        if (deserialize_namespace(ns_node->data, &ns)) {
            // Check if node name starts with namespace prefix
            if (strncmp(node->name, ns.prefix, strlen(ns.prefix)) == 0) {
                strncpy(namespace_id, ns.namespace_id, namespace_id_size - 1);
                namespace_id[namespace_id_size - 1] = '\0';
                return true;
            }
        }
    }
    
    return false;
}

// ============================================================================
// TRUST BOUNDARIES
// ============================================================================

int lattice_set_namespace_trust_boundary(persistent_lattice_t* lattice,
                                         const char* namespace_id,
                                         double min_trust_level,
                                         bool require_validation) {
    if (!lattice || !namespace_id) return -1;
    
    if (min_trust_level < 0.0 || min_trust_level > 1.0) return -1;
    
    lattice_node_t* ns_node = find_namespace_node(lattice, namespace_id);
    if (!ns_node) return -1;
    
    namespace_t ns;
    if (!deserialize_namespace(ns_node->data, &ns)) return -1;
    
    ns.trust_level = min_trust_level;
    ns.requires_validation = require_validation;
    
    char ns_data[512];
    serialize_namespace(&ns, ns_data, sizeof(ns_data));
    strncpy(ns_node->data, ns_data, sizeof(ns_node->data) - 1);
    
    lattice->dirty = true;
    
    printf("[NAMESPACE] OK Updated trust boundary for '%s': min_trust=%.3f, requires_validation=%d\n",
           namespace_id, min_trust_level, require_validation);
    
    return 0;
}

bool lattice_node_passes_trust_boundary(persistent_lattice_t* lattice,
                                        uint64_t node_id) {
    if (!lattice) return false;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return false;
    
    // Find namespace for this node
    char namespace_id[64];
    if (!lattice_node_belongs_to_namespace(lattice, node_id, namespace_id, sizeof(namespace_id))) {
        // No namespace = default trust (allow)
        return true;
    }
    
    namespace_t ns;
    if (lattice_get_namespace(lattice, namespace_id, &ns) != 0) {
        return true; // Namespace not found = allow
    }
    
    // Check if node passes trust boundary
    double node_trust = lattice_get_trust_level(lattice, node_id);
    if (node_trust < 0.0) {
        node_trust = ns.trust_level; // Use namespace default
    }
    
    return (node_trust >= ns.trust_level);
}

// ============================================================================
// NOISY-DOMAIN QUARANTINE
// ============================================================================

int lattice_quarantine_namespace(persistent_lattice_t* lattice,
                                const char* namespace_id,
                                const char* reason) {
    if (!lattice || !namespace_id || !reason) return -1;
    
    lattice_node_t* ns_node = find_namespace_node(lattice, namespace_id);
    if (!ns_node) return -1;
    
    namespace_t ns;
    if (!deserialize_namespace(ns_node->data, &ns)) return -1;
    
    ns.is_quarantined = true;
    ns.last_quarantine_check = get_current_timestamp_us();
    
    // Update namespace node
    char ns_data[512];
    serialize_namespace(&ns, ns_data, sizeof(ns_data));
    strncpy(ns_node->data, ns_data, sizeof(ns_node->data) - 1);
    
    // Create quarantine record
    quarantine_record_t q;
    memset(&q, 0, sizeof(quarantine_record_t));
    strncpy(q.namespace_id, namespace_id, sizeof(q.namespace_id) - 1);
    strncpy(q.reason, reason, sizeof(q.reason) - 1);
    q.quarantined_timestamp = get_current_timestamp_us();
    q.failure_rate = (ns.node_count > 0) ? 
                     ((double)ns.validation_failures / (double)ns.node_count) : 0.0;
    q.is_active = true;
    
    char q_data[512];
    serialize_quarantine(&q, q_data, sizeof(q_data));
    
    char q_name[128];
    snprintf(q_name, sizeof(q_name), "QUARANTINE:%s", namespace_id);
    
    lattice_add_node(lattice, LATTICE_NODE_CPT_METADATA, q_name, q_data, 0);
    
    lattice->dirty = true;
    
    printf("[NAMESPACE] WARN Quarantined namespace '%s': %s (failure_rate: %.3f)\n",
           namespace_id, reason, q.failure_rate);
    
    return 0;
}

bool lattice_is_namespace_quarantined(persistent_lattice_t* lattice,
                                     const char* namespace_id) {
    if (!lattice || !namespace_id) return false;
    
    namespace_t ns;
    if (lattice_get_namespace(lattice, namespace_id, &ns) != 0) {
        return false;
    }
    
    return ns.is_quarantined;
}

int lattice_release_namespace_quarantine(persistent_lattice_t* lattice,
                                         const char* namespace_id) {
    if (!lattice || !namespace_id) return -1;
    
    lattice_node_t* ns_node = find_namespace_node(lattice, namespace_id);
    if (!ns_node) return -1;
    
    namespace_t ns;
    if (!deserialize_namespace(ns_node->data, &ns)) return -1;
    
    ns.is_quarantined = false;
    ns.last_quarantine_check = get_current_timestamp_us();
    
    char ns_data[512];
    serialize_namespace(&ns, ns_data, sizeof(ns_data));
    strncpy(ns_node->data, ns_data, sizeof(ns_node->data) - 1);
    
    // Mark quarantine record as inactive
    char q_name[128];
    snprintf(q_name, sizeof(q_name), "QUARANTINE:%s", namespace_id);
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* q_node = &lattice->nodes[i];
        if (strcmp(q_node->name, q_name) == 0) {
            quarantine_record_t q;
            if (deserialize_quarantine(q_node->data, &q)) {
                q.is_active = false;
                char q_data[512];
                serialize_quarantine(&q, q_data, sizeof(q_data));
                strncpy(q_node->data, q_data, sizeof(q_node->data) - 1);
            }
            break;
        }
    }
    
    lattice->dirty = true;
    
    printf("[NAMESPACE] OK Released namespace '%s' from quarantine\n", namespace_id);
    
    return 0;
}

int lattice_get_quarantine_record(persistent_lattice_t* lattice,
                                  const char* namespace_id,
                                  quarantine_record_t* quarantine) {
    if (!lattice || !namespace_id || !quarantine) return -1;
    
    char q_name[128];
    snprintf(q_name, sizeof(q_name), "QUARANTINE:%s", namespace_id);
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* q_node = &lattice->nodes[i];
        if (strcmp(q_node->name, q_name) == 0) {
            return deserialize_quarantine(q_node->data, quarantine) ? 0 : -1;
        }
    }
    
    return -1;
}

// ============================================================================
// PREFIX STABILITY VALIDATION
// ============================================================================

int lattice_analyze_prefix_stability(persistent_lattice_t* lattice,
                                     const char* prefix,
                                     prefix_stability_t* result) {
    if (!lattice || !prefix || !result) return -1;
    
    memset(result, 0, sizeof(prefix_stability_t));
    strncpy(result->prefix, prefix, sizeof(result->prefix) - 1);
    
    // Use semantic drift detection to analyze stability
    drift_detection_t drift;
    if (lattice_detect_semantic_drift(lattice, prefix, 0, &drift) == 0) {
        result->confidence_variance = drift.confidence_variance;
        result->age_variance = drift.age_variance;
        result->distinct_meanings = drift.ambiguity_count;
        
        // Calculate stability score (inverse of drift severity)
        result->stability_score = 1.0 - drift.drift_severity;
        result->is_stable = (result->stability_score > 0.7);
        
        // Generate recommendation
        if (result->stability_score > 0.8) {
            result->recommendation = 0; // OK
        } else if (result->stability_score > 0.5) {
            result->recommendation = 1; // Monitor
        } else {
            result->recommendation = 2; // Quarantine
        }
    } else {
        // No nodes found or error
        result->stability_score = 1.0; // Assume stable if no data
        result->is_stable = true;
        result->recommendation = 0;
    }
    
    return 0;
}

bool lattice_validate_prefix_stability(persistent_lattice_t* lattice,
                                      const char* prefix) {
    if (!lattice || !prefix) return false;
    
    prefix_stability_t stability;
    if (lattice_analyze_prefix_stability(lattice, prefix, &stability) != 0) {
        return true; // Assume stable if can't analyze
    }
    
    // Reject if stability is too low
    return (stability.stability_score > 0.5);
}

uint32_t lattice_get_prefix_stability_recommendation(persistent_lattice_t* lattice,
                                                     const char* prefix) {
    if (!lattice || !prefix) return 0;
    
    prefix_stability_t stability;
    if (lattice_analyze_prefix_stability(lattice, prefix, &stability) != 0) {
        return 0; // OK if can't analyze
    }
    
    return stability.recommendation;
}

// ============================================================================
// NAMESPACE-AWARE NODE OPERATIONS
// ============================================================================

uint64_t lattice_add_node_with_namespace(persistent_lattice_t* lattice,
                                         lattice_node_type_t type,
                                         const char* name,
                                         const char* data,
                                         const char* namespace_id) {
    if (!lattice || !name || !data) return 0;
    
    // If namespace specified, validate it
    if (namespace_id) {
        // Check if namespace is quarantined
        if (lattice_is_namespace_quarantined(lattice, namespace_id)) {
            fprintf(stderr, "[NAMESPACE] ERROR Cannot add node to quarantined namespace '%s'\n",
                    namespace_id);
            return 0;
        }
        
        // Get namespace to check prefix
        namespace_t ns;
        if (lattice_get_namespace(lattice, namespace_id, &ns) == 0) {
            // Verify name matches namespace prefix
            if (strncmp(name, ns.prefix, strlen(ns.prefix)) != 0) {
                fprintf(stderr, "[NAMESPACE] ERROR Node name '%s' does not match namespace prefix '%s'\n",
                        name, ns.prefix);
                return 0;
            }
            
            // Check prefix stability
            if (!lattice_validate_prefix_stability(lattice, ns.prefix)) {
                fprintf(stderr, "[NAMESPACE] WARN Prefix '%s' is unstable, rejecting node\n",
                        ns.prefix);
                return 0;
            }
        }
    }
    
    // Add node normally
    uint64_t node_id = lattice_add_node(lattice, type, name, data, 0);
    
    if (node_id != 0 && namespace_id) {
        // Update namespace node count
        namespace_t ns;
        if (lattice_get_namespace(lattice, namespace_id, &ns) == 0) {
            ns.node_count++;
            ns.last_quarantine_check = get_current_timestamp_us();
            
            lattice_node_t* ns_node = find_namespace_node(lattice, namespace_id);
            if (ns_node) {
                char ns_data[512];
                serialize_namespace(&ns, ns_data, sizeof(ns_data));
                strncpy(ns_node->data, ns_data, sizeof(ns_node->data) - 1);
                lattice->dirty = true;
            }
        }
    }
    
    return node_id;
}

uint32_t lattice_query_namespace(persistent_lattice_t* lattice,
                                 const char* namespace_id,
                                 uint64_t* node_ids,
                                 uint32_t max_nodes) {
    if (!lattice || !namespace_id || !node_ids || max_nodes == 0) {
        return 0;
    }
    
    namespace_t ns;
    if (lattice_get_namespace(lattice, namespace_id, &ns) != 0) {
        return 0;
    }
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && count < max_nodes; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip historical nodes
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        
        // Check if node matches namespace prefix
        if (strncmp(node->name, ns.prefix, strlen(ns.prefix)) == 0) {
            node_ids[count++] = node->id;
        }
    }
    
    return count;
}

