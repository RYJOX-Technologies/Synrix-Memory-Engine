#define _GNU_SOURCE
#include "license_verify.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef SYNRIX_LICENSE_USE_OPENSSL
#include <openssl/evp.h>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
#include <openssl/core_names.h>
#endif
#endif

/* Public key (32 bytes Ed25519). Replace with output from:
 *   python tools/synrix_license_keygen.py --generate --private synrix_license_private.key
 */
static const unsigned char SYNRIX_LICENSE_PUBLIC_KEY[32] = {
    0xc1, 0xbc, 0xc1, 0x11, 0xac, 0xf2, 0xac, 0xfd, 0xd7, 0xca, 0xc6, 0xdc, 0xd6, 0x62, 0xb1, 0x0d, 0x96, 0xff, 0x06, 0x01, 0x64, 0x5f, 0x17, 0x2d, 0x77, 0x88, 0xd0, 0xab, 0xec, 0xdd, 0x02, 0x4b
};

#define PAYLOAD_LEN 6
#define SIG_LEN 64
#define RAW_LEN (PAYLOAD_LEN + SIG_LEN)

static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int b64decode(const char* in, size_t inlen, unsigned char* out, size_t* outlen) {
    size_t n = 0;
    unsigned int buf = 0;
    int bits = 0;
    for (size_t i = 0; i < inlen; i++) {
        char c = in[i];
        if (c == ' ' || c == '\r' || c == '\n') continue;
        const char* p = strchr(B64, c);
        if (!p) return -1;
        buf = (buf << 6) | (unsigned int)(p - B64);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            if (n < *outlen) out[n++] = (unsigned char)((buf >> bits) & 0xff);
        }
    }
    *outlen = n;
    return 0;
}

#ifdef SYNRIX_LICENSE_USE_OPENSSL
static int ed25519_verify(const unsigned char* sig, const unsigned char* msg, size_t msg_len,
                          const unsigned char* pub) {
#if OPENSSL_VERSION_NUMBER < 0x10101000L
    return -1; /* Ed25519 requires OpenSSL 1.1.1 */
#else
    EVP_PKEY* pkey = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, pub, 32);
    if (!pkey) return -1;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); return -1; }
    int ok = EVP_DigestVerifyInit(ctx, NULL, NULL, NULL, pkey) == 1
             && EVP_DigestVerify(ctx, sig, SIG_LEN, msg, msg_len) == 1;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return ok ? 0 : -1;
#endif
}
#else
static int ed25519_verify(const unsigned char* sig, const unsigned char* msg, size_t msg_len,
                          const unsigned char* pub) {
    (void)sig;
    (void)msg;
    (void)msg_len;
    (void)pub;
    return -1; /* Build without OpenSSL: license verification disabled */
}
#endif

/* Tier limits (node count): 0=100k, 1=1m, 2=10m, 3=50m, 4=unlimited */
static const uint32_t TIER_LIMITS[] = { 100000u, 1000000u, 10000000u, 50000000u, 0u };

int synrix_license_parse(const char* key, uint32_t* out_limit, int* out_unlimited) {
    if (!out_limit || !out_unlimited) return -1;
    *out_limit = 0;
    *out_unlimited = 0;

    const char* k = key;
    if (!k) k = getenv("SYNRIX_LICENSE_KEY");
    if (!k || !k[0]) return -1;

    size_t inlen = strlen(k);
    unsigned char raw[RAW_LEN];
    size_t raw_len = sizeof(raw);
    if (b64decode(k, inlen, raw, &raw_len) != 0 || raw_len != RAW_LEN) return -1;

    unsigned char* payload = raw;
    unsigned char* sig = raw + PAYLOAD_LEN;

    if (ed25519_verify(sig, payload, PAYLOAD_LEN, SYNRIX_LICENSE_PUBLIC_KEY) != 0) return -1;

    unsigned int version = payload[0];
    unsigned int tier = payload[1];
    uint32_t expiry;
    memcpy(&expiry, payload + 2, 4);
    if (version != 1 || tier > 4) return -1;
    if (expiry != 0 && (time_t)expiry < time(NULL)) return -1; /* expired */

    if (tier == 4) {
        *out_unlimited = 1;
        *out_limit = 0;
    } else {
        *out_unlimited = 0;
        *out_limit = TIER_LIMITS[tier];
    }
    return 0;
}
