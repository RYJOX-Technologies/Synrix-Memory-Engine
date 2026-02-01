# Building SYNRIX Free Tier Package (50k Node Limit)

## Quick Start

### Windows (MSYS2/MinGW-w64)

```bash
cd build/windows
bash build_free_tier_50k.sh
```

Or using the batch file:
```cmd
cd build\windows
build_free_tier_50k.bat
```

### Manual Build

```bash
cd build/windows
mkdir build_free_tier
cd build_free_tier

cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="-DSYNRIX_FREE_TIER_50K -DSYNRIX_FREE_TIER_LIMIT=50000 -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED"

cmake --build . --config Release
```

## What This Build Does

The free tier build creates `libsynrix_free_tier.dll` with:

1. **Hard-coded 50,000 node limit** - Cannot be changed at runtime
2. **Evaluation mode always enabled** - Cannot be disabled
3. **`lattice_disable_evaluation_mode()` disabled** - Returns error if called

## Code Changes

The build uses compile-time defines:
- `SYNRIX_FREE_TIER_50K` - Enables free tier build
- `SYNRIX_FREE_TIER_LIMIT=50000` - Sets the limit to 50k
- `SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED` - Prevents disabling evaluation mode

These are applied in `persistent_lattice.c`:
- `lattice_init()` - Sets limit to 50k and always enables evaluation mode
- `lattice_disable_evaluation_mode()` - Returns error in free tier builds

## Output

The build produces:
- **Windows**: `build_free_tier/bin/libsynrix_free_tier.dll`
- **Linux**: `build_free_tier/lib/libsynrix_free_tier.so`

## Testing

Test the free tier build:

```python
from synrix.raw_backend import RawSynrixBackend

# The SDK will automatically detect libsynrix_free_tier.dll
backend = RawSynrixBackend("test.lattice", max_nodes=100000)

# Add 50,000 nodes - should succeed
for i in range(50000):
    node_id = backend.add_node(f"TEST:node_{i}", f"data_{i}", node_type=3)
    if node_id == 0:
        print(f"Failed at node {i}")
        break

# Try to add 50,001st node - should fail
node_id = backend.add_node("TEST:node_50000", "data", node_type=3)
assert node_id == 0, "Should fail at 50,001st node"

# Try to disable evaluation mode - should fail
# (This would be done via C API, Python SDK doesn't expose it directly)

backend.close()
```

## Distribution

Package the free tier DLL with:
- `libsynrix_free_tier.dll` (or `.so`)
- Required dependencies (MinGW runtime DLLs on Windows)
- Python SDK (unchanged)
- Documentation

## Notes

- The limit is **compile-time only** - cannot be changed
- Error messages will show "50,000 nodes" instead of "25,000"
- The SDK automatically detects the free tier DLL name
- Users can still pass `evaluation_mode=False` in Python, but it's ignored
