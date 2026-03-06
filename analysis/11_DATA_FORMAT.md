# Synrix Memory Engine — Data Format Reversibility

*Can you extract your data from a .lattice file without the proprietary binary?*

---

## 1. What We Know About the Format

The `.lattice` file format is **not documented anywhere**. However, the Python ctypes struct definitions in `raw_backend.py` give us the exact memory layout of each node. Since the file is described as a "contiguous array of fixed-size nodes" that is memory-mapped, the file is almost certainly a direct binary dump of these structs.

---

## 2. Reconstructed Node Layout

From `raw_backend.py`:

```c
// Reconstructed from ctypes definitions
struct lattice_node_t {
    uint64_t id;              // Offset 0,   8 bytes
    uint32_t type;            // Offset 8,   4 bytes
    // (4 bytes padding for alignment)
    char     name[64];        // Offset 16,  64 bytes  (null-terminated)
    char     data[512];       // Offset 80,  512 bytes (null-terminated)
    uint64_t parent_id;       // Offset 592, 8 bytes
    uint32_t child_count;     // Offset 600, 4 bytes
    // (4 bytes padding)
    uint64_t *children;       // Offset 608, 8 bytes (POINTER - meaningless on disk)
    double   confidence;      // Offset 616, 8 bytes
    uint64_t timestamp;       // Offset 624, 8 bytes
    // payload union starts here
    union {
        struct lattice_learning_t {
            char     pattern_sequence[256]; // 256 bytes
            uint32_t frequency;            // 4 bytes
            double   success_rate;         // 8 bytes
            double   performance_gain;     // 8 bytes
            uint64_t last_used;            // 8 bytes
            uint32_t evolution_generation; // 4 bytes
        } learning;                        // ~288 bytes total
    } payload;                             // Offset 632, ~288 bytes
};
// Estimated total: ~920 bytes (may be padded to 1024 for alignment)
```

**Important caveats:**
- The exact padding and alignment depend on the compiler and platform used to build the engine
- The `children` field is a pointer — on disk it's either NULL (0) or a stale memory address
- The union size is determined by the largest member (`lattice_learning_t` at ~288 bytes)
- There may be a file header before the node array that we can't see from ctypes alone

---

## 3. Theoretical Data Extraction (Without Binary)

If the file is truly a flat array of these structs, you could extract data with:

```python
"""
Theoretical .lattice file reader — untested, based on ctypes analysis.
This attempts to read data WITHOUT the proprietary binary.
"""
import struct
import os

# Estimated struct size (may need adjustment)
NODE_SIZE_GUESS = 1024  # Likely padded to power of 2 for cache alignment

# Known field offsets (from ctypes analysis)
OFFSETS = {
    'id':         (0,   'Q'),     # uint64
    'type':       (8,   'I'),     # uint32
    'name':       (16,  '64s'),   # char[64]
    'data':       (80,  '512s'),  # char[512]
    'parent_id':  (592, 'Q'),     # uint64
    'child_count':(600, 'I'),     # uint32
    # children pointer at 608 — skip (meaningless on disk)
    'confidence': (616, 'd'),     # double
    'timestamp':  (624, 'Q'),     # uint64
}


def try_read_lattice(filepath, node_size=NODE_SIZE_GUESS):
    """Attempt to read a .lattice file based on inferred struct layout."""
    file_size = os.path.getsize(filepath)
    
    # Try to detect header size by looking for first valid node
    # (Skip bytes until we find a plausible node ID)
    with open(filepath, 'rb') as f:
        raw = f.read()
    
    # Heuristic: try different header sizes
    for header_size in [0, 64, 128, 256, 512, 1024]:
        data_region = raw[header_size:]
        if len(data_region) < node_size:
            continue
        
        # Try reading first node
        node_data = data_region[:node_size]
        try:
            node_id = struct.unpack_from('<Q', node_data, 0)[0]
            node_type = struct.unpack_from('<I', node_data, 8)[0]
            name = struct.unpack_from('<64s', node_data, 16)[0]
            name_str = name.decode('utf-8', errors='ignore').rstrip('\x00')
            
            # Plausibility check
            if 0 < node_id < 2**32 and 0 < node_type <= 10 and len(name_str) > 0:
                print(f"Possible header size: {header_size}")
                print(f"  First node: id={node_id}, type={node_type}, name='{name_str}'")
                
                # Count plausible nodes
                n_nodes = (len(data_region)) // node_size
                print(f"  Estimated nodes: {n_nodes}")
                
                # Read all nodes
                nodes = []
                for i in range(n_nodes):
                    offset = i * node_size
                    if offset + node_size > len(data_region):
                        break
                    chunk = data_region[offset:offset + node_size]
                    
                    nid = struct.unpack_from('<Q', chunk, 0)[0]
                    ntype = struct.unpack_from('<I', chunk, 8)[0]
                    nname = struct.unpack_from('<64s', chunk, 16)[0].decode('utf-8', errors='ignore').rstrip('\x00')
                    ndata = struct.unpack_from('<512s', chunk, 80)[0].decode('utf-8', errors='ignore').rstrip('\x00')
                    
                    if nid == 0 and ntype == 0:
                        continue  # Empty slot
                    
                    nodes.append({
                        'id': nid,
                        'type': ntype,
                        'name': nname,
                        'data': ndata,
                    })
                
                return nodes
        except Exception:
            continue
    
    return None
```

