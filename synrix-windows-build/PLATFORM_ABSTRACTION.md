# Platform Abstraction Design (Future)

**Note**: Only implement this if Windows becomes a first-class target. For now, MinGW-w64 is sufficient.

## Design Philosophy

**DO NOT** create a fake POSIX layer. That's a maintenance sinkhole.

**DO** abstract only the 3 critical primitives where Windows and Linux fundamentally differ:
1. File open/resize
2. Memory map + flush
3. WAL sync

Everything else stays platform-agnostic.

## Interface Design

### `storage_platform.h`

```c
#ifndef STORAGE_PLATFORM_H
#define STORAGE_PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

// Platform handle (opaque)
typedef struct platform_file platform_file_t;

// Memory mapping handle (opaque - Windows needs a handle, not just base+size)
typedef struct platform_mapping platform_mapping_t;

// File operations
platform_file_t* platform_file_open(const char* path, bool create, bool read_only);
int platform_file_close(platform_file_t* file);
int platform_file_size(platform_file_t* file, uint64_t* size);

// Preallocation (CRITICAL: resize != allocate, and Windows can't grow while mapped)
// This fully allocates the file, not just sets size
int platform_file_preallocate(platform_file_t* file, uint64_t size);

// Thread-safe I/O (no shared file pointer bugs)
// Use pread/pwrite instead of seek+read/write
ssize_t platform_pread(platform_file_t* file, void* buf, size_t count, uint64_t offset);
ssize_t platform_pwrite(platform_file_t* file, const void* buf, size_t count, uint64_t offset);

// Memory mapping
platform_mapping_t* platform_mmap(platform_file_t* file, size_t size, uint64_t offset);
int platform_munmap(platform_mapping_t* mapping);

// Get mapping view pointer/size/offset
void* platform_mapping_base(platform_mapping_t* m) {
    return m->base;
}

size_t platform_mapping_size(platform_mapping_t* m) {
    return m->size;
}

uint64_t platform_mapping_offset(platform_mapping_t* m) {
    return m->offset;
}

// Sync operations - EXPLICIT separation (critical Windows difference)
// View flush: FlushViewOfFile() - flushes memory-mapped view
int platform_sync_view(platform_mapping_t* mapping);

// File flush: FlushFileBuffers() - flushes file handle (durability)
int platform_sync_file(platform_file_t* file);

// Combined sync (both view AND file - required for Windows durability)
int platform_sync_full(platform_mapping_t* mapping, platform_file_t* file);

// WAL sync (separate from mapped data sync)
int platform_wal_sync(platform_file_t* file);

// Platform characteristics (Windows 64KB alignment is not optional)
size_t platform_allocation_granularity(void);  // Windows: 64KB, Linux: page size
size_t platform_page_size(void);              // Usually 4KB on both

// File stats
int platform_stat(const char* path, uint64_t* size, uint64_t* mtime);

#endif
```

## Implementation Notes

### Linux (`linux_storage.c`)

```c
#include "storage_platform.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Straightforward POSIX implementation
// mmap(), msync(MS_SYNC), fsync() work as expected
```

### Windows (`windows_storage.c`)

```c
#include "storage_platform.h"
#include <windows.h>

// Opaque mapping structure (Windows needs a handle)
struct platform_mapping {
    void* base;
    size_t size;
    uint64_t offset;        // Original offset (for platform_mapping_offset())
    HANDLE mapping_handle;  // Windows-specific
    platform_file_t* file;
};

// Opaque file structure (needs to track active mappings)
struct platform_file {
    HANDLE handle;
    int active_mappings;    // Track for resize policy enforcement
    // ... other fields
};

// 1. mmap equivalent with alignment enforcement
platform_mapping_t* platform_mmap(platform_file_t* file, uint64_t offset, size_t size, bool writable) {
    // POLICY: Enforce alignment - return error if not aligned
    uint64_t granularity = platform_map_granularity();
    if (offset % granularity != 0) {
        return NULL;  // Error: offset not aligned
    }
    
    // CreateFileMapping with correct flags
    DWORD prot = writable ? PAGE_READWRITE : PAGE_READONLY;
    HANDLE mapping = CreateFileMapping(file->handle, NULL, prot, 
                                       0, offset + size, NULL);
    if (!mapping) return NULL;
    
    DWORD access = writable ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
    void* base = MapViewOfFile(mapping, access, 0, offset, size);
    if (!base) {
        CloseHandle(mapping);
        return NULL;
    }
    
    // Store mapping handle (opaque to caller)
    platform_mapping_t* m = malloc(sizeof(platform_mapping_t));
    m->base = base;
    m->size = size;
    m->offset = offset;
    m->mapping_handle = mapping;
    m->file = file;
    return m;
}

// 2. Sync - EXPLICIT separation with mode flags
int platform_sync(platform_mapping_t* m, platform_sync_mode_t mode) {
    int ret = 0;
    
    if (mode & PLATFORM_SYNC_VIEW) {
        if (!FlushViewOfFile(m->base, m->size)) ret = -1;
    }
    
    if (mode & PLATFORM_SYNC_FILE) {
        if (!FlushFileBuffers(m->file->handle)) ret = -1;
    }
    
    return ret;
}

int platform_sync_range(platform_mapping_t* m, uint64_t rel_off, size_t len, platform_sync_mode_t mode) {
    // Windows: FlushViewOfFile can do partial ranges
    void* range_base = (char*)m->base + rel_off;
    int ret = 0;
    
    if (mode & PLATFORM_SYNC_VIEW) {
        if (!FlushViewOfFile(range_base, len)) ret = -1;
    }
    
    if (mode & PLATFORM_SYNC_FILE) {
        // File flush is all-or-nothing on Windows
        if (!FlushFileBuffers(m->file->handle)) ret = -1;
    }
    
    return ret;
}

// 3. Resize with active mapping check
int platform_file_resize(platform_file_t* file, uint64_t new_size) {
    // POLICY: Fail if there's an active mapping (Windows can't extend while mapped)
    if (file->active_mappings > 0) {
        return -1;  // Error: cannot resize with active mappings
    }
    
    LARGE_INTEGER li;
    li.QuadPart = new_size;
    if (!SetFilePointerEx(file->handle, li, NULL, FILE_BEGIN)) return -1;
    if (!SetEndOfFile(file->handle)) return -1;
    return 0;
}

// 4. Preallocation (fully allocate, not just resize - no sparse files)
int platform_file_preallocate(platform_file_t* file, uint64_t size) {
    // POLICY: Physically allocate, no sparse files by default
    // This is expensive but required for Windows mmap semantics
    
    // First resize
    if (platform_file_resize(file, size) != 0) return -1;
    
    // Then write zeros to force physical allocation
    // Use SetFileValidData if available (requires SeManageVolumePrivilege)
    // Otherwise, write zeros in chunks
    // ...
    return 0;
}

// 4. Thread-safe I/O (no shared file pointer)
int platform_pread(platform_file_t* file, void* buf, size_t n, uint64_t off) {
    OVERLAPPED ov = {0};
    ov.Offset = (DWORD)(off & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(off >> 32);
    DWORD bytes_read;
    if (!ReadFile(file->handle, buf, n, &bytes_read, &ov)) return -1;
    return (int)bytes_read;
}

int platform_pwrite(platform_file_t* file, const void* buf, size_t n, uint64_t off) {
    OVERLAPPED ov = {0};
    ov.Offset = (DWORD)(off & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(off >> 32);
    DWORD bytes_written;
    if (!WriteFile(file->handle, buf, n, &bytes_written, &ov)) return -1;
    return (int)bytes_written;
}

// 5. WAL sync
int platform_wal_sync(platform_file_t* file) {
    return FlushFileBuffers(file->handle) ? 0 : -1;  // File flush, not view
}

// 7. Platform characteristics
uint64_t platform_page_size(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;  // Usually 4KB
}

uint64_t platform_map_granularity(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwAllocationGranularity;  // 64KB on Windows (mapping offset alignment requirement)
}
```

