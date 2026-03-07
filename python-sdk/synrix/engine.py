"""
SYNRIX Engine Management

Handles engine detection, installation, and management.
"""

import os
import sys
import platform
import subprocess
import shutil
import zipfile
import tempfile
from pathlib import Path
from typing import Optional, Tuple
import requests
from .exceptions import SynrixError


# Engine download: default to GitHub release tag Synrix-Memory-Engine
# Windows: downloads synrix-windows-release.zip, extracts and uses the .exe inside
# Override env: SYNRIX_ENGINE_DOWNLOAD_BASE_URL, SYNRIX_ENGINE_DOWNLOAD_FILENAME (asset to download),
#               SYNRIX_ENGINE_FILENAME (installed exe name), SYNRIX_ENGINE_VERSION
GITHUB_RELEASES = "https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases"
_DEFAULT_TAG = "Synrix-Memory-Engine"  # https://github.com/.../releases/tag/Synrix-Memory-Engine
ENGINE_VERSION = os.getenv("SYNRIX_ENGINE_VERSION", "0.5.0").strip() or "0.5.0"
_DEFAULT_DOWNLOAD_BASE = f"{GITHUB_RELEASES}/download/{_DEFAULT_TAG}"
ENGINE_BASE_URL = os.getenv("SYNRIX_ENGINE_DOWNLOAD_BASE_URL", _DEFAULT_DOWNLOAD_BASE).strip() or _DEFAULT_DOWNLOAD_BASE


def get_platform_string() -> str:
    """Get platform string for engine binary."""
    system = platform.system().lower()
    machine = platform.machine().lower()
    
    if system == "linux":
        if machine in ("x86_64", "amd64"):
            return "linux-x86_64"
        elif machine in ("aarch64", "arm64"):
            return "linux-arm64"
    elif system == "darwin":
        if machine in ("x86_64", "amd64"):
            return "darwin-x86_64"
        elif machine in ("arm64", "aarch64"):
            return "darwin-arm64"
    elif system == "windows":
        if machine in ("x86_64", "amd64"):
            return "windows-x86_64"
    
    raise SynrixError(f"Unsupported platform: {system} {machine}")


def get_engine_filename() -> str:
    """Get engine binary filename for current platform. Override with SYNRIX_ENGINE_FILENAME."""
    custom = os.getenv("SYNRIX_ENGINE_FILENAME", "").strip()
    if custom:
        return custom
    platform_str = get_platform_string()
    if platform_str.startswith("windows"):
        # Zip contains synrix.exe; we install as synrix.exe
        return "synrix.exe"
    else:
        return f"synrix-server-evaluation-{ENGINE_VERSION}-{platform_str}"


def get_engine_path() -> Path:
    """Get path where engine should be installed."""
    # Use user's home directory for engine storage
    home = Path.home()
    engine_dir = home / ".synrix" / "bin"
    engine_dir.mkdir(parents=True, exist_ok=True)
    return engine_dir / get_engine_filename()


def _get_download_filename() -> str:
    """Asset filename to download (may be .zip on Windows). Override with SYNRIX_ENGINE_DOWNLOAD_FILENAME."""
    custom = os.getenv("SYNRIX_ENGINE_DOWNLOAD_FILENAME", "").strip()
    if custom:
        return custom
    if platform.system().lower() == "windows":
        return "synrix-windows-release.zip"
    return get_engine_filename()


def find_engine() -> Optional[Path]:
    """Find SYNRIX engine binary.
    
    Checks in order:
    1. User's ~/.synrix/bin directory
    2. Current directory
    3. PATH environment variable
    
    Returns:
        Path to engine binary if found, None otherwise
    """
    engine_name = get_engine_filename()
    
    # Check ~/.synrix/bin
    engine_path = get_engine_path()
    if engine_path.exists() and engine_path.is_file():
        if os.access(engine_path, os.X_OK):
            return engine_path
    
    # Check current directory
    current_dir = Path.cwd() / engine_name
    if current_dir.exists() and current_dir.is_file():
        if os.access(current_dir, os.X_OK):
            return current_dir
    
    # Check PATH
    engine_in_path = shutil.which(engine_name)
    if engine_in_path:
        return Path(engine_in_path)
    
    # Also check for synrix-server-evaluation without version
    engine_in_path = shutil.which("synrix-server-evaluation")
    if engine_in_path:
        return Path(engine_in_path)
    
    return None


def check_engine_running(port: int = 6334) -> bool:
    """Check if SYNRIX engine is already running on the given port."""
    try:
        import socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(1)
        result = sock.connect_ex(("localhost", port))
        sock.close()
        return result == 0
    except Exception:
        return False


