# Response to Joseph – License Summary Follow-up

Thanks for the review. Here are direct answers to your questions and how we’re thinking about the gaps.

---

## 1. Free tier: Do free users need a key? Default 25k or 100k?

**Free users do not need a key.**  
When no key is set (or the key is invalid/expired), the engine uses a **default cap of 25k nodes**. So:

- **No key** → 25k node limit (free tier).
- **Valid key** → Tier from the key (100k, 1m, 10m, 50m, or unlimited).

So the default is **25k**, not 100k. We can document this clearly as “free tier = 25k, no key required.”

---

## 2. Key sharing: Detection / enforcement? Hardware binding or online validation?

**Right now: no detection or enforcement of key sharing.**  
Verification is **fully offline**: the engine only checks that the key is signed with our private key and (if present) that the expiry date hasn’t passed. There is:

- **No** hardware binding.
- **No** online validation or phone-home.
- **No** built-in way to detect or block key sharing.

So if a key is shared, we don’t detect it. **Potential improvement:** add optional hardware binding (e.g. bind key to a machine ID) for customers who want it; we can do that in a future iteration.

---

## 3. User experience: Env var vs installer/script for non-technical users

**Current:** Setting `SYNRIX_LICENSE_KEY` is the supported way; that’s fine for developers.

**Improvement we agree with:** For non-technical users, we should provide:

- A small script (e.g. `.bat` / PowerShell) that prompts for the key and sets the env var for the session or user, and/or
- Installer/one-click flow that writes the key into the user environment (or a config file the engine reads) so they don’t have to touch env vars.

We’ll add this to the roadmap (simple “paste your key” script/installer step).

---

## 4. Key expiration: How does it work? Offline check or periodic validation?

**Fully offline.**  
The signed payload includes an optional **expiry** (Unix timestamp). The engine checks it locally at init:

- If `expiry == 0` → no expiry (key valid until you issue a new one).
- If `expiry > 0` and `current time > expiry` → key treated as invalid; engine falls back to default (25k).

There is **no** periodic re-validation and **no** online check. Expiry is enforced only when the app starts (or when the lattice is opened). So expiry is “offline date check at startup.”

---

## 5. License management: Tracking issued keys, revoke, prevent reuse?

**Current state:** We have the **keygen** (issue keys per tier/expiry) and the **engine** (verify signature + expiry). We do **not** have:

- A **database** of issued keys (who, when, tier, expiry).
- **Revocation** (e.g. a blocklist the engine could check).
- **Reuse prevention** (e.g. one key = one machine or one customer).

So today we can issue and verify keys, but we don’t yet have a **license management system** (track, revoke, limit reuse). Agreed that this is a gap.

**Planned direction:**  
Introduce a small **license-management layer**: e.g. store issued keys (customer, tier, expiry, optional notes) and, if we add online validation later, a way to revoke. For now we can at least track keys in a simple store (e.g. CSV/DB) so we know what we’ve issued; revocation can come in a later phase (e.g. with optional online check or hardware binding).

---

## Summary of gaps and next steps

| Gap | Current | Next step |
|-----|---------|-----------|
| Free tier clarity | Default 25k, no key | Document clearly: “Free = 25k, no key” |
| Key sharing | No detection | Optional hardware binding later |
| Non-technical UX | Env var only | Simple “paste key” script/installer |
| Expiry | Offline date check at startup | Document; no change needed for now |
| Key tracking/revoke | None | Add key log; design revoke path later |

**Bottom line:** We’ll document free-tier (25k default), add a simple key-entry script/installer, and start key tracking; we’ll treat hardware binding and revocation as the next phase. If you want to tweak any of this for the reply to Ryan, we can tighten or expand any section.
