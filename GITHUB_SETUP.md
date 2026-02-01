# Setting Up This Repo on GitHub

Short checklist so the repo is tested and ready to push to GitHub.

---

## 1. Use `.gitignore`

- A **`.gitignore`** is at the repo root. It excludes:
  - **Secrets:** `synrix_license_private.key`, `*.key`, `.env`, `*.pem`
  - **Build outputs:** `build_msys2/`, `build_free_tier_100k/`, etc., and `build/windows/build/`
  - **Binaries:** `*.zip`, `*.dll`, `*.exe`
  - **Python:** `__pycache__/`, `*.pyc`, `dist/`, `*.egg-info/`
  - **Runtime data:** `*.lattice`, `*.lattice.wal`, `*.wal`

- If you still have **`GITIGNORE.txt`**, you can remove it; **`.gitignore`** is the one to use.

---

## 2. Never Commit the Private Key

- **`synrix_license_private.key`** (in `synrix-windows-build/build/windows/`) must **not** be in the repo.
- It is listed in **`.gitignore`** (`*.key` and `synrix_license_private.key`).
- Before the first push, confirm it’s not tracked:
  ```powershell
  git status
  # If synrix_license_private.key appears, run: git rm --cached synrix-windows-build/build/windows/synrix_license_private.key
  ```

---

## 3. Repo Layout (what gets committed)

- **Source:** `synrix-windows-build/build/windows/src/`, `include/`, `CMakeLists.txt`, scripts, docs.
- **SDKs:** `synrix-sdks/agent-memory-sdk/`, `synrix-sdks/robotics-sdk/`, `synrix-sdks/synrix-rag-sdk/` (Python source; no DLLs in repo).
- **Packages:** `synrix-windows-build/packages/` (Python + config; DLLs/zips ignored).
- **Docs:** `SUMMARY_FOR_JOSEPH.md`, `TEST_NOW.md`, `PRE_PROD_CHECKLIST.md`, `build/windows/LICENSE_SIGNED_KEY.md`, etc.

Binaries (`.dll`, `.zip`) are not committed; use **GitHub Releases** to attach tier zips if needed.

---

## 4. Create the GitHub Repo

1. On GitHub: **New repository** (e.g. `synrix` or `synrix-windows-build`). Private or public as you prefer.
2. **Do not** add a README/license yet if you already have a local repo.
3. Locally (from the repo root, e.g. `synrix-windows-build`):
   ```powershell
   git init
   git add .
   git status   # Confirm no .key, no .dll, no .zip
   git commit -m "Initial commit: SYNRIX engine + signed license + SDKs"
   git branch -M main
   git remote add origin https://github.com/YOUR_ORG/synrix.git
   git push -u origin main
   ```

---

## 5. Releases (optional)

- To ship tier zips (e.g. `free_tier_100k.zip`, `unlimited.zip`), build them locally, then:
  - **Releases** → **Create a new release** → attach the `.zip` files.
- Do **not** commit large binaries to the repo; keep them in Releases only.

---

## 6. Quick Test Before Push

From `synrix-windows-build` (inner folder):

```powershell
# Keygen / format (no DLL)
python build\windows\tools\test_license_flow.py

# Stress test with unlimited key (needs DLL + OpenSSL in packages\unlimited\synrix; PATH or copy DLLs)
$env:Path = "C:\msys64\mingw64\bin;" + $env:Path
$env:SYNRIX_LICENSE_KEY = "<your_unlimited_key>"
python stress_test_binary.py --package "packages\unlimited" --dll libsynrix.dll --nodes 5000 --no-evaluation
```

If both succeed, the repo is in good shape to push.

---

**Summary:** Use the provided `.gitignore`, never commit the private key, then `git init`, `git add .`, `git status`, `git commit`, and push to your new GitHub repo. Attach tier zips via Releases if you ship them.