### Will This Work?

**Maybe.** The success depends on:

| Factor | Impact |
|--------|--------|
| Whether there's a file header | Could shift all offsets; we don't know header size |
| Struct padding/alignment | Compiler-dependent; `NODE_SIZE_GUESS` may be wrong |
| Whether the file has metadata pages | Index data might be interleaved with node data |
| Whether nodes are truly contiguous | Documentation says yes, but unverified |
| Platform differences | x86_64 vs ARM64 may have different alignment |
| Whether the prefix index is in the file | If stored inline, it changes the layout |

---

## 4. The WAL File

The `.lattice.wal` file is the write-ahead log. Its format is also undocumented, but from the C test code:

```c
lattice_add_node_with_wal(&lattice, type, name, data, parent_id);
```

The WAL likely stores:
- Operation type (ADD_NODE)
- Node data (same fields as the struct)
- Sequence number for ordering
- Possibly a checksum for integrity

Without documentation, WAL files cannot be reliably read by external tools.

---

## 5. Data Lock-In Assessment

| Question | Answer |
|----------|--------|
| Is the file format documented? | **No** |
| Can you read .lattice files without the binary? | **Theoretically possible but unreliable** |
| Can you read .lattice.wal files without the binary? | **No** |
| Is there an export tool? | **No** |
| Can you convert to a standard format? | **Not officially** |
| Can you migrate to another database? | **Only via the SDK (while it works)** |

### Migration Strategy (While Binary Still Works)

If you need to extract your data, the safest approach is to use the Python SDK:

```python
from synrix.raw_backend import RawSynrixBackend
import json

backend = RawSynrixBackend("my_data.lattice")

# Export everything via prefix query
all_nodes = backend.find_by_prefix("", limit=50000, raw=False)

# Write to JSON (portable)
with open("export.json", "w") as f:
    json.dump(all_nodes, f, indent=2)

# Or write to SQLite (structured)
import sqlite3
conn = sqlite3.connect("export.db")
conn.execute("CREATE TABLE nodes (id INT, name TEXT, data TEXT, type INT)")
for node in all_nodes:
    conn.execute("INSERT INTO nodes VALUES (?, ?, ?, ?)",
        (node['id'], node['name'], node['data'], node['type']))
conn.commit()
```

**This is the ONLY reliable extraction path**, and it requires:
- The proprietary binary (`libsynrix.so`) to be available
- The binary to be compatible with your platform
- The 25K node limit to not block reading (reading may not count, but unverified)

---

## 6. Comparison With Open Alternatives

| Database | Format Documented? | Data Extractable? | Third-Party Readers? |
|----------|:-:|:-:|:-:|
| Synrix | ❌ | ⚠️ Maybe | ❌ None |
| SQLite | ✅ [format spec](https://sqlite.org/fileformat.html) | ✅ Always | ✅ Dozens |
| LMDB | ✅ | ✅ Always | ✅ Many |
| Redis RDB | ✅ [format spec](https://rdb.fnordig.de/) | ✅ Always | ✅ Many |
| LevelDB/RocksDB | ✅ | ✅ Always | ✅ Many |
| PostgreSQL | ✅ | ✅ Always | ✅ Universal |

Every serious database documents its file format. Synrix does not.

---

## 7. Conclusions

| Assessment | Rating |
|-----------|--------|
| Format documentation | 🔴 None |
| Data extractability without binary | 🟡 Theoretically possible, unreliable |
| Data portability | 🔴 Locked to Synrix binary |
| Migration path | 🟡 Via SDK only (while binary works) |
| Long-term data safety | 🔴 If binary stops working, data may be inaccessible |
| Vendor lock-in risk | 🔴 High |

**Recommendation**: If you store any data in Synrix, maintain a parallel export to a portable format (JSON, SQLite, CSV). Do not rely on `.lattice` files as your sole data store — if the vendor disappears or the binary becomes incompatible with a future OS, your data may be unrecoverable.
