/*
 * Linux Synrix license: key resolution, JSON/v1 payload verify, HWID (machine-id SHA256).
 * Requires OpenSSL (EVP SHA256, Ed25519) and libdl for dladdr.
 */
#define _GNU_SOURCE
#include "synrix_license.h"
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>

#ifdef __linux__
#include <dlfcn.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#endif

#define PAYLOAD_SIZE  112
#define SIGNATURE_SIZE 64
#define BLOB_SIZE     (PAYLOAD_SIZE + SIGNATURE_SIZE)  /* 176 */
#define MAGIC         "SYNRIXLI"
#define LICENSE_JSON_PATH ".synrix/license.json"
#define LICENSE_KEY_FILE  "license_key"
#define MAX_KEY_B64   512
#define MAX_RAW_KEY   (MAX_KEY_B64 * 2)

/* Tier byte → node limit (spec §6) */
static const uint32_t tier_limits[] = { 25000u, 1000000u, 10000000u, 50000000u, 0u };
static const char* tier_names[] = { "25k", "1m", "10m", "50m", "unlimited" };

/* CRC32C (Castagnoli) for bytes 0..107 verification */
static uint32_t crc32c_table[256];
static int crc32c_table_initialized;

static void crc32c_init(void) {
    const uint32_t poly = 0x82F63B78u;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c >> 1) ^ (poly & ~((c & 1) - 1));
        crc32c_table[i] = c;
    }
    crc32c_table_initialized = 1;
}

static uint32_t crc32c(const uint8_t* data, size_t len) {
    if (!crc32c_table_initialized) crc32c_init();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++)
        crc = crc32c_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

/* Base64 decode; writes at most out_size bytes; returns decoded length or 0 on error */
static size_t base64_decode(const char* in, uint8_t* out, size_t out_size) {
    static const int8_t T[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,
        26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,-1,-1,
    };
    size_t n = 0;
    int buf = 0, bits = 0;
    for (; *in && n < out_size; in++) {
        int v = T[(unsigned char)*in];
        if (v < 0) continue;
        buf = (buf << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out[n++] = (uint8_t)((buf >> bits) & 0xFF);
        }
    }
    return n;
}

/* Read machine-id (same source as spec §4), strip newline; return length or 0 */
static size_t read_machine_id(char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) return 0;
    buf[0] = '\0';
    FILE* f = fopen("/etc/machine-id", "r");
    if (!f) f = fopen("/var/lib/dbus/machine-id", "r");
    if (!f) return 0;
    if (!fgets(buf, (int)buf_size, f)) { fclose(f); return 0; }
    fclose(f);
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
    return len;
}

/* Compute SHA256(machine_id_string) into hash_out_32. Returns 0 on success. */
static int get_local_hwid_hash(uint8_t* hash_out_32) {
#ifdef __linux__
    char machine_id[256];
    size_t len = read_machine_id(machine_id, sizeof(machine_id));
    if (len == 0) return -1;
    if (!hash_out_32) return -1;
    unsigned int digest_len = SHA256_DIGEST_LENGTH;
    return EVP_Digest(machine_id, len, hash_out_32, &digest_len, EVP_sha256(), NULL) ? 0 : -1;
#else
    (void)hash_out_32;
    return -1;
#endif
}

/* Get home directory: getenv("HOME") or getpwuid(getuid())->pw_dir or /tmp */
static int get_license_json_path(char* path_out, size_t path_size) {
    const char* home = getenv("HOME");
    if (home && home[0] != '\0') {
        int n = snprintf(path_out, path_size, "%s/" LICENSE_JSON_PATH, home);
        return (n > 0 && (size_t)n < path_size) ? 0 : -1;
    }
    struct passwd* pw = getpwuid(getuid());
    if (pw && pw->pw_dir && pw->pw_dir[0] != '\0') {
        int n = snprintf(path_out, path_size, "%s/" LICENSE_JSON_PATH, pw->pw_dir);
        return (n > 0 && (size_t)n < path_size) ? 0 : -1;
    }
    int n = snprintf(path_out, path_size, "/tmp/" LICENSE_JSON_PATH);
    return (n > 0 && (size_t)n < path_size) ? 0 : -1;
}

