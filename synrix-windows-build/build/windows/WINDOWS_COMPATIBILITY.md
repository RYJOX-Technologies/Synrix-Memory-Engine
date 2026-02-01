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

### Build Options

**Option 1: MinGW-w64 (Recommended for Now)**
- Fastest path to working Windows build
- POSIX compatibility layer handles most issues
- Use for initial Windows support

**Option 2: Platform Abstraction (Future)**
- Only if Windows becomes first-class
- Abstract the 3 critical primitives
- Keep everything else platform-agnostic

## Current Status

The CMakeLists.txt is configured for Windows. With MinGW-w64, most POSIX code should compile, but be aware of the gotchas above.

### Critical Areas to Watch

1. **persistent_lattice.c** - Uses `mmap()`, `munmap()`, `msync()`
   - MinGW provides these, but remember: `msync()` on Windows needs both `FlushViewOfFile()` AND `FlushFileBuffers()`

2. **wal.c** - Uses file I/O and `fsync()`
   - `fsync()` → `FlushFileBuffers()` (not just view flush)
   - WAL durability is critical - test thoroughly

3. **Large file support**
   - Any `lseek()` or `stat()` calls need 64-bit variants
   - Check for `SetFilePointer()` usage (should be `SetFilePointerEx()`)

## Implementation Strategy

### Phase 1: MinGW-w64 (Current)

1. Build with MinGW-w64 generator
2. Test basic functionality
3. Document any MinGW-specific issues
4. Accept that some edge cases may not work perfectly
5. **Pre-allocate lattice size** - no dynamic growth
6. **Disable sparse files** - fully allocate everything
7. **Document Defender exclusions** - don't debug latency ghosts
8. **Test WAL replay with torn write scenarios** - NTFS is weaker

### Phase 2: Platform Abstraction (If Needed)

Only if Windows becomes first-class:

1. Create `storage_platform.h` interface:
   ```c
   // Abstract the 3 critical primitives
   int platform_file_open(const char* path, ...);
   void* platform_mmap(int fd, size_t size, ...);
   int platform_sync(void* mapping, int fd);  // Both view AND file
   ```

2. Implement `linux_storage.c` (native POSIX)
3. Implement `windows_storage.c` (proper Windows API)
4. Keep everything else platform-agnostic

**Don't create a fake POSIX layer. Abstract the real differences.**

## Testing

After building, test with:
```python
from synrix.raw_backend import RawSynrixBackend
backend = RawSynrixBackend("test.lattice")
# Should work if DLL is correct
```
