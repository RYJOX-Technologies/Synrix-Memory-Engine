#define _GNU_SOURCE
#include "specialized_indexing.h"
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>

// Forward declarations
static uint64_t get_time_us(void);

// ============================================================================
// BLOOM FILTER FUNCTIONS
// ============================================================================

// Create enhanced Bloom filter
int enhanced_bloom_filter_create(enhanced_bloom_filter_t* filter, uint32_t expected_elements, float false_positive_rate) {
    if (!filter || expected_elements == 0 || false_positive_rate <= 0.0f || false_positive_rate >= 1.0f) {
        return -1;
    }
    
    memset(filter, 0, sizeof(enhanced_bloom_filter_t));
    
    // Calculate optimal array size and hash count
    float ln2 = log(2.0);
    float ln_fp = log(false_positive_rate);
    
    filter->array_size = (uint32_t)(-(expected_elements * ln_fp) / (ln2 * ln2));
    filter->hash_functions = (uint32_t)((filter->array_size / expected_elements) * ln2);
    
    // Ensure minimum hash functions
    if (filter->hash_functions < 1) filter->hash_functions = 1;
    if (filter->hash_functions > 8) filter->hash_functions = 8;
    
    // Round up to nearest byte
    filter->array_size = (filter->array_size + 7) / 8;
    
    filter->bit_array = (uint8_t*)calloc(filter->array_size, sizeof(uint8_t));
    if (!filter->bit_array) return -1;
    
    // Generate hash seeds
    filter->hash_seeds = (uint32_t*)malloc(filter->hash_functions * sizeof(uint32_t));
    if (!filter->hash_seeds) {
        free(filter->bit_array);
        return -1;
    }
    
    srand(time(NULL));
    for (uint32_t i = 0; i < filter->hash_functions; i++) {
        filter->hash_seeds[i] = rand();
    }
    
    filter->element_count = 0;
    filter->false_positive_rate = false_positive_rate;
    filter->actual_fp_rate = 0.0f;
    
    return 0;
}

// Add element to Bloom filter
int enhanced_bloom_filter_add(enhanced_bloom_filter_t* filter, const char* key) {
    if (!filter || !key) return -1;
    
    size_t key_len = strlen(key);
    if (key_len == 0) return -1;
    
    // Set bits for all hash functions
    for (uint32_t i = 0; i < filter->hash_functions; i++) {
        uint32_t hash = generate_bloom_hash(key, filter->hash_seeds[i]);
        uint32_t bit_pos = hash % (filter->array_size * 8);
        uint32_t byte_pos = bit_pos / 8;
        uint32_t bit_offset = bit_pos % 8;
        
        filter->bit_array[byte_pos] |= (1 << bit_offset);
    }
    
    filter->element_count++;
    filter->total_insertions++;
    
    return 0;
}

// Check if element exists in Bloom filter
bool enhanced_bloom_filter_contains(enhanced_bloom_filter_t* filter, const char* key) {
    if (!filter || !key) return false;
    
    size_t key_len = strlen(key);
    if (key_len == 0) return false;
    
    filter->total_queries++;
    
    // Check bits for all hash functions
    for (uint32_t i = 0; i < filter->hash_functions; i++) {
        uint32_t hash = generate_bloom_hash(key, filter->hash_seeds[i]);
        uint32_t bit_pos = hash % (filter->array_size * 8);
        uint32_t byte_pos = bit_pos / 8;
        uint32_t bit_offset = bit_pos % 8;
        
        if (!(filter->bit_array[byte_pos] & (1 << bit_offset))) {
            return false; // Definitely not in set
        }
    }
    
    // All bits are set - might be in set (could be false positive)
    return true;
}

// Get Bloom filter statistics
int enhanced_bloom_filter_get_stats(enhanced_bloom_filter_t* filter, uint32_t* elements, float* fp_rate, uint64_t* queries) {
    if (!filter || !elements || !fp_rate || !queries) return -1;
    
    *elements = filter->element_count;
    *fp_rate = filter->actual_fp_rate;
    *queries = filter->total_queries;
    
    return 0;
}

// Resize Bloom filter
int enhanced_bloom_filter_resize(enhanced_bloom_filter_t* filter, uint32_t new_size) {
    if (!filter || new_size == 0) return -1;
    
    // TODO: Implement Bloom filter resizing
    // This is a simplified version for demonstration
    
    return 0;
}