/* Read file; find "license_b64" and quoted value; copy value into b64_out (trimmed). Return 0 on success. */
static int read_license_b64_from_json(const char* path, char* b64_out, size_t b64_size) {
    char buf[2048];
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    const char* key = "\"license_b64\"";
    char* p = strstr(buf, key);
    if (!p) return -1;
    p += strlen(key);
    while (*p && (*p == ' ' || *p == '\t' || *p == ':')) p++;
    if (*p != '"') return -1;
    p++;
    char* start = p;
    while (*p && *p != '"') p++;
    if (*p != '"') return -1;
    size_t len = (size_t)(p - start);
    if (len >= b64_size) return -1;
    memcpy(b64_out, start, len);
    b64_out[len] = '\0';
    /* trim trailing whitespace */
    while (len > 0 && (b64_out[len - 1] == ' ' || b64_out[len - 1] == '\n' || b64_out[len - 1] == '\r')) b64_out[--len] = '\0';
    return 0;
}

/* Resolve path of loaded .so via dladdr; form dirname(lib_path)/license_key */
static int get_license_key_path_next_to_binary(char* path_out, size_t path_size) {
#ifdef __linux__
    Dl_info info;
    if (!dladdr((void*)get_license_key_path_next_to_binary, &info) || !info.dli_fname[0])
        return -1;
    const char* dir = strrchr(info.dli_fname, '/');
    if (!dir) return -1;
    size_t dlen = (size_t)(dir - info.dli_fname + 1);
    if (dlen + strlen(LICENSE_KEY_FILE) + 1 > path_size) return -1;
    memcpy(path_out, info.dli_fname, dlen);
    path_out[dlen] = '\0';
    strcat(path_out, LICENSE_KEY_FILE);
    return 0;
#else
    (void)path_out;
    (void)path_size;
    return -1;
#endif
}

/* Read one line from path into key_out (trimmed). Return 0 on success. */
static int read_one_line_file(const char* path, char* key_out, size_t key_size) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(key_out, (int)key_size, f)) { fclose(f); return -1; }
    fclose(f);
    size_t len = strlen(key_out);
    while (len > 0 && (key_out[len - 1] == '\n' || key_out[len - 1] == '\r' || key_out[len - 1] == ' ' || key_out[len - 1] == '\t')) key_out[--len] = '\0';
    return 0;
}

/* Resolve key and optionally the source path. path_out may be NULL. */
static int resolve_key_ex(const char* key_override, char* key_out, size_t key_size, char* path_out, size_t path_size) {
    if (key_override && key_override[0] != '\0') {
        strncpy(key_out, key_override, key_size - 1);
        key_out[key_size - 1] = '\0';
        if (path_out && path_size) { strncpy(path_out, "(argument)", path_size - 1); path_out[path_size - 1] = '\0'; }
        return 0;
    }
    const char* env = getenv("SYNRIX_LICENSE_KEY");
    if (env && env[0] != '\0') {
        strncpy(key_out, env, key_size - 1);
        key_out[key_size - 1] = '\0';
        if (path_out && path_size) { strncpy(path_out, "(environment SYNRIX_LICENSE_KEY)", path_size - 1); path_out[path_size - 1] = '\0'; }
        return 0;
    }
    char json_path[512];
    if (get_license_json_path(json_path, sizeof(json_path)) == 0) {
        char b64[MAX_KEY_B64];
        if (read_license_b64_from_json(json_path, b64, sizeof(b64)) == 0) {
            strncpy(key_out, b64, key_size - 1);
            key_out[key_size - 1] = '\0';
            if (path_out && path_size) { strncpy(path_out, json_path, path_size - 1); path_out[path_size - 1] = '\0'; }
            return 0;
        }
    }
    char key_path[1024];
    if (get_license_key_path_next_to_binary(key_path, sizeof(key_path)) == 0 &&
        read_one_line_file(key_path, key_out, key_size) == 0 && key_out[0] != '\0') {
        if (path_out && path_size) { strncpy(path_out, key_path, path_size - 1); path_out[path_size - 1] = '\0'; }
        return 0;
    }
    return -1;
}

