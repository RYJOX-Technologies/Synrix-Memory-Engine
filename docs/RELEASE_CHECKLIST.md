# Release Checklist (Windows + Linux)

Use this when cutting a release. See **docs/RELEASE.md** for full process; §9 covers Linux tarball.

---

## Before release

- [ ] **Build Windows** – CMake / MSYS2 → `libsynrix.dll` + runtime DLLs in build output (e.g. `build/windows/build_msys2/bin`).
- [ ] **Build Linux** (if releasing Linux) – `build/linux/build.sh` → `build/linux/out/libsynrix.so`.
- [ ] **Test SDK + lib** – Set `SYNRIX_LIB_PATH` to the folder with `libsynrix.dll` (e.g. `build/windows/build_msys2/bin`), then run `scripts/test_license_sdk_lib.py` (e.g. `--max-adds 500`). Fix any errors.

---

## Windows release

- [ ] **Create zip** – Run `build/windows/create_release_zip.ps1` (uses `build_msys2/bin` or, if missing, VS `build/Release/bin`). Produces `synrix-windows.zip`.
- [ ] **Test zip** – Run `build/windows/tools/test_release_zip.py` (optionally with `--zip path/to/synrix-windows.zip`). Must pass.
- [ ] **Optional:** Compute SHA256 for the zip (GitHub shows it on the release page; you can paste if desired).
- [ ] **GitHub release** – Create release, attach `synrix-windows.zip`.

---

## Linux release

- [ ] **Create tarball** – e.g. `synrix-linux-x86_64.tar.gz` or `synrix-linux-arm64.tar.gz` with `libsynrix.so` and bundled runtime libs. See **RELEASE.md §9**.
- [ ] **Attach** – Add tarball to the same GitHub release.
- [ ] **Optional:** SHA256 is shown on the release page.

---

## After release

- [ ] **Docs** – Ensure README / download instructions point to the new release assets.
- [ ] **Tags** – Tag the commit if you use version tags.
