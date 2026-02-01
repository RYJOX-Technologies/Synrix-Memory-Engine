#ifndef OPTIMIZED_INVERTED_INDEX_H
#define OPTIMIZED_INVERTED_INDEX_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "persistent_lattice.h"

// ============================================================================
// OPTIMIZED INVERTED INDEX - HIGH PERFORMANCE TEXT SEARCH
// ============================================================================

// Configuration for optimized inverted index
#define OPTIMIZED_INVERTED_INDEX_MAX_TERMS 100000
#define OPTIMIZED_INVERTED_INDEX_MAX_DOCS_PER_TERM 10000
#define OPTIMIZED_INVERTED_INDEX_HASH_TABLE_SIZE 65536  // 2^16 for fast modulo
#define OPTIMIZED_INVERTED_INDEX_MAX_TERM_LENGTH 64
#define OPTIMIZED_INVERTED_INDEX_BATCH_SIZE 32

// Hash table entry for fast term lookup
typedef struct {
    char term[OPTIMIZED_INVERTED_INDEX_MAX_TERM_LENGTH];
    uint32_t term_hash;
    uint32_t posting_list_offset;  // Offset in posting lists array
    uint32_t posting_list_size;    // Number of documents for this term
    uint32_t next;                 // Next entry in hash table (for collision resolution)
    bool is_used;
} optimized_term_entry_t;

// Optimized posting list entry
typedef struct {
    uint32_t node_id;
    uint32_t frequency;    // Term frequency in document
    float tf_idf_score;    // TF-IDF score
    uint32_t position;     // Position in document (for phrase search)
} optimized_posting_entry_t;

// Optimized inverted index structure
typedef struct {
    // Hash table for fast term lookup
    optimized_term_entry_t* hash_table;
    uint32_t hash_table_size;
    uint32_t hash_table_mask;  // For fast modulo operation
    
    // Posting lists storage
    optimized_posting_entry_t* posting_lists;
    uint32_t posting_lists_capacity;
    uint32_t posting_lists_size;
    
    // Document statistics
    uint32_t total_documents;
    uint32_t total_terms;
    uint32_t total_postings;
    
    // Performance optimization
    bool is_optimized;
    uint32_t* term_frequencies;  // Global term frequencies for IDF calculation
    float* idf_scores;           // Precomputed IDF scores
    
    // Memory management
    uint32_t* free_posting_offsets;  // Free list for posting list offsets
    uint32_t free_posting_count;
    uint32_t free_posting_capacity;
} optimized_inverted_index_t;

// Search result with relevance scoring
typedef struct {
    uint32_t node_id;
    float relevance_score;
    uint32_t term_matches;
    float tf_idf_sum;
} optimized_search_result_t;

// Query processing
typedef struct {
    char** terms;
    uint32_t term_count;
    bool use_phrase_search;
    bool use_fuzzy_search;
    float min_relevance_threshold;
} optimized_query_t;

// ============================================================================
// OPTIMIZED INVERTED INDEX FUNCTIONS
// ============================================================================

// Create optimized inverted index
int optimized_inverted_index_create(optimized_inverted_index_t* index, uint32_t capacity);

// Add document to index
int optimized_inverted_index_add_document(optimized_inverted_index_t* index, 
                                        const lattice_node_t* node);

// Add term to document
int optimized_inverted_index_add_term(optimized_inverted_index_t* index, 
                                    const char* term, uint32_t node_id, 
                                    uint32_t frequency, uint32_t position);

// Search for terms
int optimized_inverted_index_search(optimized_inverted_index_t* index, 
                                  const optimized_query_t* query,
                                  optimized_search_result_t* results, 
                                  uint32_t* count);

// Search for single term
int optimized_inverted_index_search_term(optimized_inverted_index_t* index, 
                                       const char* term,
                                       optimized_search_result_t* results, 
                                       uint32_t* count);

// Phrase search
int optimized_inverted_index_search_phrase(optimized_inverted_index_t* index, 
                                         const char* phrase,
                                         optimized_search_result_t* results, 
                                         uint32_t* count);

// Fuzzy search
int optimized_inverted_index_search_fuzzy(optimized_inverted_index_t* index, 
                                        const char* term, uint32_t max_distance,
                                        optimized_search_result_t* results, 
                                        uint32_t* count);

// Optimize index for faster queries
int optimized_inverted_index_optimize(optimized_inverted_index_t* index);

// Get index statistics
int optimized_inverted_index_get_stats(optimized_inverted_index_t* index, 
                                      uint32_t* total_terms, uint32_t* total_documents,
                                      uint32_t* total_postings, float* avg_posting_length);

// Destroy optimized inverted index
void optimized_inverted_index_destroy(optimized_inverted_index_t* index);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Create query from text
int optimized_query_create(optimized_query_t* query, const char* text, 
                          bool use_phrase_search, bool use_fuzzy_search);

// Destroy query
void optimized_query_destroy(optimized_query_t* query);

// Calculate TF-IDF score
float optimized_calculate_tf_idf(uint32_t term_frequency, uint32_t document_length,
                                uint32_t total_documents, uint32_t documents_with_term);

// Calculate relevance score
float optimized_calculate_relevance_score(const optimized_search_result_t* result, 
                                         const optimized_query_t* query);

// Hash function for terms
uint32_t optimized_term_hash(const char* term, size_t length);

// String similarity (for fuzzy search)
float optimized_string_similarity(const char* str1, const char* str2);

// Sort results by relevance
void optimized_sort_results_by_relevance(optimized_search_result_t* results, uint32_t count);

// ============================================================================
// PERFORMANCE MONITORING
// ============================================================================

// Get performance statistics
typedef struct {
    uint64_t total_queries;
    uint64_t total_query_time_us;
    uint64_t avg_query_time_us;
    uint64_t max_query_time_us;
    uint64_t min_query_time_us;
    uint32_t cache_hits;
    uint32_t cache_misses;
    float cache_hit_rate;
} optimized_inverted_index_stats_t;

int optimized_inverted_index_get_performance_stats(optimized_inverted_index_t* index, 
                                                  optimized_inverted_index_stats_t* stats);

// Reset performance statistics
void optimized_inverted_index_reset_stats(optimized_inverted_index_t* index);

#endif // OPTIMIZED_INVERTED_INDEX_H
