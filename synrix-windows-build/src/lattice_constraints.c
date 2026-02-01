/*
 * SYNRIX LATTICE CONSTITUTIONAL CONSTRAINTS - IMPLEMENTATION
 * ===========================================================
 * 
 * Runtime validation functions for constitutional constraints.
 * 
 * These functions enforce the non-negotiable constraints that define
 * the Binary Lattice topology.
 */

#include "lattice_constraints.h"
#include <string.h>
#include <stdio.h>

// ============================================================================
// SINGLE-WRITER VALIDATION
// ============================================================================

// Global writer lock (simplified - actual implementation uses seqlocks)
static volatile bool _writer_active = false;
static volatile uint32_t _writer_count = 0;

bool lattice_validate_single_writer(void) {
    // Check if multiple writers are active
    if (_writer_count > LATTICE_MAX_WRITERS) {
        fprintf(stderr,
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Multiple writers detected (%u > %d)\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n",
                _writer_count, LATTICE_MAX_WRITERS);
        return false;
    }
    
    if (_writer_active && _writer_count > 0) {
        fprintf(stderr,
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Writer already active\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n");
        return false;
    }
    
    return true;
}

// ============================================================================
// NO TRAVERSAL VALIDATION
// ============================================================================

bool lattice_validate_no_traversal(const char* query_type) {
    if (!query_type) {
        return true; // Null check passed to caller
    }
    
    // Check for traversal-related query types
    const char* traversal_keywords[] = {
        "traverse",
        "graph",
        "edge",
        "relationship",
        "parent",
        "child",
        "recursive",
        "pointer",
        "follow",
        NULL
    };
    
    for (int i = 0; traversal_keywords[i] != NULL; i++) {
        if (strstr(query_type, traversal_keywords[i]) != NULL) {
            fprintf(stderr,
                    "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Graph traversal detected in query type '%s'\n"
                    "This breaks the Lattice. SYNRIX uses prefix-based semantic queries, not graph traversal.\n",
                    query_type);
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// PREFIX SEMANTICS VALIDATION
// ============================================================================

bool lattice_validate_prefix_semantics(const char* node_name) {
    if (!node_name) {
        fprintf(stderr,
                "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Null node name\n"
                "This breaks the Lattice. All nodes must have semantic prefixes.\n");
        return false;
    }
    
    // Check for semantic prefix patterns
    // Valid prefixes include: ISA_, PATTERN_, FUNC_, QDRANT_COLLECTION:, DOMAIN_, etc.
    const char* valid_prefixes[] = {
        "ISA_",
        "PATTERN_",
        "FUNC_",
        "QDRANT_COLLECTION:",
        "DOMAIN_",
        "CHUNKED:",
        "CHUNK:",
        "anti_pattern|",
        "operand_constraint|",
        "OPERAND_",
        "LEARNING_",
        "KERNEL_",
        "PRIMITIVE_",
        NULL
    };
    
    // Check if node name starts with a valid prefix
    bool has_valid_prefix = false;
    for (int i = 0; valid_prefixes[i] != NULL; i++) {
        if (strncmp(node_name, valid_prefixes[i], strlen(valid_prefixes[i])) == 0) {
            has_valid_prefix = true;
            break;
        }
    }
    
    // Also allow nodes that follow pattern: "prefix|..." or "prefix:" or "prefix_"
    if (!has_valid_prefix) {
        // Check for pattern-based prefixes (e.g., "anti_pattern|operand|...")
        const char* separator = strchr(node_name, '|');
        const char* colon = strchr(node_name, ':');
        const char* underscore = strchr(node_name, '_');
        
        if (separator || colon || underscore) {
            // Has a separator, likely a valid semantic pattern
            has_valid_prefix = true;
        }
    }
    
    if (!has_valid_prefix) {
        fprintf(stderr,
                "[LATTICE-CONSTRAINTS] WARN Node name '%s' lacks semantic prefix\n"
                "SYNRIX uses prefix-based semantics. Consider: ISA_, PATTERN_, FUNC_, DOMAIN_, etc.\n"
                "This may degrade O(k) query performance.\n",
                node_name);
        // Warning, not error - allow but warn
    }
    
    return true; // Always return true (warning only)
}

// ============================================================================
// FEATURE REQUEST VALIDATION
// ============================================================================

bool lattice_validate_feature_request(const char* feature_name) {
    if (!feature_name) {
        return true; // Null check passed to caller
    }
    
    // List of features that violate constitutional constraints
    const char* forbidden_features[] = {
        "variable_length_nodes",
        "variable_node_size",
        "dynamic_node_sizing",
        "json_document_storage",
        "multi_writer",
        "concurrent_writers",
        "distributed_writes",
        "graph_traversal",
        "pointer_chasing",
        "explicit_edges",
        "relationship_graphs",
        "parent_child_pointers",
        "hierarchical_traversal",
        "persistent_hierarchy",
        "tree_structures",
        "shared_lattice",
        "distributed_lattice",
        "sql_queries",
        "acid_transactions",
        "variable_schema",
        "dynamic_indexing",
        NULL
    };
    
    // Check if feature name matches any forbidden feature
    for (int i = 0; forbidden_features[i] != NULL; i++) {
        if (strstr(feature_name, forbidden_features[i]) != NULL) {
            fprintf(stderr,
                    "[LATTICE-CONSTRAINTS] ERROR VIOLATION: Feature '%s' violates constitutional constraints\n"
                    "This breaks the Lattice. Feature '%s' is not supported.\n"
                    "See lattice_constraints.h for constitutional constraints.\n",
                    feature_name, forbidden_features[i]);
            return false;
        }
    }
    
    return true;
}

// ============================================================================
// CONSTITUTIONAL CONSTRAINTS SUMMARY
// ============================================================================

void lattice_print_constitutional_constraints(void) {
    printf("\n");
    printf("===============================================================\n");
    printf("  SYNRIX LATTICE CONSTITUTIONAL CONSTRAINTS (NON-NEGOTIABLE)\n");
    printf("===============================================================\n");
    printf("\n");
    printf("These constraints define what SYNRIX is and what it will NEVER be.\n");
    printf("They are not limitations - they are the source of performance.\n");
    printf("\n");
    printf("1. FIXED-SIZE NODES (1KB)\n");
    printf("   NEVER: Variable-length nodes\n");
    printf("   NEVER: Dynamic node sizing\n");
    printf("   NEVER: JSON document storage (use chunked storage)\n");
    printf("\n");
    printf("2. SINGLE-WRITER MODEL\n");
    printf("   NEVER: Multiple concurrent writers\n");
    printf("   NEVER: Distributed write coordination\n");
    printf("   NEVER: Multi-writer locking\n");
    printf("\n");
    printf("3. ARITHMETIC ADDRESSING (NO POINTER CHASING)\n");
    printf("   NEVER: Pointer-based node relationships\n");
    printf("   NEVER: Graph traversal algorithms\n");
    printf("   NEVER: Recursive node navigation\n");
    printf("\n");
    printf("4. PREFIX-BASED SEMANTICS (NO EXPLICIT EDGES)\n");
    printf("   NEVER: Explicit edge storage\n");
    printf("   NEVER: Relationship graphs\n");
    printf("   NEVER: Parent/child pointers\n");
    printf("\n");
    printf("5. FLAT TOPOLOGY (NO PERSISTENT HIERARCHY)\n");
    printf("   NEVER: Persistent parent/child relationships\n");
    printf("   NEVER: Tree structures on disk\n");
    printf("   NEVER: Hierarchical node organization\n");
    printf("\n");
    printf("6. SINGLE-NODE SYSTEM (NO DISTRIBUTED LATTICE)\n");
    printf("   NEVER: Shared lattice across multiple nodes\n");
    printf("   NEVER: Distributed lattice coordination\n");
    printf("   NEVER: Network-based lattice access\n");
    printf("\n");
    printf("7. BINARY LATTICE (NOT GENERAL-PURPOSE DATABASE)\n");
    printf("   NEVER: SQL queries\n");
    printf("   NEVER: ACID transactions across multiple lattices\n");
    printf("   NEVER: Variable schemas\n");
    printf("   NEVER: Dynamic indexing strategies\n");
    printf("\n");
    printf("===============================================================\n");
    printf("\n");
    printf("If a feature request would violate these constraints:\n");
    printf("  -> Answer: \"No, that breaks the Lattice.\"\n");
    printf("\n");
    printf("The rigidity is the innovation.\n");
    printf("SYNRIX is fast because it is rigid.\n");
    printf("\n");
}

