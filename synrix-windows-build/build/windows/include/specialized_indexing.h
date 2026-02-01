#ifndef SPECIALIZED_INDEXING_H
#define SPECIALIZED_INDEXING_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "advanced_indexing.h"

// ============================================================================
// SPECIALIZED INDEXING - PHASE 4
// ============================================================================

// Enhanced Bloom filter with multiple hash functions
typedef struct {
    uint8_t* bit_array;             // Bloom filter bits
    uint32_t array_size;            // Size in bits
    uint32_t hash_functions;        // Number of hash functions
    uint32_t element_count;         // Number of elements
    uint32_t false_positive_rate;   // False positive rate (ppm)
    uint32_t* hash_seeds;           // Seeds for hash functions
    uint64_t total_insertions;      // Total insertions
    uint64_t total_queries;         // Total queries
    uint64_t false_positives;       // False positive count
    float actual_fp_rate;           // Actual false positive rate
} enhanced_bloom_filter_t;

// Enhanced inverted index with term frequency and document frequency
typedef struct {
    char term[64];                  // Search term
    uint32_t* document_ids;         // Documents containing this term
    uint32_t* term_frequencies;     // Term frequency in each document
    uint32_t document_count;        // Number of documents
    uint32_t capacity;              // Capacity for documents
    float inverse_doc_frequency;    // IDF score
    float term_weight;              // Term importance weight
    uint64_t last_updated;          // Last update timestamp
    uint32_t total_occurrences;     // Total occurrences across all docs
    float avg_frequency;            // Average frequency per document
} enhanced_inverted_index_entry_t;

// Enhanced inverted index
typedef struct {
    enhanced_inverted_index_entry_t* entries; // Inverted index entries
    uint32_t term_count;            // Number of terms
    uint32_t capacity;              // Term capacity
    uint32_t total_documents;       // Total number of documents
    uint32_t total_terms;           // Total terms across all documents
    float avg_terms_per_doc;        // Average terms per document
    uint64_t last_updated;          // Last update timestamp
    bool case_sensitive;            // Case sensitivity flag
    bool use_stemming;              // Use stemming for terms
    char* stop_words[100];          // Stop words to ignore
    uint32_t stop_word_count;       // Number of stop words
} enhanced_inverted_index_t;

// Enhanced temporal index with time-based queries
typedef struct {
    uint32_t node_id;               // Node ID
    uint64_t start_time;            // Start time (microseconds)
    uint64_t end_time;              // End time (microseconds)
    uint32_t duration;              // Duration (microseconds)
    uint32_t event_type;            // Event type
    uint32_t priority;              // Priority level (0-255)
    float importance_score;         // Importance score (0-1)
    char event_description[256];    // Event description
    uint32_t* related_events;       // Related event IDs
    uint32_t related_count;         // Number of related events
    uint32_t frequency;             // Event frequency
    uint64_t last_occurrence;       // Last occurrence timestamp
} enhanced_temporal_entry_t;

// Enhanced temporal index
typedef struct {
    enhanced_temporal_entry_t* entries; // Temporal entries
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Entry capacity
    uint64_t time_range_start;      // Time range start
    uint64_t time_range_end;        // Time range end
    uint32_t* time_index;           // Time-sorted index
    uint32_t* priority_index;       // Priority-sorted index
    uint32_t* type_index;           // Type-sorted index
    uint32_t index_size;            // Index size
    bool is_sorted;                 // Whether entries are sorted
    uint64_t last_sort;             // Last sort timestamp
} enhanced_temporal_index_t;

// Geographic index for spatial queries
typedef struct {
    uint32_t node_id;               // Node ID
    float latitude;                 // Latitude coordinate
    float longitude;                // Longitude coordinate
    float altitude;                 // Altitude coordinate
    float accuracy;                 // Coordinate accuracy
    char location_name[128];        // Location name
    uint32_t location_type;         // Location type
    float bounding_box[4];          // Bounding box [min_lat, max_lat, min_lon, max_lon]
    uint32_t* nearby_nodes;         // Nearby node IDs
    uint32_t nearby_count;          // Number of nearby nodes
} geographic_entry_t;

// Geographic index
typedef struct {
    geographic_entry_t* entries;    // Geographic entries
    uint32_t count;                 // Number of entries
    uint32_t capacity;              // Entry capacity
    float* latitude_index;          // Latitude-sorted index
    float* longitude_index;         // Longitude-sorted index
    uint32_t* spatial_index;        // Spatial hash index
    uint32_t grid_size;             // Spatial grid size
    float grid_resolution;          // Grid resolution in degrees
    bool is_indexed;                // Whether spatial index is built
} geographic_index_t;

