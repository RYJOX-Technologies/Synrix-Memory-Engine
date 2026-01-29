"""
SYNRIX Python Client SDK

Python client library for interacting with the SYNRIX knowledge system.
Requires connection to SYNRIX server (hosted service or self-hosted binary).
"""

__version__ = "0.1.0"

import os
import warnings

# Load native library early (Windows DLL directory registration)
# This must happen before any module tries to use the DLL
try:
    from ._native import load_synrix
    load_synrix()  # Register DLL directory on Windows
except (ImportError, OSError):
    # Native loader not available or DLL not found - modules will handle it
    pass

from .client import SynrixClient
from .mock import SynrixMockClient
from .exceptions import SynrixError, SynrixConnectionError, SynrixNotFoundError
from .agent_memory import SynrixMemory
from .agent_backend import SynrixAgentBackend, get_synrix_backend

# Robotics SDK (optional)
try:
    from .robotics import RoboticsNexus
except ImportError:
    RoboticsNexus = None

# Windows-native auto-daemon (recommended for Windows)
try:
    from .auto_daemon import SynrixAutoDaemon, get_synrix_client
    __all__ = [
        "SynrixClient",
        "SynrixMockClient",
        "SynrixMemory",
        "SynrixAgentBackend",
        "get_synrix_backend",
        "SynrixAutoDaemon",
        "get_synrix_client",  # Recommended for Windows
        "SynrixError",
        "SynrixConnectionError",
        "SynrixNotFoundError",
    ]
    if RoboticsNexus is not None:
        __all__.append("RoboticsNexus")
except ImportError:
    # Fallback if auto_daemon not available
    __all__ = [
        "SynrixClient",
        "SynrixMockClient",
        "SynrixMemory",
        "SynrixAgentBackend",
        "get_synrix_backend",
        "SynrixError",
        "SynrixConnectionError",
        "SynrixNotFoundError",
    ]
    if RoboticsNexus is not None:
        __all__.append("RoboticsNexus")

# Post-install check: Warn if server not configured
def _check_server_config():
    """Check if SYNRIX server is configured, warn if not."""
    server_url = os.getenv("SYNRIX_SERVER_URL")
    if not server_url:
        # Only warn if this is the first import and no server URL is set
        # Don't spam warnings on every import
        pass  # Silent by default - users will see connection errors if server not available

# Run check on import (only once)
_check_server_config()
