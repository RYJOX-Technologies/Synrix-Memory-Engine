# Windows-Native Approach for SYNRIX

## The Problem with Shell Commands on Windows

Traditional approaches that use shell scripts (`.bat`, `.ps1`) or `shell=True` in subprocess calls are problematic on Windows:

- **Execution Policy Issues**: PowerShell scripts require execution policy configuration
- **Shell Overhead**: Spawning `cmd.exe` or `powershell.exe` adds unnecessary overhead
- **Path Issues**: Shell path resolution can be inconsistent
- **User Experience**: Console windows flash, scripts are visible to users
- **Friction**: Users must configure shell environments, manage scripts

## The Correct Windows-Native Approach

### 1. Stop Thinking "Run Commands" → Think "Spawn Process"

**❌ WRONG (Shell-based):**
```python
subprocess.run(["synrix.exe", "--port", "6334"], shell=True)
subprocess.run("synrix.exe --port 6334", shell=True)
os.system("synrix.exe --port 6334")
```

**✅ CORRECT (Process-based):**
```python
subprocess.Popen(
    ["synrix.exe", "--port", "6334"],
    creationflags=subprocess.CREATE_NO_WINDOW,
    close_fds=True
)
```

### 2. Python as Control Plane, Not Shell

**On Linux:**
- Shell scripts are fine (bash is standard)
- `subprocess.run()` with shell=True works well

**On Windows:**
- Python must own process lifecycle
- No shell scripts, no `.bat`, no `.ps1`
- Direct process spawning with `subprocess.Popen`

### 3. Why This Disappears Once Bundled

Once you ship:
```
synrix/
  synrix.exe
  client.py
```

And users do:
```python
from synrix.auto_daemon import get_synrix_client
client = get_synrix_client()
```

Then:
- ✅ No scripts
- ✅ No shell
- ✅ No PowerShell
- ✅ No CMD
- ✅ No execution policy
- ✅ No friction

This is exactly how:
- **Docker Desktop** works on Windows
- **Redis Windows ports** work
- **Postgres installers** work
- **VS Code** itself works

## Implementation

### Auto-Daemon Manager

The `auto_daemon.py` module implements the Windows-native approach:

```python
from synrix.auto_daemon import SynrixAutoDaemon

# Auto-starts engine if needed
daemon = SynrixAutoDaemon()
client = daemon.get_client()
node_id = client.add_node("TASK:test", "data")
```

**Key Features:**
- Uses `subprocess.Popen` with `CREATE_NO_WINDOW` flag
- No shell scripts, no `shell=True`
- Python owns process lifecycle
- Automatic engine discovery
- Port conflict detection

### Convenience Function

For simple usage:

```python
from synrix.auto_daemon import get_synrix_client

client = get_synrix_client()
node_id = client.add_node("TASK:test", "data")
```

## Windows-Specific Details

### CREATE_NO_WINDOW Flag

The `CREATE_NO_WINDOW` flag is critical:
- Prevents console window from appearing
- Clean user experience (like Docker Desktop)
- No flashing windows

### Process Lifecycle

Python manages the entire lifecycle:
- **Start**: `subprocess.Popen()` with proper flags
- **Monitor**: Check if process is alive, port is listening
- **Stop**: `process.terminate()` or `process.kill()`
- **Cleanup**: Proper resource cleanup on exit

### No Shell Dependencies

Everything is done in Python:
- No `.bat` files
- No `.ps1` files
- No `shell=True`
- No `cmd.exe`
- No `powershell.exe`

## Migration Guide

### Old Approach (Shell-based)

```python
# ❌ OLD: Shell script approach
import subprocess
subprocess.run(["start_synrix.bat"], shell=True)
client = SynrixClient()
```

### New Approach (Windows-native)

```python
# ✅ NEW: Windows-native approach
from synrix.auto_daemon import get_synrix_client
client = get_synrix_client()  # Auto-starts if needed
```

## Benefits

1. **Zero Configuration**: No execution policy, no shell setup
2. **Clean UX**: No console windows, no scripts visible
3. **Reliable**: Direct process control, no shell interpretation
4. **Cross-Platform**: Same API works on Linux (with shell) and Windows (native)
5. **Professional**: Matches how major Windows applications work

## Example: Full Integration

```python
from synrix.auto_daemon import SynrixAutoDaemon

# Context manager for automatic cleanup
with SynrixAutoDaemon() as daemon:
    client = daemon.get_client()
    
    # Use client normally
    node_id = client.add_node("TASK:write_function", "def foo(): pass")
    results = client.query_prefix("TASK:")
    
    # Engine automatically stops when context exits
```

## Summary

**The Windows-native approach:**
- ✅ Spawn processes directly with `subprocess.Popen`
- ✅ Use `CREATE_NO_WINDOW` flag
- ✅ Python owns process lifecycle
- ✅ No shell scripts, no `.bat`, no `.ps1`
- ✅ Zero configuration, zero friction

**This is how professional Windows applications work.**
