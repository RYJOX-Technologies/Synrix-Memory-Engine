/*
 * Global license usage: file-based store with locking, keyed by license key hash.
 * One global node cap per license per machine.
 */

#include "license_global.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <pwd.h>
#endif

#define LICENSE_GLOBAL_DIR "license_usage"
#define SYNRIX_DIR "Synrix"
#define SYNRIX_DIR_UNIX ".synrix"
#define FILE_SUFFIX ".dat"
#define HASH_HEX_LEN 16
#define LINE_BUF 64

static uint64_t str_hash(const char* s) {
    uint64_t h = 14695981039346656037ULL;
    if (!s) return h;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

static int get_usage_dir(char* buf, size_t buf_size) {
    if (!buf || buf_size < 2) return -1;
#ifdef _WIN32
    {
        char local[MAX_PATH];
        if (FAILED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, local)))
            return -1;
        if (snprintf(buf, buf_size, "%s\\%s\\%s", local, SYNRIX_DIR, LICENSE_GLOBAL_DIR) >= (int)buf_size)
            return -1;
    }
#else
    {
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "/tmp";
        }
        if (snprintf(buf, buf_size, "%s/%s/%s", home, SYNRIX_DIR_UNIX, LICENSE_GLOBAL_DIR) >= (int)buf_size)
            return -1;
    }
#endif
    return 0;
}

