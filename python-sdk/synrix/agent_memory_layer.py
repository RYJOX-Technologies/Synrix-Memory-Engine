"""
Agent Memory Layer – Custom memories, instant recall
====================================================
Single API for AI agents to store and recall memories using Synrix.
Uses our O(k), local-first architecture for instant recall.

Usage:
    from synrix.agent_memory_layer import get_agent_memory

    memory = get_agent_memory()
    memory.remember("preference:theme", "dark")
    memory.remember("fact:user_dog", {"name": "Max", "breed": "Lab"})
    value = memory.recall("preference:theme")
    items = memory.search("fact:")
"""

import os
import json
import time
from typing import Optional, Dict, Any, List

# Prefer raw backend (instant recall) when available
try:
    from .raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING
    RAW_AVAILABLE = True
except Exception:
    RawSynrixBackend = None
    LATTICE_NODE_LEARNING = 5
    RAW_AVAILABLE = False

from .agent_backend import get_synrix_backend


def _serialize(value: Any) -> str:
    if isinstance(value, str):
        return value
    return json.dumps(value)


def _deserialize(data: str) -> Any:
    if not data:
        return None
    try:
        return json.loads(data)
    except json.JSONDecodeError:
        return data


class AgentMemoryLayer:
    """
    Layer that lets AI agents store custom memories and recall them instantly.
    Uses Synrix (raw backend when available, else HTTP) for O(k) local storage.
    """

    def __init__(self, backend_type: str, backend: Any, lattice_path: Optional[str] = None):
        self.backend_type = backend_type
        self._backend = backend
        self._raw = backend_type == "raw"
        self._lattice_path = lattice_path

    def remember(self, key: str, value: Any, metadata: Optional[Dict] = None) -> bool:
        """
        Store a memory. Key can be a prefix-style name (e.g. preference:theme, fact:user_dog).
        Value can be str or any JSON-serializable dict.
        """
        payload = {
            "value": value,
            "metadata": metadata or {},
            "ts": time.time(),
        }
        data_str = json.dumps(payload)
        if len(data_str) > 511:
            data_str = data_str[:508] + '"}'
        try:
            if self._raw:
                self._backend.add_node(key, data_str, node_type=LATTICE_NODE_LEARNING)
                return True
            return self._backend.write(key, value, metadata) is not None
        except Exception:
            return False

    def recall(self, key: str) -> Optional[Any]:
        """Recall one memory by key (exact or prefix). Returns the stored value or None."""
        try:
            if self._raw:
                results = self._backend.find_by_prefix(key, limit=1, raw=False)
                if not results:
                    return None
                data = results[0].get("data") or ""
                if isinstance(data, bytes):
                    data = data.decode("utf-8", errors="replace")
                payload = json.loads(data) if data.strip().startswith("{") else {"value": data}
                return payload.get("value", data)
            result = self._backend.read(key)
            if not result:
                return None
            data = result.get("data") or result.get("payload", {})
            if isinstance(data, dict):
                return data.get("value")
            return data
        except Exception:
            return None

    def search(self, prefix: str, limit: int = 100) -> List[Dict[str, Any]]:
        """Search memories by prefix. Returns list of {key, value, metadata?, ts?}."""
        out = []
        try:
            if self._raw:
                results = self._backend.find_by_prefix(prefix, limit=limit, raw=False)
                for r in results:
                    name = r.get("name") or ""
                    if isinstance(name, bytes):
                        name = name.decode("utf-8", errors="replace")
                    data = r.get("data") or ""
                    if isinstance(data, bytes):
                        data = data.decode("utf-8", errors="replace")
                    try:
                        payload = json.loads(data) if data.strip().startswith("{") else {"value": data}
                    except json.JSONDecodeError:
                        payload = {"value": data}
                    out.append({
                        "key": name,
                        "value": payload.get("value", data),
                        "metadata": payload.get("metadata", {}),
                        "ts": payload.get("ts"),
                    })
                return out
            results = self._backend.query_prefix(prefix, limit=limit)
            for r in results:
                key = r.get("key", r.get("name", ""))
                data = r.get("data", r.get("payload", r))
                if isinstance(data, str):
                    try:
                        data = json.loads(data)
                    except json.JSONDecodeError:
                        data = {"value": data}
                out.append({
                    "key": key,
                    "value": data.get("value", data) if isinstance(data, dict) else data,
                    "metadata": data.get("metadata", {}) if isinstance(data, dict) else {},
                    "ts": data.get("ts") if isinstance(data, dict) else None,
                })
            return out
        except Exception:
            return []

    def status(self) -> Dict[str, Any]:
        """Backend status for debugging."""
        return {
            "backend_type": self.backend_type,
            "lattice_path": self._lattice_path,
        }


def get_agent_memory(
    lattice_path: Optional[str] = None,
    collection: str = "agent_memory",
    use_http_fallback: bool = True,
) -> AgentMemoryLayer:
    """
    Get the agent memory layer. Prefers raw backend (instant recall) when
    lattice_path and libsynrix are available; otherwise uses HTTP backend.

    Args:
        lattice_path: For raw backend, path to .lattice file.
                      Default: ~/.synrix/agent_memory.lattice
        collection: For HTTP backend, collection name.
        use_http_fallback: If True, fall back to HTTP when raw is unavailable.
    """
    if lattice_path is None:
        lattice_path = os.path.expanduser("~/.synrix/agent_memory.lattice")
    os.makedirs(os.path.dirname(lattice_path) or ".", exist_ok=True)

    if RAW_AVAILABLE and RawSynrixBackend:
        try:
            backend = RawSynrixBackend(lattice_path)
            return AgentMemoryLayer("raw", backend, lattice_path)
        except Exception:
            if not use_http_fallback:
                raise

    backend = get_synrix_backend(collection=collection)
    return AgentMemoryLayer("http", backend, None)
