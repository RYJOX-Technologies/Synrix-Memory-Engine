# Synrix Memory Engine — Binary Forensics

*Is the engine genuinely custom, or is it wrapping an existing database?*

---

## 1. Scope & Limitations

The engine binary (`synrix-server-evaluation`, `libsynrix.so`, `libsynrix.dll`) is **not included in this repository** — it's distributed via GitHub Releases as a pre-compiled download. This analysis is based on:
- What the C test files (`crash_test.c`, `wal_test.c`, `query_latency_diagnostic.c`) reveal via `#include` directives
- What the Python ctypes bindings (`raw_backend.py`) reveal via function signatures
- What the `_download_binary.py` and `engine.py` files reveal about distribution
- What the git history reveals about the source code being briefly exposed

**To perform deeper forensics, one would need to:**
1. Download the binary from GitHub Releases
2. Run `strings` to find embedded text, library names, and version strings
3. Run `file` / `ldd` (Linux) / `otool` (macOS) to identify dependencies
4. Run `strace` / `dtrace` to observe syscalls and network activity
5. Run `nm` or `objdump` for symbol analysis (if not stripped)
6. Monitor network traffic while the server runs

---

## 2. What the Include Files Tell Us

From `crash_test.c`:
```c
#include "../src/storage/lattice/persistent_lattice.h"
```

From `wal_test.c`:
```c
#include "../src/storage/lattice/persistent_lattice.h"
#include "../src/storage/lattice/wal.h"
```

This reveals the source directory structure:
```
src/
└── storage/
    └── lattice/
        ├── persistent_lattice.h   — Main lattice API header
        ├── persistent_lattice.c   — Implementation (inferred)
        ├── wal.h                  — WAL API header
        └── wal.c                  — WAL implementation (inferred)
```

**This is a narrow source tree.** A genuine database engine (SQLite, LMDB, RocksDB) has dozens or hundreds of source files. Two header files suggest the engine is relatively simple — consistent with the "mmap'd struct array + prefix index + WAL" architecture described in the documentation.

---

## 3. What the C API Reveals

From the ctypes bindings in `raw_backend.py`, the exported C functions are:

```c
// Core lifecycle
int  lattice_init(lattice*, path, max_nodes, device_id);
void lattice_cleanup(lattice*);
int  lattice_load(lattice*);
int  lattice_save(lattice*);

// Node operations
uint64_t lattice_add_node(lattice*, type, name, data, parent_id);
uint64_t lattice_add_node_binary(lattice*, type, name, data, data_len, parent_id);
uint64_t lattice_add_node_chunked(lattice*, type, name, data, data_len, parent_id);
int      lattice_get_node_data(lattice*, id, out_node);
lattice_node_t* lattice_get_node_copy(lattice*, id);
void     lattice_free_node_copy(node*);
ssize_t  lattice_get_node_chunked_size(lattice*, parent_id);
ssize_t  lattice_get_node_chunked_to_buffer(lattice*, parent_id, buffer, size);

// Prefix search
uint32_t lattice_find_nodes_by_name(lattice*, name, node_ids, max_ids);
void     lattice_build_prefix_index(lattice*);

// WAL
int  lattice_enable_wal(lattice*);
uint64_t lattice_add_node_with_wal(lattice*, type, name, data, parent_id);
int  lattice_wal_checkpoint(lattice*);
int  lattice_recover_from_wal(lattice*);
void wal_get_stats(wal*, total, checkpointed, pending);

// Persistence configuration
void lattice_configure_persistence(lattice*, auto_save, interval_nodes, interval_seconds, save_on_pressure);

// License/evaluation
int  lattice_disable_evaluation_mode(lattice*);
int  lattice_get_hardware_id(hwid_out, hwid_size);
int  lattice_get_last_error(lattice*);

// License (optional, if present)
int  synrix_license_parse(path, claims);
int  lattice_apply_license(lattice*, claims);
```

**Total: ~25 exported functions.** This is a small API surface, consistent with a purpose-built library rather than a general-purpose database.

---

## 4. Indicators of Custom vs. Wrapped Implementation

### Evidence for CUSTOM Implementation

| Indicator | Significance |
|-----------|-------------|
| Small source tree (2 headers) | Too narrow for a wrapped database |
| Fixed-size struct nodes | Not how SQLite/LMDB/Redis store data |
| `persistent_lattice.h` naming | Custom terminology, not matching any known library |
| Seqlock concurrency (mentioned in docs) | Unusual choice — not used by major databases |
| `lattice_build_prefix_index()` | Custom index construction, not a wrapper call |
| `device_id` parameter | Custom concept not from known databases |
| `lattice_configure_persistence()` with `save_on_pressure` | Custom auto-save policy |
| License enforcement baked into the API | Custom feature |

### Evidence That Would Suggest WRAPPING (Not Found)

