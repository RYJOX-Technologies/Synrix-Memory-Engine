#define _GNU_SOURCE
#include "unified_indexing_system.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>

// Get current time in microseconds
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

// ============================================================================
// LATTICE INTEGRATION FUNCTIONS
// ============================================================================

// Create unified indexing system with lattice integration
int unified_indexing_system_create(unified_indexing_system_t* system, uint32_t capacity) {
    if (!system || capacity == 0) return -1;
    
    memset(system, 0, sizeof(unified_indexing_system_t));
    
    // Initialize configuration
    system->max_concurrent_queries = 100;
    system->cache_size = 10000;
    system->routing_confidence_threshold = 0.8f;
    system->intelligent_routing_enabled = true;
    system->cross_phase_verification_enabled = true;
    system->result_caching_enabled = true;
    system->performance_monitoring_enabled = true;
    
    // Initialize performance tracking
    system->system_start_time = get_time_us();
    memset(&system->stats, 0, sizeof(unified_performance_stats_t));
    system->stats.min_query_time_us = UINT64_MAX;
    
    // Initialize all phase systems
    if (unified_indexing_system_initialize_phases(system) != 0) {
        unified_indexing_system_destroy(system);
        return -1;
    }
    
    system->is_initialized = true;
    return 0;
}

// Initialize all phase systems for lattice integration
int unified_indexing_system_initialize_phases(unified_indexing_system_t* system) {
    if (!system) return -1;
    
    // Initialize Phase 1: Multi-Dimensional Indexing
    printf("DEBUG: Creating Phase 1 system...\n");
    system->phase1_system = (advanced_indexing_system_t*)malloc(sizeof(advanced_indexing_system_t));
    if (!system->phase1_system) {
        printf("DEBUG: Failed to allocate Phase 1 system\n");
        return -1;
    }
    
    if (advanced_indexing_system_create(system->phase1_system) != 0) {
        printf("DEBUG: Failed to create Phase 1 system\n");
        free(system->phase1_system);
        system->phase1_system = NULL;
        return -1;
    }
    printf("DEBUG: Phase 1 system created successfully\n");
    
    // Initialize Phase 2: Optimized Vector Indexing
    printf("DEBUG: Creating Phase 2 system...\n");
    system->phase2_system = (optimized_vector_indexing_system_t*)malloc(sizeof(optimized_vector_indexing_system_t));
    if (!system->phase2_system) {
        printf("DEBUG: Failed to allocate Phase 2 system\n");
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
        return -1;
    }
    
    if (optimized_vector_indexing_system_create(system->phase2_system, 50000) != 0) {
        printf("DEBUG: Failed to create Phase 2 system\n");
        free(system->phase2_system);
        system->phase2_system = NULL;
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
        return -1;
    }
    printf("DEBUG: Phase 2 system created successfully\n");
    
    // Initialize Phase 3: Hierarchical Indexing
    printf("DEBUG: Creating Phase 3 system...\n");
    system->phase3_system = (hierarchical_indexing_system_t*)malloc(sizeof(hierarchical_indexing_system_t));
    if (!system->phase3_system) {
        printf("DEBUG: Failed to allocate Phase 3 system\n");
        optimized_vector_indexing_system_destroy(system->phase2_system);
        free(system->phase2_system);
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
        return -1;
    }
    
    if (hierarchical_indexing_system_create(system->phase3_system) != 0) {
        printf("DEBUG: Failed to create Phase 3 system\n");
        free(system->phase3_system);
        system->phase3_system = NULL;
        optimized_vector_indexing_system_destroy(system->phase2_system);
        free(system->phase2_system);
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
        return -1;
    }
    printf("DEBUG: Phase 3 system created successfully\n");
    
    // Initialize Phase 4: Specialized Indexing
    printf("DEBUG: Creating Phase 4 system...\n");
    system->phase4_system = (specialized_indexing_system_t*)malloc(sizeof(specialized_indexing_system_t));
    if (!system->phase4_system) {
        printf("DEBUG: Failed to allocate Phase 4 system\n");
        hierarchical_indexing_system_destroy(system->phase3_system);
        free(system->phase3_system);
        optimized_vector_indexing_system_destroy(system->phase2_system);
        free(system->phase2_system);
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
        return -1;
    }
    
    if (specialized_indexing_system_create(system->phase4_system) != 0) {
        printf("DEBUG: Failed to create Phase 4 system\n");
        free(system->phase4_system);
        system->phase4_system = NULL;
        hierarchical_indexing_system_destroy(system->phase3_system);
        free(system->phase3_system);
        optimized_vector_indexing_system_destroy(system->phase2_system);
        free(system->phase2_system);
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
        return -1;
    }
    printf("DEBUG: Phase 4 system created successfully\n");
    
    return 0;
}

