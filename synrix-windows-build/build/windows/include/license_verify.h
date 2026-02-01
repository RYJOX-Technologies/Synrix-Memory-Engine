#ifndef SYNRIX_LICENSE_VERIFY_H
#define SYNRIX_LICENSE_VERIFY_H

#include <stdint.h>

/* Signed license verification (Ed25519).
 * Key format: base64(payload || signature), payload = version(1) tier(1) expiry(4).
 * Tier: 0=100k, 1=1m, 2=10m, 3=50m, 4=unlimited.
 * Engine reads SYNRIX_LICENSE_KEY from environment; if valid, overrides tier at init.
 */

/* Parse and verify SYNRIX_LICENSE_KEY from env.
 * key: license key string (base64), or NULL to read from getenv("SYNRIX_LICENSE_KEY").
 * out_limit: on success, node limit (100000, 1000000, 10000000, 50000000) or 0 for unlimited.
 * out_unlimited: on success, 1 if tier is unlimited, else 0.
 * Returns: 0 if valid and not expired, -1 if missing/invalid/expired.
 */
int synrix_license_parse(const char* key, uint32_t* out_limit, int* out_unlimited);

#endif
