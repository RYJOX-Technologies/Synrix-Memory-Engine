# SYNRIX "Wow Proof" Demo Script (2-3 Minutes)

## Goal
Show exact, persistent memory without embeddings or cloud services.

## Setup (30 seconds)
- Open a terminal in the repo root.
- Confirm Python can import the SDK.

### Windows PowerShell
```powershell
python -c "import os, sys; sys.path.insert(0, os.path.join(os.getcwd(), 'python-sdk')); from synrix.ai_memory import get_ai_memory; print('OK')"
```

### Bash (macOS/Linux/MSYS2)
```bash
python -c "import os, sys; sys.path.insert(0, os.path.join(os.getcwd(), 'python-sdk')); from synrix.ai_memory import get_ai_memory; print('OK')"
```

## Demo Steps (90 seconds)

### 1) Add a memory with metadata + payload
#### Windows PowerShell
```powershell
python -c "import os, sys, json; sys.path.insert(0, os.path.join(os.getcwd(), 'python-sdk')); from synrix.ai_memory import get_ai_memory; m = get_ai_memory(); m.add('USER_PREF:theme', 'dark'); m.add('STATE:robot_arm', json.dumps({'x':0.4,'y':0.9,'grip':True,'speed':0.5})); print('Added memories:'); print(m.query('USER_PREF:')); print(m.query('STATE:'))"
```

#### Bash (macOS/Linux/MSYS2)
```bash
python -c "import os, sys, json; sys.path.insert(0, os.path.join(os.getcwd(), 'python-sdk')); from synrix.ai_memory import get_ai_memory; m = get_ai_memory(); m.add('USER_PREF:theme', 'dark'); m.add('STATE:robot_arm', json.dumps({'x':0.4,'y':0.9,'grip':True,'speed':0.5})); print('Added memories:'); print(m.query('USER_PREF:')); print(m.query('STATE:'))"
```

### 2) Close the process (prove persistence)
Say: "Now we restart the process. If this was just RAM or a context window, it's gone."

### 3) Reopen and retrieve exact memory
#### Windows PowerShell
```powershell
python -c "import os, sys; sys.path.insert(0, os.path.join(os.getcwd(), 'python-sdk')); from synrix.ai_memory import get_ai_memory; m = get_ai_memory(); print('Retrieved after restart:'); print(m.query('USER_PREF:')); print(m.query('STATE:'))"
```

#### Bash (macOS/Linux/MSYS2)
```bash
python -c "import os, sys; sys.path.insert(0, os.path.join(os.getcwd(), 'python-sdk')); from synrix.ai_memory import get_ai_memory; m = get_ai_memory(); print('Retrieved after restart:'); print(m.query('USER_PREF:')); print(m.query('STATE:'))"
```

## The Line to End With
"This is deterministic, persistent memory. No embeddings, no cloud, no guessing."

## Optional 30-Second Add-On
Show O(k) prefix query scale:
```bash
python - <<'PY'
from synrix.ai_memory import get_ai_memory

m = get_ai_memory()
print("Prefix query for PATTERN:")
print(m.query("PATTERN:")[:3])
PY
```