// Full-text search index
typedef struct {
    char term[64];                  // Search term
    uint32_t* document_ids;         // Documents containing term
    uint32_t* positions;            // Term positions in documents
    uint32_t* document_positions;   // Document position mapping
    uint32_t total_occurrences;     // Total occurrences
    uint32_t document_count;        // Number of documents
    float tf_idf_score;             // TF-IDF score
    uint32_t term_length;           // Term length
    bool is_stemmed;                // Whether term is stemmed
    char original_term[64];         // Original term before stemming
} fulltext_index_entry_t;

// Full-text search index
typedef struct {
    fulltext_index_entry_t* entries; // Full-text entries
    uint32_t term_count;            // Number of terms
    uint32_t capacity;              // Term capacity
    uint32_t total_documents;       // Total documents
    uint32_t total_terms;           // Total terms
    char* document_texts;           // Document text storage
    uint32_t* document_lengths;     // Document lengths
    uint32_t* document_offsets;     // Document text offsets
    bool use_stemming;              // Use stemming
    bool case_sensitive;            // Case sensitivity
    char* stop_words[200];          // Stop words
    uint32_t stop_word_count;       // Stop word count
} fulltext_index_t;

// Specialized indexing system
typedef struct {
    enhanced_bloom_filter_t* bloom_filter;     // Bloom filter
    enhanced_inverted_index_t* inverted_index; // Inverted index
    enhanced_temporal_index_t* temporal_index; // Temporal index
    geographic_index_t* geographic_index;      // Geographic index
    fulltext_index_t* fulltext_index;          // Full-text index
    bool is_initialized;                       // Initialization status
    uint64_t last_update;                      // Last update timestamp
    uint32_t total_operations;                 // Total operations
    float avg_query_time;                      // Average query time
} specialized_indexing_system_t;

// ============================================================================
// BLOOM FILTER FUNCTIONS
// ============================================================================

// Create enhanced Bloom filter
int enhanced_bloom_filter_create(enhanced_bloom_filter_t* filter, uint32_t expected_elements, float false_positive_rate);

// Add element to Bloom filter
int enhanced_bloom_filter_add(enhanced_bloom_filter_t* filter, const char* key);

// Check if element exists in Bloom filter
bool enhanced_bloom_filter_contains(enhanced_bloom_filter_t* filter, const char* key);

// Get Bloom filter statistics
int enhanced_bloom_filter_get_stats(enhanced_bloom_filter_t* filter, uint32_t* elements, float* fp_rate, uint64_t* queries);

// Resize Bloom filter
int enhanced_bloom_filter_resize(enhanced_bloom_filter_t* filter, uint32_t new_size);

// Destroy Bloom filter
void enhanced_bloom_filter_destroy(enhanced_bloom_filter_t* filter);

// ============================================================================
// INVERTED INDEX FUNCTIONS
// ============================================================================

// Create enhanced inverted index
int enhanced_inverted_index_create(enhanced_inverted_index_t* index, uint32_t capacity);

// Add document to inverted index
int enhanced_inverted_index_add_document(enhanced_inverted_index_t* index, uint32_t doc_id, const char* text);

// Search inverted index
int enhanced_inverted_index_search(enhanced_inverted_index_t* index, const char* query, uint32_t* results, uint32_t* count);

// Search with boolean operators
int enhanced_inverted_index_boolean_search(enhanced_inverted_index_t* index, const char* query, uint32_t* results, uint32_t* count);

// Calculate TF-IDF scores
int enhanced_inverted_index_calculate_tfidf(enhanced_inverted_index_t* index);

// Get term statistics
int enhanced_inverted_index_get_term_stats(enhanced_inverted_index_t* index, const char* term, uint32_t* doc_count, float* idf_score);

// Destroy inverted index
void enhanced_inverted_index_destroy(enhanced_inverted_index_t* index);

// ============================================================================
// TEMPORAL INDEX FUNCTIONS
// ============================================================================

// Create enhanced temporal index
int enhanced_temporal_index_create(enhanced_temporal_index_t* index, uint32_t capacity);

// Add temporal entry
int enhanced_temporal_index_add_entry(enhanced_temporal_index_t* index, const enhanced_temporal_entry_t* entry);

// Search by time range
int enhanced_temporal_index_search_time_range(enhanced_temporal_index_t* index, uint64_t start_time, uint64_t end_time,
                                             uint32_t* results, uint32_t* count);

