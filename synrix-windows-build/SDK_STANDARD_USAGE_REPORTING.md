# SDK standard: usage reporting to backend

**Permanent rule for all Synrix SDKs.**

---

## Rule

**Every SDK that uses the Synrix engine and supports a license key MUST report usage to the backend on startup (or when the client/memory is first obtained).**

- **What:** Send one fire-and-forget POST to the backend with `license_key`, `current_usage` (node count), and optional `hardware_id`.
- **When:** On startup or when the app first gets the Synrix client/memory (e.g. `get_synrix_client()`, `get_ai_memory()`, RAG memory init). Run in a **background thread**; never block.
- **Where:** Backend endpoint `validate-license` (or `update-usage` for periodic reporting). URL is configurable via `SYNRIX_VALIDATE_LICENSE_URL` / `SYNRIX_UPDATE_USAGE_URL`.
- **If no license key:** Do nothing. If no network: fail silently.

---

## Why

- Backend can send **warning emails** at 85% and **over-limit emails** at 100%.
- Engine still enforces limits locally; reporting is for UX and compliance only.

---

## Implementation checklist for each SDK

1. **Include `usage_report.py`** (or equivalent) with:
   - `report_usage_to_backend(license_key, current_usage, hardware_id=None)`.
   - Fire-and-forget POST to `validate-license` in a daemon thread.
   - Dependency: `requests` (or handle missing gracefully).

2. **Call it once** when the SDK exposes the engine to the app:
   - **RAG SDK:** When RAG memory is created with a license key (e.g. in `SynrixRAGMemory.__init__`).
   - **Robotics / Agent-memory SDK:** When `get_ai_memory()` (or equivalent) first creates the backend; read license from `SYNRIX_LICENSE_KEY` or `SYNRIX_RAG_LICENSE_KEY`, get node count from backend, optional `get_hardware_id()`, then call `report_usage_to_backend`.

3. **Default backend URL:**  
   `https://jdznymdwjvcmhsslewfz.supabase.co/functions/v1/validate-license`  
   Override with env if needed.

---

## Going forward

**Any new SDK** (or new entry points that create a Synrix client/memory) must follow this standard. Copy `usage_report.py` from an existing SDK and wire the single call at the appropriate place.
