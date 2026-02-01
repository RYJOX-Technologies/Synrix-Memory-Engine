# WAL Debugging Progress

## Date: 2026-01-12

## Problem Statement
WAL (Write-Ahead Log) entries are not being written on Windows. The sequence number stays at 0, indicating that `wal_append()` is never being called, even though:
- WAL is enabled (`wal_enabled=1`)
- WAL pointer is valid (`wal_ptr=0000028151507af0`)
- WAL file descriptor is valid (`fd=3`)

## What We've Fixed

### 1. Windows Error 5 (Access Denied) - ✅ FIXED
**Problem:** `lattice_save()` was failing with "Windows error 5" when trying to atomically replace the lattice file.

**Root Cause:** On Windows, you cannot replace a file that is still memory-mapped or has open handles. The code was attempting `MoveFileExW` while the file was still mmap'd.

**Fix:** Modified `lattice_save()` in `src/persistent_lattice.c` to:
- Unmap the memory-mapped region (`munmap`) before file replacement
- Close the file descriptor before file replacement
- Reopen and remap after successful replacement

**Result:** File replacement now works correctly. No more "Windows error 5" errors.

### 2. WAL Write Path - Direct Windows API - ✅ IMPLEMENTED
**Problem:** WAL writes using `pwrite()` might not be reliable on Windows.

**Fix:** Modified `wal_append()` in `src/wal.c` to use direct Windows API calls:
- `SetFilePointerEx()` to set file position
- `WriteFile()` to write data directly
- `FlushFileBuffers()` to force data to disk

**Result:** More reliable write path, but still not being called (see investigation below).

### 3. State Ledger Updates - ✅ IMPLEMENTED
**Problem:** WAL header wasn't being updated with `commit_count` and `last_valid_offset`, making recovery unreliable.

**Fix:** Modified `wal_append()` to update the WAL header immediately after writes with:
- `commit_count = wal->sequence` (tracks committed entries)
- `last_valid_offset = wal->file_pos` (tracks end of valid data)