### Linux (`linux_storage.c`)

```c
#include "storage_platform.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Opaque mapping structure (simpler on Linux)
struct platform_mapping {
    void* base;
    size_t size;
    platform_file_t* file;
};

// Straightforward POSIX implementation
// mmap(), msync(MS_SYNC), fsync() work as expected

// Sync operations with mode flags
int platform_sync(platform_mapping_t* m, platform_sync_mode_t mode) {
    int ret = 0;
    
    if (mode & PLATFORM_SYNC_VIEW) {
        if (msync(m->base, m->size, MS_SYNC) != 0) ret = -1;
    }
    
    if (mode & PLATFORM_SYNC_FILE) {
        if (fsync(m->file->fd) != 0) ret = -1;
    }
    
    return ret;
}

int platform_sync_range(platform_mapping_t* m, uint64_t rel_off, size_t len, platform_sync_mode_t mode) {
    void* range_base = (char*)m->base + rel_off;
    int ret = 0;
    
    if (mode & PLATFORM_SYNC_VIEW) {
        if (msync(range_base, len, MS_SYNC) != 0) ret = -1;
    }
    
    if (mode & PLATFORM_SYNC_FILE) {
        if (fsync(m->file->fd) != 0) ret = -1;
    }
    
    return ret;
}

// Preallocation
int platform_file_preallocate(platform_file_t* file, uint64_t size) {
    return posix_fallocate(file->fd, 0, size);
}

// Thread-safe I/O
int platform_pread(platform_file_t* file, void* buf, size_t n, uint64_t off) {
    ssize_t ret = pread(file->fd, buf, n, off);
    return (ret < 0) ? -1 : (int)ret;
}

int platform_pwrite(platform_file_t* file, const void* buf, size_t n, uint64_t off) {
    ssize_t ret = pwrite(file->fd, buf, n, off);
    return (ret < 0) ? -1 : (int)ret;
}

// Platform characteristics
uint64_t platform_page_size(void) {
    return sysconf(_SC_PAGE_SIZE);  // Usually 4KB
}

uint64_t platform_map_granularity(void) {
    return sysconf(_SC_PAGE_SIZE);  // Usually 4KB (same as page size on Linux)
}
```

## Migration Path

1. **Phase 1**: Build with MinGW-w64 (current)
2. **Phase 2**: Identify all platform-specific code
3. **Phase 3**: Create `storage_platform.h` interface
4. **Phase 4**: Implement Linux version (should be minimal changes)
5. **Phase 5**: Implement Windows version (proper Windows API)
6. **Phase 6**: Replace direct POSIX calls with platform abstraction

## Key Principles

1. **Abstract only what differs** - Don't wrap everything
2. **Windows durability is different** - Acknowledge it, don't hide it
3. **Large files from day one** - Use 64-bit APIs everywhere
4. **Test durability** - WAL correctness matters (torn writes, reordered persistence)
5. **No fake POSIX** - Real platform abstraction
6. **Pre-allocate on Windows** - No lazy growth with active mappings
7. **64KB alignment** - Windows allocation granularity requirement
8. **No sparse files on Windows** - Fully allocate everything
9. **Document Defender exclusions** - Real-world production requirement
10. **Don't promise microsecond determinism on Windows** - It's not Linux