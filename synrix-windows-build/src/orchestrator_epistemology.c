/*
 * ORCHESTRATOR EPISTEMOLOGY - IMPLEMENTATION
 * ==========================================
 * 
 * Implements trust, provenance, and conflict resolution for the orchestrator.
 */

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
    
    // Use reverse index for O(1) lookup
    uint32_t local_id = (uint32_t)(node_id & 0xFFFFFFFF);
    if (lattice->id_to_index_map && local_id < lattice->max_nodes * 10) {
        uint32_t index = lattice->id_to_index_map[local_id];
        if (index < lattice->node_count && lattice->nodes[index].id == node_id) {
            return &lattice->nodes[index];
        }
    }
    
    // Fallback: linear search
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        if (lattice->nodes[i].id == node_id) {
            return &lattice->nodes[i];
        }
    }
    
    return NULL;
}

// Serialize provenance to node data
static void serialize_provenance(const provenance_t* prov, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "PROVENANCE:source_id=%s:source_type=%s:created=%lu:context=%s:conditions=%s:trust=%.6f:validations=%u:failures=%u",
             prov->source_id, prov->source_type, prov->created_timestamp,
             prov->context, prov->conditions, prov->trust_level,
             prov->validation_count, prov->failure_count);
}

// Deserialize provenance from node data
static bool deserialize_provenance(const char* data, provenance_t* prov) {
    if (!data || !prov) return false;
    
    memset(prov, 0, sizeof(provenance_t));
    
    // Check for provenance marker
    if (strncmp(data, "PROVENANCE:", 11) != 0) {
        return false;
    }
    
    // Parse provenance fields
    sscanf(data,
           "PROVENANCE:source_id=%63[^:]:source_type=%31[^:]:created=%lu:context=%127[^:]:conditions=%255[^:]:trust=%lf:validations=%u:failures=%u",
           prov->source_id, prov->source_type, &prov->created_timestamp,
           prov->context, prov->conditions, &prov->trust_level,
           &prov->validation_count, &prov->failure_count);
    
    return true;
}

// Serialize disagreement to node data
static void serialize_disagreement(const disagreement_t* dis, char* buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "DISAGREEMENT:node1=%lu:node2=%lu:type=%s:desc=%s:severity=%.6f:detected=%lu:resolved=%d:resolution=%s",
             dis->node_id_1, dis->node_id_2, dis->conflict_type,
             dis->description, dis->severity, dis->detected_timestamp,
             dis->resolved ? 1 : 0, dis->resolution_method);
}

// Deserialize disagreement from node data
static bool deserialize_disagreement(const char* data, disagreement_t* dis) {
    if (!data || !dis) return false;
    
    memset(dis, 0, sizeof(disagreement_t));
    
    if (strncmp(data, "DISAGREEMENT:", 13) != 0) {
        return false;
    }
    
    int resolved_int = 0;
    sscanf(data,
           "DISAGREEMENT:node1=%lu:node2=%lu:type=%31[^:]:desc=%255[^:]:severity=%lf:detected=%lu:resolved=%d:resolution=%63[^:]",
           &dis->node_id_1, &dis->node_id_2, dis->conflict_type,
           dis->description, &dis->severity, &dis->detected_timestamp,
           &resolved_int, dis->resolution_method);
    
    dis->resolved = (resolved_int != 0);
    
    return true;
}

// ============================================================================
// PROVENANCE TRACKING
// ============================================================================

uint64_t lattice_add_pattern_with_provenance(persistent_lattice_t* lattice,
                                              lattice_node_type_t type,
                                              const char* name,
                                              const char* data,
                                              const provenance_t* provenance) {
    if (!lattice || !name || !data || !provenance) return 0;
    
    // Serialize provenance into node data
    char provenance_data[512];
    serialize_provenance(provenance, provenance_data, sizeof(provenance_data));
    
    // Combine original data with provenance
    char combined_data[512];
    if (strlen(data) + strlen(provenance_data) + 20 < sizeof(combined_data)) {
        snprintf(combined_data, sizeof(combined_data), "%s|%s", data, provenance_data);
    } else {
        // If too long, just use provenance
        strncpy(combined_data, provenance_data, sizeof(combined_data) - 1);
    }
    
    // Add node with provenance
    uint64_t node_id = lattice_add_node(lattice, type, name, combined_data, 0);
    
    if (node_id != 0) {
        printf("[EPISTEMOLOGY] OK Added pattern with provenance: %s (source: %s, trust: %.3f)\n",
               name, provenance->source_id, provenance->trust_level);
    }
    
    return node_id;
}

