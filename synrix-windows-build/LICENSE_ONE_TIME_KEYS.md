# One-Time License Keys

Each issued key is **unique** (different nonce) and is intended to **work only once**. The SDK calls your backend to "activate" the key before the engine uses it; the backend records the key as used and rejects reuse.

---

## Flow

1. You issue a key (keygen with optional `--customer-id`). Each key is cryptographically unique.
2. Customer sets `SYNRIX_LICENSE_KEY` and runs the app.
3. **Before** the engine reads the key, the SDK calls your **activate** endpoint with the key.
4. Backend: if this key has **never** been activated → mark it activated, return `{ "allowed": true }`. If **already** activated → return `{ "allowed": false, "reason": "already_activated" }`.
5. If the backend returns `allowed: false`, the SDK **clears** `SYNRIX_LICENSE_KEY` for that process, so the engine sees no key and runs in free tier (25k).
6. If the backend returns `allowed: true`, the SDK leaves the key set; the engine loads and applies the tier from the key.

Result: each key works only the first time it is used (per your backend’s record). Sharing or reusing a key does not grant the tier again.

---

## Backend API (for Joseph / backend dev)

**Endpoint:** `POST /activate-license` (or whatever URL you set in `SYNRIX_ACTIVATE_LICENSE_URL`).

**Request body (JSON):**

```json
{
  "license_key": "<base64 license key string>"
}
```

**Response (JSON):**

- **First use (key not yet activated):**  
  HTTP 200, body: `{ "allowed": true }`  
  Backend must record this key as activated (e.g. in a table keyed by key hash or full key).

- **Reuse (key already activated):**  
  HTTP 200, body: `{ "allowed": false, "reason": "already_activated" }`  
  (or HTTP 409 if you prefer; SDK only cares about `allowed: false` in the body.)

- **Invalid key (bad format / unknown):**  
  HTTP 200, body: `{ "allowed": false, "reason": "invalid" }`  
  SDK will clear the key so the engine runs in free tier.

**Optional:** You can include a `hardware_id` in the request (SDK can send it if you add it to the client) and store it when activating, so you know which machine first used the key.

---

## Env vars (customer / app)

| Var | Meaning |
|-----|--------|
| `SYNRIX_LICENSE_KEY` | The key you sent the customer. |
| `SYNRIX_ACTIVATE_LICENSE_URL` | Override the activate endpoint (default: your Supabase `activate-license` function). |
| `SYNRIX_ACTIVATE_STRICT` | If `1` or `true`, any network/error during activate is treated as “key not allowed” (key is cleared). If unset, only explicit `allowed: false` clears the key. |

---

## Keygen (your side)

- Each key is already **unique** (8-byte nonce or `--customer-id` hash).  
  `python tools/synrix_license_keygen.py --tier unlimited --private synrix_license_private.key`  
  → new key every time.  
- Optional: `--customer-id "email@example.com"` so the same customer always gets the same key (for re-issue or your records).  
- One-time behavior is enforced by the **backend**, not by the key format. The keygen does not need to change.

---

## Summary

- **Keygen:** Already issues unique keys; no change needed.  
- **Engine:** Unchanged; still reads `SYNRIX_LICENSE_KEY` and verifies signature/tier.  
- **SDK:** Before the engine runs, calls your activate endpoint; if the backend says not allowed, clears the key so the engine gets free tier. (Implemented in `python-sdk/synrix/license_activate.py` and wired in `RawSynrixBackend.__init__`; same for `packages/unlimited/synrix` and any other copy of the SDK.)  
- **Backend:** Implement `POST /activate-license`: first use → record key, return `allowed: true`; subsequent use → return `allowed: false`.  
Result: **each key works only once.**

**Until the backend exposes the activate endpoint:** The SDK will get a connection/404 error. By default (without `SYNRIX_ACTIVATE_STRICT`), the key is left set and the engine will use it (fail-open). Set `SYNRIX_ACTIVATE_STRICT=1` to treat errors as “key not allowed” once you want to require the backend to be up.