// Add lattice node to unified system - SEAMLESS LATTICE INTEGRATION
int unified_indexing_system_add_node(unified_indexing_system_t* system, const lattice_node_t* node) {
    if (!system || !node || !system->is_initialized) return -1;
    
    uint64_t start_time = get_time_us();
    int total_added = 0;
    
    // Add to Phase 1: Multi-Dimensional Indexing
    if (system->phase1_system) {
        if (advanced_indexing_system_add_node(system->phase1_system, (lattice_node_t*)node) == 0) {
            total_added++;
            system->stats.phase1_queries++;
        }
    }
    
    // Add to Phase 2: Optimized Vector Indexing
    if (system->phase2_system) {
        if (optimized_vector_indexing_system_add_node(system->phase2_system, node) == 0) {
            total_added++;
            system->stats.phase2_queries++;
        }
    }
    
    // Add to Phase 3: Hierarchical Indexing
    if (system->phase3_system) {
        if (hierarchical_indexing_system_add_node(system->phase3_system, node) == 0) {
            total_added++;
            system->stats.phase3_queries++;
        }
    }
    
    // Add to Phase 4: Specialized Indexing
    if (system->phase4_system) {
        if (specialized_indexing_system_add_node(system->phase4_system, node) == 0) {
            total_added++;
            system->stats.phase4_queries++;
        }
    }
    
    // Update statistics
    system->total_nodes_indexed++;
    uint64_t processing_time = get_time_us() - start_time;
    system->stats.total_query_time_us += processing_time;
    
    return (total_added > 0) ? 0 : -1;
}