// Search by event type
int enhanced_temporal_index_search_by_type(enhanced_temporal_index_t* index, uint32_t event_type, uint32_t* results, uint32_t* count);

// Search by priority
int enhanced_temporal_index_search_by_priority(enhanced_temporal_index_t* index, uint32_t min_priority, uint32_t max_priority,
                                              uint32_t* results, uint32_t* count);

// Get temporal statistics
int enhanced_temporal_index_get_stats(enhanced_temporal_index_t* index, uint32_t* total_events, uint64_t* time_span, float* avg_duration);

// Sort temporal entries
int enhanced_temporal_index_sort(enhanced_temporal_index_t* index, bool by_time, bool by_priority, bool by_type);

// Destroy temporal index
void enhanced_temporal_index_destroy(enhanced_temporal_index_t* index);

// ============================================================================
// GEOGRAPHIC INDEX FUNCTIONS
// ============================================================================

// Create geographic index
int geographic_index_create(geographic_index_t* index, uint32_t capacity, float grid_resolution);

// Add geographic entry
int geographic_index_add_entry(geographic_index_t* index, const geographic_entry_t* entry);

// Search by bounding box
int geographic_index_search_bounding_box(geographic_index_t* index, float min_lat, float max_lat, float min_lon, float max_lon,
                                        uint32_t* results, uint32_t* count);

// Search by radius
int geographic_index_search_radius(geographic_index_t* index, float center_lat, float center_lon, float radius_km,
                                  uint32_t* results, uint32_t* count);

// Find nearest neighbors
int geographic_index_find_nearest(geographic_index_t* index, float lat, float lon, uint32_t count, uint32_t* results);

// Build spatial index
int geographic_index_build_spatial_index(geographic_index_t* index);

// Destroy geographic index
void geographic_index_destroy(geographic_index_t* index);

// ============================================================================
// FULL-TEXT SEARCH FUNCTIONS
// ============================================================================

// Create full-text index
int fulltext_index_create(fulltext_index_t* index, uint32_t capacity);

// Add document to full-text index
int fulltext_index_add_document(fulltext_index_t* index, uint32_t doc_id, const char* text);

// Search full-text index
int fulltext_index_search(fulltext_index_t* index, const char* query, uint32_t* results, uint32_t* count);

// Search with phrase matching
int fulltext_index_search_phrase(fulltext_index_t* index, const char* phrase, uint32_t* results, uint32_t* count);

// Search with wildcards
int fulltext_index_search_wildcard(fulltext_index_t* index, const char* pattern, uint32_t* results, uint32_t* count);

// Calculate relevance scores
int fulltext_index_calculate_relevance(fulltext_index_t* index, const char* query, uint32_t* doc_ids, float* scores, uint32_t count);

// Destroy full-text index
void fulltext_index_destroy(fulltext_index_t* index);

// ============================================================================
// SPECIALIZED INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create specialized indexing system
int specialized_indexing_system_create(specialized_indexing_system_t* system);

// Add lattice node to specialized system
int specialized_indexing_system_add_node(specialized_indexing_system_t* system, const lattice_node_t* node);

// Search specialized system
int specialized_indexing_system_search(specialized_indexing_system_t* system, const char* query, uint32_t* results, uint32_t* count);

// Get system statistics
int specialized_indexing_system_get_stats(specialized_indexing_system_t* system, uint32_t* total_entries, float* avg_query_time);

// Optimize system
int specialized_indexing_system_optimize(specialized_indexing_system_t* system);

// Destroy specialized indexing system
void specialized_indexing_system_destroy(specialized_indexing_system_t* system);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate distance between two geographic points
float calculate_geographic_distance(float lat1, float lon1, float lat2, float lon2);

// Stem word (simple implementation)
int stem_word(const char* word, char* stemmed, size_t size);

// Check if word is stop word
bool is_stop_word(const char* word, const char** stop_words, uint32_t stop_word_count);

// Calculate TF-IDF score
float calculate_tfidf_score(uint32_t term_freq, uint32_t doc_freq, uint32_t total_docs);

// Normalize text for indexing
int normalize_text(const char* input, char* output, size_t size, bool case_sensitive, bool remove_punctuation);

// Generate hash for Bloom filter
uint32_t generate_bloom_hash(const char* key, uint32_t seed);

// Sort array of integers
void sort_uint32_array(uint32_t* array, uint32_t count);

// Remove duplicates from array
uint32_t remove_duplicates_uint32(uint32_t* array, uint32_t count);

#endif // SPECIALIZED_INDEXING_H
