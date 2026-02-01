#!/usr/bin/env python3
"""
SYNRIX Package Builder
======================

Simple build system: Change node count, press build, get package.

Usage:
    python build_package.py --limit 50000 --name free_tier_50k
    python build_package.py --limit 100000 --name free_tier_100k
    python build_package.py --unlimited --name unlimited
"""

import os
import sys
import subprocess
import shutil
import argparse
from pathlib import Path

# Configuration
PROJECT_ROOT = Path(__file__).parent
BUILD_DIR = PROJECT_ROOT / "build" / "windows"
PACKAGE_TEMPLATE = PROJECT_ROOT / "synrix_unlimited"  # Use unlimited as template
OUTPUT_DIR = PROJECT_ROOT / "packages"

def print_header(text):
    """Print formatted header"""
    print("\n" + "=" * 70)
    print(f"  {text}")
    print("=" * 70 + "\n")

def run_command(cmd, cwd=None, check=True):
    """Run a command and return result"""
    print(f"Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    try:
        if isinstance(cmd, str):
            result = subprocess.run(
                cmd,
                shell=True,
                cwd=cwd,
                check=check,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
        else:
            result = subprocess.run(
                cmd,
                cwd=cwd,
                check=check,
                capture_output=True,
                text=True,
                encoding="utf-8",
                errors="replace",
            )
        if result.stdout:
            print(result.stdout)
        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print(f"Error: {e}")
        if e.stderr:
            print(e.stderr)
        return False

def build_dll(node_limit=None, package_name="synrix"):
    """Build the SYNRIX DLL with specified node limit"""
    print_header(f"Building DLL (limit: {node_limit if node_limit else 'unlimited'})")
    
    # Create build directory
    build_path = BUILD_DIR / f"build_{package_name}"
    build_path.mkdir(parents=True, exist_ok=True)
    
    # Configure CMake (need to point to build/windows as source)
    cmake_source = BUILD_DIR  # CMakeLists.txt is in build/windows
    cmake_args = [
        "cmake",
        str(cmake_source),
        "-DCMAKE_BUILD_TYPE=Release",
    ]
    
    if node_limit:
        cmake_args.extend([
            f"-DSYNRIX_FREE_TIER_LIMIT={node_limit}",
            "-DSYNRIX_FREE_TIER_50K=ON"
        ])
        dll_name = "libsynrix_free_tier.dll"
    else:
        dll_name = "libsynrix.dll"
    
    print(f"Configuring CMake with limit: {node_limit if node_limit else 'unlimited'}")
    if not run_command(cmake_args, cwd=build_path):
        return None
    
    # Build
    print("Building DLL...")
    if not run_command(["cmake", "--build", ".", "--config", "Release"], cwd=build_path):
        return None
    
    # Find the DLL (check multiple possible locations)
    dll_path = None
    search_paths = [
        build_path / "bin" / dll_name,  # Common output location
        build_path / "Release" / dll_name,
        build_path / "Debug" / dll_name,
        build_path / dll_name,
        build_path / "libsynrix.dll",  # Sometimes named differently
        build_path / "libsynrix_free_tier.dll",
    ]
    
    for test_path in search_paths:
        if test_path.exists():
            dll_path = test_path
            break
    
    if not dll_path:
        print(f"[ERROR] DLL not found: {dll_name}")
        print(f"  Searched in: {build_path}")
        print(f"  Tried paths: {[str(p) for p in search_paths]}")
        return None
    
    print(f"[OK] DLL built: {dll_path}")
    return dll_path

def create_package(package_name, dll_path, node_limit=None):
    """Create package from template"""
    print_header(f"Creating Package: {package_name}")
    
    # Create package directory
    package_dir = OUTPUT_DIR / package_name
    if package_dir.exists():
        print(f"Removing existing package: {package_dir}")
        shutil.rmtree(package_dir)
    package_dir.mkdir(parents=True, exist_ok=True)
    
    # Copy template
    print("Copying template files...")
    for item in PACKAGE_TEMPLATE.iterdir():
        if item.name in ['.git', '__pycache__', '.lattice']:
            continue
        dest = package_dir / item.name
        if item.is_dir():
            shutil.copytree(item, dest, ignore=shutil.ignore_patterns('__pycache__', '*.pyc'))
        else:
            shutil.copy2(item, dest)
    
    # Copy DLL to synrix directory
    synrix_dir = package_dir / "synrix"
    synrix_dir.mkdir(exist_ok=True)
    dll_dest = synrix_dir / dll_path.name
    shutil.copy2(dll_path, dll_dest)
    print(f"[OK] Copied DLL to: {dll_dest}")

    # For free tier builds, remove unlimited DLL to avoid wrong load order
    if node_limit:
        unlimited_dll = synrix_dir / "libsynrix.dll"
        if unlimited_dll.exists():
            unlimited_dll.unlink()
    
    # Update package name in setup.py
    setup_py = package_dir / "setup.py"
    if setup_py.exists():
        content = setup_py.read_text(encoding="utf-8", errors="replace")
        # Update description if needed
        if node_limit:
            content = content.replace(
                'description="SYNRIX - AI Agent Memory System (Unlimited)"',
                f'description="SYNRIX - AI Agent Memory System (Free Tier: {node_limit:,} nodes)"'
            )
        setup_py.write_text(content, encoding="utf-8", errors="replace")
    
    # Update START_HERE.md with node limit
    start_here = package_dir / "START_HERE.md"
    if start_here.exists() and node_limit:
        content = start_here.read_text(encoding="utf-8", errors="replace")
        # Add node limit info if not present
        if "node limit" not in content.lower():
            content = content.replace(
                "## What You Get",
                f"## What You Get\n\n- [OK] **{node_limit:,} node limit** - Free tier evaluation version"
            )
        start_here.write_text(content, encoding="utf-8", errors="replace")
    
    # Create README with version info
    readme = package_dir / "VERSION_INFO.txt"
    with open(readme, 'w') as f:
        f.write(f"SYNRIX Package: {package_name}\n")
        f.write(f"Node Limit: {node_limit if node_limit else 'Unlimited'}\n")
        f.write(f"DLL: {dll_path.name}\n")
        f.write(f"Build Date: {__import__('datetime').datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    print(f"[OK] Package created: {package_dir}")
    return package_dir

def create_zip(package_dir):
    """Create zip file from package"""
    print_header("Creating ZIP Archive")
    
    zip_path = OUTPUT_DIR / f"{package_dir.name}.zip"
    if zip_path.exists():
        zip_path.unlink()
    
    print(f"Creating: {zip_path}")
    shutil.make_archive(str(zip_path.with_suffix('')), 'zip', package_dir.parent, package_dir.name)
    
    size_mb = zip_path.stat().st_size / (1024 * 1024)
    print(f"[OK] ZIP created: {zip_path.name} ({size_mb:.2f} MB)")
    return zip_path

def main():
    parser = argparse.ArgumentParser(description='Build SYNRIX package')
    parser.add_argument('--limit', type=int, help='Node limit for free tier (e.g., 50000, 100000)')
    parser.add_argument('--unlimited', action='store_true', help='Build unlimited version')
    parser.add_argument('--name', required=True, help='Package name (e.g., free_tier_50k, unlimited)')
    parser.add_argument('--skip-build', action='store_true', help='Skip DLL build (use existing)')
    
    args = parser.parse_args()
    
    if not args.unlimited and not args.limit:
        print("[ERROR] Must specify --limit or --unlimited")
        sys.exit(1)
    
    node_limit = None if args.unlimited else args.limit
    
    print_header(f"SYNRIX Package Builder - {args.name}")
    print(f"Node Limit: {node_limit if node_limit else 'Unlimited'}")
    print()
    
    # Step 1: Build DLL
    if not args.skip_build:
        dll_path = build_dll(node_limit, args.name)
        if not dll_path:
            print("[ERROR] DLL build failed")
            sys.exit(1)
    else:
        # Find existing DLL
        if node_limit:
            dll_name = "libsynrix_free_tier.dll"
        else:
            dll_name = "libsynrix.dll"
        
        # Search for DLL
        dll_path = None
        for build_dir in BUILD_DIR.glob("build_*"):
            for pattern in ["bin", "Release", "Debug", "."]:
                test_path = build_dir / pattern / dll_name
                if test_path.exists():
                    dll_path = test_path
                    break
            if dll_path:
                break
        
        if not dll_path:
            print(f"[ERROR] DLL not found: {dll_name}")
            print("Run without --skip-build to build it")
            sys.exit(1)
        print(f"[OK] Using existing DLL: {dll_path}")
    
    # Step 2: Create package
    package_dir = create_package(args.name, dll_path, node_limit)
    if not package_dir:
        print("[ERROR] Package creation failed")
        sys.exit(1)
    
    # Step 3: Create ZIP
    zip_path = create_zip(package_dir)
    
    # Success!
    print_header("Build Complete!")
    print(f"Package: {package_dir.name}")
    print(f"ZIP: {zip_path}")
    print(f"Size: {zip_path.stat().st_size / (1024 * 1024):.2f} MB")
    print()
    print("Ready to distribute!")

if __name__ == "__main__":
    main()