// Destroy Bloom filter
void enhanced_bloom_filter_destroy(enhanced_bloom_filter_t* filter) {
    if (!filter) return;
    
    if (filter->bit_array) free(filter->bit_array);
    if (filter->hash_seeds) free(filter->hash_seeds);
    
    memset(filter, 0, sizeof(enhanced_bloom_filter_t));
}

// ============================================================================
// INVERTED INDEX FUNCTIONS
// ============================================================================

// Create enhanced inverted index
int enhanced_inverted_index_create(enhanced_inverted_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(enhanced_inverted_index_t));
    index->capacity = capacity;
    index->case_sensitive = false;
    index->use_stemming = false;
    
    index->entries = (enhanced_inverted_index_entry_t*)calloc(capacity, sizeof(enhanced_inverted_index_entry_t));
    if (!index->entries) return -1;
    
    // Initialize stop words
    const char* default_stop_words[] = {
        "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by"
    };
    
    for (uint32_t i = 0; i < 14 && i < 100; i++) {
        index->stop_words[i] = (char*)malloc(32);
        strcpy(index->stop_words[i], default_stop_words[i]);
    }
    index->stop_word_count = 14;
    
    return 0;
}

// Add document to inverted index
int enhanced_inverted_index_add_document(enhanced_inverted_index_t* index, uint32_t doc_id, const char* text) {
    if (!index || !text || doc_id == 0) return -1;
    
    // Simple tokenization (split by spaces)
    char* text_copy = strdup(text);
    if (!text_copy) return -1;
    
    char* token = strtok(text_copy, " \t\n\r");
    while (token != NULL) {
        // Normalize token
        char normalized[64];
        if (normalize_text(token, normalized, sizeof(normalized), index->case_sensitive, true) != 0) {
            token = strtok(NULL, " \t\n\r");
            continue;
        }
        
        // Skip stop words
        if (is_stop_word(normalized, (const char**)index->stop_words, index->stop_word_count)) {
            token = strtok(NULL, " \t\n\r");
            continue;
        }
        
        // Stem word if enabled
        if (index->use_stemming) {
            char stemmed[64];
            if (stem_word(normalized, stemmed, sizeof(stemmed)) == 0) {
                strcpy(normalized, stemmed);
            }
        }
        
        // Find or create term entry
        uint32_t term_index = 0;
        bool found = false;
        
        for (uint32_t i = 0; i < index->term_count; i++) {
            if (strcmp(index->entries[i].term, normalized) == 0) {
                term_index = i;
                found = true;
                break;
            }
        }
        
        if (!found) {
            if (index->term_count >= index->capacity) {
                free(text_copy);
                return -1;
            }
            
            term_index = index->term_count;
            strncpy(index->entries[term_index].term, normalized, sizeof(index->entries[term_index].term) - 1);
            index->entries[term_index].document_ids = (uint32_t*)calloc(1000, sizeof(uint32_t));
            index->entries[term_index].term_frequencies = (uint32_t*)calloc(1000, sizeof(uint32_t));
            index->entries[term_index].capacity = 1000;
            index->entries[term_index].document_count = 0;
            index->entries[term_index].total_occurrences = 0;
            index->entries[term_index].term_weight = 1.0f;
            index->term_count++;
        }
        
        // Add document to term
        enhanced_inverted_index_entry_t* entry = &index->entries[term_index];
        
        // Check if document already exists
        bool doc_exists = false;
        for (uint32_t i = 0; i < entry->document_count; i++) {
            if (entry->document_ids[i] == doc_id) {
                entry->term_frequencies[i]++;
                doc_exists = true;
                break;
            }
        }
        
        if (!doc_exists && entry->document_count < entry->capacity) {
            entry->document_ids[entry->document_count] = doc_id;
            entry->term_frequencies[entry->document_count] = 1;
            entry->document_count++;
        }
        
        entry->total_occurrences++;
        index->total_terms++;
        
        token = strtok(NULL, " \t\n\r");
    }
    
    free(text_copy);
    index->total_documents++;
    index->last_updated = get_time_us();
    
    return 0;
}