static int resolve_key(const char* key_override, char* key_out, size_t key_size) {
    return resolve_key_ex(key_override, key_out, key_size, NULL, 0);
}

/* If key looks like JSON (starts with {), extract license_b64 value into key_out. */
static void extract_license_b64_if_json(char* key, size_t key_size) {
    while (*key == ' ' || *key == '\t') key++;
    if (*key != '{') return;
    const char* needle = "\"license_b64\"";
    char* p = strstr(key, needle);
    if (!p) return;
    p += strlen(needle);
    while (*p && (*p == ' ' || *p == '\t' || *p == ':')) p++;
    if (*p != '"') return;
    p++;
    char* start = p;
    while (*p && *p != '"') p++;
    if (*p != '"') return;
    size_t len = (size_t)(p - start);
    if (len >= key_size) return;
    memmove(key, start, len);
    key[len] = '\0';
}

/* Default Ed25519 public key (32 bytes) - same as Windows; override via SYNRIX_LICENSE_PUBKEY hex. */
static const uint8_t default_pubkey[32] = {
    0x26, 0x11, 0x64, 0x7a, 0xea, 0xea, 0x5a, 0xf3, 0xcb, 0x8e, 0x65, 0x2d, 0xa7, 0xc5, 0xcf, 0x37,
    0x59, 0x25, 0xeb, 0x55, 0xf8, 0xf3, 0xbb, 0x13, 0x3b, 0x16, 0x9e, 0x86, 0x85, 0x69, 0xa9, 0x63
};

static int ed25519_verify_len(const uint8_t* payload, size_t payload_len, const uint8_t* signature_64, const uint8_t* pubkey_32) {
#ifdef __linux__
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, pubkey_32, 32);
    if (!pkey) return -1;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return -1; }
    int ok = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) == 1 &&
             EVP_DigestVerify(ctx, signature_64, 64, payload, payload_len) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok ? 0 : -1;
#else
    (void)payload;
    (void)payload_len;
    (void)signature_64;
    (void)pubkey_32;
    return -1;
#endif
}

static int ed25519_verify(const uint8_t* payload_112, const uint8_t* signature_64, const uint8_t* pubkey_32) {
    return ed25519_verify_len(payload_112, 112, signature_64, pubkey_32);
}

/* Load pubkey from env SYNRIX_LICENSE_PUBKEY (64 hex chars) into pubkey_32. Return 0 on success; else use default. */
static void get_ed25519_pubkey(uint8_t* pubkey_32) {
    const char* hex = getenv("SYNRIX_LICENSE_PUBKEY");
    if (hex && strlen(hex) >= 64) {
        for (int i = 0; i < 32; i++) {
            unsigned a = 0, b = 0;
            char c = hex[2 * i], d = hex[2 * i + 1];
            if (c >= '0' && c <= '9') a = c - '0'; else if (c >= 'a' && c <= 'f') a = c - 'a' + 10; else if (c >= 'A' && c <= 'F') a = c - 'A' + 10;
            if (d >= '0' && d <= '9') b = d - '0'; else if (d >= 'a' && d <= 'f') b = d - 'a' + 10; else if (d >= 'A' && d <= 'F') b = d - 'A' + 10;
            pubkey_32[i] = (uint8_t)((a << 4) | b);
        }
        return;
    }
    memcpy(pubkey_32, default_pubkey, 32);
}

