# Linux cohesion checklist

What the Linux build and SDK must have so Windows and Linux behave the same. Authoritative reference for the Linux build and release tarball.

---

## Linux build

- **Script:** `build/linux/build.sh`
- **Output:** `build/linux/out/libsynrix.so`
- **Components:** Lattice storage, index, and license module. Link with `-lcrypto -ldl` as needed.
- **License:** Apply in `lattice_init()` (same as Windows) or let the SDK apply after init when the engine exposes the license API.

---

## SDK (raw_backend.py)

- **Single backend:** Only `raw_backend.py`; no server. App uses SDK + native lib (`.so` / `.dll`) only.
- **Library discovery:** `LD_LIBRARY_PATH`, `SYNRIX_LIB_PATH`, or when running from repo: `build/linux/out/` (so after build, SDK can load `libsynrix.so` without setting env).
- **License on init:** After `lattice_init()`, if the lib exposes the license API, the SDK calls it (key from env or `~/.synrix/license.json`). Same on Windows when the DLL exposes those symbols.

---

## Verified in this repo

| Item | In this repo | In Linux tree (if separate) |
|------|--------------|------------------------------|
| Windows build | `build/windows/`, create_release_zip.ps1 | — |
| Linux build script | Optional (copy from Linux tree) | `build/linux/build.sh` |
| python-sdk raw_backend | ✅ License-on-init, Linux path | Same |
| scripts/test_license_sdk_lib.py | ✅ | Same or equivalent |
| build/windows/tools/test_release_zip.py | ✅ | — |
| docs/WINDOWS_LINUX_ALIGNMENT.md | ✅ | — |

---

## Linux release – ready

When the Linux tree has:

- `build/linux/build.sh` producing `build/linux/out/libsynrix.so`
- Same key source order and tier behavior as Windows (see **WINDOWS_LINUX_ALIGNMENT.md**)

then build the tarball, attach to the GitHub release, and note the asset name (e.g. `synrix-linux-x86_64.tar.gz`) in the release notes.
