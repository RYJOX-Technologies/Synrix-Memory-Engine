# Building SYNRIX Free Tier Package (50k Node Limit)

This guide explains how to build a SYNRIX package specifically for the free tier with a hard-coded 50,000 node limit.

## Overview

The free tier build:
- **Hard-codes** the node limit to 50,000 (cannot be changed at runtime)
- **Always enables** evaluation mode (cannot be disabled)
- **Removes** the `lattice_disable_evaluation_mode()` function
- **Outputs** `libsynrix_free_tier.dll` (Windows) or `libsynrix_free_tier.so` (Linux)

## Build Steps

### Option 1: Using CMake with Free Tier Configuration

```bash
# Windows (MSYS2/MinGW-w64)
cd build/windows
mkdir build_free_tier
cd build_free_tier

# Configure with free tier defines
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED"

# Build
cmake --build . --config Release
```

### Option 2: Using Build Script

Create a build script that sets the defines:

**Windows (build_free_tier.bat):**
```batch
@echo off
cd build\windows
if not exist build_free_tier mkdir build_free_tier
cd build_free_tier

cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_C_FLAGS="-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED"

cmake --build . --config Release
```

**Linux (build_free_tier.sh):**
```bash
#!/bin/bash
cd build/windows
mkdir -p build_free_tier
cd build_free_tier

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED"

cmake --build . --config Release
```

## Code Changes Required

The following changes need to be made to `persistent_lattice.c`:

### 1. Set Free Tier Limit to 50k

```c
// In lattice_init() function
#ifdef SYNRIX_FREE_TIER_50K
    lattice->free_tier_limit = SYNRIX_FREE_TIER_LIMIT;  // 50000
#else
    lattice->free_tier_limit = 25000;  // Default 25k
#endif
```

### 2. Always Enable Evaluation Mode

```c
// In lattice_init() function
#ifdef SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED
    lattice->evaluation_mode = true;  // Always enabled, cannot be disabled
#else
    lattice->evaluation_mode = true;  // Default enabled, can be disabled
#endif
```

### 3. Remove/Disable Evaluation Mode Disable Function

```c
// In lattice_disable_evaluation_mode() function
#ifdef SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED
    // Free tier: evaluation mode cannot be disabled
    lattice->last_error = LATTICE_ERROR_FREE_TIER_LIMIT;
    fprintf(stderr, "SYNRIX Free Tier: Evaluation mode cannot be disabled.\n");
    return -1;
#else
    lattice->evaluation_mode = false;
    return 0;
#endif
```

## Verification

After building, verify the free tier build:

1. **Check DLL name**: Should be `libsynrix_free_tier.dll` (Windows)
2. **Test limit**: Try adding 50,001 nodes - should fail at exactly 50,000
3. **Test disable function**: Call `lattice_disable_evaluation_mode()` - should fail
4. **Check error messages**: Should mention "50,000 nodes" in limit messages

## Distribution

The free tier package includes:
- `libsynrix_free_tier.dll` (or `.so` on Linux)
- Required dependencies (`libgcc_s_seh-1.dll`, `libwinpthread-1.dll`, `zlib1.dll` on Windows)
- Python SDK (unchanged, but will use the free tier DLL)
- Documentation

## Python SDK Integration

The Python SDK automatically detects the DLL name:

```python
# raw_backend.py will look for:
# - libsynrix_free_tier.dll (free tier)
# - libsynrix.dll (standard)
```

Users can still pass `evaluation_mode=False` in Python, but it will be ignored in the free tier build.

## Testing

Test the free tier build:

```python
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend("test.lattice", max_nodes=100000, evaluation_mode=False)

# Add 50,000 nodes - should succeed
for i in range(50000):
    node_id = backend.add_node(f"TEST:node_{i}", f"data_{i}", node_type=3)
    if node_id == 0:
        print(f"Failed at node {i}")
        break

# Try to add 50,001st node - should fail
node_id = backend.add_node("TEST:node_50000", "data", node_type=3)
assert node_id == 0, "Should fail at 50,001st node"

backend.close()
```

## Notes

- The free tier limit is **compile-time only** - cannot be changed at runtime
- Evaluation mode is **always enabled** - cannot be disabled
- The `lattice_disable_evaluation_mode()` function returns an error in free tier builds
- Error messages will show "50,000 nodes" instead of "25,000 nodes"
