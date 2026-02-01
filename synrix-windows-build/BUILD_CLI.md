# Building SYNRIX CLI (synrix.exe)

## Quick Build

**From MSYS2 MinGW64 terminal:**

```bash
cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows
bash build_msys2.sh
```

This will build both:
- `libsynrix.dll` (library)
- `synrix.exe` (CLI executable)

## After Building

The CLI will be at:
```
build/windows/build_msys2/bin/synrix.exe
```

## Usage (Like Linux)

Once built, you can use it directly from command line:

```cmd
REM Add a node
synrix.exe add memory.lattice "MEMORY:test" "This is test data"

REM Get a node by ID
synrix.exe get memory.lattice 12345

REM Query by prefix
synrix.exe query memory.lattice "MEMORY:" 10

REM List all nodes
synrix.exe list memory.lattice

REM Count nodes
synrix.exe count memory.lattice
```

## Add to PATH (Optional)

To use `synrix.exe` from anywhere:

1. Copy `synrix.exe` to a directory in your PATH (e.g., `C:\Windows\System32` or create `C:\synrix\bin`)
2. Add that directory to your PATH environment variable

Then you can use:
```cmd
synrix add memory.lattice "MEMORY:test" "data"
```

## Example Workflow

```cmd
REM Initialize a lattice
synrix.exe add ~/.synrix.lattice "MEMORY:project_name" "My Project"

REM Add more memories
synrix.exe add ~/.synrix.lattice "MEMORY:api_key" "abc123"
synrix.exe add ~/.synrix.lattice "MEMORY:config" "port=8080"

REM Query all memories
synrix.exe query ~/.synrix.lattice "MEMORY:" 100

REM Get specific memory
synrix.exe get ~/.synrix.lattice <node_id>
```

## Output Format

All commands output JSON:

```json
{"success":true,"node_id":12345}
{"success":true,"id":12345,"name":"MEMORY:test","data":"This is test data"}
{"success":true,"count":5,"nodes":[...]}
```

This makes it easy to parse in scripts or other tools.
