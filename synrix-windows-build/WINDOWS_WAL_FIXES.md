# Windows-Specific WAL Fixes

## Problem Analysis

Your analysis was correct. Windows WAL behavior differs from Linux:

1. **File Truncation**: Windows doesn't automatically shrink files - need `SetEndOfFile`
2. **Write Cache**: Windows write cache delays durability - need `FlushFileBuffers`
3. **Header Sync**: WAL header must be synced immediately for recovery to see entries
4. **Mmap Behavior**: Windows mmap semantics differ from Linux

## Fixes Applied

### 1. Windows-Specific Hard Flush

Added `FlushFileBuffers()` calls after critical WAL operations:

```c
#ifdef _WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
    if (hFile != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(hFile);  // Bypasses Windows write cache
    }
#endif
```

**Locations:**
- After WAL header updates (flush thread)
- After checkpoint header updates
- After file truncation

### 2. SetEndOfFile for Truncation

Windows requires explicit `SetEndOfFile` after `ftruncate`:

```c
#ifdef _WIN32
    HANDLE hFile = (HANDLE)_get_osfhandle(wal->wal_fd);
    LARGE_INTEGER li;
    li.QuadPart = truncate_offset;
    if (SetFilePointerEx(hFile, li, NULL, FILE_BEGIN)) {
        SetEndOfFile(hFile);
        FlushFileBuffers(hFile);
    }
#endif
```

**Locations:**
- `wal_truncate()` - when truncating to header size
- `wal_truncate()` - when truncating at checkpoint boundary

### 3. WAL Header Sync

WAL header is now synced immediately after updates:

```c
// Update header
pwrite(wal->wal_fd, &header, sizeof(header), 0);

// Windows: Hard flush
#ifdef _WIN32
    FlushFileBuffers(hFile);
#endif

// Linux: fsync
fsync(wal->wal_fd);
```

**Why**: Recovery reads the header first. If header isn't synced, recovery sees stale sequence numbers.

### 4. Checkpoint Flush Before Recovery

`lattice_wal_checkpoint()` now flushes WAL buffer before recovering:

```c
// Flush WAL buffer BEFORE recovering
if (lattice->wal->batch_count > 0) {
    wal_flush(lattice->wal);
    wal_flush_wait(lattice->wal, lattice->wal->sequence);
}
```

**Why**: Recovery reads from WAL file. If entries are still in buffer, recovery sees 0 entries.

## Testing

After rebuilding with these fixes:

1. **WAL entries should be written to file** (not just buffered)
2. **WAL file size should grow** after adding nodes
3. **Flush should write buffered entries** to disk
4. **Recovery should see entries** (not "0 entries replayed")
5. **Checkpoint should truncate file** properly on Windows

## Expected Behavior

**Before Fix:**
- WAL file: 24 bytes (header only)
- Recovery: "0 entries replayed"
- Flush: No effect (buffer empty or not flushed)

**After Fix:**
- WAL file: Grows with entries (header + entries)
- Recovery: "N entries replayed" (N > 0)
- Flush: Writes buffered entries to disk
- Checkpoint: Truncates file properly

## Next Steps

1. Rebuild DLL with Windows-specific fixes
2. Test WAL flush functionality
3. Test WAL recovery
4. Verify checkpoint truncation works