// Search inverted index
int enhanced_inverted_index_search(enhanced_inverted_index_t* index, const char* query, uint32_t* results, uint32_t* count) {
    if (!index || !query || !results || !count) return -1;
    
    *count = 0;
    
    // Simple query processing (split by spaces)
    char* query_copy = strdup(query);
    if (!query_copy) return -1;
    
    char* token = strtok(query_copy, " \t\n\r");
    while (token != NULL) {
        // Normalize token
        char normalized[64];
        if (normalize_text(token, normalized, sizeof(normalized), index->case_sensitive, true) != 0) {
            token = strtok(NULL, " \t\n\r");
            continue;
        }
        
        // Find term
        for (uint32_t i = 0; i < index->term_count; i++) {
            if (strcmp(index->entries[i].term, normalized) == 0) {
                // Add documents to results
                for (uint32_t j = 0; j < index->entries[i].document_count && *count < 1000; j++) {
                    results[*count] = index->entries[i].document_ids[j];
                    (*count)++;
                }
                break;
            }
        }
        
        token = strtok(NULL, " \t\n\r");
    }
    
    free(query_copy);
    return 0;
}

// Search with boolean operators
int enhanced_inverted_index_boolean_search(enhanced_inverted_index_t* index, const char* query, uint32_t* results, uint32_t* count) {
    if (!index || !query || !results || !count) return -1;
    
    // TODO: Implement boolean search (AND, OR, NOT)
    // This is a simplified version for demonstration
    
    return enhanced_inverted_index_search(index, query, results, count);
}

// Calculate TF-IDF scores
int enhanced_inverted_index_calculate_tfidf(enhanced_inverted_index_t* index) {
    if (!index) return -1;
    
    for (uint32_t i = 0; i < index->term_count; i++) {
        enhanced_inverted_index_entry_t* entry = &index->entries[i];
        
        // Calculate IDF
        entry->inverse_doc_frequency = log((float)index->total_documents / entry->document_count);
        
        // Calculate average frequency
        entry->avg_frequency = (float)entry->total_occurrences / entry->document_count;
    }
    
    return 0;
}

// Get term statistics
int enhanced_inverted_index_get_term_stats(enhanced_inverted_index_t* index, const char* term, uint32_t* doc_count, float* idf_score) {
    if (!index || !term || !doc_count || !idf_score) return -1;
    
    for (uint32_t i = 0; i < index->term_count; i++) {
        if (strcmp(index->entries[i].term, term) == 0) {
            *doc_count = index->entries[i].document_count;
            *idf_score = index->entries[i].inverse_doc_frequency;
            return 0;
        }
    }
    
    return -1; // Term not found
}

// Destroy inverted index
void enhanced_inverted_index_destroy(enhanced_inverted_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        for (uint32_t i = 0; i < index->term_count; i++) {
            if (index->entries[i].document_ids) free(index->entries[i].document_ids);
            if (index->entries[i].term_frequencies) free(index->entries[i].term_frequencies);
        }
        free(index->entries);
    }
    
    if (index->stop_words) {
        for (uint32_t i = 0; i < index->stop_word_count; i++) {
            if (index->entries[i].document_ids) free(index->stop_words[i]);
        }
    }
    
    memset(index, 0, sizeof(enhanced_inverted_index_t));
}

// ============================================================================
// TEMPORAL INDEX FUNCTIONS
// ============================================================================

// Create enhanced temporal index
int enhanced_temporal_index_create(enhanced_temporal_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(enhanced_temporal_index_t));
    index->capacity = capacity;
    index->time_range_start = UINT64_MAX;
    index->time_range_end = 0;
    
    index->entries = (enhanced_temporal_entry_t*)calloc(capacity, sizeof(enhanced_temporal_entry_t));
    if (!index->entries) return -1;
    
    index->time_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    index->priority_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    index->type_index = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    
    if (!index->time_index || !index->priority_index || !index->type_index) {
        enhanced_temporal_index_destroy(index);
        return -1;
    }
    
    return 0;
}

// Add temporal entry
int enhanced_temporal_index_add_entry(enhanced_temporal_index_t* index, const enhanced_temporal_entry_t* entry) {
    if (!index || !entry || index->count >= index->capacity) return -1;
    
    index->entries[index->count] = *entry;
    
    // Update time range
    if (entry->start_time < index->time_range_start) {
        index->time_range_start = entry->start_time;
    }
    if (entry->end_time > index->time_range_end) {
        index->time_range_end = entry->end_time;
    }
    
    // Update indexes
    index->time_index[index->count] = index->count;
    index->priority_index[index->count] = index->count;
    index->type_index[index->count] = index->count;
    
    index->count++;
    return 0;
}

