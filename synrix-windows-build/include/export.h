#ifndef EXPORT_H
#define EXPORT_H

#include <stdint.h>
#include <stdbool.h>
#include "persistent_lattice.h"

#ifdef __cplusplus
extern "C" {
#endif

// Training data export functions (offline operations, don't affect storage performance)

// Export nodes to JSON format
// Returns: 0 on success, -1 on error
int lattice_export_to_json(persistent_lattice_t* lattice, 
                          const char* output_path,
                          const char* name_filter,      // Optional: filter by name prefix (NULL = all)
                          double min_confidence,        // Optional: minimum confidence (0.0 = no filter)
                          uint64_t min_timestamp,       // Optional: minimum timestamp (0 = no filter)
                          uint64_t max_timestamp);      // Optional: maximum timestamp (0 = no filter)

// Export nodes to JSON with custom callback (for filtering/transformation)
// callback: Called for each node, return true to include, false to exclude
// callback_ctx: User context passed to callback
typedef bool (*export_node_filter_t)(persistent_lattice_t* lattice, 
                                     uint64_t node_id,
                                     const lattice_node_t* node,
                                     void* callback_ctx);

int lattice_export_to_json_filtered(persistent_lattice_t* lattice,
                                   const char* output_path,
                                   export_node_filter_t filter,
                                   void* filter_ctx);

#ifdef __cplusplus
}
#endif

#endif // EXPORT_H