static int ensure_dir(const char* path) {
#ifdef _WIN32
    (void)path;
    return 0;
#else
    char parent[1024];
    size_t i = strlen(path);
    if (i >= sizeof(parent)) return -1;
    memcpy(parent, path, i + 1);
    for (;;) {
        while (i > 0 && parent[i - 1] != '/') i--;
        if (i == 0) break;
        parent[i - 1] = '\0';
        if (mkdir(parent, 0755) == 0 || errno == EEXIST) {
            if (i <= 1) break;
            i--;
            continue;
        }
        return -1;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST ? 0 : -1;
#endif
}

static void ensure_usage_dir_created(const char* dir) {
#ifdef _WIN32
    char parent[512];
    size_t dlen = strlen(dir);
    if (dlen >= sizeof(parent)) return;
    memcpy(parent, dir, dlen + 1);
    for (size_t i = dlen; i > 0; i--) {
        if (parent[i - 1] != '\\' && parent[i - 1] != '/') continue;
        parent[i - 1] = '\0';
        if (CreateDirectoryA(parent, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS)
            return;
    }
    CreateDirectoryA(dir, NULL);
#else
    ensure_dir(dir);
#endif
}

static int get_usage_file_path(char* path_out, size_t path_size, const char* key_id) {
    char dir[512];
    if (get_usage_dir(dir, sizeof(dir)) != 0) return -1;
    ensure_usage_dir_created(dir);
    if (snprintf(path_out, path_size, "%s%s%s", dir,
#ifdef _WIN32
                 "\\",
#else
                 "/",
#endif
                 key_id) >= (int)path_size)
        return -1;
    size_t n = strlen(path_out);
    if (n + sizeof(FILE_SUFFIX) > path_size) return -1;
    memcpy(path_out + n, FILE_SUFFIX, sizeof(FILE_SUFFIX));
    return 0;
}

#ifdef _WIN32
static int lock_file(HANDLE h) {
    OVERLAPPED ov = {0};
    if (LockFileEx(h, LOCKFILE_EXCLUSIVE_LOCK, 0, 0, 0x7FFFFFFF, &ov))
        return 0;
    return -1;
}
static void unlock_file(HANDLE h) {
    OVERLAPPED ov = {0};
    UnlockFileEx(h, 0, 0x7FFFFFFF, 0, &ov);
}
#else
static int lock_file(int fd) {
    return flock(fd, LOCK_EX) == 0 ? 0 : -1;
}
static void unlock_file(int fd) {
    flock(fd, LOCK_UN);
}
#endif

static int read_total_limit(
#ifdef _WIN32
    HANDLE h,
#else
    int fd,
#endif
    uint32_t* total, uint32_t* limit) {
    char buf[LINE_BUF];
    size_t n = 0;
    int have_total = 0, have_limit = 0;
    uint32_t t = 0, l = 0;
    for (;;) {
        int c;
#ifdef _WIN32
        DWORD read_len = 0;
        if (!ReadFile(h, &c, 1, &read_len, NULL) || read_len == 0)
            c = -1;
#else
        if (read(fd, &c, 1) != 1) c = -1;
#endif
        if (c == -1 || c == '\n' || n >= sizeof(buf) - 1) {
            buf[n] = '\0';
            if (n > 0) {
                unsigned long long val = 0;
                if (sscanf(buf, "%llu", &val) == 1 && val <= 0xFFFFFFFFULL) {
                    if (!have_total) { t = (uint32_t)val; have_total = 1; }
                    else if (!have_limit) { l = (uint32_t)val; have_limit = 1; break; }
                }
            }
            n = 0;
            if (c == -1) break;
            continue;
        }
        buf[n++] = (char)c;
    }
    if (have_total && have_limit) {
        *total = t;
        *limit = l;
        return 0;
    }
    return -1;
}

static int write_total_limit(
#ifdef _WIN32
    HANDLE h,
#else
    int fd,
#endif
    uint32_t total, uint32_t limit) {
    char buf[LINE_BUF];
    int len = snprintf(buf, sizeof(buf), "%u\n%u\n", (unsigned)total, (unsigned)limit);
    if (len <= 0 || (size_t)len >= sizeof(buf)) return -1;
#ifdef _WIN32
    SetFilePointer(h, 0, NULL, FILE_BEGIN);
    SetEndOfFile(h);
    {
        DWORD written = 0;
        if (!WriteFile(h, buf, (DWORD)len, &written, NULL) || (size_t)written != (size_t)len)
            return -1;
    }
#else
    if (ftruncate(fd, 0) != 0) return -1;
    if (lseek(fd, 0, SEEK_SET) != 0) return -1;
    if (write(fd, buf, (size_t)len) != (ssize_t)len) return -1;
#endif
    return 0;
}

int license_global_register(uint32_t node_count, uint32_t limit) {
    if (limit == 0) return 0;
    const char* key = getenv("SYNRIX_LICENSE_KEY");
    uint64_t h = key && key[0] ? str_hash(key) : str_hash("free");
    char key_hex[HASH_HEX_LEN + 1];
    snprintf(key_hex, sizeof(key_hex), "%016llx", (unsigned long long)h);

    char path[512];
    if (get_usage_file_path(path, sizeof(path), key_hex) != 0) return -1;

#ifdef _WIN32
    HANDLE fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) return -1;
    if (lock_file(fd) != 0) { CloseHandle(fd); return -1; }
    uint32_t total = 0, stored_limit = limit;
    if (read_total_limit(fd, &total, &stored_limit) != 0) {
        total = 0;
        stored_limit = limit;
    }
    total += node_count;
    if (write_total_limit(fd, total, stored_limit) != 0) { unlock_file(fd); CloseHandle(fd); return -1; }
    unlock_file(fd);
    CloseHandle(fd);
#else
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return -1;
    if (lock_file(fd) != 0) { close(fd); return -1; }
    uint32_t total = 0, stored_limit = limit;
    if (read_total_limit(fd, &total, &stored_limit) != 0) {
        total = 0;
        stored_limit = limit;
    }
    total += node_count;
    if (write_total_limit(fd, total, stored_limit) != 0) { unlock_file(fd); close(fd); return -1; }
    unlock_file(fd);
    close(fd);
#endif
    return 0;
}

int license_global_add_one(uint32_t limit) {
    if (limit == 0) return 0;
    const char* key = getenv("SYNRIX_LICENSE_KEY");
    uint64_t h = key && key[0] ? str_hash(key) : str_hash("free");
    char key_hex[HASH_HEX_LEN + 1];
    snprintf(key_hex, sizeof(key_hex), "%016llx", (unsigned long long)h);

    char path[512];
    if (get_usage_file_path(path, sizeof(path), key_hex) != 0) return -1;

#ifdef _WIN32
    HANDLE fd = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fd == INVALID_HANDLE_VALUE) return -1;
    if (lock_file(fd) != 0) { CloseHandle(fd); return -1; }
    uint32_t total = 0, stored_limit = limit;
    if (read_total_limit(fd, &total, &stored_limit) != 0) {
        total = 0;
        stored_limit = limit;
    }
    if (total >= stored_limit) {
        unlock_file(fd);
        CloseHandle(fd);
        return -1;
    }
    total++;
    if (write_total_limit(fd, total, stored_limit) != 0) { unlock_file(fd); CloseHandle(fd); return -1; }
    unlock_file(fd);
    CloseHandle(fd);
#else
    int fd = open(path, O_RDWR | O_CREAT, 0644);
    if (fd < 0) return -1;
    if (lock_file(fd) != 0) { close(fd); return -1; }
    uint32_t total = 0, stored_limit = limit;
    if (read_total_limit(fd, &total, &stored_limit) != 0) {
        total = 0;
        stored_limit = limit;
    }
    if (total >= stored_limit) {
        unlock_file(fd);
        close(fd);
        return -1;
    }
    total++;
    if (write_total_limit(fd, total, stored_limit) != 0) { unlock_file(fd); close(fd); return -1; }
    unlock_file(fd);
    close(fd);
#endif
    return 0;
}
