"""
SYNRIX Native Library Loader
============================

Handles platform-specific loading of libsynrix.dll/.so with proper
Windows DLL directory registration.

This eliminates the need for:
- Scripts to set environment variables
- SYNRIX_LIB_PATH environment variable
- PowerShell gymnastics
- Manual DLL path management

Usage:
    from synrix._native import load_synrix
    load_synrix()  # Called automatically on import
"""

import os
import ctypes
import sys
import platform
from pathlib import Path


# Global flag to prevent multiple loads
_loaded = False
_lib = None


def load_synrix():
    """
    Load the SYNRIX native library (libsynrix.dll/.so).
    
    On Windows, this uses os.add_dll_directory() to register the package
    directory for DLL resolution, which is required for Windows 10+ security.
    
    This follows the pattern used by NumPy, PyTorch, SQLite, etc.
    
    Returns:
        The loaded library object (ctypes.CDLL/WinDLL)
        
    Raises:
        OSError: If the library cannot be found or loaded
    """
    global _loaded, _lib
    
    # Reset if previous load failed
    if _loaded and _lib is None:
        _loaded = False
    
    if _loaded and _lib is not None:
        return _lib
    
    # Determine library name based on platform
    if platform.system() == "Windows":
        lib_names = ["libsynrix_free_tier.dll", "libsynrix.dll"]
        lib_ext = ".dll"
    else:
        lib_names = ["libsynrix_free_tier.so", "libsynrix.so"]
        lib_ext = ".so"
    
    # Find the package directory (where this file is located)
    package_dir = Path(__file__).parent.resolve()
    
    # Try each library name in the package directory
    for lib_name in lib_names:
        lib_path = package_dir / lib_name
        
        # Check if path exists
        exists = lib_path.exists()
        if not exists:
            continue
        
        # Windows: Register DLL directory BEFORE loading
        if platform.system() == "Windows":
            # CRITICAL: This is what makes it work on Windows 10+
            # Without this, Windows ignores the package directory
            os.add_dll_directory(str(package_dir))
            
            # Also add MinGW bin directory if it exists (for runtime DLLs)
            mingw_paths = [
                r"C:\msys64\mingw64\bin",
                r"C:\msys64\usr\bin",
                os.path.join(os.path.expanduser("~"), "msys64", "mingw64", "bin"),
            ]
            for mingw_path in mingw_paths:
                if os.path.exists(mingw_path):
                    os.add_dll_directory(mingw_path)
                    break
            
            # Load the DLL
            try:
                _lib = ctypes.WinDLL(str(lib_path))
                _loaded = True
                return _lib
            except OSError as e:
                # Store error for better diagnostics
                last_error = str(e)
                # Try next library name
                continue
        else:
            # Linux/macOS: Standard CDLL loading
            try:
                _lib = ctypes.CDLL(str(lib_path))
                _loaded = True
                return _lib
            except OSError as e:
                # Try next library name
                continue
    
    # If not found in package directory, try environment variable (fallback)
    synrix_lib_path = os.environ.get("SYNRIX_LIB_PATH")
    if synrix_lib_path:
        lib_path = Path(synrix_lib_path)
        if lib_path.exists():
            if platform.system() == "Windows":
                os.add_dll_directory(str(lib_path.parent))
                _lib = ctypes.WinDLL(str(lib_path))
            else:
                _lib = ctypes.CDLL(str(lib_path))
            _loaded = True
            return _lib
    
    # Library not found
    raise OSError(
        f"Could not find SYNRIX library. Tried:\n"
        f"  - {package_dir / lib_names[0]}\n"
        f"  - {package_dir / lib_names[1]}\n"
        f"  - SYNRIX_LIB_PATH environment variable\n\n"
        f"Please ensure libsynrix.dll (or libsynrix_free_tier.dll) is in:\n"
        f"  - {package_dir}\n"
        f"  - Or set SYNRIX_LIB_PATH environment variable"
    )


def get_lib():
    """
    Get the loaded library object.
    
    Returns:
        The loaded library object, or None if not loaded yet
    """
    return _lib


# Auto-load on import (for convenience)
# Note: We don't set _loaded=True on failure, so load_synrix() can be called again
try:
    load_synrix()
except OSError:
    # Don't fail on import - let individual modules handle it
    # Reset _loaded flag so load_synrix() can be called again
    _loaded = False
    _lib = None
    pass
