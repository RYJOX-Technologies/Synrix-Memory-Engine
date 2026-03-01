/*
 * Synrix license parsing (Linux): key resolution, JSON/v1 payload verify, HWID binding.
 * Compatible with Windows key flow: same payload format, tier limits, Ed25519.
 */
#ifndef SYNRIX_LICENSE_H
#define SYNRIX_LICENSE_H

#include "persistent_lattice.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve key (arg → SYNRIX_LICENSE_KEY → ~/.synrix/license.json → license_key next to .so),
 * decode, verify (magic, CRC32C, Ed25519, expiry, HWID), fill claims.
 * key_override: if non-NULL, use this key (base64 or JSON with "license_b64"); else use source order.
 * Returns 0 on success, -1 on invalid/missing key. */
int synrix_license_parse(const char* key_override, lattice_license_claims_t* claims);

/* Same as parse but report source path into path_used (path_size bytes). Use for "license status" CLI. */
int synrix_license_status(char* path_used, size_t path_size, lattice_license_claims_t* claims);

#ifdef __cplusplus
}
#endif

#endif
