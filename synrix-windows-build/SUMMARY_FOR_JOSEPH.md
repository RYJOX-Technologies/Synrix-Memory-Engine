# Summary for Joseph – What’s Updated / Better

Brief overview of where we are and what’s improved for production.

---

## What’s better

**1. One binary, not many**  
We no longer ship different DLLs per tier (e.g. free-tier vs unlimited). There is a **single engine** (`libsynrix.dll`). Everyone gets the same file; the **license key** decides the limit at runtime.

**2. Signed license keys (your ask)**  
- Keys are **Ed25519-signed**. Only someone with the **private key** (you / your build process) can create valid keys.  
- Customers set one env var (`SYNRIX_LICENSE_KEY`) with the key you send. No re-download to change tiers.  
- **Tampering:** If someone edits the key (e.g. to change tier or expiry), the signature fails and the engine falls back to the default cap (~25k). They can’t “fake” a higher tier without your private key.

**3. Tier controlled by key**  
You issue one key per tier: 100k, 1m, 10m, 50m, or unlimited. Same binary for all; upgrade = send a new key.

**4. Production readiness**  
- Unlimited key: stress test passed (26k+ nodes) with the single DLL.  
- OpenSSL DLLs are bundled in the packages we ship so customers don’t hit “silent crash” on first run.  
- Tier zip files have been refreshed so they contain the single DLL + OpenSSL.  
- Agent-memory and robotics SDKs both load the DLL and run; smoke tests passed.

---

## What you need to know

- **Security:** Keys can’t be forged without the private key. Keep `synrix_license_private.key` secure and off the repo.  
- **Issuing keys:** From `build/windows/`:  
  `python tools/synrix_license_keygen.py --tier &lt;100k|1m|10m|50m|unlimited&gt; --private synrix_license_private.key`  
  Send the one-line output to the customer.  
- **Customer side:** They set `SYNRIX_LICENSE_KEY` to that value (env var), then run the app. Same binary for every tier.

---

## One-line summary

**Single binary, tier set by a signed license key only you can issue; keys are tamper-proof; packaging and SDKs are updated and tested for prod.**
