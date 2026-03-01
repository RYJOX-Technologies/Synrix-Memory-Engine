#define _GNU_SOURCE
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <shlobj.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif
#endif

// Linux usage path: $HOME/.synrix/license_usage/<hash>.dat (per spec)
#define SYNRIX_USAGE_DIR ".synrix/license_usage"
#define USAGE_FILE_SUFFIX ".dat"

// Get hardware ID (stable, unique identifier for license tracking)
int lattice_get_hardware_id(char* hwid_out, size_t hwid_size) {
    if (!hwid_out || hwid_size < 65) {
        return -1;
    }

    char combined[256] = {0};

#ifdef _WIN32
    /* Windows: use MachineGuid from registry */
    char guid[128] = {0};
    HKEY hKey;
    LONG rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    if (rc == ERROR_SUCCESS) {
        DWORD type = 0, size = (DWORD)sizeof(guid);
        RegQueryValueExA(hKey, "MachineGuid", NULL, &type, (LPBYTE)guid, &size);
        RegCloseKey(hKey);
    }
    snprintf(combined, sizeof(combined), "win|%s", guid);
#else
    /* Linux: MAC + CPU + machine-id */
    char mac_addr[32] = {0};
    char cpu_info[64] = {0};
    char machine_id[64] = {0};

    FILE* f = fopen("/sys/class/net/eth0/address", "r");
    if (!f) f = fopen("/sys/class/net/wlan0/address", "r");
    if (f) {
        if (fgets(mac_addr, sizeof(mac_addr), f)) {
            size_t len = strlen(mac_addr);
            if (len > 0 && mac_addr[len-1] == '\n') mac_addr[len-1] = '\0';
        }
        fclose(f);
    }

    f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "Serial", 6) == 0 || strncmp(line, "processor", 9) == 0) {
                char* colon = strchr(line, ':');
                if (colon) {
                    strncpy(cpu_info, colon + 1, sizeof(cpu_info) - 1);
                    size_t len = strlen(cpu_info);
                    while (len > 0 && (cpu_info[len-1] == ' ' || cpu_info[len-1] == '\n' || cpu_info[len-1] == '\t'))
                        cpu_info[--len] = '\0';
                    if (strlen(cpu_info) > 0) break;
                }
            }
        }
        fclose(f);
    }

    f = fopen("/etc/machine-id", "r");
    if (!f) f = fopen("/var/lib/dbus/machine-id", "r");
    if (f) {
        if (fgets(machine_id, sizeof(machine_id), f)) {
            size_t len = strlen(machine_id);
            if (len > 0 && machine_id[len-1] == '\n') machine_id[len-1] = '\0';
        }
        fclose(f);
    }

    snprintf(combined, sizeof(combined), "%s|%s|%s", mac_addr, cpu_info, machine_id);
#endif

    uint64_t hash = 14695981039346656037ULL;
    for (size_t i = 0; i < strlen(combined); i++) {
        hash ^= (uint64_t)combined[i];
        hash *= 1099511628211ULL;
    }

    snprintf(hwid_out, hwid_size, "%016llx", (unsigned long long)hash);
    return 0;
}

// Get last error code
lattice_error_code_t lattice_get_last_error(persistent_lattice_t* lattice) {
    if (!lattice) {
        return LATTICE_ERROR_NULL_POINTER;
    }
    return lattice->last_error;
}

// Sanitize license_id for use in path (alphanumeric and dash only)
static void sanitize_license_id(const char* license_id, char* out, size_t out_size) {
    if (!out || out_size == 0) return;
    out[0] = '\0';
    if (!license_id) return;
    size_t j = 0;
    for (size_t i = 0; license_id[i] != '\0' && j < out_size - 1; i++) {
        char c = license_id[i];
        if (isalnum((unsigned char)c) || c == '-' || c == '_') {
            out[j++] = c;
        }
    }
    out[j] = '\0';
    if (j == 0) {
        (void)strncpy(out, "free", out_size - 1);
        out[out_size - 1] = '\0';
    }
}

static int get_usage_path(const char* license_id, char* path_out, size_t path_size) {
    const char* home = NULL;
#ifdef _WIN32
    home = getenv("USERPROFILE");
    if (!home || home[0] == '\0') home = getenv("APPDATA");
#else
    home = getenv("HOME");
#endif
    if (!home || home[0] == '\0') return -1;
    char safe_id[128];
    sanitize_license_id(license_id, safe_id, sizeof(safe_id));
#ifdef _WIN32
    int n = snprintf(path_out, path_size, "%s\\" SYNRIX_USAGE_DIR "\\%s" USAGE_FILE_SUFFIX, home, safe_id);
#else
    int n = snprintf(path_out, path_size, "%s/" SYNRIX_USAGE_DIR "/%s" USAGE_FILE_SUFFIX, home, safe_id);
#endif
    if (n < 0 || (size_t)n >= path_size) return -1;
    return 0;
}

static int ensure_usage_dir(const char* path) {
    char dir[512];
    const char* last_slash = strrchr(path, '/');
#ifdef _WIN32
    const char* last_bslash = strrchr(path, '\\');
    if (last_bslash && (!last_slash || last_bslash > last_slash))
        last_slash = last_bslash;
#endif
    if (!last_slash || (size_t)(last_slash - path) >= sizeof(dir)) return -1;
    size_t dlen = (size_t)(last_slash - path);
    memcpy(dir, path, dlen);
    dir[dlen] = '\0';
    for (size_t i = 1; i < dlen; i++) {
        if (dir[i] == '/' || dir[i] == '\\') {
            char saved = dir[i];
            dir[i] = '\0';
            if (mkdir(dir, 0700) != 0 && errno != EEXIST) return -1;
            dir[i] = saved;
        }
    }
    if (mkdir(dir, 0700) != 0 && errno != EEXIST) return -1;
    return 0;
}

// On load: ensure usage file exists (create with 0 if not)
int license_global_register(const char* license_id) {
    char path[512];
    if (get_usage_path(license_id, path, sizeof(path)) != 0) return -1;
    if (ensure_usage_dir(path) != 0) return -1;
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 0;
    }
    f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "0\n");
    fclose(f);
    return 0;
}

// Before every add: read count, if count >= cap return -1; else increment and write back
int license_global_add_one(const char* license_id, uint32_t cap) {
    char path[512];
    if (get_usage_path(license_id, path, sizeof(path)) != 0) return -1;
    if (license_global_register(license_id) != 0) return -1;
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    unsigned long count = 0;
    if (fscanf(f, "%lu", &count) != 1) count = 0;
    fclose(f);
    if (cap > 0 && count >= (unsigned long)cap) return -1;  /* cap 0 = unlimited */
    f = fopen(path, "w");
    if (!f) return -1;
    int ok = (fprintf(f, "%lu\n", count + 1) >= 0);
    fclose(f);
    return ok ? 0 : -1;
}
