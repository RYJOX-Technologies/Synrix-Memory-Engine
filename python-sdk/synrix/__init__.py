"""
SYNRIX Python Client SDK

Python client library for interacting with the SYNRIX knowledge system.
Requires connection to SYNRIX server (hosted service or self-hosted binary).
"""

__version__ = "0.1.0"

import os
import warnings

from .client import SynrixClient
from .mock import SynrixMockClient
from .exceptions import SynrixError, SynrixConnectionError, SynrixNotFoundError
from .agent_memory import SynrixMemory
from .agent_backend import SynrixAgentBackend, get_synrix_backend
from .engine import init as _engine_init, find_engine

__all__ = [
    "SynrixClient",
    "SynrixMockClient",
    "SynrixMemory",
    "SynrixAgentBackend",
    "get_synrix_backend",
    "SynrixError",
    "SynrixConnectionError",
    "SynrixNotFoundError",
    "init",
]


def init():
    """Initialize SYNRIX - check for engine and provide helpful error if missing.
    
    This function checks if the SYNRIX engine is available. If not found,
    it prints a helpful error message directing users to install it.
    
    Example:
        >>> import synrix
        >>> synrix.init()
        SYNRIX engine not found.
        Run: synrix install-engine
    
    Returns:
        bool: True if engine is found, False otherwise
    """
    engine_found, engine_path, error_msg = _engine_init()
    
    if not engine_found:
        print(error_msg)
        return False
    
    return True


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