int lattice_get_provenance(persistent_lattice_t* lattice,
                           uint64_t node_id,
                           provenance_t* provenance) {
    if (!lattice || !provenance) return -1;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return -1;
    
    // Try to deserialize provenance from node data
    if (deserialize_provenance(node->data, provenance)) {
        return 0;
    }
    
    // Check if provenance is embedded (after | separator)
    char* prov_start = strstr(node->data, "|PROVENANCE:");
    if (prov_start) {
        return deserialize_provenance(prov_start + 1, provenance) ? 0 : -1;
    }
    
    return -1;
}

double lattice_update_provenance_trust(persistent_lattice_t* lattice,
                                       uint64_t node_id,
                                       bool validation_success) {
    if (!lattice) return -1.0;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return -1.0;
    
    provenance_t prov;
    if (lattice_get_provenance(lattice, node_id, &prov) != 0) {
        // No provenance, can't update
        return -1.0;
    }
    
    // Update validation counts
    if (validation_success) {
        prov.validation_count++;
        // Increase trust slightly on success
        prov.trust_level = fmin(1.0, prov.trust_level + 0.01);
    } else {
        prov.failure_count++;
        // Decrease trust more on failure
        prov.trust_level = fmax(0.0, prov.trust_level - 0.05);
    }
    
    // Update node data with new provenance
    char provenance_data[512];
    serialize_provenance(&prov, provenance_data, sizeof(provenance_data));
    
    // Preserve original data if it exists
    char* original_data = strstr(node->data, "|PROVENANCE:");
    if (original_data) {
        *original_data = '\0'; // Truncate at provenance marker
        char combined[512];
        snprintf(combined, sizeof(combined), "%s|%s", node->data, provenance_data);
        strncpy(node->data, combined, sizeof(node->data) - 1);
    } else {
        strncpy(node->data, provenance_data, sizeof(node->data) - 1);
    }
    
    lattice->dirty = true;
    
    return prov.trust_level;
}

// ============================================================================
// CONFIDENCE AGGREGATION
// ============================================================================

int lattice_aggregate_confidence(persistent_lattice_t* lattice,
                                 const uint64_t* node_ids,
                                 uint32_t node_count,
                                 aggregated_confidence_t* result) {
    if (!lattice || !node_ids || !result || node_count == 0) return -1;
    
    memset(result, 0, sizeof(aggregated_confidence_t));
    
    double trust_weighted_sum = 0.0;
    double trust_sum = 0.0;
    double confidence_sum = 0.0;
    
    // Check for conflicts
    for (uint32_t i = 0; i < node_count; i++) {
        for (uint32_t j = i + 1; j < node_count; j++) {
            disagreement_t dis;
            if (lattice_detect_disagreement(lattice, node_ids[i], node_ids[j], &dis) == 1) {
                result->has_conflicts = true;
                result->conflict_count++;
            }
        }
    }
    
    // Aggregate confidence with trust weighting
    for (uint32_t i = 0; i < node_count; i++) {
        lattice_node_t* node = find_node_by_id(lattice, node_ids[i]);
        if (!node) continue;
        
        double trust = 0.5; // Default trust
        provenance_t prov;
        if (lattice_get_provenance(lattice, node_ids[i], &prov) == 0) {
            trust = prov.trust_level;
        }
        
        double weighted_confidence = node->confidence * trust;
        trust_weighted_sum += weighted_confidence;
        trust_sum += trust;
        confidence_sum += node->confidence;
    }
    
    result->source_count = node_count;
    result->trust_weighted_sum = trust_weighted_sum;
    result->trust_sum = trust_sum;
    
    // Calculate aggregated confidence
    if (trust_sum > 0.0) {
        result->aggregated_confidence = trust_weighted_sum / trust_sum;
    } else {
        result->aggregated_confidence = confidence_sum / node_count;
    }
    
    return 0;
}

