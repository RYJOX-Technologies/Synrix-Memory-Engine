#ifndef SYS_MMAN_H
#define SYS_MMAN_H

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <sys/types.h>

#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define MAP_SHARED  0x01
#define MAP_PRIVATE 0x02
#define MAP_FAILED  ((void *)-1)

// madvise flags (Linux-specific, no-op on Windows)
#define MADV_NORMAL     0
#define MADV_RANDOM     1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED   3
#define MADV_DONTNEED   4

// msync flags
#define MS_SYNC      0x4
#define MS_ASYNC     0x1
#define MS_INVALIDATE 0x2

// madvise function (no-op on Windows, just return 0)
static inline int madvise(void* addr, size_t len, int advice) {
    (void)addr;
    (void)len;
    (void)advice;
    return 0; // No-op on Windows
}

// sysconf constants
#define _SC_PAGESIZE 30

// sysconf function
static inline long sysconf(int name) {
    if (name == _SC_PAGESIZE) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwPageSize;
    }
    return -1;
}

// pread/pwrite (thread-safe I/O)
static inline ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    
    OVERLAPPED ov = {0};
    ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(offset >> 32);
    
    DWORD bytesRead = 0;
    if (!ReadFile(hFile, buf, (DWORD)count, &bytesRead, &ov)) {
        return -1;
    }
    return (ssize_t)bytesRead;
}

static inline ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) return -1;
    
    OVERLAPPED ov = {0};
    ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)(offset >> 32);
    
    DWORD bytesWritten = 0;
    if (!WriteFile(hFile, buf, (DWORD)count, &bytesWritten, &ov)) {
        return -1;
    }
    return (ssize_t)bytesWritten;
}

// fsync - declared here, defined in windows_compat.c
#ifdef _WIN32
int fsync(int fd);
#else
// On POSIX, fsync is in unistd.h
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset) {
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) {
        return MAP_FAILED;
    }
    
    HANDLE hMapping = CreateFileMapping(hFile, NULL, 
        (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY,
        0, len + offset, NULL);
    if (!hMapping) {
        return MAP_FAILED;
    }
    
    void* ptr = MapViewOfFile(hMapping,
        (prot & PROT_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
        (DWORD)(offset >> 32), (DWORD)(offset & 0xFFFFFFFF), len);
    
    CloseHandle(hMapping);
    return ptr ? ptr : MAP_FAILED;
}

static inline int munmap(void* addr, size_t len) {
    (void)len; // Unused on Windows
    return UnmapViewOfFile(addr) ? 0 : -1;
}

static inline int msync(void* addr, size_t len, int flags) {
    (void)flags; // MS_SYNC/MS_ASYNC - both do the same on Windows
    if (!FlushViewOfFile(addr, len)) {
        return -1;
    }
    // Note: On Windows, also need FlushFileBuffers() for full durability
    // This is handled at a higher level in the code
    return 0;
}

// getpagesize() - Windows equivalent
static inline size_t getpagesize(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

#ifdef __cplusplus
}
#endif

#else
// On non-Windows, use the system header
#include <sys/mman.h>
#include <unistd.h> // for getpagesize
#endif

#endif // SYS_MMAN_H
