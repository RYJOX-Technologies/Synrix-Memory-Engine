#!/usr/bin/env python3
"""Store SYNRIX build knowledge in the lattice"""

import sys
import os
import json

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "python-sdk"))

from synrix.raw_backend import RawSynrixBackend

def store_build_knowledge():
    """Store knowledge about building tier-specific packages"""
    
    lattice_path = "lattice/cursor_ai_memory.lattice"
    backend = RawSynrixBackend(lattice_path, max_nodes=100000, evaluation_mode=False)
    
    print("Storing SYNRIX build knowledge in lattice...")
    print()
    
    # Pattern: Building tier-specific packages
    pattern_data = {
        "title": "Building SYNRIX Tier-Specific Packages",
        "description": "How to build SYNRIX packages with hard-coded node limits for different tiers",
        "tier_limits": {
            "free_tier": 50000,
            "pro_tier": "unlimited",
            "enterprise_tier": "unlimited"
        },
        "build_method": "compile_time_defines",
        "key_concepts": [
            "CMake options for tier configuration",
            "Compile-time defines for hard-coded limits",
            "Output name changes based on tier",
            "Evaluation mode enforcement"
        ]
    }
    
    backend.add_node(
        "PATTERN:build_tier_specific_package",
        json.dumps(pattern_data),
        node_type=5  # PATTERN
    )
    print("OK: Stored pattern: build_tier_specific_package")
    
    # Constraint: CMake configuration for free tier
    constraint_data = {
        "cmake_options": [
            "-DSYNRIX_FREE_TIER_50K=ON",
            "-DSYNRIX_FREE_TIER_LIMIT=50000",
            "-DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED=ON"
        ],
        "c_flags": "-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED",
        "output_name": "libsynrix_free_tier",
        "code_changes": {
            "lattice_init": "Uses SYNRIX_FREE_TIER_LIMIT define to set free_tier_limit",
            "lattice_disable_evaluation_mode": "Returns error if SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED is defined"
        }
    }
    
    backend.add_node(
        "CONSTRAINT:free_tier_cmake_config",
        json.dumps(constraint_data),
        node_type=2  # CONSTRAINT
    )
    print("OK: Stored constraint: free_tier_cmake_config")
    
    # Work: Build script structure
    work_data = {
        "script_location": "build/windows/build_free_tier_50k.sh",
        "build_steps": [
            "Check for CMake and GCC",
            "Create build_free_tier directory",
            "Configure CMake with tier-specific options",
            "Build with --config Release",
            "Verify output DLL name"
        ],
        "msys2_required": True,
        "generator": "MinGW Makefiles",
        "build_type": "Release"
    }
    
    backend.add_node(
        "WORK:free_tier_build_script",
        json.dumps(work_data),
        node_type=1  # TASK
    )
    print("OK: Stored work: free_tier_build_script")
    
    # Pattern: Adapting for different tiers
    adapt_pattern = {
        "title": "Adapting Build for Different Tier Limits",
        "steps": [
            "1. Update SYNRIX_FREE_TIER_LIMIT value in CMake options",
            "2. Update SYNRIX_FREE_TIER_LIMIT in C_FLAGS",
            "3. Update output name (e.g., libsynrix_pro_tier)",
            "4. Update code comments mentioning the limit",
            "5. Update error messages to show correct limit"
        ],
        "example_free_tier": {
            "limit": 50000,
            "cmake_option": "-DSYNRIX_FREE_TIER_LIMIT=50000",
            "output": "libsynrix_free_tier"
        },
        "example_pro_tier": {
            "limit": 100000,
            "cmake_option": "-DSYNRIX_PRO_TIER_LIMIT=100000",
            "output": "libsynrix_pro_tier",
            "note": "Would need to add SYNRIX_PRO_TIER define"
        }
    }
    
    backend.add_node(
        "PATTERN:adapt_tier_build",
        json.dumps(adapt_pattern),
        node_type=5  # PATTERN
    )
    print("OK: Stored pattern: adapt_tier_build")
    
    # Constraint: Code changes required
    code_changes = {
        "file": "build/windows/src/persistent_lattice.c",
        "changes": [
            {
                "function": "lattice_init",
                "location": "line ~47",
                "change": "Use #ifdef SYNRIX_FREE_TIER_50K to set free_tier_limit from SYNRIX_FREE_TIER_LIMIT define"
            },
            {
                "function": "lattice_disable_evaluation_mode",
                "location": "line ~5818",
                "change": "Add #ifdef SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED to return error in free tier builds"
            }
        ],
        "cmake_changes": {
            "file": "CMakeLists.txt",
            "additions": [
                "option(SYNRIX_FREE_TIER_50K)",
                "add_definitions for tier defines",
                "Conditional OUTPUT_NAME based on tier"
            ]
        }
    }
    
    backend.add_node(
        "CONSTRAINT:tier_build_code_changes",
        json.dumps(code_changes),
        node_type=2  # CONSTRAINT
    )
    print("OK: Stored constraint: tier_build_code_changes")
    
    # Work: Verification steps
    verification = {
        "title": "Verifying Tier Build",
        "checks": [
            "DLL name matches tier (e.g., libsynrix_free_tier.dll)",
            "Try adding nodes up to limit - should succeed",
            "Try adding node at limit+1 - should fail with correct error message",
            "Try calling lattice_disable_evaluation_mode() - should fail in free tier",
            "Error messages show correct limit (e.g., '50,000 nodes')"
        ],
        "test_code": """
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend("test.lattice", max_nodes=100000)
# Add nodes up to limit
for i in range(50000):
    node_id = backend.add_node(f"TEST:node_{i}", f"data_{i}", node_type=3)
    if node_id == 0:
        print(f"Failed at node {i}")
        break

# Try to add 50,001st node - should fail
node_id = backend.add_node("TEST:node_50000", "data", node_type=3)
assert node_id == 0, "Should fail at 50,001st node"
"""
    }
    
    backend.add_node(
        "WORK:verify_tier_build",
        json.dumps(verification),
        node_type=1  # TASK
    )
    print("OK: Stored work: verify_tier_build")
    
    # Pattern: Quick reference
    quick_ref = {
        "title": "Quick Reference: Building Tier Packages",
        "free_tier_50k": {
            "cmake": "cmake .. -DSYNRIX_FREE_TIER_50K=ON -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED=ON",
            "c_flags": "-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED",
            "output": "libsynrix_free_tier.dll"
        },
        "for_different_tier": {
            "change_limit": "Update SYNRIX_FREE_TIER_LIMIT value",
            "change_output": "Update OUTPUT_NAME in CMakeLists.txt",
            "change_defines": "Update all occurrences of limit value in code"
        }
    }
    
    backend.add_node(
        "PATTERN:tier_build_quick_ref",
        json.dumps(quick_ref),
        node_type=5  # PATTERN
    )
    print("OK: Stored pattern: tier_build_quick_ref")
    
    backend.close()
    
    print()
    print("SUCCESS: All build knowledge stored in lattice!")
    print()
    print("Query examples:")
    print("  - Find patterns: PATTERN:build_tier*")
    print("  - Find constraints: CONSTRAINT:*tier*")
    print("  - Find work: WORK:*tier*")

if __name__ == "__main__":
    store_build_knowledge()