int lattice_aggregate_confidence_by_prefix(persistent_lattice_t* lattice,
                                           const char* prefix,
                                           aggregated_confidence_t* result) {
    if (!lattice || !prefix || !result) return -1;
    
    // Find all nodes with this prefix
    uint64_t node_ids[1000];
    uint32_t node_count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && node_count < 1000; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip historical nodes
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        
        if (strncmp(node->name, prefix, strlen(prefix)) == 0) {
            node_ids[node_count++] = node->id;
        }
    }
    
    if (node_count == 0) {
        return -1;
    }
    
    return lattice_aggregate_confidence(lattice, node_ids, node_count, result);
}

// ============================================================================
// DISAGREEMENT PRESERVATION
// ============================================================================

int lattice_detect_disagreement(persistent_lattice_t* lattice,
                                uint64_t node_id_1,
                                uint64_t node_id_2,
                                disagreement_t* disagreement) {
    if (!lattice || !disagreement) return -1;
    
    lattice_node_t* node1 = find_node_by_id(lattice, node_id_1);
    lattice_node_t* node2 = find_node_by_id(lattice, node_id_2);
    
    if (!node1 || !node2) return -1;
    
    memset(disagreement, 0, sizeof(disagreement_t));
    disagreement->node_id_1 = node_id_1;
    disagreement->node_id_2 = node_id_2;
    disagreement->detected_timestamp = get_current_timestamp_us();
    disagreement->resolved = false;
    
    // Check for same prefix but different data (contradiction)
    if (strcmp(node1->name, node2->name) == 0) {
        if (strcmp(node1->data, node2->data) != 0) {
            strcpy(disagreement->conflict_type, "contradiction");
            snprintf(disagreement->description, sizeof(disagreement->description),
                     "Same name (%s) but different data", node1->name);
            disagreement->severity = 0.8;
            return 1; // Disagreement found
        }
    }
    
    // Check for incompatible contexts
    provenance_t prov1, prov2;
    bool has_prov1 = (lattice_get_provenance(lattice, node_id_1, &prov1) == 0);
    bool has_prov2 = (lattice_get_provenance(lattice, node_id_2, &prov2) == 0);
    
    if (has_prov1 && has_prov2) {
        // Check if contexts are incompatible
        if (strcmp(prov1.context, prov2.context) != 0) {
            // Check if conditions are mutually exclusive
            if (strstr(prov1.conditions, "only") && strstr(prov2.conditions, "only")) {
                strcpy(disagreement->conflict_type, "context_mismatch");
                snprintf(disagreement->description, sizeof(disagreement->description),
                         "Incompatible contexts: %s vs %s", prov1.context, prov2.context);
                disagreement->severity = 0.6;
                return 1;
            }
        }
        
        // Check for incompatible conditions
        if (strlen(prov1.conditions) > 0 && strlen(prov2.conditions) > 0) {
            // Simple heuristic: if both have "only" conditions, they might be incompatible
            if (strstr(prov1.conditions, "only") && strstr(prov2.conditions, "only") &&
                strcmp(prov1.conditions, prov2.conditions) != 0) {
                strcpy(disagreement->conflict_type, "incompatible");
                snprintf(disagreement->description, sizeof(disagreement->description),
                         "Incompatible conditions: %s vs %s", prov1.conditions, prov2.conditions);
                disagreement->severity = 0.7;
                return 1;
            }
        }
    }
    
    // Check for confidence mismatch (high confidence on both but different values)
    if (node1->confidence > 0.8 && node2->confidence > 0.8) {
        double conf_diff = fabs(node1->confidence - node2->confidence);
        if (conf_diff > 0.3) {
            strcpy(disagreement->conflict_type, "confidence_mismatch");
            snprintf(disagreement->description, sizeof(disagreement->description),
                     "High confidence but different values: %.3f vs %.3f",
                     node1->confidence, node2->confidence);
            disagreement->severity = 0.5;
            return 1;
        }
    }
    
    return 0; // No disagreement
}