// Unified search with intelligent routing - LATTICE-OPTIMIZED SEARCH
int unified_indexing_system_search(unified_indexing_system_t* system, 
                                  const unified_query_t* query,
                                  unified_search_result_t* results, 
                                  uint32_t* count) {
    if (!system || !query || !results || !count || !system->is_initialized) return -1;
    
    uint64_t query_start = get_time_us();
    *count = 0;
    
    // Update query statistics
    system->stats.total_queries++;
    
    // Intelligent routing - determine best phases for this query
    uint32_t target_phases[4];
    uint32_t phase_count = 0;
    
    if (unified_indexing_route_query(system, query, target_phases, &phase_count) != 0) {
        return -1;
    }
    
    // Search across selected phases
    unified_search_result_t phase_results[4][1000];
    uint32_t phase_counts[4] = {0};
    
    for (uint32_t i = 0; i < phase_count; i++) {
        uint32_t phase = target_phases[i];
        
        switch (phase) {
            case 1: // Multi-Dimensional Indexing
                if (system->phase1_system) {
                    uint32_t temp_results[1000];
                    uint32_t temp_count = 0;
                    if (advanced_indexing_system_search(system->phase1_system, query->query_text, 
                                                       temp_results, &temp_count) == 0) {
                        // Convert to unified results
                        for (uint32_t j = 0; j < temp_count && j < 1000; j++) {
                            phase_results[0][phase_counts[0]].node_id = temp_results[j];
                            phase_results[0][phase_counts[0]].relevance_score = 1.0f;
                            phase_results[0][phase_counts[0]].confidence_score = 0.9f;
                            phase_results[0][phase_counts[0]].source_phase = 1;
                            strcpy(phase_results[0][phase_counts[0]].source_index, "advanced_indexing");
                            phase_results[0][phase_counts[0]].source_score = 1.0f;
                            phase_counts[0]++;
                        }
                    }
                }
                break;
                
            case 2: // Optimized Vector Indexing
                if (system->phase2_system) {
                    optimized_search_result_t temp_results[1000];
                    uint32_t temp_count = 0;
                    if (optimized_vector_indexing_system_search(system->phase2_system, query->query_text,
                                                               temp_results, &temp_count) == 0) {
                        // Convert to unified results
                        for (uint32_t j = 0; j < temp_count && j < 1000; j++) {
                            phase_results[1][phase_counts[1]].node_id = temp_results[j].node_id;
                            phase_results[1][phase_counts[1]].relevance_score = temp_results[j].similarity_score;
                            phase_results[1][phase_counts[1]].confidence_score = temp_results[j].cluster_confidence;
                            phase_results[1][phase_counts[1]].source_phase = 2;
                            strcpy(phase_results[1][phase_counts[1]].source_index, "vector_indexing");
                            phase_results[1][phase_counts[1]].source_score = temp_results[j].similarity_score;
                            phase_counts[1]++;
                        }
                    }
                }
                break;
                
            case 3: // Hierarchical Indexing
                if (system->phase3_system) {
                    // Create a simple tree search query
                    tree_search_query_t tree_query;
                    strncpy(tree_query.path_pattern, query->query_text, sizeof(tree_query.path_pattern) - 1);
                    tree_query.path_pattern[sizeof(tree_query.path_pattern) - 1] = '\0';
                    tree_query.min_level = 0;
                    tree_query.max_level = 10;
                    tree_query.node_type = TREE_NODE_LEAF; // Default to leaf nodes
                    tree_query.min_weight = 0.0f;
                    tree_query.max_weight = 1.0f;
                    tree_query.max_results = 1000;
                    tree_query.use_regex = false;
                    tree_query.include_subtrees = false;
                    
                    tree_search_result_t tree_result;
                    if (hierarchical_indexing_system_search(system->phase3_system, &tree_query, &tree_result) == 0) {
                        // Convert to unified results
                        for (uint32_t j = 0; j < tree_result.count && j < 1000; j++) {
                            phase_results[2][phase_counts[2]].node_id = tree_result.node_ids[j];
                            phase_results[2][phase_counts[2]].relevance_score = tree_result.scores ? tree_result.scores[j] : 0.8f;
                            phase_results[2][phase_counts[2]].confidence_score = 0.7f;
                            phase_results[2][phase_counts[2]].source_phase = 3;
                            strcpy(phase_results[2][phase_counts[2]].source_index, "hierarchical_indexing");
                            phase_results[2][phase_counts[2]].source_score = tree_result.scores ? tree_result.scores[j] : 0.8f;
                            phase_counts[2]++;
                        }
                    }
                }
                break;
                
            case 4: // Specialized Indexing
                if (system->phase4_system) {
                    // Use simple search for specialized system
                    uint32_t temp_results[1000];
                    uint32_t temp_count = 0;
                    if (specialized_indexing_system_search(system->phase4_system, query->query_text,
                                                         temp_results, &temp_count) == 0) {
                        // Convert to unified results
                        for (uint32_t j = 0; j < temp_count && j < 1000; j++) {
                            phase_results[3][phase_counts[3]].node_id = temp_results[j];
                            phase_results[3][phase_counts[3]].relevance_score = 0.9f;
                            phase_results[3][phase_counts[3]].confidence_score = 0.8f;
                            phase_results[3][phase_counts[3]].source_phase = 4;
                            strcpy(phase_results[3][phase_counts[3]].source_index, "specialized_indexing");
                            phase_results[3][phase_counts[3]].source_score = 0.9f;
                            phase_counts[3]++;
                        }
                    }
                }
                break;
        }
    }
    
    // Merge and rank results from all phases
    uint32_t total_results = 0;
    for (uint32_t i = 0; i < 4; i++) {
        for (uint32_t j = 0; j < phase_counts[i] && total_results < query->max_results; j++) {
            results[total_results] = phase_results[i][j];
            results[total_results].processing_time_us = get_time_us() - query_start;
            total_results++;
        }
    }
    
    // Cross-phase verification if enabled
    if (system->cross_phase_verification_enabled && phase_count > 1) {
        unified_indexing_cross_phase_verify(system, query, results, &total_results);
    }
    
    // Rank and merge results
    unified_indexing_rank_and_merge_results(results, total_results, query);
    
    *count = total_results;
    
    // Update performance statistics
    uint64_t query_time = get_time_us() - query_start;
    system->stats.total_query_time_us += query_time;
    system->stats.avg_query_time_us = system->stats.total_query_time_us / system->stats.total_queries;
    
    if (query_time < system->stats.min_query_time_us) {
        system->stats.min_query_time_us = query_time;
    }
    if (query_time > system->stats.max_query_time_us) {
        system->stats.max_query_time_us = query_time;
    }
    
    if (phase_count > 1) {
        system->stats.cross_phase_queries++;
    }
    
    return 0;
}

