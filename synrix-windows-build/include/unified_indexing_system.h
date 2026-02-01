#ifndef UNIFIED_INDEXING_SYSTEM_H
#define UNIFIED_INDEXING_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "persistent_lattice.h"
#include "advanced_indexing.h"
#include "optimized_vector_indexing.h"
#include "hierarchical_indexing.h"
#include "specialized_indexing.h"

// ============================================================================
// UNIFIED INDEXING SYSTEM - CROWN JEWEL OF ALL OPTIMIZATIONS
// ============================================================================

// Query types for intelligent routing
typedef enum {
    QUERY_TYPE_EXACT_MATCH,        // Use hash tables, B+ trees
    QUERY_TYPE_SEMANTIC_SEARCH,    // Use vector indexing, LSH
    QUERY_TYPE_RANGE_QUERY,        // Use B+ trees, composite index
    QUERY_TYPE_FUZZY_SEARCH,       // Use fuzzy inverted index
    QUERY_TYPE_HIERARCHICAL,       // Use hierarchical trees
    QUERY_TYPE_TEMPORAL,           // Use temporal index
    QUERY_TYPE_GEOGRAPHIC,         // Use geographic index
    QUERY_TYPE_FULL_TEXT,          // Use inverted index
    QUERY_TYPE_MULTI_CRITERIA,     // Use composite index
    QUERY_TYPE_AUTO_DETECT         // Automatically detect best type
} unified_query_type_t;

// Query complexity levels
typedef enum {
    COMPLEXITY_SIMPLE,             // Single term, exact match
    COMPLEXITY_MEDIUM,             // Multi-term, some processing
    COMPLEXITY_COMPLEX,            // Complex queries, multiple phases
    COMPLEXITY_EXTREME             // Cross-phase, heavy processing
} unified_query_complexity_t;

// Performance tiers for routing decisions
typedef enum {
    PERFORMANCE_TIER_CRITICAL,     // < 1ms response time
    PERFORMANCE_TIER_FAST,         // < 10ms response time
    PERFORMANCE_TIER_NORMAL,       // < 100ms response time
    PERFORMANCE_TIER_ACCEPTABLE    // < 1000ms response time
} unified_performance_tier_t;

// Unified query structure
typedef struct {
    char* query_text;              // Original query text
    unified_query_type_t type;     // Query type
    unified_query_complexity_t complexity; // Query complexity
    unified_performance_tier_t performance_tier; // Required performance
    
    // Query parameters
    uint32_t max_results;          // Maximum results to return
    float similarity_threshold;    // Similarity threshold for fuzzy search
    uint64_t time_range_start;     // Start time for temporal queries
    uint64_t time_range_end;       // End time for temporal queries
    float* geographic_bounds;      // Bounding box for geographic queries
    
    // Routing hints
    bool prefer_vector_search;     // Prefer vector-based search
    bool prefer_exact_match;       // Prefer exact matching
    bool allow_fuzzy;              // Allow fuzzy matching
    bool use_caching;              // Use result caching
    
    // Performance tracking
    uint64_t query_start_time;     // Query start timestamp
    uint64_t max_query_time_us;    // Maximum allowed query time
} unified_query_t;

// Unified search result
typedef struct {
    uint32_t node_id;              // Lattice node ID
    float relevance_score;         // Overall relevance score (0.0-1.0)
    float confidence_score;        // Confidence in result (0.0-1.0)
    
    // Source information
    uint32_t source_phase;         // Which phase generated this result
    char source_index[64];         // Which specific index
    float source_score;            // Score from source index
    
    // Result metadata
    uint32_t match_count;          // Number of matching terms
    char* matched_terms;           // Terms that matched
    uint64_t processing_time_us;   // Time to process this result
    
    // Cross-phase information
    bool verified_by_multiple;     // Verified by multiple phases
    uint32_t verification_count;   // Number of phases that verified
    float cross_phase_score;       // Score from cross-phase verification
} unified_search_result_t;

// Performance statistics
typedef struct {
    uint64_t total_queries;        // Total queries processed
    uint64_t total_query_time_us;  // Total query processing time
    uint64_t avg_query_time_us;    // Average query time
    uint64_t max_query_time_us;    // Maximum query time
    uint64_t min_query_time_us;    // Minimum query time
    
    // Phase performance
    uint64_t phase1_queries;       // Queries handled by Phase 1
    uint64_t phase2_queries;       // Queries handled by Phase 2
    uint64_t phase3_queries;       // Queries handled by Phase 3
    uint64_t phase4_queries;       // Queries handled by Phase 4
    
    // Cross-phase performance
    uint64_t cross_phase_queries;  // Queries using multiple phases
    uint64_t routing_hits;         // Successful routing decisions
    uint64_t routing_misses;       // Failed routing decisions
    
    // Quality metrics
    float avg_relevance_score;     // Average relevance score
    float avg_confidence_score;    // Average confidence score
    uint64_t cache_hits;           // Cache hits
    uint64_t cache_misses;         // Cache misses
} unified_performance_stats_t;

