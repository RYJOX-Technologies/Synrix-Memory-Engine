# Usage reporting to Joseph's backend

Joseph added support so the backend can store `current_usage` (node count) and send warning emails. **We do not currently call `validate-license`** — the engine is offline-first. To enable his flow, the SDK needs to send usage on startup (and optionally periodically).

---

## What Joseph built

1. **validate-license** — now accepts optional `current_usage`. If the SDK sends it, the backend updates the profile.
2. **update-usage** — standalone endpoint: `POST { license_key, current_usage }` for flexible reporting.
3. Defaults updated to 25k.

---

## What Ryan needs to add

**One small helper** (fire-and-forget, don’t block startup) that POSTs to his backend, plus **one or two call sites** where we have `license_key` + `current_usage` (+ optional `hardware_id`).

---

## 1. Shared helper (add once, use from any SDK)

Put this in a small module (e.g. `synrix_rag/usage_report.py` in RAG SDK, or a shared `synrix/usage_report.py` in robotics/agent-memory). Configure the base URL via env (e.g. `SYNRIX_BACKEND_URL`) or use Joseph’s URL.

```python
import os
import threading
import requests

# Joseph's Supabase project URL for Edge Functions
SYNRIX_VALIDATE_URL = os.getenv(
    "SYNRIX_VALIDATE_LICENSE_URL",
    "https://jdznymdwjvcmhsslewfz.supabase.co/functions/v1/validate-license"
)
SYNRIX_UPDATE_USAGE_URL = os.getenv(
    "SYNRIX_UPDATE_USAGE_URL",
    "https://jdznymdwjvcmhsslewfz.supabase.co/functions/v1/update-usage"
)


def report_usage_to_backend(license_key: str, current_usage: int, hardware_id: str = None):
    """
    Report current node usage to backend (fire-and-forget).
    Enables warning emails at 85% and over-limit emails.
    """
    if not license_key or current_usage is None:
        return
    payload = {"license_key": license_key, "current_usage": current_usage}
    if hardware_id:
        payload["hardware_id"] = hardware_id

    def _post():
        try:
            requests.post(SYNRIX_VALIDATE_URL, json=payload, timeout=5)
        except Exception:
            pass

    threading.Thread(target=_post, daemon=True).start()
```

---

## 2. Where to call it

### Option A — RAG SDK (recommended first)

When the RAG memory is initialized and we have a license key, report once:

- In **`synrix_rag/rag_memory.py`** (e.g. in `__init__` after `license_manager` is set, or in a method that runs on first use), add:

```python
# After we have self.synrix_memory and self.license_manager.license_key:
from synrix_rag.usage_report import report_usage_to_backend
license_key = self.license_manager.license_key
if license_key:
    try:
        current = self.license_manager.get_current_node_count(self.synrix_memory)
        hwid = getattr(self.synrix_memory.backend, "get_hardware_id", lambda: None)()
        report_usage_to_backend(license_key, current, hwid)
    except Exception:
        pass
```

- Or call it from **`licensing.py`** inside `get_current_node_count` when we have a non-empty `license_key` (report once per session or throttle to avoid spam).

### Option B — Standalone update-usage (periodic)

If you want to report every hour while the app runs:

```python
def start_usage_reporting(license_key: str, get_count_cb, interval_seconds: int = 3600):
    def _loop():
        while True:
            try:
                n = get_count_cb()
                requests.post(SYNRIX_UPDATE_USAGE_URL, json={"license_key": license_key, "current_usage": n}, timeout=5)
            except Exception:
                pass
            threading.Event().wait(interval_seconds)
    threading.Thread(target=_loop, daemon=True).start()
```

---

## 3. Robotics / agent-memory SDK

- If the app uses **RawSynrixBackend** (DLL): you have `backend.get_hardware_id()` and need node count. The raw backend doesn’t expose `node_count` from the lattice today; you could add a `lattice_get_node_count` in C and bind it, or use a different way to get count (e.g. query and count). Then call `report_usage_to_backend(license_key, count, backend.get_hardware_id())` once after opening the lattice.
- If the app uses **SynrixClient** (HTTP): the server would need to expose a “node count” or “stats” endpoint; then the client could call that and then `report_usage_to_backend(...)`. Otherwise skip reporting for that path until the server has that API.

---

## 4. Summary for Joseph

- **Backend:** Already done (validate-license + update-usage + 25k defaults).
- **Ryan:** Add the helper above and one call in the RAG SDK when we have `license_key` and `current_usage`. That gives Joseph usage on startup for RAG users. Optional: periodic reporting via `update-usage`; optional: same for robotics/agent when backend exposes count.
