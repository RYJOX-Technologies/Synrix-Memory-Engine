/*
 * synrix_platform.h — Fills POSIX gaps on MinGW-w64 (MSYS2).
 * MinGW provides: unistd.h, pthread.h, sys/time.h, sys/stat.h, fcntl.h, stdatomic.h
 * MinGW is missing: sys/mman.h, pread/pwrite, fsync, sysconf, getpagesize
 *
 * Include AFTER standard headers in .c files that use mmap or pread/pwrite.
 */
#ifndef SYNRIX_PLATFORM_H
#define SYNRIX_PLATFORM_H

#ifdef _WIN32

#include <windows.h>
#include <io.h>

/* ── ssize_t (in case MinGW version doesn't define it) ───────────────────── */
#ifndef _SSIZE_T_DEFINED
#ifdef __MINGW64__
/* MinGW-w64 typically defines ssize_t in sys/types.h */
#else
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif
#endif

/* ── Memory mapping (sys/mman.h replacement) ─────────────────────────────── */
#ifndef PROT_READ
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FAILED    ((void*)-1)
#define MS_SYNC       0

static inline void* mmap(void* addr, size_t length, int prot, int flags,
                          int fd, long long offset) {
    (void)addr; (void)flags;
    DWORD flProtect = PAGE_READONLY;
    DWORD dwAccess  = FILE_MAP_READ;
    if (prot & PROT_WRITE) {
        flProtect = PAGE_READWRITE;
        dwAccess  = FILE_MAP_WRITE;
    }
    HANDLE hFile = (HANDLE)_get_osfhandle(fd);
    if (hFile == INVALID_HANDLE_VALUE) return MAP_FAILED;
    DWORD szHigh = (DWORD)((length + offset) >> 32);
    DWORD szLow  = (DWORD)((length + offset) & 0xFFFFFFFF);
    HANDLE hMap = CreateFileMappingA(hFile, NULL, flProtect, szHigh, szLow, NULL);
    if (!hMap) return MAP_FAILED;
    DWORD offHigh = (DWORD)(offset >> 32);
    DWORD offLow  = (DWORD)(offset & 0xFFFFFFFF);
    void* ptr = MapViewOfFile(hMap, dwAccess, offHigh, offLow, length);
    CloseHandle(hMap);
    return ptr ? ptr : MAP_FAILED;
}

static inline int munmap(void* addr, size_t length) {
    (void)length;
    return UnmapViewOfFile(addr) ? 0 : -1;
}

static inline int msync(void* addr, size_t length, int flags) {
    (void)flags;
    return FlushViewOfFile(addr, length) ? 0 : -1;
}
#endif /* PROT_READ */

/* ── pread / pwrite (not in MinGW) ───────────────────────────────────────── */
#ifndef SYNRIX_HAVE_PREAD
#define SYNRIX_HAVE_PREAD

static inline ssize_t pread(int fd, void* buf, size_t count, off_t offset) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    OVERLAPPED ov = {0};
    ov.Offset     = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)((uint64_t)offset >> 32);
    DWORD nread = 0;
    if (!ReadFile(h, buf, (DWORD)count, &nread, &ov)) return -1;
    return (ssize_t)nread;
}

static inline ssize_t pwrite(int fd, const void* buf, size_t count, off_t offset) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    OVERLAPPED ov = {0};
    ov.Offset     = (DWORD)(offset & 0xFFFFFFFF);
    ov.OffsetHigh = (DWORD)((uint64_t)offset >> 32);
    DWORD nwritten = 0;
    if (!WriteFile(h, buf, (DWORD)count, &nwritten, &ov)) return -1;
    return (ssize_t)nwritten;
}
#endif /* SYNRIX_HAVE_PREAD */

/* ── fsync / fdatasync (MinGW has _commit) ───────────────────────────────── */
#ifndef fsync
#define fsync(fd)     _commit(fd)
#endif
#ifndef fdatasync
#define fdatasync(fd) _commit(fd)
#endif

/* ── Page size ──────────────────────────────────────────────────────────── */
#ifndef SYNRIX_HAVE_PAGESIZE
#define SYNRIX_HAVE_PAGESIZE

static inline int getpagesize(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwPageSize;
}

#ifndef _SC_PAGESIZE
#define _SC_PAGESIZE 30
#endif
static inline long sysconf(int name) {
    if (name == _SC_PAGESIZE) return (long)getpagesize();
    return -1;
}
#endif /* SYNRIX_HAVE_PAGESIZE */

/* ── ftruncate (MinGW may have it, but ensure it's available) ────────────── */
#ifndef SYNRIX_HAVE_FTRUNCATE
#define SYNRIX_HAVE_FTRUNCATE
/* MinGW-w64 provides ftruncate in unistd.h; if not, fall back to _chsize_s */
#endif

#else
/* ── Linux / native POSIX ────────────────────────────────────────────────── */
#include <sys/mman.h>
#endif /* _WIN32 */

#endif /* SYNRIX_PLATFORM_H */