// Search by time range
int enhanced_temporal_index_search_time_range(enhanced_temporal_index_t* index, uint64_t start_time, uint64_t end_time,
                                             uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        enhanced_temporal_entry_t* entry = &index->entries[i];
        
        // Check for time overlap
        if (entry->start_time <= end_time && entry->end_time >= start_time) {
            results[*count] = entry->node_id;
            (*count)++;
        }
    }
    
    return 0;
}

// Search by event type
int enhanced_temporal_index_search_by_type(enhanced_temporal_index_t* index, uint32_t event_type, uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        if (index->entries[i].event_type == event_type) {
            results[*count] = index->entries[i].node_id;
            (*count)++;
        }
    }
    
    return 0;
}

// Search by priority
int enhanced_temporal_index_search_by_priority(enhanced_temporal_index_t* index, uint32_t min_priority, uint32_t max_priority,
                                              uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        if (index->entries[i].priority >= min_priority && index->entries[i].priority <= max_priority) {
            results[*count] = index->entries[i].node_id;
            (*count)++;
        }
    }
    
    return 0;
}

// Get temporal statistics
int enhanced_temporal_index_get_stats(enhanced_temporal_index_t* index, uint32_t* total_events, uint64_t* time_span, float* avg_duration) {
    if (!index || !total_events || !time_span || !avg_duration) return -1;
    
    *total_events = index->count;
    *time_span = index->time_range_end - index->time_range_start;
    
    float total_duration = 0.0f;
    for (uint32_t i = 0; i < index->count; i++) {
        total_duration += index->entries[i].duration;
    }
    
    *avg_duration = index->count > 0 ? total_duration / index->count : 0.0f;
    
    return 0;
}

// Sort temporal entries
int enhanced_temporal_index_sort(enhanced_temporal_index_t* index, bool by_time, bool by_priority, bool by_type) {
    if (!index) return -1;
    
    // TODO: Implement sorting
    // This is a simplified version for demonstration
    
    index->is_sorted = true;
    index->last_sort = get_time_us();
    
    return 0;
}

// Destroy temporal index
void enhanced_temporal_index_destroy(enhanced_temporal_index_t* index) {
    if (!index) return;
    
    if (index->entries) free(index->entries);
    if (index->time_index) free(index->time_index);
    if (index->priority_index) free(index->priority_index);
    if (index->type_index) free(index->type_index);
    
    memset(index, 0, sizeof(enhanced_temporal_index_t));
}

// ============================================================================
// GEOGRAPHIC INDEX FUNCTIONS
// ============================================================================

// Create geographic index
int geographic_index_create(geographic_index_t* index, uint32_t capacity, float grid_resolution) {
    if (!index || capacity == 0 || grid_resolution <= 0.0f) return -1;
    
    memset(index, 0, sizeof(geographic_index_t));
    index->capacity = capacity;
    index->grid_resolution = grid_resolution;
    index->grid_size = (uint32_t)(360.0f / grid_resolution);
    
    index->entries = (geographic_entry_t*)calloc(capacity, sizeof(geographic_entry_t));
    if (!index->entries) return -1;
    
    index->latitude_index = (float*)calloc(capacity, sizeof(float));
    index->longitude_index = (float*)calloc(capacity, sizeof(float));
    index->spatial_index = (uint32_t*)calloc(index->grid_size * index->grid_size, sizeof(uint32_t));
    
    if (!index->latitude_index || !index->longitude_index || !index->spatial_index) {
        geographic_index_destroy(index);
        return -1;
    }
    
    return 0;
}

// Add geographic entry
int geographic_index_add_entry(geographic_index_t* index, const geographic_entry_t* entry) {
    if (!index || !entry || index->count >= index->capacity) return -1;
    
    index->entries[index->count] = *entry;
    index->latitude_index[index->count] = entry->latitude;
    index->longitude_index[index->count] = entry->longitude;
    
    index->count++;
    return 0;
}