// Unified indexing system
typedef struct {
    // Phase systems
    advanced_indexing_system_t* phase1_system;      // Multi-dimensional
    optimized_vector_indexing_system_t* phase2_system; // Vector indexing
    hierarchical_indexing_system_t* phase3_system;  // Hierarchical
    specialized_indexing_system_t* phase4_system;   // Specialized
    
    // Routing system
    bool intelligent_routing_enabled;               // Enable smart routing
    bool cross_phase_verification_enabled;         // Enable cross-phase verification
    bool result_caching_enabled;                   // Enable result caching
    bool performance_monitoring_enabled;           // Enable performance monitoring
    
    // Performance tracking
    unified_performance_stats_t stats;             // Performance statistics
    uint64_t system_start_time;                    // System start time
    
    // Configuration
    uint32_t max_concurrent_queries;               // Max concurrent queries
    uint32_t cache_size;                           // Result cache size
    float routing_confidence_threshold;            // Routing confidence threshold
    
    // Status
    bool is_initialized;                           // System initialized
    bool is_optimized;                             // System optimized
    uint32_t total_nodes_indexed;                  // Total nodes indexed
} unified_indexing_system_t;

// ============================================================================
// UNIFIED INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create unified indexing system
int unified_indexing_system_create(unified_indexing_system_t* system, 
                                  uint32_t capacity);

// Initialize all phase systems
int unified_indexing_system_initialize_phases(unified_indexing_system_t* system);

// Add lattice node to unified system
int unified_indexing_system_add_node(unified_indexing_system_t* system, 
                                    const lattice_node_t* node);

// Unified search function
int unified_indexing_system_search(unified_indexing_system_t* system, 
                                  const unified_query_t* query,
                                  unified_search_result_t* results, 
                                  uint32_t* count);

// Intelligent query routing
unified_query_type_t unified_indexing_detect_query_type(const char* query_text);

// Query complexity analysis
unified_query_complexity_t unified_indexing_analyze_complexity(const unified_query_t* query);

// Performance tier determination
unified_performance_tier_t unified_indexing_determine_performance_tier(const unified_query_t* query);

// Cross-phase verification
int unified_indexing_cross_phase_verify(unified_indexing_system_t* system,
                                       const unified_query_t* query,
                                       unified_search_result_t* results,
                                       uint32_t* count);

// Result ranking and merging
int unified_indexing_rank_and_merge_results(unified_search_result_t* results,
                                           uint32_t count,
                                           const unified_query_t* query);

// Performance optimization
int unified_indexing_system_optimize(unified_indexing_system_t* system);

// Get system statistics
int unified_indexing_system_get_stats(unified_indexing_system_t* system,
                                     unified_performance_stats_t* stats);

// Reset performance statistics
void unified_indexing_system_reset_stats(unified_indexing_system_t* system);

// Destroy unified indexing system
void unified_indexing_system_destroy(unified_indexing_system_t* system);

// ============================================================================
// QUERY PROCESSING FUNCTIONS
// ============================================================================

// Create unified query
int unified_query_create(unified_query_t* query, const char* query_text,
                        unified_query_type_t type, uint32_t max_results);

// Destroy unified query
void unified_query_destroy(unified_query_t* query);

// Parse query text for routing hints
int unified_query_parse_routing_hints(unified_query_t* query);

// Validate query parameters
bool unified_query_validate(const unified_query_t* query);

// ============================================================================
// ROUTING AND OPTIMIZATION FUNCTIONS
// ============================================================================

// Route query to best phase system
int unified_indexing_route_query(unified_indexing_system_t* system,
                                const unified_query_t* query,
                                uint32_t* target_phases,
                                uint32_t* phase_count);

// Optimize query for specific phase
int unified_indexing_optimize_query_for_phase(const unified_query_t* query,
                                             uint32_t phase,
                                             unified_query_t* optimized_query);

// Cache management
int unified_indexing_cache_result(unified_indexing_system_t* system,
                                 const unified_query_t* query,
                                 const unified_search_result_t* results,
                                 uint32_t count);

int unified_indexing_get_cached_result(unified_indexing_system_t* system,
                                      const unified_query_t* query,
                                      unified_search_result_t* results,
                                      uint32_t* count);

// Performance monitoring
int unified_indexing_start_performance_monitoring(unified_indexing_system_t* system);
int unified_indexing_stop_performance_monitoring(unified_indexing_system_t* system);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get phase system by ID
void* unified_indexing_get_phase_system(unified_indexing_system_t* system, uint32_t phase);

// Check if phase is available
bool unified_indexing_is_phase_available(unified_indexing_system_t* system, uint32_t phase);

// Get phase performance metrics
int unified_indexing_get_phase_performance(unified_indexing_system_t* system,
                                          uint32_t phase,
                                          uint64_t* queries_handled,
                                          uint64_t* avg_time_us,
                                          float* success_rate);

// System health check
int unified_indexing_system_health_check(unified_indexing_system_t* system,
                                        bool* is_healthy,
                                        char* health_message,
                                        size_t message_size);

#endif // UNIFIED_INDEXING_SYSTEM_H