int lattice_record_disagreement(persistent_lattice_t* lattice,
                                const disagreement_t* disagreement) {
    if (!lattice || !disagreement) return -1;
    
    // Create a disagreement node
    char dis_name[64];
    snprintf(dis_name, sizeof(dis_name), "DISAGREEMENT:%lu:%lu",
             disagreement->node_id_1, disagreement->node_id_2);
    
    char dis_data[512];
    serialize_disagreement(disagreement, dis_data, sizeof(dis_data));
    
    uint64_t dis_node_id = lattice_add_node(lattice, LATTICE_NODE_CPT_METADATA,
                                            dis_name, dis_data, 0);
    
    if (dis_node_id != 0) {
        printf("[EPISTEMOLOGY] OK Recorded disagreement: %s (severity: %.3f)\n",
               disagreement->conflict_type, disagreement->severity);
    }
    
    return (dis_node_id != 0) ? 0 : -1;
}

uint32_t lattice_get_disagreements(persistent_lattice_t* lattice,
                                   uint64_t node_id,
                                   disagreement_t* disagreements,
                                   uint32_t max_disagreements) {
    if (!lattice || !disagreements || max_disagreements == 0) return 0;
    
    uint32_t count = 0;
    char search_prefix[64];
    snprintf(search_prefix, sizeof(search_prefix), "DISAGREEMENT:%lu:", node_id);
    
    for (uint32_t i = 0; i < lattice->node_count && count < max_disagreements; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        if (strncmp(node->name, search_prefix, strlen(search_prefix)) == 0) {
            if (deserialize_disagreement(node->data, &disagreements[count])) {
                count++;
            }
        }
    }
    
    return count;
}

int lattice_resolve_disagreement(persistent_lattice_t* lattice,
                                 uint64_t node_id_1,
                                 uint64_t node_id_2,
                                 const char* resolution_method) {
    if (!lattice || !resolution_method) return -1;
    
    // Find disagreement node
    char dis_name[64];
    snprintf(dis_name, sizeof(dis_name), "DISAGREEMENT:%lu:%lu", node_id_1, node_id_2);
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (strcmp(node->name, dis_name) == 0) {
            // Update disagreement record
            disagreement_t dis;
            if (deserialize_disagreement(node->data, &dis)) {
                dis.resolved = true;
                strncpy(dis.resolution_method, resolution_method, sizeof(dis.resolution_method) - 1);
                
                char dis_data[512];
                serialize_disagreement(&dis, dis_data, sizeof(dis_data));
                strncpy(node->data, dis_data, sizeof(node->data) - 1);
                
                lattice->dirty = true;
                return 0;
            }
        }
    }
    
    return -1;
}

// ============================================================================
// TRUST BOUNDARIES
// ============================================================================

uint32_t lattice_query_with_trust_boundary(persistent_lattice_t* lattice,
                                           const char* prefix,
                                           const trust_boundary_t* trust_boundary,
                                           uint64_t* node_ids,
                                           uint32_t max_nodes) {
    if (!lattice || !prefix || !trust_boundary || !node_ids || max_nodes == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && count < max_nodes; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip historical nodes
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        
        // Check prefix match
        if (strncmp(node->name, prefix, strlen(prefix)) != 0) continue;
        
        // Check trust level
        double trust = 0.5; // Default
        provenance_t prov;
        if (lattice_get_provenance(lattice, node->id, &prov) == 0) {
            trust = prov.trust_level;
        }
        
        if (trust < trust_boundary->min_trust_level) continue;
        
        // Check context filter
        if (trust_boundary->context_filter) {
            if (lattice_get_provenance(lattice, node->id, &prov) == 0) {
                if (strcmp(prov.context, trust_boundary->context_filter) != 0) {
                    continue;
                }
            } else {
                continue; // No provenance = no context match
            }
        }
        
        // Check source type filter
        if (trust_boundary->source_type_filter) {
            if (lattice_get_provenance(lattice, node->id, &prov) == 0) {
                if (strcmp(prov.source_type, trust_boundary->source_type_filter) != 0) {
                    continue;
                }
            } else {
                continue;
            }
        }
        
        // Check validation count
        if (lattice_get_provenance(lattice, node->id, &prov) == 0) {
            if (prov.validation_count < trust_boundary->min_validation_count) {
                continue;
            }
        } else if (trust_boundary->min_validation_count > 0) {
            continue; // No provenance = no validations
        }
        
        // Check for conflicts
        if (trust_boundary->exclude_conflicts) {
            disagreement_t dis;
            // Check if this node has any disagreements
            uint32_t dis_count = lattice_get_disagreements(lattice, node->id, &dis, 1);
            if (dis_count > 0 && !dis.resolved) {
                continue; // Skip nodes with unresolved conflicts
            }
        }
        
        node_ids[count++] = node->id;
    }
    
    return count;
}

