#!/usr/bin/env python3
"""
Script to create SYNRIX installer .exe using PyInstaller
Run this to create install_synrix.exe
"""

import subprocess
import sys
from pathlib import Path

def main():
    """Create installer executable"""
    print("Creating SYNRIX installer executable...")
    print()
    
    # Check PyInstaller
    try:
        import PyInstaller
        print("[OK] PyInstaller found")
    except ImportError:
        print("Installing PyInstaller...")
        subprocess.run([sys.executable, "-m", "pip", "install", "pyinstaller"], check=True)
        print("[OK] PyInstaller installed")
    
    # Get paths
    script_dir = Path(__file__).parent.resolve()
    installer_script = script_dir / "installer.py"
    icon_path = None  # Can add icon later if needed
    
    # PyInstaller command
    cmd = [
        "pyinstaller",
        "--onefile",
        "--windowed",  # No console window (use --console to see output)
        "--name", "install_synrix",
        "--add-data", f"{script_dir / 'synrix'};synrix",  # Include synrix package
        "--add-data", f"{script_dir / 'setup.py'};.",  # Include setup.py
        str(installer_script)
    ]
    
    if icon_path and icon_path.exists():
        cmd.extend(["--icon", str(icon_path)])
    
    print("Running PyInstaller...")
    print(f"Command: {' '.join(cmd)}")
    print()
    
    try:
        subprocess.run(cmd, check=True)
        print()
        print("[OK] Installer created successfully.")
        print(f"  Output: {script_dir / 'dist' / 'install_synrix.exe'}")
        print()
        print("You can now distribute install_synrix.exe")
    except subprocess.CalledProcessError as e:
        print(f"ERROR: Failed to create installer: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
