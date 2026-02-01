#define _GNU_SOURCE
#include "persistent_lattice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif

// Get hardware ID (stable, unique identifier for license tracking)
// Uses MAC address, CPU info, and disk serial to create a stable HWID
int lattice_get_hardware_id(char* hwid_out, size_t hwid_size) {
    if (!hwid_out || hwid_size < 65) {
        return -1;  // Need at least 65 bytes (64 hex chars + null terminator)
    }
    
    char mac_addr[32] = {0};
    char cpu_info[64] = {0};
    char machine_id[64] = {0};
    
    // Try to get MAC address from /sys/class/net/eth0/address (Linux)
    FILE* f = fopen("/sys/class/net/eth0/address", "r");
    if (f) {
        if (fgets(mac_addr, sizeof(mac_addr), f)) {
            // Remove newline
            size_t len = strlen(mac_addr);
            if (len > 0 && mac_addr[len-1] == '\n') {
                mac_addr[len-1] = '\0';
            }
        }
        fclose(f);
    }
    
    // If no eth0, try wlan0
    if (strlen(mac_addr) == 0) {
        f = fopen("/sys/class/net/wlan0/address", "r");
        if (f) {
            if (fgets(mac_addr, sizeof(mac_addr), f)) {
                size_t len = strlen(mac_addr);
                if (len > 0 && mac_addr[len-1] == '\n') {
                    mac_addr[len-1] = '\0';
                }
            }
            fclose(f);
        }
    }
    
    // Get CPU info from /proc/cpuinfo (Linux)
    f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "Serial", 6) == 0 || strncmp(line, "processor", 9) == 0) {
                // Extract serial or processor ID
                char* colon = strchr(line, ':');
                if (colon) {
                    strncpy(cpu_info, colon + 1, sizeof(cpu_info) - 1);
                    // Clean whitespace
                    size_t len = strlen(cpu_info);
                    while (len > 0 && (cpu_info[len-1] == ' ' || cpu_info[len-1] == '\n' || cpu_info[len-1] == '\t')) {
                        cpu_info[--len] = '\0';
                    }
                    if (strlen(cpu_info) > 0) break;
                }
            }
        }
        fclose(f);
    }
    
    // Get machine ID from /etc/machine-id (Linux)
    f = fopen("/etc/machine-id", "r");
    if (f) {
        if (fgets(machine_id, sizeof(machine_id), f)) {
            size_t len = strlen(machine_id);
            if (len > 0 && machine_id[len-1] == '\n') {
                machine_id[len-1] = '\0';
            }
        }
        fclose(f);
    }
    
    // If no machine-id, try /var/lib/dbus/machine-id
    if (strlen(machine_id) == 0) {
        f = fopen("/var/lib/dbus/machine-id", "r");
        if (f) {
            if (fgets(machine_id, sizeof(machine_id), f)) {
                size_t len = strlen(machine_id);
                if (len > 0 && machine_id[len-1] == '\n') {
                    machine_id[len-1] = '\0';
                }
            }
            fclose(f);
        }
    }
    
    // Combine all identifiers into a hash-like string
    // Simple hash: combine all strings and create hex representation
    char combined[256];
    snprintf(combined, sizeof(combined), "%s|%s|%s", mac_addr, cpu_info, machine_id);
    
    // Create a simple hash (Fowler-Noll-Vo hash variant)
    uint64_t hash = 14695981039346656037ULL;  // FNV-1a offset basis
    for (size_t i = 0; i < strlen(combined); i++) {
        hash ^= (uint64_t)combined[i];
        hash *= 1099511628211ULL;  // FNV-1a prime
    }
    
    // Convert to hex string (64-bit = 16 hex chars, but we'll use 32 for safety)
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