// Search by bounding box
int geographic_index_search_bounding_box(geographic_index_t* index, float min_lat, float max_lat, float min_lon, float max_lon,
                                        uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        geographic_entry_t* entry = &index->entries[i];
        
        if (entry->latitude >= min_lat && entry->latitude <= max_lat &&
            entry->longitude >= min_lon && entry->longitude <= max_lon) {
            results[*count] = entry->node_id;
            (*count)++;
        }
    }
    
    return 0;
}

// Search by radius
int geographic_index_search_radius(geographic_index_t* index, float center_lat, float center_lon, float radius_km,
                                  uint32_t* results, uint32_t* count) {
    if (!index || !results || !count) return -1;
    
    *count = 0;
    
    for (uint32_t i = 0; i < index->count; i++) {
        geographic_entry_t* entry = &index->entries[i];
        
        float distance = calculate_geographic_distance(center_lat, center_lon, entry->latitude, entry->longitude);
        if (distance <= radius_km) {
            results[*count] = entry->node_id;
            (*count)++;
        }
    }
    
    return 0;
}

// Find nearest neighbors
int geographic_index_find_nearest(geographic_index_t* index, float lat, float lon, uint32_t count, uint32_t* results) {
    if (!index || !results || count == 0) return -1;
    
    // TODO: Implement nearest neighbor search
    // This is a simplified version for demonstration
    
    return 0;
}

// Build spatial index
int geographic_index_build_spatial_index(geographic_index_t* index) {
    if (!index) return -1;
    
    // TODO: Implement spatial index building
    // This is a simplified version for demonstration
    
    index->is_indexed = true;
    return 0;
}

// Destroy geographic index
void geographic_index_destroy(geographic_index_t* index) {
    if (!index) return;
    
    if (index->entries) free(index->entries);
    if (index->latitude_index) free(index->latitude_index);
    if (index->longitude_index) free(index->longitude_index);
    if (index->spatial_index) free(index->spatial_index);
    
    memset(index, 0, sizeof(geographic_index_t));
}

// ============================================================================
// FULL-TEXT SEARCH FUNCTIONS
// ============================================================================

// Create full-text index
int fulltext_index_create(fulltext_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(fulltext_index_t));
    index->capacity = capacity;
    index->case_sensitive = false;
    index->use_stemming = false;
    
    index->entries = (fulltext_index_entry_t*)calloc(capacity, sizeof(fulltext_index_entry_t));
    if (!index->entries) return -1;
    
    return 0;
}

// Add document to full-text index
int fulltext_index_add_document(fulltext_index_t* index, uint32_t doc_id, const char* text) {
    if (!index || !text || doc_id == 0) return -1;
    
    // TODO: Implement full-text indexing
    // This is a simplified version for demonstration
    
    index->total_documents++;
    return 0;
}

// Search full-text index
int fulltext_index_search(fulltext_index_t* index, const char* query, uint32_t* results, uint32_t* count) {
    if (!index || !query || !results || !count) return -1;
    
    // TODO: Implement full-text search
    // This is a simplified version for demonstration
    
    *count = 0;
    return 0;
}

// Search with phrase matching
int fulltext_index_search_phrase(fulltext_index_t* index, const char* phrase, uint32_t* results, uint32_t* count) {
    if (!index || !phrase || !results || !count) return -1;
    
    // TODO: Implement phrase search
    // This is a simplified version for demonstration
    
    *count = 0;
    return 0;
}

// Search with wildcards
int fulltext_index_search_wildcard(fulltext_index_t* index, const char* pattern, uint32_t* results, uint32_t* count) {
    if (!index || !pattern || !results || !count) return -1;
    
    // TODO: Implement wildcard search
    // This is a simplified version for demonstration
    
    *count = 0;
    return 0;
}

// Calculate relevance scores
int fulltext_index_calculate_relevance(fulltext_index_t* index, const char* query, uint32_t* doc_ids, float* scores, uint32_t count) {
    if (!index || !query || !doc_ids || !scores || count == 0) return -1;
    
    // TODO: Implement relevance scoring
    // This is a simplified version for demonstration
    
    for (uint32_t i = 0; i < count; i++) {
        scores[i] = 1.0f;
    }
    
    return 0;
}

