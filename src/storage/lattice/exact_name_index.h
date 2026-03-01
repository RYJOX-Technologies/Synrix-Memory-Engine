/*
 * Exact-name index: full name string -> node_id (O(1) exact lookups).
 * Built with prefix index; updated on every node add/update so exact lookups
 * never touch the prefix-bucket scan.
 */

#ifndef EXACT_NAME_INDEX_H
#define EXACT_NAME_INDEX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXACT_NAME_MAX_LEN 64

typedef struct exact_name_index_t exact_name_index_t;

/* Allocate and initialize. Caller owns the struct; call exact_name_index_cleanup before free. */
exact_name_index_t* exact_name_index_create(void);
void exact_name_index_cleanup(exact_name_index_t* index);
void exact_name_index_destroy(exact_name_index_t* index);

/* Get node_id for exact name. Returns 0 if not found. */
uint64_t exact_name_index_get(exact_name_index_t* index, const char* name);

/* Put name -> node_id. Overwrites if name already present. */
void exact_name_index_put(exact_name_index_t* index, const char* name, uint64_t node_id);

/* Remove name from index (for future rename support). */
void exact_name_index_remove(exact_name_index_t* index, const char* name);

/* Build from parallel arrays (same pass as prefix index build). */
void exact_name_index_build(exact_name_index_t* index,
                            const char** node_names,
                            const uint64_t* node_ids,
                            uint32_t node_count);

#ifdef __cplusplus
}
#endif

#endif /* EXACT_NAME_INDEX_H */
