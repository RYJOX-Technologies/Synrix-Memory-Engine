cmake_minimum_required(VERSION 3.15)
project(synrix C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Free tier build options
set(SYNRIX_FREE_TIER_50K OFF CACHE BOOL "Build free tier version with 50k node limit")
set(SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED OFF CACHE BOOL "Always enable evaluation mode (free tier)")

# Set free tier limit if specified
if(SYNRIX_FREE_TIER_50K)
    if(NOT DEFINED SYNRIX_FREE_TIER_LIMIT)
        set(SYNRIX_FREE_TIER_LIMIT 50000 CACHE STRING "Free tier node limit")
    endif()
    add_definitions(-DSYNRIX_FREE_TIER_50K)
    add_definitions(-DSYNRIX_FREE_TIER_LIMIT=${SYNRIX_FREE_TIER_LIMIT})
    add_definitions(-DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED)
    message(STATUS "Building free tier version with ${SYNRIX_FREE_TIER_LIMIT} node limit")
    set(SYNRIX_IS_FREE_TIER_BUILD TRUE)
else()
    set(SYNRIX_IS_FREE_TIER_BUILD FALSE)
endif()

# Source files (core library)
set(CORE_SOURCES
    ../../src/persistent_lattice.c
    ../../src/wal.c
    ../../src/dynamic_prefix_index.c
    ../../src/semantic_index.c
    ../../src/export.c
    ../../src/checksum.c
    ../../src/license_utils.c
    ../../src/seqlock.c
)

# Optional/supporting files
set(OPTIONAL_SOURCES
    ../../src/cluster_persistence.c
    ../../src/namespace_isolation.c
    ../../src/lattice_constraints.c
    ../../src/isolation.c
    ../../src/integrity_validator.c
    ../../src/intelligent_lattice_loader.c
    ../../src/lattice_dump.c
)

# Create shared library
add_library(synrix SHARED ${CORE_SOURCES} ${OPTIONAL_SOURCES})

# Include directories
target_include_directories(synrix PUBLIC ../../include)

# Compiler options
target_compile_options(synrix PRIVATE
    -Wall
    -Wextra
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-g>
)

# Output name
if(SYNRIX_IS_FREE_TIER_BUILD)
    set_target_properties(synrix PROPERTIES
        OUTPUT_NAME "synrix_free_tier"
        PREFIX "lib"
    )
else()
    set_target_properties(synrix PROPERTIES
        OUTPUT_NAME "synrix"
        PREFIX "lib"
    )
endif()

# Install rules
install(TARGETS synrix
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