// Destroy full-text index
void fulltext_index_destroy(fulltext_index_t* index) {
    if (!index) return;
    
    if (index->entries) {
        for (uint32_t i = 0; i < index->term_count; i++) {
            if (index->entries[i].document_ids) free(index->entries[i].document_ids);
            if (index->entries[i].positions) free(index->entries[i].positions);
            if (index->entries[i].document_positions) free(index->entries[i].document_positions);
        }
        free(index->entries);
    }
    
    if (index->document_texts) free(index->document_texts);
    if (index->document_lengths) free(index->document_lengths);
    if (index->document_offsets) free(index->document_offsets);
    
    if (index->stop_words) {
        for (uint32_t i = 0; i < index->stop_word_count; i++) {
            if (index->stop_words[i]) free(index->stop_words[i]);
        }
    }
    
    memset(index, 0, sizeof(fulltext_index_t));
}

// ============================================================================
// SPECIALIZED INDEXING SYSTEM FUNCTIONS
// ============================================================================

// Create specialized indexing system
int specialized_indexing_system_create(specialized_indexing_system_t* system) {
    if (!system) return -1;
    
    memset(system, 0, sizeof(specialized_indexing_system_t));
    
    system->bloom_filter = (enhanced_bloom_filter_t*)malloc(sizeof(enhanced_bloom_filter_t));
    system->inverted_index = (enhanced_inverted_index_t*)malloc(sizeof(enhanced_inverted_index_t));
    system->temporal_index = (enhanced_temporal_index_t*)malloc(sizeof(enhanced_temporal_index_t));
    system->geographic_index = (geographic_index_t*)malloc(sizeof(geographic_index_t));
    system->fulltext_index = (fulltext_index_t*)malloc(sizeof(fulltext_index_t));
    
    if (!system->bloom_filter || !system->inverted_index || !system->temporal_index ||
        !system->geographic_index || !system->fulltext_index) {
        specialized_indexing_system_destroy(system);
        return -1;
    }
    
    if (enhanced_bloom_filter_create(system->bloom_filter, 100000, 0.01f) != 0) {
        specialized_indexing_system_destroy(system);
        return -1;
    }
    
    if (enhanced_inverted_index_create(system->inverted_index, 10000) != 0) {
        specialized_indexing_system_destroy(system);
        return -1;
    }
    
    if (enhanced_temporal_index_create(system->temporal_index, 10000) != 0) {
        specialized_indexing_system_destroy(system);
        return -1;
    }
    
    if (geographic_index_create(system->geographic_index, 1000, 0.1f) != 0) {
        specialized_indexing_system_destroy(system);
        return -1;
    }
    
    if (fulltext_index_create(system->fulltext_index, 10000) != 0) {
        specialized_indexing_system_destroy(system);
        return -1;
    }
    
    system->is_initialized = true;
    return 0;
}

// Add lattice node to specialized system
int specialized_indexing_system_add_node(specialized_indexing_system_t* system, const lattice_node_t* node) {
    if (!system || !node || !system->is_initialized) return -1;
    
    // Add to Bloom filter
    enhanced_bloom_filter_add(system->bloom_filter, node->name);
    
    // Add to inverted index
    enhanced_inverted_index_add_document(system->inverted_index, node->id, node->data);
    
    // Add to full-text index
    fulltext_index_add_document(system->fulltext_index, node->id, node->data);
    
    system->last_update = node->timestamp;
    system->total_operations++;
    
    return 0;
}

// Search specialized system
int specialized_indexing_system_search(specialized_indexing_system_t* system, const char* query, uint32_t* results, uint32_t* count) {
    if (!system || !query || !results || !count || !system->is_initialized) return -1;
    
    *count = 0;
    
    // Use inverted index for text search
    return enhanced_inverted_index_search(system->inverted_index, query, results, count);
}

// Get system statistics
int specialized_indexing_system_get_stats(specialized_indexing_system_t* system, uint32_t* total_entries, float* avg_query_time) {
    if (!system || !total_entries || !avg_query_time) return -1;
    
    *total_entries = system->total_operations;
    *avg_query_time = system->avg_query_time;
    
    return 0;
}

// Optimize system
int specialized_indexing_system_optimize(specialized_indexing_system_t* system) {
    if (!system || !system->is_initialized) return -1;
    
    // Calculate TF-IDF scores
    enhanced_inverted_index_calculate_tfidf(system->inverted_index);
    
    // Sort temporal entries
    enhanced_temporal_index_sort(system->temporal_index, true, false, false);
    
    // Build spatial index
    geographic_index_build_spatial_index(system->geographic_index);
    
    return 0;
}

