"""
SYNRIX Auto-Daemon Manager (Windows-Native)

Python control plane for SYNRIX engine process lifecycle.
Uses Windows-native subprocess.Popen with CREATE_NO_WINDOW flag.

This is the correct Windows approach:
- No shell scripts (.bat, .ps1)
- No shell=True
- No cmd.exe or powershell.exe
- Python owns the process lifecycle

Usage:
    from synrix.auto_daemon import SynrixAutoDaemon
    
    # Auto-starts engine if needed
    daemon = SynrixAutoDaemon()
    client = daemon.get_client()
    node_id = client.add_node("TASK:test", "data")
"""

import subprocess
import os
import sys
import time
import platform
from typing import Optional, Dict, Any
from pathlib import Path


class SynrixAutoDaemon:
    """
    Auto-daemon manager for SYNRIX engine.
    
    Windows-native approach: Spawns process directly using subprocess.Popen
    with CREATE_NO_WINDOW flag. No shell scripts, no shell=True.
    
    This is how Docker Desktop, Redis Windows ports, Postgres installers work.
    """
    
    def __init__(
        self,
        engine_path: Optional[str] = None,
        port: int = 6334,
        lattice_path: Optional[str] = None,
        auto_start: bool = True
    ):
        """
        Initialize auto-daemon manager.
        
        Args:
            engine_path: Path to synrix.exe (auto-detected if None)
            port: Server port (default: 6334)
            lattice_path: Path to lattice file (default: ~/.synrix.lattice)
            auto_start: If True, automatically start engine if not running
        """
        self.port = port
        self.process: Optional[subprocess.Popen] = None
        self.engine_path = engine_path or self._find_engine()
        self.lattice_path = lattice_path or self._default_lattice_path()
        self.auto_start = auto_start
        
        if auto_start:
            self._ensure_running()
    
    def _find_engine(self) -> str:
        """Find synrix.exe in common locations"""
        # Check environment variable first
        env_path = os.environ.get("SYNRIX_ENGINE_PATH")
        if env_path and os.path.exists(env_path):
            return env_path
        
        # Check relative to Python SDK
        current_file = Path(__file__).resolve()
        python_sdk_dir = current_file.parent
        project_root = python_sdk_dir.parent.parent
        
        # Common locations
        search_paths = [
            # Bundled with package
            python_sdk_dir / "synrix.exe",
            python_sdk_dir.parent / "bin" / "synrix.exe",
            # Build directories
            project_root / "build" / "windows" / "build" / "Release" / "synrix.exe",
            project_root / "build" / "windows" / "build" / "Debug" / "synrix.exe",
            project_root / "build" / "windows" / "build_free_tier" / "bin" / "synrix.exe",
            # Root directory
            project_root / "synrix.exe",
        ]
        
        for path in search_paths:
            if path.exists():
                return str(path)
        
        raise FileNotFoundError(
            "synrix.exe not found. Set SYNRIX_ENGINE_PATH environment variable "
            "or place synrix.exe in one of the standard locations."
        )
    
    def _default_lattice_path(self) -> str:
        """Get default lattice path"""
        home = Path.home()
        return str(home / ".synrix.lattice")
    
    def _is_running(self) -> bool:
        """Check if engine is running on the configured port"""
        try:
            import socket
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(0.1)
            result = sock.connect_ex(("localhost", self.port))
            sock.close()
            return result == 0
        except Exception:
            return False
    
    def _spawn_process(self) -> subprocess.Popen:
        """
        Spawn SYNRIX engine process using Windows-native approach.
        
        This is the correct way:
        - subprocess.Popen with list of arguments (not shell=True)
        - CREATE_NO_WINDOW flag on Windows (no console window)
        - close_fds=True for proper cleanup
        - No shell scripts, no .bat, no .ps1
        """
        if not os.path.exists(self.engine_path):
            raise FileNotFoundError(f"Engine not found: {self.engine_path}")
        
        # Build command arguments
        args = [
            self.engine_path,
            "--port", str(self.port),
            "--lattice-path", self.lattice_path
        ]
        
        # Windows-native process creation flags
        creation_flags = 0
        if platform.system() == "Windows":
            # CREATE_NO_WINDOW: Don't show console window
            # This is the key flag for Windows-native approach
            creation_flags = subprocess.CREATE_NO_WINDOW
        
        # Spawn process directly (no shell)
        process = subprocess.Popen(
            args,
            creationflags=creation_flags,
            close_fds=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        return process
    
    def _ensure_running(self):
        """Ensure engine is running, start if needed"""
        if self._is_running():
            return  # Already running
        
        if self.process is not None:
            # Check if our process is still alive
            if self.process.poll() is None:
                return  # Our process is still running
            # Process died, reset
            self.process = None
        
        # Start the engine
        self.process = self._spawn_process()
        
        # Wait for engine to be ready
        max_wait = 10  # 10 seconds
        for _ in range(max_wait * 10):
            if self._is_running():
                return
            time.sleep(0.1)
        
        # Check if process died
        if self.process.poll() is not None:
            stdout, stderr = self.process.communicate()
            error_msg = stderr.decode('utf-8', errors='ignore') if stderr else ""
            raise RuntimeError(
                f"SYNRIX engine failed to start on port {self.port}. "
                f"Error: {error_msg[:200]}"
            )
        
        raise RuntimeError(
            f"SYNRIX engine failed to start on port {self.port} (timeout)"
        )
    
    def get_client(self):
        """Get SYNRIX client (auto-starts engine if needed)"""
        if self.auto_start:
            self._ensure_running()
        
        from .client import SynrixClient
        return SynrixClient(host="localhost", port=self.port)
    
    def stop(self):
        """Stop the engine process"""
        if self.process is not None:
            try:
                self.process.terminate()
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            except Exception:
                pass
            finally:
                self.process = None
    
    def restart(self):
        """Restart the engine"""
        self.stop()
        time.sleep(0.5)
        self._ensure_running()
    
    def __enter__(self):
        """Context manager entry"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit"""
        self.stop()


# Convenience function for simple usage
def get_synrix_client(
    port: int = 6334,
    lattice_path: Optional[str] = None,
    auto_start: bool = True
) -> Any:
    """
    Get SYNRIX client with auto-daemon management.
    
    This is the recommended way to use SYNRIX on Windows:
    - No shell scripts needed
    - No manual process management
    - Python owns the lifecycle
    
    Example:
        >>> from synrix.auto_daemon import get_synrix_client
        >>> client = get_synrix_client()
        >>> node_id = client.add_node("TASK:test", "data")
    """
    daemon = SynrixAutoDaemon(
        port=port,
        lattice_path=lattice_path,
        auto_start=auto_start
    )
    return daemon.get_client()
