"""
Pytest fixtures for agent-memory SDK tests.

When Synrix DLL is available (SYNRIX_LIB_PATH set), tests use the real backend.
Otherwise tests that require the backend are skipped.
"""

import os
import tempfile
import pytest


@pytest.fixture
def temp_lattice_path():
    """A temporary lattice file path that is removed after the test."""
    fd, path = tempfile.mkstemp(suffix=".lattice")
    os.close(fd)
    try:
        if os.path.exists(path):
            os.remove(path)
    except Exception:
        pass
    yield path
    try:
        if os.path.exists(path):
            os.remove(path)
    except Exception:
        pass


@pytest.fixture
def ai_memory(temp_lattice_path):
    """
    AIMemory instance with real backend if DLL is available.
    Skips the test if backend cannot be loaded or init fails.
    """
    try:
        from synrix.ai_memory import AIMemory
        memory = AIMemory(lattice_path=temp_lattice_path)
    except Exception as e:
        pytest.skip("AIMemory init failed (no DLL?): {}".format(e))
    if memory.backend is None:
        pytest.skip("Raw backend not available (no Synrix DLL)")
    try:
        yield memory
    finally:
        memory.close()