**Result:** State Ledger is now updated, but only if `wal_append()` is called (which it isn't).

## Current Investigation

### Observation
We see:
- `!!! DEBUG: ENTERED lattice_add_node_internal` - Function is being called ✅
- `[LATTICE-ADD] WAL state: wal_enabled=1, wal_ptr=0000028151507af0` - WAL is enabled ✅
- `[LATTICE-ADD] WAL details: enabled=1, fd=3, sequence=0` - WAL context is valid ✅

But we DON'T see:
- `!!! DEBUG: Reached end of node creation, about to check WAL` - Not reached ❌
- `!!! DEBUG: About to check WAL in lattice_add_node_internal` - Not reached ❌
- `[WAL-DEBUG] Immediate write: file_pos=...` - Not reached ❌
- `!!! DEBUG: ENTERED wal_append` - Not reached ❌

### Hypothesis
The function `lattice_add_node_internal` is being called, but it's returning early before reaching the WAL write block at the end of the function. Possible causes:

1. **Early Return:** There's a `return node->id;` statement before the WAL write block
2. **Different Code Path:** The function might be using a different code path that doesn't include the WAL write block
3. **Conditional Skip:** The WAL write block might be inside a conditional that's not being met

### Next Steps
1. Trace through `lattice_add_node_internal` to find all return points
2. Check if there's a code path that skips the WAL write block
3. Verify that the WAL write block is actually at the end of the function (not in a conditional)
4. Add more debug points throughout the function to trace execution flow

## What's Working

### Core Functionality - ✅ WORKING
- **Node Addition:** Nodes are being added to memory correctly
- **Persistence:** Nodes are being saved to disk correctly (all 3 nodes persist across sessions)
- **File Operations:** Atomic file replacement works on Windows
- **Memory Management:** Unmapping/remapping works correctly

### WAL Infrastructure - ✅ WORKING
- **WAL Initialization:** WAL file is created and initialized correctly
- **WAL Recovery:** Recovery logic works (correctly stops at sentinel)
- **WAL State:** WAL context is valid and enabled

### WAL Writes - ❌ NOT WORKING
- **WAL Append:** `wal_append()` is never being called
- **Sequence Increment:** Sequence stays at 0
- **Entry Writing:** No WAL entries are being written

## Impact Assessment

### Criticality: Medium
- **Main Save Works:** The primary persistence mechanism (direct lattice file save) works correctly
- **WAL is Safety Net:** WAL is for crash recovery, not primary persistence
- **Data Safety:** Nodes are being saved, so data is not being lost
- **Recovery:** If the process crashes, WAL recovery won't work (but main file is saved)

### Recommendation
Continue investigation to understand why WAL write block isn't being reached. This is important for:
1. Crash recovery (WAL replay)
2. Understanding the code flow
3. Ensuring all code paths are correct

## Files Modified

1. `src/persistent_lattice.c`
   - Fixed `lattice_save()` to unmap/close before file replacement on Windows
   - Added nuclear debug to `lattice_add_node()` and `lattice_add_node_internal()`
   - Added debug before WAL write block

2. `src/wal.c`
   - Changed Windows write path to use `WriteFile()` API directly
   - Added State Ledger updates (`commit_count`, `last_valid_offset`)
   - Added extensive debug logging

3. `build/windows/src/persistent_lattice.c` (copied from main source)
4. `build/windows/src/wal.c` (copied from main source)

## Test Results

### Test: Add 3 nodes, flush, close, reopen
**Result:** ✅ All 3 nodes persist correctly
**WAL Status:** ❌ Sequence stays at 0, no entries written

### Test: File replacement
**Result:** ✅ No "Windows error 5" errors
**Status:** ✅ File replacement works correctly

## Debug Output Analysis

```
!!! DEBUG: ENTERED lattice_add_node for name: TEST:node_0
[LATTICE-ADD] WAL state: wal_enabled=1, wal_ptr=0000028151507af0
!!! DEBUG: ENTERED lattice_add_node_internal for name: TEST:node_0
```

**Analysis:**
- `lattice_add_node()` is called ✅
- `lattice_add_node_internal()` is called ✅
- WAL is enabled and valid ✅
- But execution stops here - WAL write block is never reached ❌

## Next Investigation Steps

1. Add debug at every return point in `lattice_add_node_internal`
2. Check if there's a code path that uses `lattice_add_node_binary()` instead
3. Verify the function structure - is the WAL write block actually at the end?
4. Check if there's a conditional wrapper around the WAL write block

## BREAKTHROUGH: Function Structure Issue

### Discovery (2026-01-12 17:05)
**CRITICAL FINDING:** The WAL write block is NOT in `lattice_add_node_internal`!

**Evidence:**
- `lattice_add_node_internal` returns at line 1380: `return node->id;`
- WAL write block debug is at line 2018: `!!! DEBUG: Reached end of node creation...`
- Line 2018 is in a DIFFERENT function: `lattice_add_node_binary()`

**Function Call Chain:**
1. `lattice_add_node()` (line 1384) - Public API
2. Calls `lattice_add_node_internal()` (line 1434) - Internal function
3. `lattice_add_node_internal()` returns at line 1380 - **NO WAL WRITE BLOCK**
4. `lattice_add_node_binary()` (line 1806) - Has WAL write block at line 2018

**Root Cause:**
The WAL write block is only in `lattice_add_node_binary()`, not in `lattice_add_node_internal()`. The regular `lattice_add_node()` -> `lattice_add_node_internal()` path doesn't include WAL writes!

**Solution:**
Move the WAL write block from `lattice_add_node_binary()` to `lattice_add_node_internal()` so it's executed for all node additions, not just binary node additions.

### Fix Applied (2026-01-12 17:10)
**Action:** Added WAL write block to `lattice_add_node_internal()` before the return statement at line 1380.

**Changes:**
- Copied WAL write block logic from `lattice_add_node_binary()` (lines 2018-2080)
- Added to `lattice_add_node_internal()` before `return node->id;` at line 1380
- Includes all debug logging and error handling
- Uses same packed data format: `type(1) | name_len(4) | name | data_len(4) | data | parent_id(8)`

**Expected Result:**
- WAL write block should now be executed for all node additions
- `wal_append()` should be called
- Sequence should increment (1, 2, 3...)
- WAL entries should be written to disk

**Next Test:**
Rebuild and test to verify WAL writes are now working.

## SUCCESS! WAL is Now Working (2026-01-12 17:06)

### Test Results
**Test:** Add 3 nodes, flush, close, reopen
**Result:** ✅ **COMPLETE SUCCESS**

### Evidence of Success

1. **Sequence Incrementing** ✅
   - Sequence: 0 → 1 → 2 → 3
   - Each `wal_append()` call returns the correct sequence number

2. **WAL Entries Written** ✅
   - Batch count: 0 → 1 → 2 → 3
   - All 3 entries successfully written to WAL

3. **WAL Recovery Working** ✅
   - Recovery message: "3 entries replayed"
   - State Ledger working: "commit_count=3, reading up to offset 238"
   - Recovery correctly stops at sentinel (offset 238)

4. **Persistence** ✅
   - All 3 nodes persist correctly across sessions
   - No data loss

### Debug Output Analysis

**First Session (Adding Nodes):**
```
!!! DEBUG: ENTERED wal_append: wal=0000013fc4507af0, enabled=1, fd=3, operation=1, node_id=...
[LATTICE-DEBUG] wal_append returned: 1
[LATTICE-DEBUG] wal_append returned: 2
[LATTICE-DEBUG] wal_append returned: 3
[LATTICE-WAL] 🔄 Flushing WAL buffer (3 entries) to disk...
[WAL-FLUSH] ✅ Flushing batch: 3 entries, 198 bytes (0.19 KB)
```

**Second Session (Recovery):**
```
[WAL] 📊 State Ledger: commit_count=3, reading up to offset 238
[WAL] ✅ Recovery complete (mmap'd): 3 entries replayed
```

**Note:** On second session, `wal_enabled=0` during recovery is **expected behavior** - prevents double-logging nodes restored from WAL.

### What Fixed It

**Root Cause:** WAL write block was in `lattice_add_node_binary()` but `lattice_add_node()` calls `lattice_add_node_internal()`, which returned without executing the WAL write block.

**Solution:** Added WAL write block to `lattice_add_node_internal()` before the return statement at line 1380.

### Status: ✅ RESOLVED

- ✅ WAL writes working
- ✅ WAL recovery working
- ✅ State Ledger working
- ✅ Sequence incrementing correctly
- ✅ All nodes persisting correctly
- ✅ Windows file operations working (no Error 5)

**The WAL is now fully functional on Windows!**
