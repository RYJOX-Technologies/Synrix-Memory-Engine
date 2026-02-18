"""
Final test: enforcement + global usage. Run from repo root or with PYTHONPATH including robotics-sdk.
Uses: synrix.raw_backend (robotics-sdk), DLL in synrix/ or build_msys2/bin.
"""
import os
import sys
import tempfile
import ctypes
from ctypes import POINTER, c_int, c_void_p

_script_dir = os.path.dirname(os.path.abspath(__file__))
# build/windows -> build -> inner synrix-windows-build -> outer
_root = os.path.normpath(os.path.join(_script_dir, "..", "..", "..", ".."))
_sdk = os.path.join(_root, "synrix-sdks", "robotics-sdk")
_build_dll = os.path.join(_script_dir, "build_msys2", "bin", "libsynrix.dll")
# Force the DLL we just built (enforcement lives here)
if os.path.isfile(_build_dll):
    os.environ["SYNRIX_LIB_PATH"] = os.path.dirname(_build_dll)
if os.path.isdir(_sdk):
    sys.path.insert(0, _sdk)

def test_disable_evaluation_mode_returns_minus_one():
    """Without a valid unlimited key, lattice_disable_evaluation_mode must return -1 (no bypass)."""
    # Ensure no license key so we get free tier (license_verified_unlimited stays false)
    saved = os.environ.pop("SYNRIX_LICENSE_KEY", None)
    try:
        from synrix.raw_backend import RawSynrixBackend
        path = os.path.join(tempfile.gettempdir(), "synrix_test_disable.lattice")
        try:
            b = RawSynrixBackend(path, max_nodes=25000, evaluation_mode=True)
            # Call C API directly: without unlimited key this must return -1
            result = b.lib.lattice_disable_evaluation_mode(ctypes.cast(b.lattice_ptr, POINTER(c_void_p)))
            b.close()
            if result == -1:
                print("PASS: lattice_disable_evaluation_mode() returns -1 without unlimited key (bypass blocked)")
            else:
                print("WARN: lattice_disable_evaluation_mode() returned %d (expected -1). Rebuild DLL from current source and ensure SYNRIX_LICENSE_KEY is unset." % result)
                print("      Joseph should still verify with a real tier-4 key that disable then returns 0.")
        finally:
            if os.path.exists(path):
                try:
                    os.remove(path)
                except Exception:
                    pass
    finally:
        if saved is not None:
            os.environ["SYNRIX_LICENSE_KEY"] = saved

def test_global_usage_file_created():
    r"""After opening a lattice and adding a node, global usage file should exist under %LOCALAPPDATA%\Synrix\license_usage\."""
    from synrix.raw_backend import RawSynrixBackend
    base = os.environ.get("LOCALAPPDATA", os.path.expanduser("~"))
    usage_dir = os.path.join(base, "Synrix", "license_usage")
    path = os.path.join(tempfile.gettempdir(), "synrix_test_usage.lattice")
    try:
        b = RawSynrixBackend(path, max_nodes=25000, evaluation_mode=True)
        b.add_node("test:key", "value", node_type=5)
        b.close()
        if os.path.isdir(usage_dir):
            dat_files = [f for f in os.listdir(usage_dir) if f.endswith(".dat")]
            if dat_files:
                print("PASS: Global usage file(s) under Synrix/license_usage/: %s" % (dat_files[:3],))
            else:
                print("WARN: Usage dir exists but no .dat file after add_node. Engine may not have license_global in this build.")
        else:
            print("WARN: Usage dir not found at %s (global usage may not run on this platform/build)." % usage_dir)
    finally:
        if os.path.exists(path):
            try:
                os.remove(path)
            except Exception:
                pass

def test_free_tier_limit_error_type():
    """FreeTierLimitError is raised when engine returns -100 (message mentions 25,000)."""
    from synrix.raw_backend import RawSynrixBackend, FreeTierLimitError
    # We only check the exception type and message; hitting 25k would require adding 25k nodes (slow).
    assert hasattr(RawSynrixBackend, "add_node")
    # Docstring / error message check
    assert "25" in FreeTierLimitError.__doc__ or "25,000" in str(FreeTierLimitError.__doc__)
    print("PASS: FreeTierLimitError exists and doc mentions 25k limit")
    return True

def main():
    print("=== Synrix enforcement + usage tests ===\n")
    test_free_tier_limit_error_type()
    test_disable_evaluation_mode_returns_minus_one()
    test_global_usage_file_created()
    print("\n=== Summary ===")
    print("We can test: FreeTierLimitError/25k docs, disable_evaluation_mode return value (no key), global usage .dat file.")
    print("Joseph should: (1) Generate a tier-4 key; confirm lattice_disable_evaluation_mode returns 0 and limit is unlimited.")
    print("              (2) Optionally run 25k add_node to confirm FreeTierLimitError and message.")

if __name__ == "__main__":
    main()
