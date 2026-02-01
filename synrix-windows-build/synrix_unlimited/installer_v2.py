#!/usr/bin/env python3
"""
SYNRIX One-Click Installer v2
Automatically installs all dependencies and SYNRIX package
"""

import os
import sys
import subprocess
import platform
from pathlib import Path
import urllib.request
import shutil

def print_header(text):
    """Print formatted header"""
    print("\n" + "=" * 70)
    print(f"  {text}")
    print("=" * 70 + "\n")

def print_step(step_num, text):
    """Print step with formatting"""
    print(f"[{step_num}] {text}")

def check_python():
    """Check Python version"""
    version = sys.version_info
    if version.major < 3 or (version.major == 3 and version.minor < 8):
        print("[ERROR] Python 3.8+ required!")
        print(f"  Current version: {version.major}.{version.minor}.{version.micro}")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    # Check architecture
    is_64bit = platform.machine().endswith('64') or platform.architecture()[0] == '64bit'
    if not is_64bit:
        print("[ERROR] 64-bit Python required!")
        print("  SYNRIX requires 64-bit Python on Windows")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    print(f"[OK] Python {version.major}.{version.minor}.{version.micro} (64-bit)")
    return True

def check_vc2013_runtime():
    """Check if Visual C++ 2013 Runtime is installed"""
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 
                           r"SOFTWARE\Microsoft\VisualStudio\12.0\VC\Runtimes\x64")
        winreg.CloseKey(key)
        return True
    except (FileNotFoundError, OSError):
        return False

def install_vc2013_runtime():
    """Download and install Visual C++ 2013 Runtime"""
    print_step(1, "Installing Visual C++ 2013 Runtime...")
    
    if check_vc2013_runtime():
        print("  [OK] Already installed")
        return True
    
    print("  [INFO] Downloading VC++ 2013 Redistributable...")
    
    temp_dir = Path(os.environ.get('TEMP', '.')) / 'synrix_installer'
    temp_dir.mkdir(exist_ok=True)
    installer_path = temp_dir / 'vcredist_x64.exe'
    
    try:
        url = 'https://download.microsoft.com/download/2/E/6/2E61CFA4-993B-4DD4-91DA-3737CD5CD6E3/vcredist_x64.exe'
        urllib.request.urlretrieve(url, installer_path)
        print("  [OK] Download complete")
    except Exception as e:
        print(f"  [ERROR] Download failed: {e}")
        print("\n  Please download manually from:")
        print("  https://www.microsoft.com/en-us/download/details.aspx?id=40784")
        return False
    
    print("  [INFO] Installing (this may take a minute)...")
    try:
        result = subprocess.run(
            [str(installer_path), '/install', '/quiet', '/norestart'],
            timeout=120,
            capture_output=True
        )
        if result.returncode == 0:
            print("  [OK] Installation complete")
            return True
        else:
            print(f"  [ERROR] Installation failed (code: {result.returncode})")
            return False
    except subprocess.TimeoutExpired:
        print("  [ERROR] Installation timed out")
        return False
    except Exception as e:
        print(f"  [ERROR] Installation failed: {e}")
        return False
    finally:
        # Cleanup
        if installer_path.exists():
            installer_path.unlink()

def check_zlib():
    """Check if zlib1.dll exists in synrix directory"""
    script_dir = Path(__file__).parent
    synrix_dir = script_dir / "synrix"
    zlib_path = synrix_dir / "zlib1.dll"
    return zlib_path.exists()

def download_zlib():
    """Download zlib1.dll"""
    print_step(2, "Downloading zlib1.dll...")
    
    if check_zlib():
        print("  [OK] Already present")
        return True
    
    script_dir = Path(__file__).parent
    synrix_dir = script_dir / "synrix"
    synrix_dir.mkdir(exist_ok=True)
    zlib_path = synrix_dir / "zlib1.dll"
    
    print("  [INFO] Downloading from MinGW-w64 releases...")
    
    # Try direct download from a reliable source
    zlib_urls = [
        "https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0-16.0.6-11.0.0-ucrt-r1/winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.0-r1.zip",
        # Fallback: direct zlib1.dll if available
    ]
    
    temp_dir = Path(os.environ.get('TEMP', '.')) / 'synrix_installer'
    temp_dir.mkdir(exist_ok=True)
    zip_path = temp_dir / 'mingw.zip'
    
    try:
        # Download zip
        print("  [INFO] Downloading MinGW-w64 package...")
        urllib.request.urlretrieve(zlib_urls[0], zip_path)
        print("  [OK] Download complete")
        
        # Extract zlib1.dll
        print("  [INFO] Extracting zlib1.dll...")
        import zipfile
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            # Find zlib1.dll in bin directory
            for member in zip_ref.namelist():
                if 'zlib1.dll' in member and '/bin/' in member:
                    with zip_ref.open(member) as source:
                        with open(zlib_path, 'wb') as target:
                            target.write(source.read())
                    print("  [OK] zlib1.dll extracted")
                    break
            else:
                print("  [ERROR] zlib1.dll not found in archive")
                return False
        
        return True
    except Exception as e:
        print(f"  [ERROR] Download failed: {e}")
        print("\n  Please download manually:")
        print("  1. Go to: https://github.com/brechtsanders/winlibs_mingw/releases")
        print(f"  2. Extract zlib1.dll from bin/ directory")
        print(f"  3. Copy to: {zlib_path}")
        return False
    finally:
        # Cleanup
        if zip_path.exists():
            zip_path.unlink()

