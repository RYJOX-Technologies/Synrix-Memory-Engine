#define _GNU_SOURCE
#include "optimized_inverted_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Fast hash function for terms (FNV-1a)
uint32_t optimized_term_hash(const char* term, size_t length) {
    if (!term || length == 0) return 0;
    
    uint32_t hash = 0x811c9dc5; // FNV offset basis
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)term[i];
        hash *= 0x01000193; // FNV prime
    }
    return hash;
}

// String similarity for fuzzy search (Levenshtein distance)
float optimized_string_similarity(const char* str1, const char* str2) {
    if (!str1 || !str2) return 0.0f;
    
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    
    if (len1 == 0) return (len2 == 0) ? 1.0f : 0.0f;
    if (len2 == 0) return 0.0f;
    
    // Simple similarity based on common characters
    int common = 0;
    int min_len = (len1 < len2) ? len1 : len2;
    
    for (int i = 0; i < min_len; i++) {
        if (str1[i] == str2[i]) common++;
    }
    
    return (float)common / (float)((len1 > len2) ? len1 : len2);
}

// Calculate TF-IDF score
float optimized_calculate_tf_idf(uint32_t term_frequency, uint32_t document_length,
                                uint32_t total_documents, uint32_t documents_with_term) {
    if (document_length == 0 || total_documents == 0 || documents_with_term == 0) {
        return 0.0f;
    }
    
    float tf = (float)term_frequency / (float)document_length;
    float idf = logf((float)total_documents / (float)documents_with_term);
    
    return tf * idf;
}

// Calculate relevance score
float optimized_calculate_relevance_score(const optimized_search_result_t* result, 
                                         const optimized_query_t* query) {
    if (!result || !query) return 0.0f;
    
    // Simple relevance scoring based on TF-IDF sum and term matches
    float base_score = result->tf_idf_sum;
    float match_bonus = (float)result->term_matches * 0.1f;
    
    return base_score + match_bonus;
}

// Sort results by relevance (simple bubble sort for small arrays)
void optimized_sort_results_by_relevance(optimized_search_result_t* results, uint32_t count) {
    if (!results || count <= 1) return;
    
    for (uint32_t i = 0; i < count - 1; i++) {
        for (uint32_t j = 0; j < count - i - 1; j++) {
            if (results[j].relevance_score < results[j + 1].relevance_score) {
                optimized_search_result_t temp = results[j];
                results[j] = results[j + 1];
                results[j + 1] = temp;
            }
        }
    }
}

// ============================================================================
// OPTIMIZED INVERTED INDEX IMPLEMENTATION
// ============================================================================

// Create optimized inverted index
int optimized_inverted_index_create(optimized_inverted_index_t* index, uint32_t capacity) {
    if (!index || capacity == 0) return -1;
    
    memset(index, 0, sizeof(optimized_inverted_index_t));
    
    // Initialize hash table
    index->hash_table_size = OPTIMIZED_INVERTED_INDEX_HASH_TABLE_SIZE;
    index->hash_table_mask = index->hash_table_size - 1; // For fast modulo
    
    index->hash_table = (optimized_term_entry_t*)calloc(index->hash_table_size, 
                                                       sizeof(optimized_term_entry_t));
    if (!index->hash_table) return -1;
    
    // Initialize posting lists
    index->posting_lists_capacity = capacity * 10; // 10x over-provisioning
    index->posting_lists = (optimized_posting_entry_t*)calloc(index->posting_lists_capacity, 
                                                             sizeof(optimized_posting_entry_t));
    if (!index->posting_lists) {
        free(index->hash_table);
        return -1;
    }
    
    // Initialize term frequencies and IDF scores
    index->term_frequencies = (uint32_t*)calloc(OPTIMIZED_INVERTED_INDEX_MAX_TERMS, sizeof(uint32_t));
    index->idf_scores = (float*)calloc(OPTIMIZED_INVERTED_INDEX_MAX_TERMS, sizeof(float));
    
    if (!index->term_frequencies || !index->idf_scores) {
        optimized_inverted_index_destroy(index);
        return -1;
    }
    
    // Initialize free posting offsets
    index->free_posting_capacity = 1000;
    index->free_posting_offsets = (uint32_t*)malloc(index->free_posting_capacity * sizeof(uint32_t));
    if (!index->free_posting_offsets) {
        optimized_inverted_index_destroy(index);
        return -1;
    }
    
    return 0;
}