// ============================================================================
// LATTICE-OPTIMIZED QUERY ROUTING
// ============================================================================

// Detect query type for intelligent routing
unified_query_type_t unified_indexing_detect_query_type(const char* query_text) {
    if (!query_text) return QUERY_TYPE_AUTO_DETECT;
    
    // Analyze query text for routing hints
    if (strstr(query_text, "similar") || strstr(query_text, "like") || strstr(query_text, "related")) {
        return QUERY_TYPE_SEMANTIC_SEARCH;
    }
    
    if (strstr(query_text, "between") || strstr(query_text, "range") || strstr(query_text, "from") || strstr(query_text, "to")) {
        return QUERY_TYPE_RANGE_QUERY;
    }
    
    if (strstr(query_text, "fuzzy") || strstr(query_text, "approximate") || strstr(query_text, "typo")) {
        return QUERY_TYPE_FUZZY_SEARCH;
    }
    
    if (strstr(query_text, "hierarchy") || strstr(query_text, "tree") || strstr(query_text, "parent") || strstr(query_text, "child")) {
        return QUERY_TYPE_HIERARCHICAL;
    }
    
    if (strstr(query_text, "time") || strstr(query_text, "date") || strstr(query_text, "when")) {
        return QUERY_TYPE_TEMPORAL;
    }
    
    if (strstr(query_text, "location") || strstr(query_text, "geo") || strstr(query_text, "lat") || strstr(query_text, "lon")) {
        return QUERY_TYPE_GEOGRAPHIC;
    }
    
    if (strstr(query_text, " ") || strstr(query_text, "text") || strstr(query_text, "content")) {
        return QUERY_TYPE_FULL_TEXT;
    }
    
    // Default to exact match for simple queries
    return QUERY_TYPE_EXACT_MATCH;
}

// Route query to best phase systems
int unified_indexing_route_query(unified_indexing_system_t* system,
                                const unified_query_t* query,
                                uint32_t* target_phases,
                                uint32_t* phase_count) {
    if (!system || !query || !target_phases || !phase_count) return -1;
    
    *phase_count = 0;
    
    // Determine query type if auto-detect
    unified_query_type_t query_type = query->type;
    if (query_type == QUERY_TYPE_AUTO_DETECT) {
        query_type = unified_indexing_detect_query_type(query->query_text);
    }
    
    // Route based on query type and performance requirements
    switch (query_type) {
        case QUERY_TYPE_EXACT_MATCH:
            target_phases[(*phase_count)++] = 1; // Multi-dimensional
            if (query->performance_tier <= PERFORMANCE_TIER_FAST) {
                target_phases[(*phase_count)++] = 3; // Hierarchical
            }
            break;
            
        case QUERY_TYPE_SEMANTIC_SEARCH:
            target_phases[(*phase_count)++] = 2; // Vector indexing
            if (query->performance_tier <= PERFORMANCE_TIER_NORMAL) {
                target_phases[(*phase_count)++] = 1; // Multi-dimensional
            }
            break;
            
        case QUERY_TYPE_RANGE_QUERY:
            target_phases[(*phase_count)++] = 1; // Multi-dimensional
            target_phases[(*phase_count)++] = 3; // Hierarchical
            break;
            
        case QUERY_TYPE_FUZZY_SEARCH:
            target_phases[(*phase_count)++] = 1; // Multi-dimensional (inverted index)
            if (query->performance_tier <= PERFORMANCE_TIER_NORMAL) {
                target_phases[(*phase_count)++] = 2; // Vector indexing
            }
            break;
            
        case QUERY_TYPE_HIERARCHICAL:
            target_phases[(*phase_count)++] = 3; // Hierarchical
            break;
            
        case QUERY_TYPE_TEMPORAL:
            target_phases[(*phase_count)++] = 4; // Specialized (temporal)
            break;
            
        case QUERY_TYPE_GEOGRAPHIC:
            target_phases[(*phase_count)++] = 4; // Specialized (geographic)
            break;
            
        case QUERY_TYPE_FULL_TEXT:
            target_phases[(*phase_count)++] = 1; // Multi-dimensional (inverted index)
            if (query->performance_tier <= PERFORMANCE_TIER_NORMAL) {
                target_phases[(*phase_count)++] = 2; // Vector indexing
            }
            break;
            
        case QUERY_TYPE_MULTI_CRITERIA:
            target_phases[(*phase_count)++] = 1; // Multi-dimensional
            target_phases[(*phase_count)++] = 2; // Vector indexing
            target_phases[(*phase_count)++] = 3; // Hierarchical
            break;
            
        default:
            // Fallback to all phases
            target_phases[(*phase_count)++] = 1;
            target_phases[(*phase_count)++] = 2;
            target_phases[(*phase_count)++] = 3;
            target_phases[(*phase_count)++] = 4;
            break;
    }
    
    system->stats.routing_hits++;
    return 0;
}

