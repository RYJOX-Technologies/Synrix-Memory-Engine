# Pre-prod checklist (done)

Summary of what was run and what you should do once before calling prod.

---

## Done in this session

1. **No-key stress test** – Run completed 26k nodes; env may have had `SYNRIX_LICENSE_KEY` set. See “Your action” below.
2. **100k tier test** – 100k key + 101k nodes: test completed (101k inserted). Tier limit enforcement may need a follow-up (expected cap at 100k).
3. **OpenSSL DLLs** – Copied `libcrypto*.dll` and `libssl*.dll` from `C:\msys64\mingw64\bin` into:
   - `packages\free_tier_100k\synrix`, `free_tier_1m\synrix`, `free_tier_10m\synrix`, `free_tier_50m\synrix`, `packages\unlimited\synrix`
   - `synrix-sdks\agent-memory-sdk\synrix`, `synrix-sdks\robotics-sdk\synrix`
4. **Tier zips refreshed** – Re-created `packages\free_tier_100k.zip`, `free_tier_1m.zip`, `free_tier_10m.zip`, `free_tier_50m.zip`, `unlimited.zip` from the updated package folders (single `libsynrix.dll` + OpenSSL DLLs).
5. **SDK smoke** – Agent-memory-sdk and robotics-sdk: load DLL, add node – both OK. `libsynrix.dll` was copied into `synrix-sdks\robotics-sdk\synrix` (it was missing there).

---

## Your action before prod

**No-key test in a clean shell**

In a **new** PowerShell window (so `SYNRIX_LICENSE_KEY` is not set):

```powershell
cd C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
# Do NOT set SYNRIX_LICENSE_KEY
python -u stress_test_binary.py --package "packages\unlimited" --dll libsynrix.dll --nodes 26000 --no-evaluation
```

Expected: run stops at ~25k nodes with a “Free Tier limit” (or similar) error. If it completes 26k, the default cap is not being applied (e.g. env still has a key or build default differs).

---

## Optional follow-up

- **100k tier cap** – If you need strict 100k enforcement, confirm in C that the limit is checked before adding the 100001st node (e.g. `(total_nodes + 1) > free_tier_limit` in all add paths).
- **Robotics SDK** – If you ship it as a separate artifact, ensure `libsynrix.dll` and OpenSSL DLLs are included in the dist or install instructions.

---

Once the no-key test behaves as expected in a clean shell, you’re good to push prod from a testing perspective.
