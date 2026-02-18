/*
 * SEMANTIC AGING MODEL - IMPLEMENTATION
 * =====================================
 * 
 * Implements semantic drift handling over long time horizons.
 */

#include "semantic_aging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

// Get current timestamp in microseconds
static uint64_t get_current_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// Convert microseconds to days
static double microseconds_to_days(uint64_t us) {
    return (double)us / (1000000.0 * 86400.0);
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

// Extract version info from node data
static bool extract_version_info(lattice_node_t* node, concept_version_t* version) {
    if (!node || !version) return false;
    
    memset(version, 0, sizeof(concept_version_t));
    version->node_id = node->id;
    version->created_timestamp = node->timestamp;
    
    // Parse version info from node data (format: "VERSION:prev_id:superseded_by:version_num:reason")
    if (strncmp(node->data, "VERSION:", 8) == 0) {
        sscanf(node->data, "VERSION:%lu:%lu:%u:%127s",
               &version->previous_version_id,
               &version->superseded_by_id,
               &version->version_number,
               version->reason);
        version->is_historical = (version->superseded_by_id != 0);
        if (version->is_historical) {
            // Try to get superseded timestamp from data
            // Format: "VERSION:...:superseded_timestamp:..."
            char* timestamp_str = strstr(node->data, ":superseded:");
            if (timestamp_str) {
                sscanf(timestamp_str, ":superseded:%lu", &version->superseded_timestamp);
            }
        }
    } else {
        // First version (no version info)
        version->previous_version_id = 0;
        version->superseded_by_id = 0;
        version->version_number = 1;
        version->is_historical = false;
    }
    
    return true;
}

// ============================================================================
// CONCEPT VERSIONING
// ============================================================================

int lattice_mark_concept_historical(persistent_lattice_t* lattice, 
                                     uint64_t node_id, 
                                     uint64_t superseded_by_id,
                                     const char* reason) {
    if (!lattice) return -1;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) {
        fprintf(stderr, "[SEMANTIC-AGING] ERROR Node %lu not found\n", node_id);
        return -1;
    }
    
    // Get current version info
    concept_version_t current_version;
    extract_version_info(node, &current_version);
    
    // Update node data with historical marker
    char version_data[512];
    snprintf(version_data, sizeof(version_data),
             "VERSION:%lu:%lu:%u:%s:superseded:%lu",
             current_version.previous_version_id,
             superseded_by_id,
             current_version.version_number,
             reason ? reason : "superseded",
             get_current_timestamp_us());
    
    // Preserve original data prefix if it exists
    if (strlen(node->data) > 0 && strlen(version_data) + strlen(node->data) < sizeof(node->data) - 50) {
        char original_data[256];
        strncpy(original_data, node->data, sizeof(original_data) - 1);
        snprintf(node->data, sizeof(node->data), "%s|ORIGINAL:%s", version_data, original_data);
    } else {
        strncpy(node->data, version_data, sizeof(node->data) - 1);
    }
    
    // Update timestamp to mark when it was superseded
    node->timestamp = get_current_timestamp_us();
    
    // Mark as historical in node name (add HISTORICAL: prefix)
    char historical_name[64];
    if (strncmp(node->name, "HISTORICAL:", 11) != 0) {
        snprintf(historical_name, sizeof(historical_name), "HISTORICAL:%s", node->name);
        strncpy(node->name, historical_name, sizeof(node->name) - 1);
    }
    
    lattice->dirty = true;
    
    printf("[SEMANTIC-AGING] OK Marked node %lu as historical (superseded by %lu)\n",
           node_id, superseded_by_id);
    
    return 0;
}

uint32_t lattice_get_concept_lineage(persistent_lattice_t* lattice,
                                     uint64_t node_id,
                                     concept_version_t* versions,
                                     uint32_t max_versions) {
    if (!lattice || !versions || max_versions == 0) return 0;
    
    uint32_t count = 0;
    uint64_t current_id = node_id;
    
    // Trace back through previous versions
    while (current_id != 0 && count < max_versions) {
        lattice_node_t* node = find_node_by_id(lattice, current_id);
        if (!node) break;
        
        concept_version_t version;
        if (extract_version_info(node, &version)) {
            versions[count] = version;
            count++;
            
            // Move to previous version
            current_id = version.previous_version_id;
        } else {
            break;
        }
    }
    
    return count;
}