// Add document to index
int optimized_inverted_index_add_document(optimized_inverted_index_t* index, 
                                        const lattice_node_t* node) {
    if (!index || !node) return -1;
    
    // Extract terms from node name and data
    char combined_text[1024];
    snprintf(combined_text, sizeof(combined_text), "%s %s", node->name, node->data);
    
    // Simple tokenization (split by spaces)
    char* token = strtok(combined_text, " \t\n\r");
    uint32_t position = 0;
    
    while (token != NULL && position < 100) { // Limit to 100 terms per document
        // Convert to lowercase for case-insensitive search
        for (int i = 0; token[i]; i++) {
            if (token[i] >= 'A' && token[i] <= 'Z') {
                token[i] = token[i] - 'A' + 'a';
            }
        }
        
        // Add term to index
        optimized_inverted_index_add_term(index, token, node->id, 1, position);
        
        token = strtok(NULL, " \t\n\r");
        position++;
    }
    
    index->total_documents++;
    return 0;
}

// Add term to document
int optimized_inverted_index_add_term(optimized_inverted_index_t* index, 
                                    const char* term, uint32_t node_id, 
                                    uint32_t frequency, uint32_t position) {
    if (!index || !term || node_id == 0) return -1;
    
    size_t term_len = strlen(term);
    if (term_len == 0 || term_len >= OPTIMIZED_INVERTED_INDEX_MAX_TERM_LENGTH) return -1;
    
    // Calculate hash for term
    uint32_t term_hash = optimized_term_hash(term, term_len);
    uint32_t hash_index = term_hash & index->hash_table_mask;
    
    // Find existing term or empty slot
    uint32_t slot_index = hash_index;
    uint32_t attempts = 0;
    
    while (attempts < index->hash_table_size) {
        optimized_term_entry_t* entry = &index->hash_table[slot_index];
        
        if (!entry->is_used) {
            // Create new term entry
            strncpy(entry->term, term, sizeof(entry->term) - 1);
            entry->term[sizeof(entry->term) - 1] = '\0';
            entry->term_hash = term_hash;
            entry->posting_list_offset = index->posting_lists_size;
            entry->posting_list_size = 0;
            entry->next = 0;
            entry->is_used = true;
            
            index->total_terms++;
            break;
        } else if (strcmp(entry->term, term) == 0) {
            // Term already exists, add to posting list
            break;
        } else {
            // Collision, try next slot
            slot_index = (slot_index + 1) & index->hash_table_mask;
            attempts++;
        }
    }
    
    if (attempts >= index->hash_table_size) {
        return -1; // Hash table full
    }
    
    optimized_term_entry_t* entry = &index->hash_table[slot_index];
    
    // Add posting to list
    if (index->posting_lists_size >= index->posting_lists_capacity) {
        // Resize posting lists
        uint32_t new_capacity = index->posting_lists_capacity * 2;
        optimized_posting_entry_t* new_posting_lists = (optimized_posting_entry_t*)realloc(
            index->posting_lists, new_capacity * sizeof(optimized_posting_entry_t));
        
        if (!new_posting_lists) return -1;
        
        index->posting_lists = new_posting_lists;
        index->posting_lists_capacity = new_capacity;
    }
    
    optimized_posting_entry_t* posting = &index->posting_lists[index->posting_lists_size];
    posting->node_id = node_id;
    posting->frequency = frequency;
    posting->position = position;
    posting->tf_idf_score = 0.0f; // Will be calculated during optimization
    
    entry->posting_list_size++;
    index->posting_lists_size++;
    index->total_postings++;
    
    return 0;
}

// Search for single term
int optimized_inverted_index_search_term(optimized_inverted_index_t* index, 
                                       const char* term,
                                       optimized_search_result_t* results, 
                                       uint32_t* count) {
    if (!index || !term || !results || !count) return -1;
    
    *count = 0;
    
    // Convert term to lowercase
    char lower_term[OPTIMIZED_INVERTED_INDEX_MAX_TERM_LENGTH];
    strncpy(lower_term, term, sizeof(lower_term) - 1);
    lower_term[sizeof(lower_term) - 1] = '\0';
    
    for (int i = 0; lower_term[i]; i++) {
        if (lower_term[i] >= 'A' && lower_term[i] <= 'Z') {
            lower_term[i] = lower_term[i] - 'A' + 'a';
        }
    }
    
    // Find term in hash table
    uint32_t term_hash = optimized_term_hash(lower_term, strlen(lower_term));
    uint32_t hash_index = term_hash & index->hash_table_mask;
    uint32_t attempts = 0;
    
    while (attempts < index->hash_table_size) {
        optimized_term_entry_t* entry = &index->hash_table[hash_index];
        
        if (!entry->is_used) {
            break; // Term not found
        } else if (strcmp(entry->term, lower_term) == 0) {
            // Found term, collect results
            for (uint32_t i = 0; i < entry->posting_list_size && *count < 1000; i++) {
                optimized_posting_entry_t* posting = &index->posting_lists[entry->posting_list_offset + i];
                
                results[*count].node_id = posting->node_id;
                results[*count].relevance_score = posting->tf_idf_score;
                results[*count].term_matches = 1;
                results[*count].tf_idf_sum = posting->tf_idf_score;
                (*count)++;
            }
            break;
        } else {
            // Collision, try next slot
            hash_index = (hash_index + 1) & index->hash_table_mask;
            attempts++;
        }
    }
    
    return 0;
}

