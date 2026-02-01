# SYNRIX Windows Release Checklist

## Pre-Release Verification

### Build Verification

- [ ] **Standard Build (Unlimited)**
  ```bash
  cd build/windows
  bash build_msys2.sh
  ```
  - [ ] DLL created: `build_msys2/bin/libsynrix.dll`
  - [ ] Size: ~612KB
  - [ ] No build errors or warnings

- [ ] **Free Tier Build (50k Limit)**
  ```bash
  cd build/windows
  bash build_free_tier_50k.sh
  ```
  - [ ] DLL created: `build_free_tier/bin/libsynrix_free_tier.dll`
  - [ ] Size: ~612KB
  - [ ] No build errors or warnings

### Functionality Tests

- [ ] **Robustness Test Suite**
  ```bash
  python test_windows_robust.py
  ```
  - [ ] All 7/7 tests passing
  - [ ] No crashes or errors
  - [ ] All operations complete successfully

- [ ] **Basic Operations**
  - [ ] Add nodes works
  - [ ] Get nodes works
  - [ ] Query by prefix works
  - [ ] Save/checkpoint works
  - [ ] No "Permission denied" errors
  - [ ] No "Failed to replace file" errors

- [ ] **Persistence**
  - [ ] Nodes persist across sessions
  - [ ] WAL recovery works
  - [ ] Checkpoint applies WAL correctly
  - [ ] Data integrity verified

- [ ] **Free Tier Limit**
  - [ ] 50k node limit enforced
  - [ ] Error message displays correctly
  - [ ] Delete operations work after limit
  - [ ] WAL recovery bypasses limit

### Performance Benchmarks

- [ ] **Run Benchmarks**
  ```bash
  python benchmark_windows.py
  ```
  - [ ] Node addition: ~2-3 μs/node
  - [ ] Node lookup: ~15-25 μs
  - [ ] Prefix queries: ~250-300 μs
  - [ ] All operations sub-millisecond

### Package Verification

- [ ] **Free Tier Package**
  ```powershell
  .\create_free_tier_package.ps1
  ```
  - [ ] Package created: `dist/synrix-free-tier-50k-windows-1.0.0.zip`
  - [ ] Contains `libsynrix_free_tier.dll`
  - [ ] Contains all dependencies
  - [ ] Contains Python SDK
  - [ ] Contains `install.bat`
  - [ ] Contains `INSTALL.txt` and `README.txt`

- [ ] **Package Contents Verified**
  - [ ] DLL loads correctly
  - [ ] Dependencies present (libgcc, libwinpthread, zlib)
  - [ ] Python SDK installs correctly
  - [ ] Test installation on clean system

### Documentation

- [ ] **Updated Documentation**
  - [ ] `BUILD.md` - MSYS2 instructions
  - [ ] `WINDOWS_COMPATIBILITY.md` - All fixes documented
  - [ ] `AI_AGENT_USABILITY.md` - Performance analysis
  - [ ] `BENCHMARK_SUMMARY.md` - Performance metrics
  - [ ] `RELEASE_CHECKLIST.md` - This file

- [ ] **Package Documentation**
  - [ ] `INSTALL.txt` - Simple installation guide
  - [ ] `README.txt` - Detailed documentation
  - [ ] All instructions clear and tested

### Code Quality

- [ ] **No Known Bugs**
  - [ ] Windows file replacement working
  - [ ] WAL recovery working
  - [ ] Free tier limit working
  - [ ] No memory leaks
  - [ ] No crashes in stress tests

- [ ] **Code Review**
  - [ ] Windows-specific fixes reviewed
  - [ ] Error handling adequate
  - [ ] Edge cases handled

## Release Package Contents

### Free Tier Package (`synrix-free-tier-50k-windows-1.0.0.zip`)

```
synrix-free-tier-50k-windows-1.0.0/
├── bin/
│   ├── libsynrix_free_tier.dll
│   ├── libgcc_s_seh-1.dll
│   ├── libwinpthread-1.dll
│   └── zlib1.dll
├── python-sdk/
│   ├── synrix/
│   ├── setup.py
│   └── README.md
├── install.bat
├── INSTALL.txt
└── README.txt
```

### Standard Package (For Creator/Internal Use)

- `libsynrix.dll` (unlimited nodes)
- Dependencies
- Python SDK
- Documentation

## Release Steps

1. ✅ **Build Verification** - All builds successful
2. ✅ **Testing** - All tests passing
3. ✅ **Documentation** - All docs updated
4. ⏳ **Performance Tuning** - Optimize if needed
5. ⏳ **Integration Testing** - Real agent workloads
6. ⏳ **Final Package** - Create release packages
7. ⏳ **Release Notes** - Document changes

## Known Limitations

- **Performance**: ~20x slower lookups than Linux (DLL overhead, acceptable for agents)
- **Durability**: NTFS semantics differ from Linux (WAL ensures correctness)
- **Platform**: Linux remains reference platform for strict durability

## Post-Release

- [ ] Monitor for issues
- [ ] Collect performance metrics from users
- [ ] Plan optimizations if needed
- [ ] Update documentation based on feedback