uint64_t lattice_create_concept_version(persistent_lattice_t* lattice,
                                       uint64_t previous_node_id,
                                       lattice_node_type_t type,
                                       const char* name,
                                       const char* data,
                                       const char* reason) {
    if (!lattice || !name || !data) return 0;
    
    // Get previous version info
    lattice_node_t* prev_node = find_node_by_id(lattice, previous_node_id);
    concept_version_t prev_version;
    uint32_t next_version_num = 1;
    
    if (prev_node) {
        extract_version_info(prev_node, &prev_version);
        next_version_num = prev_version.version_number + 1;
    }
    
    // Create new node
    uint64_t new_node_id = lattice_add_node(lattice, type, name, data, 0);
    if (new_node_id == 0) return 0;
    
    // Add version info to new node
    lattice_node_t* new_node = find_node_by_id(lattice, new_node_id);
    if (new_node) {
        char version_data[512];
        snprintf(version_data, sizeof(version_data),
                 "VERSION:%lu:0:%u:%s",
                 previous_node_id,
                 next_version_num,
                 reason ? reason : "new_version");
        strncpy(new_node->data, version_data, sizeof(new_node->data) - 1);
        new_node->data[sizeof(new_node->data) - 1] = '\0';
    }
    
    // Mark previous version as historical
    if (prev_node) {
        lattice_mark_concept_historical(lattice, previous_node_id, new_node_id, reason);
    }
    
    printf("[SEMANTIC-AGING] OK Created version %u of concept (node %lu)\n",
           next_version_num, new_node_id);
    
    return new_node_id;
}

// ============================================================================
// SEMANTIC HALF-LIFE
// ============================================================================

int lattice_calculate_semantic_half_life(persistent_lattice_t* lattice,
                                         const char* prefix,
                                         semantic_half_life_t* result) {
    if (!lattice || !prefix || !result) return -1;
    
    memset(result, 0, sizeof(semantic_half_life_t));
    strncpy(result->prefix, prefix, sizeof(result->prefix) - 1);
    
    // Find all nodes with this prefix
    double total_confidence = 0.0;
    uint32_t node_count = 0;
    uint64_t oldest_ts = UINT64_MAX;
    uint64_t newest_ts = 0;
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip historical nodes
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        
        if (strncmp(node->name, prefix, strlen(prefix)) == 0) {
            total_confidence += node->confidence;
            node_count++;
            
            if (node->timestamp < oldest_ts) oldest_ts = node->timestamp;
            if (node->timestamp > newest_ts) newest_ts = node->timestamp;
        }
    }
    
    if (node_count == 0) {
        fprintf(stderr, "[SEMANTIC-AGING] WARN No nodes found with prefix '%s'\n", prefix);
        return -1;
    }
    
    result->current_confidence = total_confidence / node_count;
    result->node_count = node_count;
    result->oldest_timestamp = oldest_ts;
    result->newest_timestamp = newest_ts;
    
    // Calculate decay rate based on age spread
    if (newest_ts > oldest_ts) {
        double age_days = microseconds_to_days(newest_ts - oldest_ts);
        if (age_days > 0) {
            // Estimate decay rate: confidence decreases by 1% per day of age spread
            result->decay_rate = 0.01 / age_days;
        } else {
            result->decay_rate = 0.0; // No decay if all nodes are same age
        }
    } else {
        result->decay_rate = 0.0;
    }
    
    // Calculate half-life: time until confidence decays to 50%
    if (result->decay_rate > 0) {
        // Half-life = ln(2) / decay_rate (in days)
        result->half_life_days = log(2.0) / result->decay_rate;
    } else {
        result->half_life_days = INFINITY; // No decay
    }
    
    return 0;
}

double lattice_apply_semantic_decay(persistent_lattice_t* lattice,
                                    uint64_t node_id,
                                    double decay_rate) {
    if (!lattice || decay_rate < 0) return 0.0;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return 0.0;
    
    // Skip historical nodes
    if (strncmp(node->name, "HISTORICAL:", 11) == 0) {
        return node->confidence; // Don't decay historical nodes
    }
    
    // Calculate age in days
    uint64_t current_ts = get_current_timestamp_us();
    double age_days = microseconds_to_days(current_ts - node->timestamp);
    
    // Apply exponential decay: new_confidence = old_confidence * e^(-decay_rate * age)
    double new_confidence = node->confidence * exp(-decay_rate * age_days);
    
    // Clamp to valid range
    if (new_confidence < 0.0) new_confidence = 0.0;
    if (new_confidence > 1.0) new_confidence = 1.0;
    
    // Update node confidence
    node->confidence = new_confidence;
    lattice->dirty = true;
    
    return new_confidence;
}

// ============================================================================
// DRIFT DETECTION
// ============================================================================

