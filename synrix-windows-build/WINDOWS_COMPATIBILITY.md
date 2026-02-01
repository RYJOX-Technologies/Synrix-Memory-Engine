# Windows Compatibility Notes

## Critical Windows Gotchas (Not Sugarcoated)

**Strategic Recommendation**: Primary target is Linux (native, mmap-first, no compromises). Windows support via MinGW-w64 for fastest bring-up. If Windows becomes first-class, abstract only 3 primitives, not a full POSIX shim.

### Sharp Edges Where Windows Looks Equivalent But Isn't

1. **mmap Parity is NOT Real**
   - `FlushViewOfFile()` ≠ `msync(MS_SYNC)`
   - **MUST also call `FlushFileBuffers(file_handle)`** or you don't get durability
   - Windows mappings are page-granular; partial flush semantics differ
   - File must be opened with `GENERIC_READ | GENERIC_WRITE` and sharing flags correct or mapping fails silently

2. **Large Files (>2GB)**
   - `SetFilePointer()` is a trap - **use `SetFilePointerEx()`** or you will break >2GB lattices
   - Same for `_stat()` → use `_stat64()`

3. **WAL Durability**
   - `fsync()` maps to `FlushFileBuffers()`, not view flush
   - If WAL correctness matters (it does), treat mapped data and WAL as separate durability domains

4. **File Locking**
   - POSIX advisory locks ≠ Windows mandatory locks
   - If you rely on `flock()` semantics anywhere, you'll need `LockFileEx()` and explicit discipline

5. **Permissions**
   - Don't try to emulate POSIX perms. Windows ACLs are a different universe
   - **Best move: ignore permissions entirely at the engine level on Windows**

6. **Time Precision**
   - `GetSystemTimeAsFileTime()` is low resolution and subject to adjustments
   - **Prefer `QueryPerformanceCounter()` + frequency for latency metrics**
   - Don't try to fake `gettimeofday()` precision

7. **Signals / Threads**
   - If you use `pthread_*`, signals, `usleep`, `nanosleep` - you'll need real replacements
   - MinGW papers over some of this, MSVC does not

8. **File Growth + mmap**
   - **On Windows, you cannot safely extend a file backing an active mapping**
   - POSIX often "just works"; Windows requires:
     - Unmap view
     - Resize file
     - Re-CreateFileMapping
     - Remap
   - **If SYNRIX ever grows lattices dynamically → this must be explicit**
   - **Rule: Pre-allocate lattice size on Windows. No lazy growth.**

9. **Sparse Files & Hole Punching**
   - `fallocate()`, sparse tricks, hole punching → not portable
   - NTFS sparse files exist, but semantics differ and interact badly with mmap + durability
   - **Recommendation: Disable sparse files on Windows**
   - **Always fully allocate lattice + WAL**

10. **Crash Consistency ≠ Linux**
    - NTFS + memory-mapped I/O has weaker crash guarantees
    - Even with `FlushViewOfFile()` + `FlushFileBuffers()`, ordering is not always preserved
    - **WAL replay must assume:**
      - Torn writes
      - Reordered persistence
    - This reinforces the "separate durability domains" point

11. **Alignment & Page Size**
    - Windows page size is usually 4KB, but **allocation granularity is 64KB**
    - `MapViewOfFile()` offsets **must be 64KB aligned**
    - If you ever do partial mapping / remapping → alignment bugs await

12. **Antivirus / Defender Interference**
    - mmap'd DB files + frequent flushes can trigger:
      - Latency spikes
      - Access denial
    - **Real-world fix: document excluding lattice/WAL paths from Defender**
    - Don't debug ghosts later

## Windows-Specific Fixes Applied

### 1. Atomic File Replacement ✅
- **Issue**: `rename()` fails if target exists on Windows
- **Fix**: Use `MoveFileEx` with `MOVEFILE_REPLACE_EXISTING` for atomic replacement
- **Location**: `src/persistent_lattice.c` in `lattice_save()` function
- **Status**: ✅ Fixed and tested

### 2. Memory-Mapped File Handling ✅
- **Issue**: Cannot delete/replace memory-mapped files on Windows
- **Fix**: Unmap and close file before atomic replacement
- **Location**: `src/persistent_lattice.c` in `lattice_save()` function
- **Status**: ✅ Fixed and tested

### 3. WAL Recovery Bypass ✅
- **Issue**: WAL recovery was counting against free tier limit
- **Fix**: Created `lattice_add_node_internal()` that bypasses limit check
- **Location**: `src/persistent_lattice.c`
- **Status**: ✅ Fixed - recovery restores state, doesn't "add" nodes

### 4. Error Message Formatting ✅
- **Issue**: Unicode emojis cause Windows encoding errors
- **Fix**: Use plain text (PASS/FAIL) instead of emojis
- **Status**: ✅ Fixed in all test scripts

### 5. DLL Path Detection ✅
- **Issue**: DLL path detection fails in test scripts
- **Fix**: Set `SYNRIX_LIB_PATH` environment variable before importing
- **Status**: ✅ Documented and working

## Strategic Approach (No Sugarcoating)

### DO NOT Write a Full POSIX Shim

That's a maintenance sinkhole. Don't do it.

### Recommended Path for SYNRIX

**Primary Target: Linux** (native, mmap-first, no compromises)

**Windows Strategy:**

1. **MinGW-w64 first** for fastest bring-up
   - Provides POSIX compatibility layer
   - Most functions work out of the box
   - **Fine as a compatibility crutch, not a correctness guarantee**

2. **If/when Windows becomes first-class:**

   Abstract only 3 primitives:
   - File open/resize
   - Memory map + flush
   - WAL sync

   Everything else stays platform-agnostic.

   Architecture:
   ```
   storage_platform.h
     ├── linux_storage.c
     └── windows_storage.c
   ```

   Not a fake POSIX layer - real platform abstraction.

### Strategic Verdict (Blunt)

- **"Linux first, Windows tolerated" stance is exactly right**
- **MinGW-w64 is fine as a compatibility crutch, not a correctness guarantee**
- **The 3-primitive abstraction is the only sane long-term Windows story**
- **Do NOT promise Windows microsecond determinism — ever**

## Build Options

**Option 1: MinGW-w64 (Recommended for Now)**
- Fastest path to working Windows build
- POSIX compatibility layer handles most issues
- Use for initial Windows support

**Option 2: Platform Abstraction (Future)**
- Only if Windows becomes first-class
- Abstract the 3 critical primitives
- Keep everything else platform-agnostic

## Current Status

✅ **Windows build is production-ready for agent workloads**

- All 7/7 robustness tests passing
- Atomic file replacement working
- WAL recovery verified
- Performance acceptable (sub-millisecond operations)
- Free tier package ready for distribution

### Critical Areas Verified

1. **persistent_lattice.c** - ✅ Windows atomic file replacement fixed
2. **wal.c** - ✅ WAL recovery bypasses free tier limit
3. **File I/O** - ✅ Atomic replacement using MoveFileEx
4. **Memory mapping** - ✅ Proper unmapping before file operations

## Testing

After building, run robustness tests:
```bash
python test_windows_robust.py
```

Expected: **7/7 tests passing**

## Performance Characteristics

- **Node Addition**: ~2.3 μs/node (matches Linux)
- **Node Lookup**: ~20.9 μs (acceptable, ~20x slower than Linux due to DLL overhead)
- **Prefix Queries**: ~281 μs (acceptable, ~3x slower than Linux)
- **All Operations**: Sub-millisecond (production-ready for agents)

See `BENCHMARK_SUMMARY.md` for detailed performance analysis.