/* Legacy: 70 (6+64), 78 (14+64), 86 (22+64). Payload: ver(1), tier(1), expiry_u32(4), [nonce(8)], [machine_binding(8)]. */
static int verify_legacy(const uint8_t* blob, size_t decoded, lattice_license_claims_t* claims) {
#ifdef __linux__
    if (decoded != 70 && decoded != 78 && decoded != 86) return -1;
    size_t payload_len = decoded - 64;
    const uint8_t* payload = blob;
    const uint8_t* signature = blob + payload_len;
    if (payload[0] != 1) return -1;
    uint8_t tier_byte = payload[1];
    if (tier_byte > 4) return -1;
    uint32_t expires_u32;
    memcpy(&expires_u32, payload + 2, 4);
    uint64_t expires_at = (uint64_t)expires_u32;
    if (expires_at != 0 && (uint64_t)time(NULL) >= expires_at) return -1;
    if (payload_len >= 22) {
        uint8_t local_hwid[32];
        if (get_local_hwid_hash(local_hwid) != 0) return -1;
        if (memcmp(local_hwid, payload + 14, 8) != 0) return -1;
    }
    uint8_t pubkey[32];
    get_ed25519_pubkey(pubkey);
    if (ed25519_verify_len(payload, payload_len, signature, pubkey) != 0) return -1;
    claims->node_limit = tier_limits[tier_byte];
    claims->exp = expires_at;
    strncpy(claims->tier, tier_names[tier_byte], sizeof(claims->tier) - 1);
    claims->tier[sizeof(claims->tier) - 1] = '\0';
    return 0;
#else
    (void)blob;
    (void)decoded;
    (void)claims;
    return -1;
#endif
}

/* Parse and verify key_buf (b64 or JSON with license_b64), fill claims. Key buffer may be modified. */
static int parse_from_key(char* key_buf, size_t key_size, lattice_license_claims_t* claims) {
    extract_license_b64_if_json(key_buf, key_size);
    uint8_t blob[BLOB_SIZE + 8];
    size_t decoded = base64_decode(key_buf, blob, sizeof(blob));
    if (decoded == 177) decoded = 176;
    if (decoded == 70 || decoded == 78 || decoded == 86)
        return verify_legacy(blob, decoded, claims);
    if (decoded != BLOB_SIZE) return -1;
    const uint8_t* payload = blob;
    const uint8_t* signature = blob + PAYLOAD_SIZE;
    if (memcmp(payload, MAGIC, 8) != 0) return -1;
    if (payload[8] != 1) return -1;
    uint8_t tier_byte = payload[9];
    if (tier_byte > 4) return -1;
    uint32_t crc_stored;
    memcpy(&crc_stored, payload + 108, 4);
    if (crc32c(payload, 108) != crc_stored) return -1;
    uint8_t pubkey[32];
    get_ed25519_pubkey(pubkey);
    if (ed25519_verify(payload, signature, pubkey) != 0) return -1;
    uint64_t expires_at;
    memcpy(&expires_at, payload + 20, 8);
    if (expires_at != 0 && (uint64_t)time(NULL) >= expires_at) return -1;
    uint8_t local_hwid[32];
    if (get_local_hwid_hash(local_hwid) != 0) return -1;
    if (memcmp(local_hwid, payload + 44, 32) != 0) return -1;
    claims->node_limit = tier_limits[tier_byte];
    claims->exp = expires_at;
    strncpy(claims->tier, tier_names[tier_byte], sizeof(claims->tier) - 1);
    claims->tier[sizeof(claims->tier) - 1] = '\0';
    return 0;
}

int synrix_license_parse(const char* key_override, lattice_license_claims_t* claims) {
    if (!claims) return -1;
    memset(claims, 0, sizeof(*claims));
    char key_buf[MAX_RAW_KEY];
    if (resolve_key(key_override, key_buf, sizeof(key_buf)) != 0) return -1;
    return parse_from_key(key_buf, sizeof(key_buf), claims);
}

int synrix_license_status(char* path_used, size_t path_size, lattice_license_claims_t* claims) {
    if (!claims) return -1;
    memset(claims, 0, sizeof(*claims));
    char key_buf[MAX_RAW_KEY];
    if (resolve_key_ex(NULL, key_buf, sizeof(key_buf), path_used, path_size) != 0) return -1;
    return parse_from_key(key_buf, sizeof(key_buf), claims);
}