int lattice_detect_semantic_drift(persistent_lattice_t* lattice,
                                   const char* prefix,
                                   uint32_t time_window_days,
                                   drift_detection_t* result) {
    if (!lattice || !prefix || !result) return -1;
    
    memset(result, 0, sizeof(drift_detection_t));
    strncpy(result->prefix, prefix, sizeof(result->prefix) - 1);
    
    // Collect nodes with this prefix
    double* confidences = NULL;
    uint64_t* timestamps = NULL;
    uint32_t node_count = 0;
    uint32_t capacity = 100;
    
    confidences = (double*)malloc(capacity * sizeof(double));
    timestamps = (uint64_t*)malloc(capacity * sizeof(uint64_t));
    
    if (!confidences || !timestamps) {
        free(confidences);
        free(timestamps);
        return -1;
    }
    
    uint64_t window_start = 0;
    if (time_window_days > 0) {
        window_start = get_current_timestamp_us() - ((uint64_t)time_window_days * 86400 * 1000000);
    }
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        
        // Skip historical nodes
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        
        // Check prefix match
        if (strncmp(node->name, prefix, strlen(prefix)) != 0) continue;
        
        // Check time window
        if (time_window_days > 0 && node->timestamp < window_start) continue;
        
        // Grow arrays if needed
        if (node_count >= capacity) {
            capacity *= 2;
            confidences = (double*)realloc(confidences, capacity * sizeof(double));
            timestamps = (uint64_t*)realloc(timestamps, capacity * sizeof(uint64_t));
            if (!confidences || !timestamps) break;
        }
        
        confidences[node_count] = node->confidence;
        timestamps[node_count] = node->timestamp;
        node_count++;
    }
    
    if (node_count == 0) {
        free(confidences);
        free(timestamps);
        return -1;
    }
    
    // Calculate variance in confidence scores
    double mean_confidence = 0.0;
    for (uint32_t i = 0; i < node_count; i++) {
        mean_confidence += confidences[i];
    }
    mean_confidence /= node_count;
    
    double variance = 0.0;
    for (uint32_t i = 0; i < node_count; i++) {
        double diff = confidences[i] - mean_confidence;
        variance += diff * diff;
    }
    variance /= node_count;
    result->confidence_variance = variance;
    
    // Calculate variance in ages
    double mean_age = 0.0;
    uint64_t current_ts = get_current_timestamp_us();
    for (uint32_t i = 0; i < node_count; i++) {
        mean_age += microseconds_to_days(current_ts - timestamps[i]);
    }
    mean_age /= node_count;
    
    double age_variance = 0.0;
    for (uint32_t i = 0; i < node_count; i++) {
        double age = microseconds_to_days(current_ts - timestamps[i]);
        double diff = age - mean_age;
        age_variance += diff * diff;
    }
    age_variance /= node_count;
    result->age_variance = age_variance;
    
    // Detect prefix ambiguity (simplified: high variance = ambiguity)
    result->ambiguity_count = (variance > 0.1) ? 2 : 1; // Simplified heuristic
    
    // Determine if drift exists
    bool has_confidence_drift = (variance > 0.15); // High variance = drift
    bool has_age_drift = (age_variance > 30.0); // High age variance = drift
    
    result->has_drift = has_confidence_drift || has_age_drift;
    
    if (has_confidence_drift && has_age_drift) {
        strcpy(result->drift_type, "combined");
        result->drift_severity = fmin(1.0, (variance / 0.3) + (age_variance / 60.0));
    } else if (has_confidence_drift) {
        strcpy(result->drift_type, "confidence");
        result->drift_severity = fmin(1.0, variance / 0.3);
    } else if (has_age_drift) {
        strcpy(result->drift_type, "age");
        result->drift_severity = fmin(1.0, age_variance / 60.0);
    } else {
        strcpy(result->drift_type, "none");
        result->drift_severity = 0.0;
    }
    
    free(confidences);
    free(timestamps);
    
    return 0;
}

