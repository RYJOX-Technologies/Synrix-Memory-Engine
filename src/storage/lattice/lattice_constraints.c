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
                "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Multiple writers detected (%u > %d)\n"
                "This breaks the Lattice. SYNRIX supports only ONE writer at a time.\n",
                _writer_count, LATTICE_MAX_WRITERS);
        return false;
    }
    
    if (_writer_active && _writer_count > 0) {
        fprintf(stderr,
                "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Writer already active\n"
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
                    "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Graph traversal detected in query type '%s'\n"
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

// Global strict mode flag (can be set per-lattice in future)
static bool _prefix_validation_strict_mode = true;  // Default: strict (reject invalid)

// Set prefix validation strictness mode
void lattice_set_prefix_validation_strict(bool strict) {
    _prefix_validation_strict_mode = strict;
}

// Get current strictness mode
bool lattice_get_prefix_validation_strict(void) {
    return _prefix_validation_strict_mode;
}

bool lattice_validate_prefix_semantics(const char* node_name) {
    if (!node_name) {
        fprintf(stderr,
                "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Null node name\n"
                "This breaks the Lattice. All nodes must have semantic prefixes.\n");
        return false;
    }
    
    // Check for garbage prefixes (reject these immediately)
    const char* garbage_prefixes[] = {
        "TEMP_",
        "RANDOM_",
        "GARBAGE_",
        "DEBUG_",
        "TMP_",
        "UNKNOWN_",
        "INVALID_",
        NULL
    };
    
    for (int i = 0; garbage_prefixes[i] != NULL; i++) {
        if (strncmp(node_name, garbage_prefixes[i], strlen(garbage_prefixes[i])) == 0) {
            fprintf(stderr,
                    "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Node name '%s' uses garbage prefix '%s'\n"
                    "Garbage prefixes degrade O(k) query performance and cause prefix explosion.\n"
                    "Use semantic prefixes: ISA_, PATTERN_, FUNC_, DOMAIN_, etc.\n",
                    node_name, garbage_prefixes[i]);
            return false;  // Always reject garbage prefixes
        }
    }
    
    // Check for semantic prefix patterns
    // Valid prefixes include: ISA_, PATTERN_, FUNC_, QDRANT_COLLECTION:, DOMAIN_, etc.
    const char* valid_prefixes[] = {
        "ISA_",
        "PATTERN_",
        "FUNC_",
        "QDRANT_COLLECTION:",
        "QDRANT_POINT:",
        "DOMAIN_",
        "CHUNKED:",
        "CHUNK:",
        "C:",  // Chunked storage prefix (2-byte optimization)
        "anti_pattern|",
        "operand_constraint|",
        "OPERAND_",
        "LEARNING_",
        "KERNEL_",
        "PRIMITIVE_",
        "CONSTRAINT_",
        "FAILURE_",
        "TASK_",
        "COLLECTION:",
        "INTERFACE_",
        "OBJECTIVE_",
        "COMPONENT_",
        "SYSTEM_",
        "LINEAGE:",
        "BACKREFDIR:",
        "BACKREF:",
        "TEST_",
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
    
    // Check for namespace prefixes (AGENT_*, USER_*, SESSION_*, TENANT_*)
    // Namespaces allow agents to store anything within their namespace
    // Format: NAMESPACE_PREFIX:agent_prefix or NAMESPACE_PREFIX_agent_prefix
    if (!has_valid_prefix) {
        const char* namespace_prefixes[] = {
            "AGENT_",
            "USER_",
            "SESSION_",
            "TENANT_",
            "NAMESPACE:",
            NULL
        };
        
        for (int i = 0; namespace_prefixes[i] != NULL; i++) {
            size_t ns_prefix_len = strlen(namespace_prefixes[i]);
            if (strncmp(node_name, namespace_prefixes[i], ns_prefix_len) == 0) {
                // Found namespace prefix - check for separator (colon or underscore after namespace)
                const char* after_ns = node_name + ns_prefix_len;
                
                // Allow namespace:anything or namespace_anything
                // This allows agents to store anything within their namespace
                if (*after_ns == ':' || *after_ns == '_' || 
                    (after_ns[0] >= 'A' && after_ns[0] <= 'Z') ||
                    (after_ns[0] >= 'a' && after_ns[0] <= 'z') ||
                    (after_ns[0] >= '0' && after_ns[0] <= '9')) {
                    // Valid namespace prefix - allow any suffix
                    has_valid_prefix = true;
                    break;
                }
            }
        }
    }
    
    // Fallback: Allow nodes with semantic pattern (prefix|... or prefix:... or prefix_...)
    // But require minimum length and semantic structure
    if (!has_valid_prefix) {
        const char* separator = strchr(node_name, '|');
        const char* colon = strchr(node_name, ':');
        const char* underscore = strchr(node_name, '_');
        
        // Must have a separator AND minimum prefix length (at least 3 chars before separator)
        if (separator || colon || underscore) {
            const char* first_sep = separator;
            if (colon && (!first_sep || colon < first_sep)) first_sep = colon;
            if (underscore && (!first_sep || underscore < first_sep)) first_sep = underscore;
            
            if (first_sep) {
                size_t prefix_len = (size_t)(first_sep - node_name);
                // Require minimum 3 characters for prefix (e.g., "ISA_", "PAT_", etc.)
                if (prefix_len >= 3 && prefix_len <= 32) {
                    // Check that prefix is not all digits or random characters
                    bool has_alpha = false;
                    for (size_t j = 0; j < prefix_len; j++) {
                        if ((node_name[j] >= 'A' && node_name[j] <= 'Z') ||
                            (node_name[j] >= 'a' && node_name[j] <= 'z')) {
                            has_alpha = true;
                            break;
                        }
                    }
                    if (has_alpha) {
                        has_valid_prefix = true;
                    }
                }
            }
        }
    }
    
    // Handle validation result based on strict mode
    if (!has_valid_prefix) {
        if (_prefix_validation_strict_mode) {
            // STRICT MODE: Reject invalid prefixes
            fprintf(stderr,
                    "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Node name '%s' lacks valid semantic prefix\n"
                    "SYNRIX uses prefix-based semantics for O(k) queries.\n"
                    "Valid options:\n"
                    "  • System prefixes: ISA_, PATTERN_, FUNC_, DOMAIN_, CONSTRAINT_, FAILURE_, TASK_, etc.\n"
                    "  • Namespace prefixes: AGENT_*:, USER_*:, SESSION_*:, TENANT_*: (allows any suffix)\n"
                    "  • Format: PREFIX_suffix or PREFIX:suffix or prefix|suffix\n"
                    "For agents: Use namespace prefixes (e.g., AGENT_123:TEMP_xyz) to store anything.\n"
                    "This node will be rejected to prevent prefix explosion.\n",
                    node_name);
            return false;  // Reject in strict mode
        } else {
            // PERMISSIVE MODE: Warn but allow
            fprintf(stderr,
                    "[LATTICE-CONSTRAINTS] ⚠️  WARNING: Node name '%s' lacks semantic prefix\n"
                    "SYNRIX uses prefix-based semantics. Consider: ISA_, PATTERN_, FUNC_, DOMAIN_, etc.\n"
                    "This may degrade O(k) query performance.\n",
                    node_name);
            return true;  // Allow in permissive mode (backward compatibility)
        }
    }
    
    return true;  // Valid prefix found
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
                    "[LATTICE-CONSTRAINTS] ❌ VIOLATION: Feature '%s' violates constitutional constraints\n"
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
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("  SYNRIX LATTICE CONSTITUTIONAL CONSTRAINTS (NON-NEGOTIABLE)\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("These constraints define what SYNRIX is and what it will NEVER be.\n");
    printf("They are not limitations—they are the source of performance.\n");
    printf("\n");
    printf("1. FIXED-SIZE NODES (1KB)\n");
    printf("   ✅ NEVER: Variable-length nodes\n");
    printf("   ✅ NEVER: Dynamic node sizing\n");
    printf("   ✅ NEVER: JSON document storage (use chunked storage)\n");
    printf("\n");
    printf("2. SINGLE-WRITER MODEL\n");
    printf("   ✅ NEVER: Multiple concurrent writers\n");
    printf("   ✅ NEVER: Distributed write coordination\n");
    printf("   ✅ NEVER: Multi-writer locking\n");
    printf("\n");
    printf("3. ARITHMETIC ADDRESSING (NO POINTER CHASING)\n");
    printf("   ✅ NEVER: Pointer-based node relationships\n");
    printf("   ✅ NEVER: Graph traversal algorithms\n");
    printf("   ✅ NEVER: Recursive node navigation\n");
    printf("\n");
    printf("4. PREFIX-BASED SEMANTICS (NO EXPLICIT EDGES)\n");
    printf("   ✅ NEVER: Explicit edge storage\n");
    printf("   ✅ NEVER: Relationship graphs\n");
    printf("   ✅ NEVER: Parent/child pointers\n");
    printf("\n");
    printf("5. FLAT TOPOLOGY (NO PERSISTENT HIERARCHY)\n");
    printf("   ✅ NEVER: Persistent parent/child relationships\n");
    printf("   ✅ NEVER: Tree structures on disk\n");
    printf("   ✅ NEVER: Hierarchical node organization\n");
    printf("\n");
    printf("6. SINGLE-NODE SYSTEM (NO DISTRIBUTED LATTICE)\n");
    printf("   ✅ NEVER: Shared lattice across multiple nodes\n");
    printf("   ✅ NEVER: Distributed lattice coordination\n");
    printf("   ✅ NEVER: Network-based lattice access\n");
    printf("\n");
    printf("7. BINARY LATTICE (NOT GENERAL-PURPOSE DATABASE)\n");
    printf("   ✅ NEVER: SQL queries\n");
    printf("   ✅ NEVER: ACID transactions across multiple lattices\n");
    printf("   ✅ NEVER: Variable schemas\n");
    printf("   ✅ NEVER: Dynamic indexing strategies\n");
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
    printf("If a feature request would violate these constraints:\n");
    printf("  → Answer: \"No, that breaks the Lattice.\"\n");
    printf("\n");
    printf("The rigidity is the innovation.\n");
    printf("SYNRIX is fast because it is rigid.\n");
    printf("\n");
}