def download_engine(progress: bool = True) -> Path:
    """Download SYNRIX engine binary (or zip on Windows; then extract and use .exe inside).
    
    Args:
        progress: Show download progress
        
    Returns:
        Path to downloaded engine binary
        
    Raises:
        SynrixError: If download fails
    """
    platform_str = get_platform_string()
    download_filename = _get_download_filename()
    engine_path = get_engine_path()
    
    if not ENGINE_BASE_URL:
        raise SynrixError(
            "Engine auto-download is not configured. Set SYNRIX_ENGINE_DOWNLOAD_BASE_URL to a base URL, "
            f"or download the engine manually from {GITHUB_RELEASES}"
        )
    download_url = f"{ENGINE_BASE_URL.rstrip('/')}/{download_filename}"
    
    print(f"Downloading SYNRIX engine for {platform_str}...")
    
    try:
        # Use GitHub API to get asset URL first (direct release URL often 404s from scripts)
        if "github.com" in ENGINE_BASE_URL and "releases/download" in ENGINE_BASE_URL:
            api_url = "https://api.github.com/repos/RYJOX-Technologies/Synrix-Memory-Engine/releases/tags/Synrix-Memory-Engine"
            r = requests.get(api_url, timeout=15, headers={"Accept": "application/vnd.github+json", "User-Agent": "Synrix-Engine-Installer/1.0"})
            r.raise_for_status()
            data = r.json()
            assets = data.get("assets") or []
            asset = next((a for a in assets if a.get("name") == download_filename), None)
            if not asset:
                raise SynrixError(f"Asset {download_filename} not found in release. Available: {[a.get('name') for a in assets]}")
            asset_url = asset.get("url")
            if not asset_url:
                raise SynrixError("Release asset has no URL")
            print(f"URL: GitHub release asset (via API)")
            response = requests.get(asset_url, stream=True, timeout=90, headers={"Accept": "application/octet-stream", "User-Agent": "Synrix-Engine-Installer/1.0"}, allow_redirects=True)
            response.raise_for_status()
        else:
            print(f"URL: {download_url}")
            response = requests.get(download_url, stream=True, timeout=60, headers={"Accept": "application/octet-stream", "User-Agent": "Synrix-Engine-Installer/1.0"}, allow_redirects=True)
            response.raise_for_status()
        
        total_size = int(response.headers.get("content-length", 0))
        downloaded = 0
        
        is_zip = download_filename.lower().endswith(".zip")
        if is_zip:
            dest = Path(tempfile.gettempdir()) / download_filename
        else:
            dest = engine_path
        
        with open(dest, "wb") as f:
            for chunk in response.iter_content(chunk_size=65536):
                if chunk:
                    f.write(chunk)
                    downloaded += len(chunk)
                    if progress and total_size > 0:
                        percent = (downloaded / total_size) * 100
                        print(f"\rProgress: {percent:.1f}% ({downloaded}/{total_size} bytes)", end="", flush=True)
        
        if progress:
            print()  # New line after progress
        
        if is_zip:
            # Extract zip and find the .exe; copy exe + all files in same dir (DLLs) to engine_path.parent
            extract_dir = dest.with_suffix("")
            extract_dir.mkdir(parents=True, exist_ok=True)
            try:
                with zipfile.ZipFile(dest, "r") as zf:
                    zf.extractall(extract_dir)
                exes = list(Path(extract_dir).rglob("*.exe"))
                # Prefer synrix.exe, then any exe with "synrix" in the name
                synrix_exe = next((p for p in exes if p.name.lower() == "synrix.exe"), None)
                synrix_exes = [p for p in exes if "synrix" in p.name.lower()]
                chosen = ([synrix_exe] if synrix_exe else synrix_exes) or exes
                if not chosen:
                    raise SynrixError("No .exe found in synrix-windows-release.zip")
                src_exe = chosen[0]
                engine_dir = engine_path.parent
                # Copy entire folder so DLLs sit next to the exe (Windows needs them in same dir)
                for f in src_exe.parent.iterdir():
                    if f.is_file():
                        shutil.copy2(f, engine_dir / f.name)
            finally:
                dest.unlink(missing_ok=True)
                shutil.rmtree(extract_dir, ignore_errors=True)
        else:
            # Make executable (Unix-like systems)
            if not platform_str.startswith("windows"):
                os.chmod(engine_path, 0o755)
        
        print(f"[OK] Engine downloaded to: {engine_path}")
        return engine_path
        
    except requests.exceptions.RequestException as e:
        raise SynrixError(f"Failed to download engine: {e}")
    except Exception as e:
        # Clean up partial download
        if engine_path.exists():
            engine_path.unlink(missing_ok=True)
        raise SynrixError(f"Failed to install engine: {e}")


def verify_engine(engine_path: Path) -> bool:
    """Verify that engine binary runs (--version for server builds; fallback run for CLI-only)."""
    try:
        result = subprocess.run(
            [str(engine_path), "--version"],
            capture_output=True,
            timeout=5,
            text=True
        )
        if result.returncode == 0:
            return True
        # Windows release zip may contain CLI-only binary (no --version); accept "runs and prints something"
        if sys.platform == "win32" and (result.stdout or result.stderr):
            return True
    except Exception:
        pass
    try:
        r = subprocess.run(
            [str(engine_path)],
            capture_output=True,
            timeout=5,
            text=True,
            cwd=engine_path.parent,
        )
        if r.returncode in (0, 1) and (r.stdout or r.stderr):
            return True
    except Exception:
        pass
    return False


def install_engine(force: bool = False) -> Path:
    """Install SYNRIX engine binary.
    
    Args:
        force: Force re-download even if engine exists
        
    Returns:
        Path to installed engine binary
        
    Raises:
        SynrixError: If installation fails
    """
    engine_path = get_engine_path()
    
    # Check if already installed
    if engine_path.exists() and not force:
        print(f"Engine already installed at: {engine_path}")
        if verify_engine(engine_path):
            print("[OK] Engine verified and ready to use")
            return engine_path
        else:
            print("[!] Existing engine failed verification, re-downloading...")
            engine_path.unlink()
    
    # Download engine
    try:
        engine_path = download_engine()
        
        # Verify
        if verify_engine(engine_path):
            print("[OK] Engine installed and verified")
            return engine_path
        else:
            raise SynrixError("Downloaded engine failed verification")
            
    except Exception as e:
        raise SynrixError(f"Failed to install engine: {e}")


def init() -> Tuple[bool, Optional[Path], Optional[str]]:
    """Initialize SYNRIX - check for engine and return status.
    
    Returns:
        Tuple of (engine_found, engine_path, error_message)
    """
    engine_path = find_engine()
    
    if engine_path:
        return True, engine_path, None
    else:
        error_msg = (
            "SYNRIX engine not found.\n"
            "Run: synrix install-engine"
        )
        return False, None, error_msg