uint32_t lattice_detect_prefix_ambiguity(persistent_lattice_t* lattice,
                                         const char* prefix,
                                         uint32_t ambiguity_threshold) {
    if (!lattice || !prefix) return 0;
    
    // Simplified: count distinct confidence clusters
    // In a full implementation, this would use semantic similarity
    double* confidences = NULL;
    uint32_t node_count = 0;
    uint32_t capacity = 100;
    
    confidences = (double*)malloc(capacity * sizeof(double));
    if (!confidences) return 0;
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        if (strncmp(node->name, prefix, strlen(prefix)) != 0) continue;
        
        if (node_count >= capacity) {
            capacity *= 2;
            confidences = (double*)realloc(confidences, capacity * sizeof(double));
            if (!confidences) break;
        }
        
        confidences[node_count++] = node->confidence;
    }
    
    if (node_count == 0) {
        free(confidences);
        return 0;
    }
    
    // Count distinct confidence clusters (simplified heuristic)
    // Group confidences into clusters (within 0.1 of each other)
    uint32_t distinct_meanings = 1;
    double cluster_center = confidences[0];
    
    for (uint32_t i = 1; i < node_count; i++) {
        if (fabs(confidences[i] - cluster_center) > 0.1) {
            distinct_meanings++;
            cluster_center = confidences[i];
        }
    }
    
    free(confidences);
    
    return (distinct_meanings >= ambiguity_threshold) ? distinct_meanings : 0;
}

uint32_t lattice_detect_confidence_anomalies(persistent_lattice_t* lattice,
                                              const char* prefix,
                                              uint64_t* anomaly_node_ids,
                                              uint32_t max_anomalies) {
    if (!lattice || !anomaly_node_ids || max_anomalies == 0) return 0;
    
    uint32_t anomaly_count = 0;
    
    // Calculate mean and stddev of confidence for this prefix
    double sum = 0.0;
    double sum_sq = 0.0;
    uint32_t node_count = 0;
    
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        if (prefix && strncmp(node->name, prefix, strlen(prefix)) != 0) continue;
        
        sum += node->confidence;
        sum_sq += node->confidence * node->confidence;
        node_count++;
    }
    
    if (node_count < 2) return 0;
    
    double mean = sum / node_count;
    double variance = (sum_sq / node_count) - (mean * mean);
    double stddev = sqrt(variance);
    
    // Find anomalies (confidence > mean + 2*stddev or < mean - 2*stddev)
    for (uint32_t i = 0; i < lattice->node_count && anomaly_count < max_anomalies; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (strncmp(node->name, "HISTORICAL:", 11) == 0) continue;
        if (prefix && strncmp(node->name, prefix, strlen(prefix)) != 0) continue;
        
        double z_score = (node->confidence - mean) / (stddev > 0 ? stddev : 1.0);
        if (fabs(z_score) > 2.0) { // 2 standard deviations
            anomaly_node_ids[anomaly_count++] = node->id;
        }
    }
    
    return anomaly_count;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool lattice_is_node_historical(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return false;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return false;
    
    return (strncmp(node->name, "HISTORICAL:", 11) == 0);
}

uint64_t lattice_get_current_version(persistent_lattice_t* lattice, uint64_t node_id) {
    if (!lattice) return 0;
    
    lattice_node_t* node = find_node_by_id(lattice, node_id);
    if (!node) return 0;
    
    // If already current, return it
    if (strncmp(node->name, "HISTORICAL:", 11) != 0) {
        return node_id;
    }
    
    // Extract original name (remove HISTORICAL: prefix)
    char original_name[64];
    if (strlen(node->name) > 11) {
        strncpy(original_name, node->name + 11, sizeof(original_name) - 1);
    } else {
        return 0;
    }
    
    // Find current version with same base name
    for (uint32_t i = 0; i < lattice->node_count; i++) {
        lattice_node_t* candidate = &lattice->nodes[i];
        if (strncmp(candidate->name, "HISTORICAL:", 11) == 0) continue;
        
        if (strcmp(candidate->name, original_name) == 0) {
            return candidate->id;
        }
    }
    
    return 0;
}

uint32_t lattice_get_historical_versions(persistent_lattice_t* lattice,
                                         uint64_t current_node_id,
                                         uint64_t* historical_ids,
                                         uint32_t max_historical) {
    if (!lattice || !historical_ids || max_historical == 0) return 0;
    
    lattice_node_t* current_node = find_node_by_id(lattice, current_node_id);
    if (!current_node) return 0;
    
    // Get base name (without any version prefix)
    char base_name[64];
    base_name[0] = '\0';
    strncpy(base_name, current_node->name, sizeof(base_name) - 1);
    base_name[sizeof(base_name) - 1] = '\0';
    
    // Find all historical versions
    uint32_t count = 0;
    for (uint32_t i = 0; i < lattice->node_count && count < max_historical; i++) {
        lattice_node_t* node = &lattice->nodes[i];
        if (strncmp(node->name, "HISTORICAL:", 11) != 0) continue;
        
        // Check if it's a historical version of this concept
        if (strlen(node->name) > 11 && strcmp(node->name + 11, base_name) == 0) {
            historical_ids[count++] = node->id;
        }
    }
    
    return count;
}