double lattice_get_trust_level(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return -1.0;
    
    provenance_t prov;
    if (lattice_get_provenance(lattice, node_id, &prov) == 0) {
        return prov.trust_level;
    }
    
    // Default trust if no provenance
    return 0.5;
}

int lattice_set_trust_level(persistent_lattice_t* lattice,
                            uint64_t node_id,
                            double trust_level) {
    if (!lattice) return -1;
    
    if (trust_level < 0.0 || trust_level > 1.0) return -1;
    
    provenance_t prov;
    if (lattice_get_provenance(lattice, node_id, &prov) != 0) {
        // Create default provenance
        memset(&prov, 0, sizeof(provenance_t));
        strcpy(prov.source_id, "manual");
        strcpy(prov.source_type, "manual");
        prov.created_timestamp = get_current_timestamp_us();
        strcpy(prov.context, "unknown");
        strcpy(prov.conditions, "none");
    }
    
    prov.trust_level = trust_level;
    
    // Update node data
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return -1;
    
    char provenance_data[512];
    serialize_provenance(&prov, provenance_data, sizeof(provenance_data));
    
    // Preserve original data
    char* original_data = strstr(node->data, "|PROVENANCE:");
    if (original_data) {
        *original_data = '\0';
        char combined[512];
        snprintf(combined, sizeof(combined), "%s|%s", node->data, provenance_data);
        strncpy(node->data, combined, sizeof(node->data) - 1);
    } else {
        strncpy(node->data, provenance_data, sizeof(node->data) - 1);
    }
    
    lattice->dirty = true;
    
    return 0;
}

// ============================================================================
// CONTEXT-AWARE QUERIES
// ============================================================================

uint32_t lattice_query_by_context(persistent_lattice_t* lattice,
                                  const char* prefix,
                                  const char* context,
                                  uint64_t* node_ids,
                                  uint32_t max_nodes) {
    if (!lattice || !prefix || !context || !node_ids || max_nodes == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count && count < max_nodes; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip historical nodes
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        
        // Check prefix match
        if (strncmp(node->name, prefix, strlen(prefix)) != 0) continue;
        
        // Check context validity
        if (lattice_is_valid_in_context(lattice, node->id, context)) {
            node_ids[count++] = node->id;
        }
    }
    
    return count;
}

bool lattice_is_valid_in_context(persistent_lattice_t* lattice,
                                 uint64_t node_id,
                                 const char* context) {
    if (!lattice || !context) return false;
    
    provenance_t prov;
    if (lattice_get_provenance(lattice, node_id, &prov) != 0) {
        // No provenance = assume valid in all contexts
        return true;
    }
    
    // Check if context matches
    if (strcmp(prov.context, context) == 0) {
        return true;
    }
    
    // Check if conditions allow this context
    if (strlen(prov.conditions) > 0) {
        // Simple heuristic: if conditions contain "only", it's restrictive
        if (strstr(prov.conditions, "only")) {
            // Check if context is mentioned in conditions
            if (strstr(prov.conditions, context) != NULL) {
                return true;
            }
            return false;
        }
    }
    
    // Default: valid if no restrictions
    return true;
}

