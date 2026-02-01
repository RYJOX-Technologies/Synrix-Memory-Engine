# SYNRIX Package Builder Guide

## Quick Start

### Build a Single Version

**Free Tier 50k:**
```bash
python build_package.py --limit 50000 --name free_tier_50k
```

**Free Tier 100k:**
```bash
python build_package.py --limit 100000 --name free_tier_100k
```

**Unlimited:**
```bash
python build_package.py --unlimited --name unlimited
```

### Build All Versions

**Windows:**
```bash
build_all_versions.bat
```

**Linux/MSYS2:**
```bash
python build_package.py --limit 50000 --name free_tier_50k
python build_package.py --limit 100000 --name free_tier_100k
python build_package.py --unlimited --name unlimited
```

## How It Works

1. **Builds DLL** with specified node limit (or unlimited)
2. **Copies template** from `synrix_unlimited/`
3. **Replaces DLL** with the newly built one
4. **Updates metadata** (package name, node limit info)
5. **Creates ZIP** ready to distribute

## Output

All packages are created in `packages/`:
- `packages/free_tier_50k/` - Package directory
- `packages/free_tier_50k.zip` - Ready to send
- `packages/free_tier_100k/` - Package directory
- `packages/free_tier_100k.zip` - Ready to send
- `packages/unlimited/` - Package directory
- `packages/unlimited.zip` - Ready to send

## Customization

### Change Node Limit

Just change the `--limit` value:
```bash
python build_package.py --limit 25000 --name free_tier_25k
```

### Skip DLL Build

If DLL is already built:
```bash
python build_package.py --limit 50000 --name free_tier_50k --skip-build
```

## Requirements

- Python 3.8+
- CMake
- MinGW-w64 (for Windows builds)
- MSYS2 (for Windows builds)

## What Gets Built

Each package includes:
- ✅ SYNRIX Python package
- ✅ DLL with correct node limit
- ✅ All MinGW runtime DLLs
- ✅ One-click installer (installer_v2)
- ✅ All dependency installers
- ✅ Comprehensive documentation
- ✅ Diagnostic tools

## Template

The builder uses `synrix_unlimited/` as the template. All packages are based on this, with:
- Different DLL (with node limit)
- Updated metadata
- Version-specific documentation

## Notes

- The template (`synrix_unlimited/`) should always have the latest installer and documentation
- Node limits are hard-coded in the DLL at build time
- All packages use the same installer system (installer_v2)
- ZIP files are ready to distribute immediately
