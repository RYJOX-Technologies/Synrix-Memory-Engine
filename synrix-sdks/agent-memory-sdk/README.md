# SYNRIX Agent Memory SDK

Python SDK for SYNRIX agent memory (local, deterministic, persistent).

## What this is
This SDK is the Python client library. It requires the SYNRIX engine DLL.
For internal distribution, we use the free tier (100k node limit) and
provide upgrade instructions when the limit is reached.

## Requirements
- Windows 10+
- Python 3.8+
- SYNRIX free tier package (DLL + runtime deps)

## Quick start (Windows)

1) Install the SDK
```bash
cd agent-memory-sdk
pip install -e .
```

2) Provide the engine DLL
Option A: copy into `agent-memory-sdk/synrix/`
- `libsynrix.dll` (single engine; tier set by license key)
- `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `zlib1.dll` (runtime deps)

Option B: set an explicit DLL path
```powershell
set SYNRIX_LIB_PATH=C:\path\to\libsynrix.dll
```

Tier (100k / 1M / 10M / 50M / unlimited) is set by `SYNRIX_LICENSE_KEY`. No key = default cap (e.g. 25k). See LICENSE_SIGNED_KEY.md in the build folder.

3) Use the SDK
```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("PROJECT:architecture", "Microservices with REST APIs")
results = memory.query("PROJECT:")
print(results[:3])
```

## Tier / limit
Node limit is set by your license key (`SYNRIX_LICENSE_KEY`). When the limit is reached, the SDK raises `FreeTierLimitError` with upgrade instructions.

## Examples
See `examples/` for working demos and scripts.
