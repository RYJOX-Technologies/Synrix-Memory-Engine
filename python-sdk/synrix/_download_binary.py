"""
Download SYNRIX binary during pip install
Automatically downloads the platform-specific binary from GitHub releases
"""

import os
import sys
import platform
import urllib.request
import urllib.error
from pathlib import Path


def get_platform_info():
    """Determine platform and architecture for binary download"""
    system = platform.system().lower()
    machine = platform.machine().lower()
    
    # Map platform to release asset name
    if system == "linux":
        if machine in ["aarch64", "arm64"]:
            return "linux-arm64"
        elif machine in ["x86_64", "amd64"]:
            return "linux-x86_64"
    elif system == "windows":
        if machine in ["x86_64", "amd64"]:
            return "windows-x64"
    elif system == "darwin":  # macOS
        if machine in ["arm64", "aarch64"]:
            return "macos-arm64"
        elif machine == "x86_64":
            return "macos-x86_64"
    
    return None


def get_binary_info(version="0.1.0"):
    """Get binary download URL and filename for current platform"""
    platform_name = get_platform_info()
    if not platform_name:
        return None, None, None
    
    # Determine binary name and extension
    if platform.system() == "Windows":
        binary_name = "libsynrix.dll"
        asset_name = f"synrix-server-free-tier-{version}-{platform_name}"
    else:
        binary_name = "libsynrix.so"
        asset_name = f"synrix-server-free-tier-{version}-{platform_name}"
    
    # GitHub releases URL
    # Option 1: Direct from GitHub releases
    github_url = f"https://github.com/synrix/synrix/releases/download/v{version}/{asset_name}"
    
    # Option 2: From your website/CDN (if you have one)
    # cdn_url = f"https://downloads.synrix.dev/v{version}/{asset_name}"
    
    return github_url, binary_name, asset_name


def get_install_location():
    """Get the best location to install the binary"""
    # Try to use site-packages/synrix/bin/ (package-specific)
    try:
        import synrix
        synrix_path = Path(synrix.__file__).parent
        bin_dir = synrix_path / "bin"
        bin_dir.mkdir(exist_ok=True)
        return bin_dir
    except:
        pass
    
    # Fallback: user's home directory
    home = Path.home()
    synrix_dir = home / ".synrix" / "bin"
    synrix_dir.mkdir(parents=True, exist_ok=True)
    return synrix_dir


def download_binary(version="0.1.0", verbose=False):
    """
    Download the SYNRIX binary for the current platform
    
    Args:
        version: SYNRIX version to download
        verbose: Print progress messages
    
    Returns:
        Path to downloaded binary, or None if failed
    """
    url, binary_name, asset_name = get_binary_info(version)
    
    if not url:
        if verbose:
            print(f"⚠️  Platform {platform.system()}/{platform.machine()} not supported for auto-download")
        return None
    
    install_dir = get_install_location()
    binary_path = install_dir / binary_name
    
    # Check if already downloaded
    if binary_path.exists():
        if verbose:
            print(f"✅ Binary already exists at {binary_path}")
        return str(binary_path)
    
    if verbose:
        print(f"📥 Downloading SYNRIX binary for {platform.system()}/{platform.machine()}...")
        print(f"   URL: {url}")
        print(f"   Destination: {binary_path}")
    
    try:
        # Download with progress
        def show_progress(block_num, block_size, total_size):
            if verbose and total_size > 0:
                downloaded = block_num * block_size
                percent = min(100, (downloaded * 100) // total_size)
                sys.stdout.write(f"\r   Progress: {percent}%")
                sys.stdout.flush()
        
        urllib.request.urlretrieve(url, binary_path, show_progress)
        
        if verbose:
            print(f"\n✅ Binary downloaded successfully to {binary_path}")
        
        # Make executable on Unix
        if platform.system() != "Windows":
            os.chmod(binary_path, 0o755)
        
        # Set environment variable so SDK can find it
        env_path = str(install_dir)
        if platform.system() == "Windows":
            # Add to PATH (user-level, requires user action)
            if verbose:
                print(f"\n💡 To use the binary, add to PATH:")
                print(f"   set PATH={env_path};%PATH%")
        else:
            # Add to LD_LIBRARY_PATH
            if verbose:
                print(f"\n💡 To use the binary, add to LD_LIBRARY_PATH:")
                print(f"   export LD_LIBRARY_PATH={env_path}:$LD_LIBRARY_PATH")
        
        return str(binary_path)
        
    except urllib.error.HTTPError as e:
        if verbose:
            print(f"\n❌ Download failed: HTTP {e.code}")
            print(f"   URL: {url}")
            print(f"   You may need to download manually from GitHub releases")
        return None
    except urllib.error.URLError as e:
        if verbose:
            print(f"\n❌ Download failed: {e.reason}")
            print(f"   Check your internet connection")
        return None
    except Exception as e:
        if verbose:
            print(f"\n❌ Download failed: {e}")
        return None


def post_install_download(verbose=True):
    """
    Post-install hook: Download binary after pip install
    
    This is called automatically by setuptools after installation.
    """
    # Get version from package
    try:
        import synrix
        version = synrix.__version__
    except:
        version = "0.1.0"
    
    # Download binary
    binary_path = download_binary(version, verbose=verbose)
    
    if binary_path:
        # Create a symlink or add to path detection
        # The existing _find_synrix_lib() will find it in the install location
        return binary_path
    
    return None


if __name__ == "__main__":
    # Test download
    print("Testing binary download...")
    result = post_install_download(verbose=True)
    if result:
        print(f"\n✅ Success! Binary at: {result}")
    else:
        print("\n❌ Download failed (this is OK if platform not supported)")
