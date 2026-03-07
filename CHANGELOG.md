# Changelog

## [Unreleased]

(Reserved for future changes.)

## [1.0]

### Summary

**O(k) prefix queries at scale, license system, security hardening, Windows console fix.** Ready for production use.

### Major improvements

- **O(k) prefix queries at scale** — ~0.28ms at 50K nodes (hundreds of times faster than before). No 10K threshold; incremental index at all sizes.
- **License system** — Free 25K tier; unlimited with signed key. Ed25519 verification, expiry-based revocation. All four critical paths tested (no key, invalid key, valid unlimited, expired key).
- **Security hardened** — Public bypass removed (`evaluation_mode` internal only, not exported). Free tier 25K strictly enforced unless valid key.
- **Windows console** — Unicode replaced with ASCII in engine messages.

### Fixed

- **[CRITICAL]** Removed 10K indexing threshold that caused O(n) query spikes and potential corruption from full index rebuilds. Prefix index now incremental at all dataset sizes.
- License apply logic: unlimited keys (`node_limit == 0`) now apply correctly.

### Added

- License key resolution: `SYNRIX_LICENSE_KEY` env, `~/.synrix/license.json`, `license_key` next to binary.
- Ship tests: `test_no_key_25k_limit.py`, `test_invalid_key_25k_limit.py`, `test_valid_key_unlimited.py`, `test_expired_key_25k_limit.py`.
- [LICENSE_TESTING_MATRIX.md](LICENSE_TESTING_MATRIX.md), [docs/LICENSE_FLOW.md](docs/LICENSE_FLOW.md).

### Known limitations (1.x)

- V1 format keys (176-byte with HWID): legacy 78-byte keys tested.
- File-based key resolution (`.synrix/license.json`, `license_key` file) not yet in test matrix.
- Non-unlimited tier caps (100K, 1M, etc.) not yet tested.

Testing matrix: see [LICENSE_TESTING_MATRIX.md](LICENSE_TESTING_MATRIX.md).

## [Previous Releases]

See git history for earlier changes.
