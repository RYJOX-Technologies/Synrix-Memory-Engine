# For Joseph – What to Send & Use

Everything in one place so you can upload it or build a database.

---

## 0. What to send Joseph (signing key – do this first)

**Ryan:** Send Joseph this **one line** (the PKCS8 base64 of the license signing private key) over a secure channel (e.g. encrypted, not in plain email). If you re-export from `synrix_license_private.key`, use the new output and update this doc.

```
MC4CAQAwBQYDK2VwBCIEIFXfeC1UKs8yb2pwqhnciptuP5l3GLL8yHeUVNudwUKf
```

**Joseph:** After you receive it, set it in Supabase secrets so the backend can sign license keys the engine will accept:

```bash
supabase secrets set LICENSE_SIGNING_PRIVATE_KEY=MC4CAQAwBQYDK2VwBCIEIFXfeC1UKs8yb2pwqhnciptuP5l3GLL8yHeUVNudwUKf
```

The engine is already updated with the matching **public** key. Ryan should **rebuild** the DLL (see `build/windows/BUILD.md` or run `build.ps1` from `build/windows/`) and ship the new build; then keys your backend signs with this private key will verify in the engine.

---

## 1. User journey (exact flow)

See **USER_JOURNEY_AND_FLOW.md** in this folder. It has:

- Free user flow (no key → 25k)
- Paid user flow (we send key → customer pastes → tier applied)
- Upgrade flow (new key → paste → restart → more nodes)
- Invalid/expired key (falls back to 25k)

Use this for support docs, sales, and onboarding.

---

## 2. Answers to your questions

See **RESPONSE_TO_JOSEPH.md** in this folder. It has:

- Free tier = 25k, no key
- Key sharing = no detection today; optional hardware binding later
- UX = env var for devs; we’ll add “paste key” script for non-technical users
- Expiry = offline check at startup
- License management = we’ll add key tracking; revocation in a later phase

Use this for internal alignment and roadmap.

---

## 3. High-level summary (what you already saw)

See **SUMMARY_FOR_JOSEPH.md** in this folder. One-pager: single binary, signed keys, tier by key, prod-ready.

Use this for exec/partner summaries.

---

## 4. Format for your database (key tracking)

Use the table below to design your key-tracking DB or spreadsheet. Each row = one issued key.

| Field | Description | Example |
|-------|-------------|--------|
| **key_id** | Internal ID | 1, 2, 3… |
| **customer** | Customer name or ID | Acme Corp |
| **email** | Contact email | license@acme.com |
| **tier** | 25k / 1m / 10m / 50m / unlimited | 1m |
| **issued_at** | Date we issued the key | 2026-01-23 |
| **expires_at** | Optional expiry (0 = no expiry) | 2027-01-23 or blank |
| **key_sent** | How we sent it (email, dashboard) | Email |
| **notes** | Optional (order ID, revoke reason) | Order #1234 |
| **revoked** | Yes/No (for future revocation) | No |

**What we actually send the customer:** One line of text (the base64 key). We don’t send key_id or internal IDs to the customer.

**Keygen (our side):**  
From `synrix-windows-build/build/windows/`:

```bash
python tools/synrix_license_keygen.py --tier <25k|1m|10m|50m|unlimited> [--expiry <unix_ts>] --private synrix_license_private.key
```

Output = one line (the key). We store that key in our DB linked to customer/tier/expiry; we send that one line to the customer.

---

## 5. Files to upload or attach

If you’re putting this in a shared drive or DB:

- **USER_JOURNEY_AND_FLOW.md** – user journey and flows
- **RESPONSE_TO_JOSEPH.md** – Q&A and gaps/next steps
- **SUMMARY_FOR_JOSEPH.md** – one-pager summary
- **FOR_JOSEPH_PACK.md** – this file (index + DB format)

You can also export the “Format for your database” table to CSV or paste it into your DB schema.

---

## 6. When will this be in “upload” format?

All of the above is in Markdown in this repo. You can:

- **Copy-paste** from these files into your database or docs.
- **Upload the repo** (or this folder) and link to the files.
- **Export** the DB table (Section 4) to CSV/Excel and import into your system.

If you want a single PDF or Word doc that combines: user journey + Q&A + summary + DB table, say the format and I’ll outline it so Ryan can generate it.
