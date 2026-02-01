cmake_minimum_required(VERSION 3.15)
project(synrix C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Free tier build options (must be before project() or use set() with CACHE)
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

# Windows-specific settings
if(WIN32)
    add_definitions(-DSYNRIX_EXPORTS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Source files (core library)
set(CORE_SOURCES
    src/persistent_lattice.c
    src/wal.c
    src/dynamic_prefix_index.c
    src/semantic_index.c
    src/export.c
    src/checksum.c
    src/license_utils.c
    src/seqlock.c  # Required by WAL
)

# Optional/supporting files (add as needed)
set(OPTIONAL_SOURCES
    src/cluster_persistence.c
    src/namespace_isolation.c
    src/lattice_constraints.c
    src/isolation.c
    src/integrity_validator.c
    src/intelligent_lattice_loader.c
    src/lattice_dump.c
)

# Headers
set(HEADERS
    include/persistent_lattice.h
    include/wal.h
    include/dynamic_prefix_index.h
    include/semantic_index.h
    include/export.h
    include/checksum.h
    include/license_utils.h
    include/seqlock.h  # Required by WAL
    include/cluster_persistence.h
    include/namespace_isolation.h
    include/lattice_constraints.h
    include/isolation.h
    include/integrity_validator.h
    include/intelligent_lattice_loader.h
)

# Create shared library
add_library(synrix SHARED ${CORE_SOURCES} ${OPTIONAL_SOURCES} ${HEADERS})

# Include directories
target_include_directories(synrix PUBLIC include)

# Compiler options
target_compile_options(synrix PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<C_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

# Optimization
target_compile_options(synrix PRIVATE
    $<$<CONFIG:Release>:-O2>
    $<$<CONFIG:Debug>:-g>
)

# Windows-specific output name
if(WIN32)
    # Check if building free tier version
    if(SYNRIX_IS_FREE_TIER_BUILD)
        set_target_properties(synrix PROPERTIES
            OUTPUT_NAME "libsynrix_free_tier"
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    else()
        set_target_properties(synrix PROPERTIES
            OUTPUT_NAME "libsynrix"
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    endif()
else()
    # Check if building free tier version
    if(SYNRIX_IS_FREE_TIER_BUILD)
        set_target_properties(synrix PROPERTIES
            OUTPUT_NAME "synrix_free_tier"
            PREFIX "lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    else()
        set_target_properties(synrix PROPERTIES
            OUTPUT_NAME "synrix"
            PREFIX "lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )
    endif()
endif()

# Install rules
install(TARGETS synrix
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS} DESTINATION include/synrix)