// ============================================================================
// LATTICE-INTEGRATED CROSS-PHASE VERIFICATION
// ============================================================================

// Cross-phase verification for lattice nodes
int unified_indexing_cross_phase_verify(unified_indexing_system_t* system,
                                       const unified_query_t* query,
                                       unified_search_result_t* results,
                                       uint32_t* count) {
    if (!system || !query || !results || !count) return -1;
    
    // For each result, check if it appears in multiple phases
    for (uint32_t i = 0; i < *count; i++) {
        uint32_t verification_count = 0;
        float total_score = 0.0f;
        
        // Check if this node_id appears in other phases
        for (uint32_t j = 0; j < *count; j++) {
            if (i != j && results[i].node_id == results[j].node_id) {
                verification_count++;
                total_score += results[j].source_score;
            }
        }
        
        if (verification_count > 0) {
            results[i].verified_by_multiple = true;
            results[i].verification_count = verification_count + 1;
            results[i].cross_phase_score = (results[i].source_score + total_score) / (verification_count + 1);
            
            // Boost relevance score for cross-phase verification
            results[i].relevance_score = fminf(1.0f, results[i].relevance_score * 1.2f);
            results[i].confidence_score = fminf(1.0f, results[i].confidence_score * 1.1f);
        }
    }
    
    return 0;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Rank and merge results
int unified_indexing_rank_and_merge_results(unified_search_result_t* results,
                                           uint32_t count,
                                           const unified_query_t* query) {
    if (!results || count == 0) return -1;
    
    // Simple ranking by relevance score (can be enhanced)
    for (uint32_t i = 0; i < count - 1; i++) {
        for (uint32_t j = i + 1; j < count; j++) {
            if (results[i].relevance_score < results[j].relevance_score) {
                unified_search_result_t temp = results[i];
                results[i] = results[j];
                results[j] = temp;
            }
        }
    }
    
    return 0;
}

// Get system statistics
int unified_indexing_system_get_stats(unified_indexing_system_t* system,
                                     unified_performance_stats_t* stats) {
    if (!system || !stats) return -1;
    
    *stats = system->stats;
    return 0;
}

// Destroy unified indexing system
void unified_indexing_system_destroy(unified_indexing_system_t* system) {
    if (!system) return;
    
    if (system->phase1_system) {
        advanced_indexing_system_destroy(system->phase1_system);
        free(system->phase1_system);
    }
    
    if (system->phase2_system) {
        optimized_vector_indexing_system_destroy(system->phase2_system);
        free(system->phase2_system);
    }
    
    if (system->phase3_system) {
        hierarchical_indexing_system_destroy(system->phase3_system);
        free(system->phase3_system);
    }
    
    if (system->phase4_system) {
        specialized_indexing_system_destroy(system->phase4_system);
        free(system->phase4_system);
    }
    
    memset(system, 0, sizeof(unified_indexing_system_t));
}

// Create unified query
int unified_query_create(unified_query_t* query, const char* query_text,
                        unified_query_type_t type, uint32_t max_results) {
    if (!query || !query_text) return -1;
    
    memset(query, 0, sizeof(unified_query_t));
    
    query->query_text = strdup(query_text);
    if (!query->query_text) return -1;
    
    query->type = type;
    query->max_results = max_results;
    query->similarity_threshold = 0.7f;
    query->prefer_vector_search = false;
    query->prefer_exact_match = true;
    query->allow_fuzzy = false;
    query->use_caching = true;
    query->max_query_time_us = 1000000; // 1 second default
    
    return 0;
}

// Destroy unified query
void unified_query_destroy(unified_query_t* query) {
    if (!query) return;
    
    if (query->query_text) {
        free(query->query_text);
    }
    
    if (query->geographic_bounds) {
        free(query->geographic_bounds);
    }
    
    memset(query, 0, sizeof(unified_query_t));
}
