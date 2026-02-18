/*
 * Signed license verification (Ed25519).
 * Key format: base64(payload || signature).
 * Legacy: payload = version(1) tier(1) expiry(4) = 6 bytes, raw = 70 bytes.
 * Unique: payload = version(1) tier(1) expiry(4) nonce(8) = 14 bytes, raw = 78 bytes.
 * Tier: 0=25k(starter), 1=1m(indie), 2=10m(growth), 3=50m(business), 4=unlimited(scale).
 * Requires OpenSSL (SYNRIX_LICENSE_USE_OPENSSL) for verification; otherwise all keys are rejected.
 */

#include "license_verify.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PAYLOAD_LEN_LEGACY 6
#define PAYLOAD_LEN_UNIQUE 14
#define SIG_LEN 64
#define RAW_LEN_LEGACY (PAYLOAD_LEN_LEGACY + SIG_LEN)
#define RAW_LEN_UNIQUE (PAYLOAD_LEN_UNIQUE + SIG_LEN)
#define RAW_LEN_MAX RAW_LEN_UNIQUE

/* Tier limits: 0=25k(starter), 1=1m, 2=10m, 3=50m, 4=unlimited(0) */
static const uint32_t TIER_LIMITS[5] = {
    25000u,      /* tier 0 = starter */
    1000000u,    /* tier 1 = indie */
    10000000u,   /* tier 2 = growth */
    50000000u,   /* tier 3 = business */
    0u           /* tier 4 = unlimited */
};

/* Ed25519 public key (32 bytes) – must match private key used by backend to sign keys */
static const unsigned char SYNRIX_LICENSE_PUBLIC_KEY[32] = {
    0x77, 0x44, 0x87, 0x22, 0x3f, 0xb3, 0x52, 0xd9, 0xf3, 0x30, 0x18, 0xce, 0x6d, 0xba, 0x5b, 0x14, 0x01, 0xdb, 0x28, 0x4d, 0x27, 0xd3, 0xa9, 0xd4, 0x56, 0x0b, 0x3c, 0xe8, 0x1d, 0x91, 0x82, 0x7a
};

#ifdef SYNRIX_LICENSE_USE_OPENSSL
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>

/* Decode base64 into buf; strip leading/trailing whitespace from input. Returns decoded length or -1. */
static int b64_decode(const char* in, unsigned char* buf, size_t buf_size) {
    const char* p = in;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    size_t len = 0;
    while (p[len] && p[len] != ' ' && p[len] != '\t' && p[len] != '\r' && p[len] != '\n') len++;
    if (len == 0) return -1;
    BIO* b64 = BIO_new(BIO_f_base64());
    if (!b64) return -1;
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO* bio = BIO_new_mem_buf(p, (int)len);
    if (!bio) { BIO_free(b64); return -1; }
    bio = BIO_push(b64, bio);
    int n = BIO_read(bio, buf, (int)buf_size);
    BIO_free_all(bio);
    if (n != RAW_LEN_LEGACY && n != RAW_LEN_UNIQUE) return -1;
    return n;
}

static int ed25519_verify(const unsigned char* msg, size_t msg_len, const unsigned char* sig, size_t sig_len) {
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, SYNRIX_LICENSE_PUBLIC_KEY, 32);
    if (!pkey) return -1;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return -1; }
    int ok = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) == 1
             && EVP_DigestVerify(ctx, sig, sig_len, msg, msg_len) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok ? 0 : -1;
}
#else
static int b64_decode(const char* in, unsigned char* buf, size_t buf_size) {
    (void)in;
    (void)buf;
    (void)buf_size;
    return -1; /* no OpenSSL: reject */
}
static int ed25519_verify(const unsigned char* msg, size_t msg_len, const unsigned char* sig, size_t sig_len) {
    (void)msg;
    (void)msg_len;
    (void)sig;
    (void)sig_len;
    return -1;
}
#endif

int synrix_license_parse(const char* key, uint32_t* out_limit, int* out_unlimited) {
    if (!out_limit || !out_unlimited) return -1;
    const char* k = key;
    if (!k) k = getenv("SYNRIX_LICENSE_KEY");
    if (!k || !*k) return -1;

#ifdef SYNRIX_LICENSE_USE_OPENSSL
    unsigned char raw[RAW_LEN_MAX];
    int raw_len = b64_decode(k, raw, sizeof(raw));
    if (raw_len != RAW_LEN_LEGACY && raw_len != RAW_LEN_UNIQUE) return -1;
    size_t payload_len = (size_t)(raw_len - SIG_LEN);
    if (ed25519_verify(raw, payload_len, raw + payload_len, SIG_LEN) != 0) return -1;

    /* payload: version(1) tier(1) expiry(4) little-endian [nonce(8) if unique] */
    unsigned char version = raw[0];
    unsigned char tier = raw[1];
    uint32_t expiry = (uint32_t)raw[2] | ((uint32_t)raw[3] << 8) | ((uint32_t)raw[4] << 16) | ((uint32_t)raw[5] << 24);
    if (version != 1) return -1;
    if (tier > 4) return -1;
    if (expiry != 0) {
        time_t now = time(NULL);
        if (now == (time_t)-1 || (uint32_t)now >= expiry) return -1;
    }
    *out_limit = TIER_LIMITS[tier];
    *out_unlimited = (tier == 4) ? 1 : 0;
    return 0;
#else
    (void)out_limit;
    (void)out_unlimited;
    return -1; /* OpenSSL not available: reject all keys */
#endif
}
