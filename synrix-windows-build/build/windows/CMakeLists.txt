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

# Windows-specific settings
if(WIN32)
    add_definitions(-DSYNRIX_EXPORTS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Signed license verification (Ed25519 via OpenSSL)
find_package(OpenSSL 1.1.1 QUIET)
if(OpenSSL_FOUND)
    set(SYNRIX_LICENSE_USE_OPENSSL 1 CACHE INTERNAL "")
    message(STATUS "OpenSSL found: signed license key verification ENABLED")
else()
    message(STATUS "OpenSSL not found: signed license keys will be rejected (install: pacman -S mingw-w64-x86_64-openssl)")
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
    src/license_verify.c
    src/license_global.c
    src/seqlock.c  # Required by WAL
    src/windows_compat.c  # Windows compatibility functions
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
    src/orchestrator_epistemology.c
    src/semantic_aging.c
)

# Headers (for installation only, not as sources)
set(HEADERS
    include/persistent_lattice.h
    include/wal.h
    include/dynamic_prefix_index.h
    include/semantic_index.h
    include/export.h
    include/checksum.h
    include/seqlock.h  # Required by WAL
    include/cluster_persistence.h
    include/namespace_isolation.h
    include/lattice_constraints.h
    include/isolation.h
    include/integrity_validator.h
    include/intelligent_lattice_loader.h
    include/license_verify.h
    include/license_global.h
)

# Create shared library (sources only, headers are included via include_directories)
add_library(synrix SHARED ${CORE_SOURCES} ${OPTIONAL_SOURCES})

# Include directories
target_include_directories(synrix PUBLIC include)
if(OpenSSL_FOUND)
    target_compile_definitions(synrix PRIVATE SYNRIX_LICENSE_USE_OPENSSL=1)
    target_link_libraries(synrix OpenSSL::Crypto)
endif()

# Link libraries
# zlib for compression (checksum.c, intelligent_lattice_loader.c)
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_link_libraries(synrix ${ZLIB_LIBRARIES})
    target_include_directories(synrix PRIVATE ${ZLIB_INCLUDE_DIRS})
else()
    # Fallback: try common MinGW zlib names
    find_library(ZLIB_LIB z zlib zlib1 PATHS /mingw64/lib /usr/lib)
    if(ZLIB_LIB)
        target_link_libraries(synrix ${ZLIB_LIB})
    else()
        # Last resort: link by name (MinGW usually has it)
        target_link_libraries(synrix z)
    endif()
endif()

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

# CLI executable
add_executable(synrix_cli src/synrix_cli.c)
target_link_libraries(synrix_cli synrix)
target_include_directories(synrix_cli PRIVATE include)

# Windows-specific output name for CLI
if(WIN32)
    set_target_properties(synrix_cli PROPERTIES
        OUTPUT_NAME "synrix"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# Install rules
install(TARGETS synrix synrix_cli
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${HEADERS} DESTINATION include/synrix)


