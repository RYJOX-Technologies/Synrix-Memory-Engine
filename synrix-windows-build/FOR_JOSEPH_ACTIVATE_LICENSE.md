# For Joseph: Activate-License Endpoint (One-Time Keys)

We need each Synrix license key to work **only once**. The SDK will call a new backend endpoint before the engine uses the key. You record the key as “activated” on first use and reject reuse.

---

## What to build

**One new Supabase Edge Function** (or HTTP endpoint) with this contract.

**URL (must be reachable at):**
```
https://jdznymdwjvcmhsslewfz.supabase.co/functions/v1/activate-license
```
(Same host as your existing `validate-license`; function name `activate-license`.)

**Method:** `POST`  
**Content-Type:** `application/json`

**Request body:**
```json
{
  "license_key": "<the full base64 license key string the customer has in SYNRIX_LICENSE_KEY>"
}
```

**Responses (all JSON):**

| Case | HTTP | Body | What to do |
|------|------|------|------------|
| First time this key is seen | 200 | `{ "allowed": true }` | Persist this key as “activated” (see below), then return this. |
| Same key used again | 200 | `{ "allowed": false, "reason": "already_activated" }` | Key already in your activated store → return this. |
| Invalid/malformed key | 200 | `{ "allowed": false, "reason": "invalid" }` | Optional; use if you want to reject bad keys. |

The SDK only checks the **body**: if `allowed` is `true` it keeps the key; if `allowed` is `false` it clears the key for that process (user gets free tier). HTTP status can be 200 for all of the above if you like; we don’t require 409.

---

## Storage

You need a **persistent store of “activated” keys** so you can tell “first use” from “reuse”.

- **Option A:** New table, e.g. `activated_license_keys`, with a column that stores the key (or a hash of the key, e.g. SHA-256 of the base64 string). On request: if key/hash not in table → insert it, return `{ "allowed": true }`; if already in table → return `{ "allowed": false, "reason": "already_activated" }`.
- **Option B:** Reuse/extend existing license/profile storage: when you see a key, check if it’s already marked “activated” (or “first_used_at” set); if not, mark it and return `allowed: true`; if yes, return `allowed: false`.

Important: **one row (or one record) per key**. Same key sent again must be detected as already activated.

---

## Pseudocode

```
POST /activate-license
  body = parse JSON → license_key

  if license_key is missing or empty
    return 200, { "allowed": false, "reason": "invalid" }

  key_id = hash(license_key)   // e.g. SHA-256 hex, or store raw key

  if key_id exists in activated_license_keys (or your store)
    return 200, { "allowed": false, "reason": "already_activated" }

  insert key_id (and optionally timestamp, customer_id, etc.) into activated_license_keys
  return 200, { "allowed": true }
```

---

## Optional: hardware_id later

The request body could later include `hardware_id` if we add it from the SDK. You can accept it now and store it when activating (e.g. “this key was first used on machine X”). Not required for “one-time” to work.

---

## Summary for Joseph

1. Add an **Edge Function** (or route) **`activate-license`** at the URL above.
2. **Request:** JSON with `license_key` (string).
3. **Logic:** If key not in “activated” store → add it, return `{ "allowed": true }`. If already in store → return `{ "allowed": false, "reason": "already_activated" }`.
4. **Storage:** Any table or store that maps key (or key hash) to “already used”. No need to validate signature or tier—we only need “have we seen this key before?”

Once this is live, the SDK (already shipped) will call it automatically; no app changes needed. Each key will work only the first time it’s used.
