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
    # Use multiple methods to find the package directory for robustness
    package_dir = None
    possible_dirs = []
    
    # Method 1: Use __file__ (most reliable)
    try:
        if __file__:
            package_dir = Path(__file__).parent.resolve()
            possible_dirs.append(("__file__", package_dir))
    except (NameError, AttributeError):
        pass
    
    # Method 2: Try to find synrix package in sys.path
    for path_str in sys.path:
        if path_str:
            try:
                path = Path(path_str).resolve()
                # Check if this path contains synrix package
                synrix_dir = path / "synrix"
                if synrix_dir.exists() and (synrix_dir / "_native.py").exists():
                    possible_dirs.append(("sys.path", synrix_dir))
                    if package_dir is None:
                        package_dir = synrix_dir
            except (OSError, ValueError):
                continue
    
    # Method 3: Check current working directory
    try:
        cwd = Path.cwd()
        synrix_dir = cwd / "synrix"
        if synrix_dir.exists() and (synrix_dir / "_native.py").exists():
            possible_dirs.append(("cwd", synrix_dir))
            if package_dir is None:
                package_dir = synrix_dir
    except OSError:
        pass
    
    # If still no package_dir, use __file__ as fallback even if it might not work
    if package_dir is None:
        try:
            package_dir = Path(__file__).parent.resolve()
        except (NameError, AttributeError):
            raise OSError(
                "Could not determine SYNRIX package directory. "
                "Please ensure the synrix package is properly installed or in your Python path."
            )
    
    # Environment override: use explicit DLL path when provided
    env_lib_path = os.environ.get("SYNRIX_LIB_PATH")
    if env_lib_path:
        env_path = Path(env_lib_path).expanduser().resolve()
        if not env_path.exists():
            raise OSError(f"SYNRIX_LIB_PATH is set but file does not exist: {env_path}")
        if platform.system() == "Windows":
            os.add_dll_directory(str(env_path.parent))
        try:
            _lib = ctypes.WinDLL(str(env_path)) if platform.system() == "Windows" else ctypes.CDLL(str(env_path))
            _loaded = True
            return _lib
        except OSError as e:
            raise OSError(f"Failed to load SYNRIX_LIB_PATH DLL ({env_path}): {e}")

    # Try each library name in the package directory
    tried_paths = []
    for lib_name in lib_names:
        lib_path = package_dir / lib_name
        tried_paths.append(str(lib_path))
        
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
                # On Windows, we need to ensure DLL dependencies can be found
                # The runtime DLLs (libgcc_s_seh-1.dll, etc.) should be in the same directory
                _lib = ctypes.WinDLL(str(lib_path))
                _loaded = True
                return _lib
            except OSError as e:
                # Store error for better diagnostics
                last_error = str(e)
                error_str = str(e).lower()
                
                # Check if it's a dependency issue
                if "dependent" in error_str or "dll" in error_str or "not found" in error_str:
                    # Check for missing MinGW runtime DLLs
                    missing_deps = []
                    for dep_name in ["libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"]:
                        dep_path = package_dir / dep_name
                        if not dep_path.exists():
                            missing_deps.append(dep_name)
                    
                    if missing_deps:
                        raise OSError(
                            f"Failed to load {lib_name}: {e}\n\n"
                            f"Missing dependencies: {', '.join(missing_deps)}\n"
                            f"Expected location: {package_dir}\n"
                            f"Please ensure all MinGW runtime DLLs are in the same directory as libsynrix.dll"
                        )
                    else:
                        # DLLs exist but still can't load - show the actual error
                        raise OSError(
                            f"Failed to load {lib_name} from {lib_path}:\n"
                            f"  {e}\n\n"
                            f"All required DLLs are present in {package_dir}, but loading failed.\n"
                            f"This may indicate:\n"
                            f"  1. Architecture mismatch (32-bit vs 64-bit)\n"
                            f"  2. Corrupted DLL file\n"
                            f"  3. Windows security blocking the DLL\n\n"
                            f"Try setting SYNRIX_LIB_PATH environment variable:\n"
                            f"  set SYNRIX_LIB_PATH={lib_path}"
                        )
                else:
                    # Unknown error - show it anyway
                    raise OSError(
                        f"Failed to load {lib_name} from {lib_path}:\n"
                        f"  {e}\n\n"
                        f"Try setting SYNRIX_LIB_PATH environment variable:\n"
                        f"  set SYNRIX_LIB_PATH={lib_path}"
                    )
                # If we get here, try next library name
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
    
    # Library not found - provide detailed diagnostics
    error_msg = f"Could not find SYNRIX library.\n\n"
    error_msg += f"Package directory: {package_dir}\n"
    error_msg += f"Package directory exists: {package_dir.exists()}\n\n"
    error_msg += f"Tried paths:\n"
    for path in tried_paths:
        error_msg += f"  - {path} (exists: {Path(path).exists()})\n"
    error_msg += f"\nPossible package directories found:\n"
    for method, path in possible_dirs:
        error_msg += f"  - {method}: {path}\n"
    error_msg += f"\nSYNRIX_LIB_PATH: {os.environ.get('SYNRIX_LIB_PATH', 'not set')}\n"
    error_msg += f"Current working directory: {os.getcwd()}\n"
    error_msg += f"Python sys.path:\n"
    for i, path in enumerate(sys.path[:5]):  # Show first 5
        error_msg += f"  [{i}] {path}\n"
    error_msg += f"\nTroubleshooting:\n"
    error_msg += f"  1. Verify libsynrix.dll exists in: {package_dir}\n"
    error_msg += f"  2. Set SYNRIX_LIB_PATH environment variable to full DLL path\n"
    error_msg += f"  3. Ensure you're running from the synrix_unlimited directory\n"
    error_msg += f"  4. Check that all MinGW runtime DLLs are present:\n"
    error_msg += f"     - libgcc_s_seh-1.dll\n"
    error_msg += f"     - libstdc++-6.dll\n"
    error_msg += f"     - libwinpthread-1.dll\n"
    
    raise OSError(error_msg)


def get_lib():
    """
    Get the loaded library object.
    
    Returns:
        The loaded library object, or None if not loaded yet
    """
    return _lib


# Auto-load on import (for convenience)
# Note: We don't set _loaded=True on failure, so load_synrix() can be called again
# Also suppress debug output by redirecting stderr (for production builds)
try:
    import os
    import sys
    # Redirect stderr to suppress debug output from DLL
    # This is a runtime solution - the DLL still has debug code, but output is suppressed
    if hasattr(sys, 'stderr'):
        # On Windows, redirect to nul; on Unix, redirect to /dev/null
        if sys.platform == 'win32':
            devnull = open(os.devnull, 'w')
        else:
            devnull = open('/dev/null', 'w')
        # Store original stderr
        _original_stderr = sys.stderr
        # Redirect (but allow errors to still show)
        # We'll restore stderr after DLL load
        sys.stderr = devnull
    load_synrix()
    # Restore stderr after load
    if hasattr(sys, 'stderr') and '_original_stderr' in globals():
        sys.stderr = _original_stderr
        devnull.close()
except OSError:
    # Restore stderr if load failed
    if hasattr(sys, 'stderr') and '_original_stderr' in globals():
        sys.stderr = _original_stderr
        if 'devnull' in locals():
            devnull.close()
    # Don't fail on import - let individual modules handle it
    # Reset _loaded flag so load_synrix() can be called again
    _loaded = False
    _lib = None
    pass
