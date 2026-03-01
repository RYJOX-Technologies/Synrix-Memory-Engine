# SDK consolidation: what to do

## Current state

| Location | Contents | Package name |
|----------|----------|--------------|
| **python-sdk/** | One `synrix/` package (raw_backend, ai_memory, agent_memory, langchain, etc.) + 5 hero demos + Qdrant/Mem0 benchmarks + 90+ legacy examples | `synrix` 0.2.0 |
| **synrix-sdks/agent-memory-sdk/** | **Full duplicate** of synrix/ (raw_backend, agent_*, ai_memory, etc.) + tests + agent-specific examples | `synrix` 0.1.0 |
| **synrix-sdks/robotics-sdk/** | **Full duplicate** of synrix/ + `robotics.py` + robotics examples | (same or similar) |
| **synrix-sdks/synrix-rag-sdk/** | RAG examples only (no separate synrix package) | — |

Problems:

- **Two packages named `synrix`** (python-sdk and agent-memory-sdk) with different versions and different code.
- **Three copies** of core code (python-sdk, agent-memory-sdk, robotics-sdk), so fixes (e.g. raw_backend) have to be applied in multiple places.
- **Unclear entrypoint** for users: “pip install synrix” and “which repo folder?” are ambiguous.

---

## Recommendation: one SDK, one package

**Goal:** One source of truth for the Synrix Python SDK and one clear place for demos.

### Option A (recommended): Keep `python-sdk/` only, retire `synrix-sdks/`

1. **Treat `python-sdk/` as the only SDK.**
   - It already has the latest core (raw_backend with all fixes), langchain, agent_memory, ai_memory, and the hero demos.
   - It’s the one you’ve been pushing and documenting (README, Medium, releases).

2. **Pull in the unique bits from synrix-sdks:**
   - From **agent-memory-sdk**: copy over any missing or better tests into `python-sdk/tests/`; copy `auto_daemon.py`, `usage_report.py` into `python-sdk/synrix/` if you still need them; add one agent-memory example to `python-sdk/examples/` if it’s better than what you have.
   - From **robotics-sdk**: copy `synrix/robotics.py` into `python-sdk/synrix/`; add `robotics_quick_demo.py` (or equivalent) to `python-sdk/examples/`.
   - From **synrix-rag-sdk**: add the best RAG example(s) to `python-sdk/examples/` (e.g. `rag_simple_demo.py`).

3. **Remove or archive `synrix-sdks/` in the repo.**
   - Either delete the folder or move it to something like `_archive/synrix-sdks/` and add a short README there: “Consolidated into python-sdk. See python-sdk/ and README.”

4. **Repo layout after consolidation:**
   ```
   Synrix-Memory-Engine/
   ├── README.md
   ├── LICENSE
   ├── .gitignore
   └── python-sdk/
       ├── synrix/           # single source of truth (core + agent + langchain + robotics)
       ├── examples/         # hero demos + reasoning benchmarks + agent/rag/robotics demos
       ├── tests/            # optional, if you bring over agent-memory tests
       ├── setup.py
       ├── pyproject.toml
       └── README.md
   ```

5. **README at repo root:** Say clearly: “The Synrix Python SDK and all demos live in **python-sdk/**. Install with `pip install -e python-sdk/` or from PyPI when published.”

**Pros:** One package, one place to fix bugs, no duplicate `synrix` names, clear for users and for you.  
**Cons:** You have to do a one-time merge of robotics (and any agent/rag bits you care about) and then delete or archive synrix-sdks.

---

### Option B: Keep two folders but stop duplicating core

- **python-sdk/** = core package `synrix` (raw_backend, ai_memory, agent_memory, langchain, etc.) and all demos. This is what you install: `pip install -e python-sdk/`.
- **synrix-sdks/** = thin “product” packages that **depend on** `synrix` and only add a little code:
  - `synrix-sdks/agent-memory-sdk` → depends on `synrix`, contains only agent-specific extras (e.g. `auto_daemon`, `usage_report`) and examples; **no** copy of raw_backend.
  - `synrix-sdks/robotics-sdk` → depends on `synrix`, contains only `robotics.py` and robotics examples.
  - `synrix-sdks/synrix-rag-sdk` → depends on `synrix`, RAG examples only.

That implies refactoring agent-memory-sdk and robotics-sdk to remove their full `synrix/` copy and depend on the package from python-sdk (or from PyPI). More work than Option A, and you still have two top-level SDK folders to explain.

---

## What makes most sense

**Option A** is the one that makes the most sense unless you have a strong reason to keep separate PyPI packages (e.g. “synrix”, “synrix-agent”, “synrix-robotics”) with separate versioning.

- One package name: **synrix**.
- One folder for SDK + demos: **python-sdk/**.
- No duplicate core, no confusion about which “synrix” is canonical.
- You can still add optional “extras” or submodules (e.g. `synrix.robotics`, `synrix.agent`) inside that single package later if you want.

**Concrete steps:**

1. Copy into **python-sdk** any unique files you care about from synrix-sdks (robotics.py, agent/rag examples, tests).
2. Update **python-sdk/README.md** to list: core usage, hero demos, agent/rag/robotics examples.
3. Remove **synrix-sdks/** from the repo (or move to `_archive/synrix-sdks/` and document that it’s retired).
4. Update the **repo README** to say: “SDK and demos are in **python-sdk/**.”

After that, “combine them based on the contents” is done: one combined SDK in `python-sdk/`, with synrix-sdks content folded in and the duplicate folder removed.
