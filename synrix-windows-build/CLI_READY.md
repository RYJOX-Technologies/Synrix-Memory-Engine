# SYNRIX CLI is Ready! 🎉

## Build Complete

The CLI executable has been successfully built:
- **Location:** `build/windows/build_msys2/bin/synrix.exe`
- **Status:** ✅ Built and ready to use

## Usage (Just Like Linux!)

### From MSYS2 Terminal (Recommended)

```bash
cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build

# Add to memory
./build/windows/build_msys2/bin/synrix.exe add memory.lattice "MEMORY:test" "data"

# Query memory
./build/windows/build_msys2/bin/synrix.exe query memory.lattice "MEMORY:" 10

# Get specific node
./build/windows/build_msys2/bin/synrix.exe get memory.lattice <node_id>

# Count nodes
./build/windows/build_msys2/bin/synrix.exe count memory.lattice
```

### From PowerShell/CMD

Make sure `libsynrix.dll` is in the same directory as `synrix.exe` or in your PATH:

```cmd
cd C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build
set PATH=%CD%\build\windows\build_msys2\bin;%PATH%

synrix.exe add memory.lattice "MEMORY:test" "data"
synrix.exe query memory.lattice "MEMORY:" 10
```

## Commands

- **`add <lattice> <name> <data>`** - Add a node
- **`get <lattice> <node_id>`** - Get node by ID (O(1))
- **`query <lattice> <prefix> [limit]`** - Query by prefix (O(k))
- **`list <lattice> [prefix]`** - List nodes (alias for query)
- **`count <lattice>`** - Count all nodes

## Output Format

All commands output JSON:

```json
{"success":true,"node_id":6308444734549393409}
{"success":true,"count":1,"nodes":[{"id":12345,"name":"MEMORY:test","data":"data"}]}
{"success":true,"count":5}
```

## No Python Scripts Needed!

You can now use SYNRIX directly from the command line, just like on Linux:

```bash
synrix.exe add memory.lattice "key" "value"
synrix.exe query memory.lattice "prefix"
```

**Windows-native, zero configuration, direct command-line access!**

## Next Steps

1. **Add to PATH** (optional): Copy `synrix.exe` and `libsynrix.dll` to a directory in your PATH
2. **Test it**: Run the commands above to verify it works
3. **Use it**: Start using it in your workflows!