// Destroy specialized indexing system
void specialized_indexing_system_destroy(specialized_indexing_system_t* system) {
    if (!system) return;
    
    if (system->bloom_filter) {
        enhanced_bloom_filter_destroy(system->bloom_filter);
        free(system->bloom_filter);
    }
    
    if (system->inverted_index) {
        enhanced_inverted_index_destroy(system->inverted_index);
        free(system->inverted_index);
    }
    
    if (system->temporal_index) {
        enhanced_temporal_index_destroy(system->temporal_index);
        free(system->temporal_index);
    }
    
    if (system->geographic_index) {
        geographic_index_destroy(system->geographic_index);
        free(system->geographic_index);
    }
    
    if (system->fulltext_index) {
        fulltext_index_destroy(system->fulltext_index);
        free(system->fulltext_index);
    }
    
    memset(system, 0, sizeof(specialized_indexing_system_t));
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Calculate distance between two geographic points
float calculate_geographic_distance(float lat1, float lon1, float lat2, float lon2) {
    const float R = 6371.0f; // Earth's radius in kilometers
    
    float dlat = (lat2 - lat1) * M_PI / 180.0f;
    float dlon = (lon2 - lon1) * M_PI / 180.0f;
    
    float a = sinf(dlat/2) * sinf(dlat/2) + cosf(lat1 * M_PI / 180.0f) * cosf(lat2 * M_PI / 180.0f) * sinf(dlon/2) * sinf(dlon/2);
    float c = 2 * atan2f(sqrtf(a), sqrtf(1-a));
    
    return R * c;
}

// Stem word (simple implementation)
int stem_word(const char* word, char* stemmed, size_t size) {
    if (!word || !stemmed || size == 0) return -1;
    
    // Simple stemming: remove common suffixes
    size_t len = strlen(word);
    if (len < 4) {
        strncpy(stemmed, word, size - 1);
        stemmed[size - 1] = '\0';
        return 0;
    }
    
    strncpy(stemmed, word, size - 1);
    stemmed[size - 1] = '\0';
    
    // Remove common suffixes
    if (len > 4 && strcmp(word + len - 4, "ing") == 0) {
        stemmed[len - 4] = '\0';
    } else if (len > 3 && strcmp(word + len - 3, "ed") == 0) {
        stemmed[len - 3] = '\0';
    } else if (len > 3 && strcmp(word + len - 3, "er") == 0) {
        stemmed[len - 3] = '\0';
    } else if (len > 3 && strcmp(word + len - 3, "ly") == 0) {
        stemmed[len - 3] = '\0';
    }
    
    return 0;
}

// Check if word is stop word
bool is_stop_word(const char* word, const char** stop_words, uint32_t stop_word_count) {
    if (!word || !stop_words || stop_word_count == 0) return false;
    
    for (uint32_t i = 0; i < stop_word_count; i++) {
        if (stop_words[i] && strcmp(word, stop_words[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

// Calculate TF-IDF score
float calculate_tfidf_score(uint32_t term_freq, uint32_t doc_freq, uint32_t total_docs) {
    if (doc_freq == 0 || total_docs == 0) return 0.0f;
    
    float tf = (float)term_freq;
    float idf = log((float)total_docs / doc_freq);
    
    return tf * idf;
}

// Normalize text for indexing
int normalize_text(const char* input, char* output, size_t size, bool case_sensitive, bool remove_punctuation) {
    if (!input || !output || size == 0) return -1;
    
    size_t len = strlen(input);
    size_t out_pos = 0;
    
    for (size_t i = 0; i < len && out_pos < size - 1; i++) {
        char c = input[i];
        
        if (remove_punctuation && !isalnum(c) && c != ' ') {
            continue;
        }
        
        if (!case_sensitive && isalpha(c)) {
            c = tolower(c);
        }
        
        output[out_pos++] = c;
    }
    
    output[out_pos] = '\0';
    return 0;
}

// Generate hash for Bloom filter
uint32_t generate_bloom_hash(const char* key, uint32_t seed) {
    if (!key) return 0;
    
    uint32_t hash = seed;
    size_t len = strlen(key);
    
    for (size_t i = 0; i < len; i++) {
        hash = hash * 31 + (uint8_t)key[i];
    }
    
    return hash;
}

// Get current timestamp in microseconds
static uint64_t get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}