def install_synrix():
    """Install SYNRIX package"""
    print_step(3, "Installing SYNRIX package...")
    
    script_dir = Path(__file__).parent
    package_dir = str(script_dir).rstrip('\\/')
    
    print(f"  [INFO] Package directory: {package_dir}")
    
    try:
        result = subprocess.run(
            [sys.executable, '-m', 'pip', 'install', '-e', package_dir],
            capture_output=True,
            text=True,
            timeout=60
        )
        
        if result.returncode == 0:
            print("  [OK] Installation complete")
            return True
        else:
            print(f"  [ERROR] Installation failed:")
            print(result.stderr)
            return False
    except subprocess.TimeoutExpired:
        print("  [ERROR] Installation timed out")
        return False
    except Exception as e:
        print(f"  [ERROR] Installation failed: {e}")
        return False

def test_installation():
    """Test that SYNRIX works"""
    print_step(4, "Testing installation...")
    
    try:
        from synrix.ai_memory import get_ai_memory
        memory = get_ai_memory()
        memory.add("TEST:installer", "SYNRIX installer test")
        results = memory.query("TEST:")
        if len(results) > 0:
            print("  [OK] SYNRIX is working!")
            return True
        else:
            print("  [ERROR] Installation test failed")
            return False
    except ImportError as e:
        print(f"  [ERROR] Cannot import synrix: {e}")
        print("\n  Troubleshooting:")
        print("  1. Make sure you ran: pip install -e .")
        print("  2. Check that synrix package is in Python path")
        return False
    except Exception as e:
        print(f"  [ERROR] Test failed: {e}")
        print("\n  SYNRIX may still work, but there's an issue.")
        print("  Try running: python -c \"from synrix.ai_memory import get_ai_memory; m = get_ai_memory()\"")
        return False

def main():
    """Main installer function"""
    print_header("SYNRIX One-Click Installer")
    
    print("This installer will:")
    print("  1. Check Python version")
    print("  2. Install Visual C++ 2013 Runtime (if needed)")
    print("  3. Download zlib1.dll (if needed)")
    print("  4. Install SYNRIX package")
    print("  5. Test installation")
    print()
    
    input("Press Enter to continue...")
    
    # Step 0: Check Python
    print_step(0, "Checking Python...")
    if not check_python():
        input("\nPress Enter to exit...")
        sys.exit(1)
    print()
    
    # Step 1: Install VC++ Runtime
    if not install_vc2013_runtime():
        print("\n[WARNING] VC++ 2013 Runtime installation failed.")
        print("SYNRIX may not work without it.")
        response = input("Continue anyway? (y/n): ")
        if response.lower() != 'y':
            sys.exit(1)
    print()
    
    # Step 2: Download zlib1.dll
    if not download_zlib():
        print("\n[WARNING] zlib1.dll download failed.")
        print("SYNRIX may not work without it.")
        response = input("Continue anyway? (y/n): ")
        if response.lower() != 'y':
            sys.exit(1)
    print()
    
    # Step 3: Install SYNRIX
    if not install_synrix():
        print("\n[ERROR] SYNRIX installation failed.")
        print("\nTroubleshooting:")
        print("  1. Make sure you have write permissions")
        print("  2. Try running as administrator")
        print("  3. Check that pip is working: python -m pip --version")
        input("\nPress Enter to exit...")
        sys.exit(1)
    print()
    
    # Step 4: Test installation
    if not test_installation():
        print("\n[WARNING] Installation test failed, but SYNRIX may still work.")
        print("Try using it in your code to verify.")
    print()
    
    # Success!
    print_header("Installation Complete!")
    print("SYNRIX has been successfully installed.")
    print()
    print("You can now use SYNRIX in your Python scripts:")
    print()
    print("  from synrix.ai_memory import get_ai_memory")
    print("  memory = get_ai_memory()")
    print("  memory.add('TEST', 'Works!')")
    print()
    input("Press Enter to exit...")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInstallation cancelled by user.")
        sys.exit(1)
    except Exception as e:
        print(f"\n[ERROR] Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        input("\nPress Enter to exit...")
        sys.exit(1)