// Search for terms
int optimized_inverted_index_search(optimized_inverted_index_t* index, 
                                  const optimized_query_t* query,
                                  optimized_search_result_t* results, 
                                  uint32_t* count) {
    if (!index || !query || !results || !count) return -1;
    
    *count = 0;
    
    // Simple implementation: search each term and combine results
    for (uint32_t i = 0; i < query->term_count && *count < 1000; i++) {
        optimized_search_result_t term_results[1000];
        uint32_t term_count = 0;
        
        if (optimized_inverted_index_search_term(index, query->terms[i], 
                                               term_results, &term_count) == 0) {
            // Add results to combined list
            for (uint32_t j = 0; j < term_count && *count < 1000; j++) {
                // Check if result already exists
                bool found = false;
                for (uint32_t k = 0; k < *count; k++) {
                    if (results[k].node_id == term_results[j].node_id) {
                        // Combine scores
                        results[k].relevance_score += term_results[j].relevance_score;
                        results[k].term_matches++;
                        results[k].tf_idf_sum += term_results[j].tf_idf_sum;
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    results[*count] = term_results[j];
                    (*count)++;
                }
            }
        }
    }
    
    // Sort results by relevance
    optimized_sort_results_by_relevance(results, *count);
    
    return 0;
}

// Phrase search (simplified implementation)
int optimized_inverted_index_search_phrase(optimized_inverted_index_t* index, 
                                         const char* phrase,
                                         optimized_search_result_t* results, 
                                         uint32_t* count) {
    if (!index || !phrase || !results || !count) return -1;
    
    // For now, just do a simple term search
    // In a full implementation, this would check for consecutive positions
    return optimized_inverted_index_search_term(index, phrase, results, count);
}

// Fuzzy search
int optimized_inverted_index_search_fuzzy(optimized_inverted_index_t* index, 
                                        const char* term, uint32_t max_distance,
                                        optimized_search_result_t* results, 
                                        uint32_t* count) {
    if (!index || !term || !results || !count) return -1;
    
    *count = 0;
    
    // Simple fuzzy search: check all terms for similarity
    for (uint32_t i = 0; i < index->hash_table_size && *count < 1000; i++) {
        optimized_term_entry_t* entry = &index->hash_table[i];
        
        if (entry->is_used) {
            float similarity = optimized_string_similarity(term, entry->term);
            
            if (similarity > 0.7f) { // Threshold for fuzzy matching
                // Add results for this term
                for (uint32_t j = 0; j < entry->posting_list_size && *count < 1000; j++) {
                    optimized_posting_entry_t* posting = &index->posting_lists[entry->posting_list_offset + j];
                    
                    results[*count].node_id = posting->node_id;
                    results[*count].relevance_score = posting->tf_idf_score * similarity;
                    results[*count].term_matches = 1;
                    results[*count].tf_idf_sum = posting->tf_idf_score * similarity;
                    (*count)++;
                }
            }
        }
    }
    
    // Sort results by relevance
    optimized_sort_results_by_relevance(results, *count);
    
    return 0;
}

// Optimize index for faster queries
int optimized_inverted_index_optimize(optimized_inverted_index_t* index) {
    if (!index) return -1;
    
    // Calculate IDF scores for all terms
    for (uint32_t i = 0; i < index->hash_table_size; i++) {
        optimized_term_entry_t* entry = &index->hash_table[i];
        
        if (entry->is_used) {
            // Count documents containing this term
            uint32_t documents_with_term = 0;
            for (uint32_t j = 0; j < entry->posting_list_size; j++) {
                optimized_posting_entry_t* posting = &index->posting_lists[entry->posting_list_offset + j];
                if (posting->node_id != 0) {
                    documents_with_term++;
                }
            }
            
            // Calculate IDF score
            if (documents_with_term > 0) {
                float idf = logf((float)index->total_documents / (float)documents_with_term);
                
                // Update TF-IDF scores for all postings of this term
                for (uint32_t j = 0; j < entry->posting_list_size; j++) {
                    optimized_posting_entry_t* posting = &index->posting_lists[entry->posting_list_offset + j];
                    posting->tf_idf_score = (float)posting->frequency * idf;
                }
            }
        }
    }
    
    index->is_optimized = true;
    return 0;
}

// Get index statistics
int optimized_inverted_index_get_stats(optimized_inverted_index_t* index, 
                                      uint32_t* total_terms, uint32_t* total_documents,
                                      uint32_t* total_postings, float* avg_posting_length) {
    if (!index || !total_terms || !total_documents || !total_postings || !avg_posting_length) {
        return -1;
    }
    
    *total_terms = index->total_terms;
    *total_documents = index->total_documents;
    *total_postings = index->total_postings;
    
    if (index->total_terms > 0) {
        *avg_posting_length = (float)index->total_postings / (float)index->total_terms;
    } else {
        *avg_posting_length = 0.0f;
    }
    
    return 0;
}

// Get performance statistics
int optimized_inverted_index_get_performance_stats(optimized_inverted_index_t* index, 
                                                  optimized_inverted_index_stats_t* stats) {
    if (!index || !stats) return -1;
    
    // This would be implemented with actual performance tracking
    memset(stats, 0, sizeof(optimized_inverted_index_stats_t));
    
    return 0;
}

// Reset performance statistics
void optimized_inverted_index_reset_stats(optimized_inverted_index_t* index) {
    if (!index) return;
    
    // Reset performance counters
    // This would be implemented with actual performance tracking
}

// Destroy optimized inverted index
void optimized_inverted_index_destroy(optimized_inverted_index_t* index) {
    if (!index) return;
    
    if (index->hash_table) free(index->hash_table);
    if (index->posting_lists) free(index->posting_lists);
    if (index->term_frequencies) free(index->term_frequencies);
    if (index->idf_scores) free(index->idf_scores);
    if (index->free_posting_offsets) free(index->free_posting_offsets);
    
    memset(index, 0, sizeof(optimized_inverted_index_t));
}

// ============================================================================
// QUERY PROCESSING
// ============================================================================

// Create query from text
int optimized_query_create(optimized_query_t* query, const char* text, 
                          bool use_phrase_search, bool use_fuzzy_search) {
    if (!query || !text) return -1;
    
    memset(query, 0, sizeof(optimized_query_t));
    
    // Simple tokenization
    char* text_copy = strdup(text);
    if (!text_copy) return -1;
    
    // Count terms
    char* token = strtok(text_copy, " \t\n\r");
    uint32_t term_count = 0;
    
    while (token != NULL) {
        term_count++;
        token = strtok(NULL, " \t\n\r");
    }
    
    if (term_count == 0) {
        free(text_copy);
        return -1;
    }
    
    // Allocate term array
    query->terms = (char**)malloc(term_count * sizeof(char*));
    if (!query->terms) {
        free(text_copy);
        return -1;
    }
    
    // Tokenize again and store terms
    strcpy(text_copy, text);
    token = strtok(text_copy, " \t\n\r");
    uint32_t i = 0;
    
    while (token != NULL && i < term_count) {
        query->terms[i] = strdup(token);
        if (!query->terms[i]) {
            // Cleanup on error
            for (uint32_t j = 0; j < i; j++) {
                free(query->terms[j]);
            }
            free(query->terms);
            free(text_copy);
            return -1;
        }
        i++;
        token = strtok(NULL, " \t\n\r");
    }
    
    query->term_count = term_count;
    query->use_phrase_search = use_phrase_search;
    query->use_fuzzy_search = use_fuzzy_search;
    query->min_relevance_threshold = 0.1f;
    
    free(text_copy);
    return 0;
}

// Destroy query
void optimized_query_destroy(optimized_query_t* query) {
    if (!query) return;
    
    if (query->terms) {
        for (uint32_t i = 0; i < query->term_count; i++) {
            if (query->terms[i]) {
                free(query->terms[i]);
            }
        }
        free(query->terms);
    }
    
    memset(query, 0, sizeof(optimized_query_t));
}