| Indicator | Present? |
|-----------|:--------:|
| SQLite-style WAL filenames (`-wal`, `-shm`) | ❌ Uses `.lattice.wal` |
| BerkeleyDB/LMDB-style lock files | ❌ Not observed |
| Known library strings in binary | ❓ Would need `strings` analysis |
| Known library symbols in exports | ❓ Would need `nm` analysis |
| Third-party license notices | ❓ Would need binary inspection |

### Probable Verdict

**The engine is likely genuinely custom code** — a C program that:
1. `mmap()`s a file of fixed-size structs
2. Maintains a prefix index (trie or sorted array)
3. Implements a simple WAL (append-only journal + checkpoint)
4. Uses seqlocks for concurrent read access
5. Adds license enforcement (25K node limit, hardware ID)

This is ~2,000-5,000 lines of C, which is feasible for a solo developer with systems programming experience. The crash tests and WAL tests demonstrate genuine C programming ability.

---

## 5. The Binary Distribution Model

### How the Binary is Distributed

From `_download_binary.py` and `engine.py`:

```
GitHub Releases → zip/tar → ~/.synrix/bin/synrix-server-evaluation
                          → ~/.synrix/bin/libsynrix.so (or .dll)
```

### Asset Naming

```
synrix-server-evaluation-{version}-{platform}
synrix-server-free-tier-{version}-{platform}
```

Two different binary names exist (`server-evaluation` vs `server-free-tier`), suggesting either different builds or rename confusion.

### Platforms Claimed

| Platform | Binary |
|----------|--------|
| Windows x86_64 | `.exe` |
| Linux x86_64 | ELF binary |
| Linux ARM64 | ELF binary |
| macOS ARM64 | ❓ Claimed but `darwin-arm64` in code |
| macOS x86_64 | ❓ Claimed but not tested |

---

## 6. Security Concerns (Unverifiable Without Binary)

| Concern | Risk Level | Why |
|---------|-----------|-----|
| Phone-home / telemetry | ❓ Unknown | Binary opens HTTP port; could make outbound connections |
| Hardware fingerprinting | 🔴 Confirmed | `lattice_get_hardware_id()` collects device-specific identifiers |
| License enforcement | 🟡 Confirmed | `lattice_get_last_error()` returns -100 for free tier limit |
| Data exfiltration | ❓ Unknown | Binary has network capabilities; would need traffic monitoring |
| Backdoors | ❓ Unknown | Proprietary binary, no audit possible |
| Supply chain | 🟡 Medium | Downloaded from GitHub Releases without checksum verification |

### The Hardware ID Function

From `raw_backend.py`:
```python
def get_hardware_id(self) -> Optional[str]:
    """Get hardware ID (stable, unique identifier for license tracking)."""
    hwid_buffer = ctypes.create_string_buffer(65)  # 64 hex chars
    result = self.lib.lattice_get_hardware_id(hwid_buffer, 65)
    return hwid_buffer.value.decode('utf-8') if result == 0 else None
```

The engine generates a 64-character hex string that uniquely identifies the machine. This is used for license tracking. The data collected to generate this ID is unknown — it could be:
- MAC address hash
- CPU serial number
- Disk serial number
- Combination of hardware identifiers

---

## 7. What Would Be Needed for Full Analysis

| Analysis | Tool | What It Would Reveal |
|----------|------|---------------------|
| Embedded strings | `strings binary \| grep -i sqlite\|lmdb\|level\|rocks` | Whether the engine wraps a known database |
| Dynamic dependencies | `ldd` (Linux) / `otool -L` (macOS) | Linked libraries |
| Symbol table | `nm -D binary` (if not stripped) | Exported/imported function names |
| Syscall trace | `strace` (Linux) / `dtrace` (macOS) | File I/O, network calls, mmap usage |
| Network monitoring | `tcpdump` / `wireshark` while server runs | Whether it phones home |
| Binary size analysis | `ls -la` on the binary | Small (~100KB-1MB) suggests custom; large (>5MB) suggests embedded DB |
| Disassembly | `objdump -d` / Ghidra / IDA | Full implementation analysis |

---

## 8. Conclusions

| Question | Assessment | Confidence |
|----------|-----------|:----------:|
| Is the engine genuinely custom C code? | **Probably yes** | 🟡 Medium |
| Does it wrap SQLite/LMDB/Redis internally? | **Probably not** | 🟡 Medium |
| Is it a simple mmap + prefix index + WAL? | **Most likely** | 🟢 High |
| Could it be ~2-5K lines of C? | **Yes, plausible** | 🟢 High |
| Does it phone home? | **Unknown** | 🔴 Cannot assess |
| Is the hardware ID collection concerning? | **Yes** | 🟢 High |
| Should you run this binary in production without source? | **No** | 🟢 High |

The engine is most likely a genuine, custom C implementation of a straightforward data structure (mmap'd struct array + prefix index + WAL). The systems programming quality visible in the test files supports this. However, the proprietary distribution model, hardware fingerprinting, and inability to audit make it unsuitable for any environment where security or data sovereignty matters.
