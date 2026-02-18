"""
One-time license activation.

Before the engine uses SYNRIX_LICENSE_KEY, the SDK calls the backend to "activate"
the key. The backend records each key as used; if the key was already activated,
it returns allowed=false and we clear SYNRIX_LICENSE_KEY so the engine runs in
free tier. This makes each key work only once (or once per backend policy).

Env:
  SYNRIX_ACTIVATE_LICENSE_URL  - Backend endpoint (POST JSON { "license_key": "..." }).
  SYNRIX_ACTIVATE_STRICT       - If "1", on network/error treat key as invalid (clear it).
"""

import json
import os
import urllib.request
import urllib.error


# Default activate endpoint (override with SYNRIX_ACTIVATE_LICENSE_URL)
_DEFAULT_URL = "https://jdznymdwjvcmhsslewfz.supabase.co/functions/v1/activate-license"


def ensure_license_activated() -> None:
    """
    If SYNRIX_LICENSE_KEY is set, call the backend to activate it (one-time).
    If the backend says already_activated or disallowed, clear the key so the
    engine sees no key and uses free tier. Call this before loading the engine
    or opening the lattice.
    """
    key = os.environ.get("SYNRIX_LICENSE_KEY", "").strip()
    if not key:
        return

    url = os.environ.get("SYNRIX_ACTIVATE_LICENSE_URL", _DEFAULT_URL).strip()
    if not url:
        return

    strict = os.environ.get("SYNRIX_ACTIVATE_STRICT", "").strip().lower() in ("1", "true", "yes")

    try:
        data = json.dumps({"license_key": key}).encode("utf-8")
        req = urllib.request.Request(
            url,
            data=data,
            headers={"Content-Type": "application/json"},
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=10) as resp:
            body = resp.read().decode("utf-8")
            try:
                out = json.loads(body)
            except json.JSONDecodeError:
                if strict:
                    os.environ.pop("SYNRIX_LICENSE_KEY", None)
                return
            allowed = out.get("allowed", False)
            if not allowed:
                os.environ.pop("SYNRIX_LICENSE_KEY", None)
    except (urllib.error.URLError, urllib.error.HTTPError, OSError, TimeoutError):
        if strict:
            os.environ.pop("SYNRIX_LICENSE_KEY", None)
