#!/usr/bin/env python3
"""
SYNRIX Free Tier Installer
Double-click to install SYNRIX on Windows
"""

import sys
import os
import subprocess
import shutil
from pathlib import Path

def print_header(text):
    """Print formatted header"""
    print("\n" + "=" * 60)
    print(f"  {text}")
    print("=" * 60 + "\n")

def check_python():
    """Check Python version"""
    version = sys.version_info
    if version.major < 3 or (version.major == 3 and version.minor < 8):
        print("ERROR: Python 3.8+ required!")
        print(f"  Current version: {version.major}.{version.minor}.{version.micro}")
        input("\nPress Enter to exit...")
        sys.exit(1)
    print(f"[OK] Python {version.major}.{version.minor}.{version.micro} detected")
    return True

def find_python_exe():
    """Find Python executable"""
    return sys.executable

def install_package(package_dir):
    """Install SYNRIX package"""
    print("Installing SYNRIX package...")
    
    try:
        # Use pip to install in development mode
        python_exe = find_python_exe()
        result = subprocess.run(
            [python_exe, "-m", "pip", "install", "-e", str(package_dir)],
            capture_output=True,
            text=True,
            check=True
        )
        print("[OK] Package installed successfully")
        return True
    except subprocess.CalledProcessError as e:
        print(f"ERROR: Installation failed!")
        print(f"  {e.stderr}")
        return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False

def test_installation():
    """Test that SYNRIX can be imported"""
    print("Testing installation...")
    
    try:
        # Test import
        result = subprocess.run(
            [sys.executable, "-c", 
             "from synrix.ai_memory import get_ai_memory; "
             "m = get_ai_memory(); "
             "m.add('TEST:install', 'SYNRIX installed!'); "
             "print(f'[OK] Installation test passed. Found {len(m.query(\"TEST:\"))} nodes')"],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            print(result.stdout.strip())
            return True
        else:
            print(f"ERROR: Test failed!")
            print(f"  {result.stderr}")
            return False
    except Exception as e:
        print(f"ERROR: Test failed: {e}")
        return False

def create_example_script(install_dir):
    """Create example usage script"""
    example_script = install_dir / "example_usage.py"
    example_content = '''"""
SYNRIX Example Usage
"""

from synrix.ai_memory import get_ai_memory

# Get memory instance
memory = get_ai_memory()

# Store information
memory.add("PROJECT:name", "My Project")
memory.add("FIX:bug_123", "Fixed null pointer")

# Query by prefix
results = memory.query("FIX:")
for r in results:
    print(f"{r['name']}: {r['data']}")

# Get count
count = memory.count()
print(f"\\nTotal nodes: {count}")
'''
    
    try:
        example_script.write_text(example_content)
        print(f"[OK] Example script created: {example_script}")
        return True
    except Exception as e:
        print(f"WARNING: Could not create example script: {e}")
        return False

def main():
    """Main installer function"""
    print_header("SYNRIX Free Tier Installer")
    
    # Get package directory (where installer is located)
    package_dir = Path(__file__).parent.resolve()
    
    print(f"Package directory: {package_dir}")
    print(f"Installing to: {sys.prefix}")
    print()
    
    # Check Python
    if not check_python():
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    # Check if package exists
    setup_py = package_dir / "setup.py"
    if not setup_py.exists():
        print("ERROR: setup.py not found!")
        print(f"  Expected at: {setup_py}")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    # Install package
    if not install_package(package_dir):
        print("\nInstallation failed. Please check the errors above.")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    # Test installation
    if not test_installation():
        print("\nInstallation completed but test failed.")
        print("SYNRIX may still work - try importing it manually.")
        input("\nPress Enter to exit...")
        sys.exit(1)
    
    # Create example script
    create_example_script(package_dir)
    
    # Success
    print_header("Installation Complete!")
    print("SYNRIX has been successfully installed.")
    print("\nYou can now use SYNRIX in your Python scripts:")
    print("  from synrix.ai_memory import get_ai_memory")
    print("  memory = get_ai_memory()")
    print("\nSee example_usage.py for a complete example.")
    print("\nDocumentation:")
    print("  - README.md - Full documentation")
    print("  - QUICK_START.md - 5-minute guide")
    print("  - AI_AGENT_GUIDE.md - Comprehensive examples")
    
    input("\nPress Enter to exit...")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\nInstallation cancelled.")
        sys.exit(1)
    except Exception as e:
        print(f"\nERROR: Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        input("\nPress Enter to exit...")
        sys.exit(1)
