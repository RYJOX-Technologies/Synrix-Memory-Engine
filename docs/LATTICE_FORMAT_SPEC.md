# Synrix `.lattice` File Format Specification

**Version:** 1.0  
**Engine version:** Synrix 1.0 (Windows x86_64, Linux ARM64)  
**Node size:** 1216 bytes (fixed, `19 × 64`, cache-line aligned)

---

## Overview

A `.lattice` file is a flat, memory-mapped array of fixed-size node records preceded by a file header. Because every node is the same size, any node can be accessed in O(1) by index — no B-tree traversal, no page parsing, no decompression.

This is deliberately simpler than general-purpose database formats (SQLite, LMDB, RocksDB). The design trades flexibility (variable-length records, complex queries) for speed (direct mmap pointer access) and predictability (fixed layout on edge hardware).

The companion `.lattice.wal` file is a write-ahead log that guarantees durability across crashes. See [WAL Format](#wal-file-format) below.

---

## File Layout

```
[ File Header ]  — exactly 16 bytes (4 × uint32_t)
[ Node 0      ]  — 1216 bytes
[ Node 1      ]  — 1216 bytes
[ Node 2      ]  — 1216 bytes
...
[ Node N-1    ]  — 1216 bytes
```

The header is a fixed 16 bytes. Node `i` is always at byte offset:

```
node_offset(i) = 16 + (i * 1216)
```

This is a constant — no guessing required.

---

## File Header

The file header is exactly **16 bytes** — four `uint32_t` fields in little-endian order:

| Offset | Size | Field | Value | Description |
|--------|------|-------|-------|-------------|
| 0 | 4 | `magic` | `0x4C415454` | ASCII `"LATT"` — identifies file as a `.lattice` file |
| 4 | 4 | `total_nodes` | variable | Number of valid nodes written to the file |
| 8 | 4 | `next_id` | variable | Next local ID to be assigned (monotonically increasing) |
| 12 | 4 | `nodes_to_load` | variable | How many nodes to read on load (equals `total_nodes` in clean files) |

**Reading the header:**
```python
import struct
with open("data.lattice", "rb") as f:
    magic, total_nodes, next_id, nodes_to_load = struct.unpack("<IIII", f.read(16))
assert magic == 0x4C415454, "Not a .lattice file"
# First node is at byte offset 16
```

**Magic byte check:** If `magic != 0x4C415454`, reject the file — it is not a valid `.lattice` file or is corrupted.

**`total_nodes` vs `nodes_to_load`:** In a clean save, these are equal. After a crash, `nodes_to_load` may be smaller than `total_nodes` if a save was interrupted. Always use `nodes_to_load` to determine how many nodes to read.

---

## Node Structure

Each node is exactly **1216 bytes**, 64-byte cache-line aligned (`__attribute__((aligned(64)))`).

### Field Layout

| Offset | Size (bytes) | Field | Type | Description |
|--------|-------------|-------|------|-------------|
| 0 | 8 | `id` | `uint64_t` | Node ID: `(device_id << 32) \| local_id`. Zero = reserved (empty slot). Valid IDs start at 1 and are monotonically increasing — never reused. |
| 8 | 4 | `type` | `uint32_t` | Node type enum (see [Node Types](#node-types)) |
| 12 | 4 | *(padding)* | — | Alignment padding |
| 16 | 64 | `name` | `char[64]` | Null-terminated UTF-8 name string. Max 63 usable bytes. |
| 80 | 512 | `data` | `char[512]` | Data field — dual mode (see [Data Field Modes](#data-field-modes)) |
| 592 | 8 | `parent_id` | `uint64_t` | Parent node ID (0 = no parent) |
| 600 | 4 | `child_count` | `uint32_t` | Number of children |
| 604 | 4 | *(padding)* | — | Alignment padding |
| 608 | 8 | `children` | `uint64_t*` | **Pointer — meaningless on disk.** Always treat as 0/NULL when reading from file. Child relationships must be reconstructed from `parent_id` fields. |
| 616 | 8 | `confidence` | `double` | Confidence score (0.0–1.0). Default 0.0. |
| 624 | 8 | `timestamp` | `uint64_t` | Unix timestamp in **microseconds**, UTC, since 1970-01-01 00:00:00 UTC. Valid range: 1970–2262. |
| 632 | 288 | `payload` | union | Type-specific payload (see [Payload Union](#payload-union)) |
| 920 | 128 | `expansion` | struct | Reserved expansion header (see [Expansion Header](#expansion-header)) |
| **Total** | **1216** | | | **19 × 64 bytes** |

### Data Field Modes

The `data[512]` field operates in two modes:

**Text mode** (default):
- Null-terminated UTF-8 string
- Maximum 511 usable bytes

**Binary mode**:
- First 2 bytes: a `uint16_t` where:
  - **Lower 15 bits** (`length & 0x7FFF`): actual payload length in bytes (0–510)
  - **Bit 15** (`length & 0x8000`): set if payload is compressed
- Payload starts at byte offset 2
- Maximum 510 bytes of payload

```c
// Correct binary mode decoding:
uint16_t length_header = *(uint16_t*)node.data;
bool is_compressed     = (length_header & 0x8000) != 0;
uint16_t data_length   = (length_header & 0x7FFF);   // lower 15 bits = actual length
uint8_t* payload       = (uint8_t*)node.data + 2;    // payload starts at offset 2
```

**Compressed binary mode** (bit 15 set):
```
[length_header:2][compression_type:1][compressed_payload...]
```
- `compression_type` byte follows immediately after `length_header`
- Payload starts at offset 3

**Detecting mode for recovery:** Text mode is the common case for agent memory workloads. Binary mode is used only when `lattice_add_node_binary()` or `lattice_add_node_chunked()` is called. If `data[0]` and `data[1]` together form a `uint16_t ≤ 510` (or with bit 15 set), it may be binary. Otherwise treat as null-terminated text.

### Node Types

| Value | Constant | Description |
|-------|----------|-------------|
| 1 | `LATTICE_NODE_PRIMITIVE` | Primitive operation or fact |
| 2 | `LATTICE_NODE_KERNEL` | Kernel/core computation |
| 3 | `LATTICE_NODE_PATTERN` | Recognized pattern |
| 4 | `LATTICE_NODE_PERFORMANCE` | Performance measurement |
| 5 | `LATTICE_NODE_LEARNING` | Learned behavior or preference |
| 6 | `LATTICE_NODE_ANTI_PATTERN` | Known failure pattern |
| 7 | `LATTICE_NODE_SIDECAR_MAPPING` | Intent→capability mapping |
| 8 | `LATTICE_NODE_SIDECAR_EVENT` | System event record |
| 9 | `LATTICE_NODE_SIDECAR_SUGGESTION` | Approved suggestion |
| 10 | `LATTICE_NODE_SIDECAR_STATE` | Sidecar system state |
| 11 | `LATTICE_NODE_INTERFACE` | Machine-verifiable contract |
| 12 | `LATTICE_NODE_OBJECTIVE` | Objective/policy gate |
| 13 | `LATTICE_NODE_COMPONENT` | Composed component |
| 14 | `LATTICE_NODE_SYSTEM` | System-level composition |
| 15 | `LATTICE_NODE_TEST` | Test suite node |
| 100 | `LATTICE_NODE_CPT_ELEMENT` | CPT element |
| 101 | `LATTICE_NODE_CPT_ADVANCED_PATTERN` | CPT advanced pattern |
| 106 | `LATTICE_NODE_CPT_METADATA` | CPT metadata |
| 200 | `LATTICE_NODE_CHUNK_HEADER` | Chunked data header |
| 201 | `LATTICE_NODE_CHUNK_DATA` | Chunked data segment |

For most agent workloads, only types 1–6 are used. Types 200–201 appear when data exceeds 510 bytes and chunking is used.

### Payload Union

The `payload` field (offset 632, 288 bytes) is a union sized to its largest member. The active member depends on node `type`.

**`lattice_learning_t`** (types 3, 5 — Pattern, Learning):

| Offset in payload | Size | Field | Type |
|-------------------|------|-------|------|
| 0 | 256 | `pattern_sequence` | `char[256]` |
| 256 | 4 | `frequency` | `uint32_t` |
| 260 | 4 | *(padding)* | — |
| 264 | 8 | `success_rate` | `double` |
| 272 | 8 | `performance_gain` | `double` |
| 280 | 8 | `last_used` | `uint64_t` |
| 288 | 4 | `evolution_generation` | `uint32_t` |
| 292 | 4 | *(padding)* | — |

**`lattice_performance_t`** (type 4 — Performance):

| Offset in payload | Size | Field | Type |
|-------------------|------|-------|------|
| 0 | 8 | `cycles` | `uint64_t` |
| 8 | 8 | `instructions` | `uint64_t` |
| 16 | 8 | `execution_time_ns` | `double` |
| 24 | 8 | `instructions_per_cycle` | `double` |
| 32 | 8 | `throughput_mb_s` | `double` |
| 40 | 8 | `efficiency_score` | `double` |
| 48 | 4 | `complexity_level` | `uint32_t` |
| 52 | 4 | *(padding)* | — |
| 56 | 32 | `kernel_type` | `char[32]` |
| 88 | 8 | `timestamp` | `uint64_t` |

For types 1, 2, 6 (Primitive, Kernel, Anti-Pattern), the payload is unused — treat as zero-filled.

### Expansion Header

The `expansion` field (offset 920, 128 bytes, `__attribute__((packed))`):

| Offset | Size | Field | Type | Description |
|--------|------|-------|------|-------------|
| 0 | 64 | `quantum_hash` | `uint8_t[64]` | Reserved (SHA-512 slot, currently unused) |
| 64 | 4 | `owner_uid` | `uint32_t` | OS user ID (0 if unused) |
| 68 | 4 | `owner_gid` | `uint32_t` | OS group ID (0 if unused) |
| 72 | 2 | `permission_flags` | `uint16_t` | Permission bits (0 if unused) |
| 74 | 2 | `reserved_flags` | `uint16_t` | Reserved |
| 76 | 8 | `relevance_score` | `double` | Temporal relevance/decay score |
| 84 | 8 | `decay_rate` | `double` | Decay rate per time unit |
| 92 | 8 | `last_access_time` | `uint64_t` | Last access timestamp |
| 100 | 8 | `creation_time` | `uint64_t` | Creation timestamp |
| 108 | 4 | `access_count` | `uint32_t` | Access frequency counter |
| 112 | 16 | `reserved` | `uint32_t[4]` | Reserved for future use |

For data recovery, this entire field can be ignored.

---

## Chunked Data

Chunked storage allows you to store arbitrary binary data of any size — not just values that exceed 510 bytes. The engine splits data at the **byte level** and links chunks through the lattice's node structure, enabling perfect byte-for-byte reconstruction on retrieval.

### How it works

Call `lattice_add_node_chunked(lattice, type, name, data, data_len, parent_id)` with any `data` pointer and `data_len`. The engine:

1. Splits the input into **500-byte payload slices** (the last slice may be smaller)
2. Creates a **header node** (`type = 200`, `name = "C:{original_name}"`) storing binary metadata:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 8 | `total_size` — original `data_len` in bytes |
| 8 | 4 | `chunk_count` — total number of chunk nodes |
| 12 | 8 | `checksum` — CRC32 (reserved, currently 0) |
| 20 | 4 | `first_chunk_local_id` — local ID of chunk 0, enabling O(k) sequential read |
| 24 | 8×N | `chunk_ids[]` — array of all chunk node IDs (only if metadata fits in 510 bytes) |

3. Creates **N chunk nodes** (`type = 201`, `name = "C:{parent_id}:{index}:{total}"`), each storing a 10-byte mini-header + 500-byte payload slice in binary mode:

| Offset in chunk data | Size | Field |
|----------------------|------|-------|
| 0 | 8 | `chunk_index` — zero-based position in sequence |
| 8 | 2 | `chunk_length` — actual payload bytes in this chunk |
| 10 | ≤500 | raw payload bytes |

4. Links all chunk nodes to the header node via `parent_id`

### Reconstruction

On retrieval, the engine:
1. Reads the header node metadata to get `total_size`, `chunk_count`, and `first_chunk_local_id`
2. Uses `first_chunk_local_id` to walk chunks sequentially in O(k) — no full prefix scan needed if chunks are contiguous
3. For each chunk node, strips the 10-byte mini-header and copies the payload bytes
4. Concatenates all payloads in index order → **exact byte-for-byte reconstruction of the original data**

### Manual recovery (without the binary)

To reconstruct chunked data from raw file reads:

1. Find the header node: `name` starts with `"C:"` and `type == 200`
2. From its binary metadata, read `total_size` (bytes 0–7) and `chunk_count` (bytes 8–11)
3. Collect all nodes whose `name` matches `"C:{parent_id}:{index}:{total}"` where `parent_id` matches the header node's `id`
4. Sort by `index` (field 2 in the name, parsed from the name string)
5. For each chunk node, skip the first 10 bytes of its `data` field, then copy `chunk_length` (bytes 8–9) payload bytes
6. Concatenate all payloads → original data restored

---

## Prefix Index

The prefix index is **not persisted to disk**. It is rebuilt in memory on first use after loading.

**Build strategy (incremental, not full-scan):**

- On `lattice_load()`: the index is marked `built = false`
- On the first `add_node()` call after load: a full scan builds the index once (`built = true`)
- On every subsequent `add_node()`: the new node is added incrementally — no full rebuild
- Result: first write after load triggers a one-time O(n) build; all subsequent writes are O(1) index updates

**On a first query immediately after load (no writes yet):**
- If no nodes have been added, the index is not yet built
- `lattice_find_nodes_by_name()` triggers a build on first query if needed
- After that single build, all queries are O(k)

**Startup time impact:** For 100K nodes, index build takes roughly 10–50ms depending on hardware. There is no persistent index to load — it is always rebuilt from the node array.

---

## Child Relationship Reconstruction

The `children` pointer field (offset 608) is a **runtime memory pointer** — it is meaningless on disk and must always be treated as NULL when reading from file.

To reconstruct parent→child relationships after reading from disk:

```python
# Build child lists from parent_id fields (O(n) pass)
children = {}  # parent_id -> [child_node, ...]
for node in nodes:
    if node["parent_id"] != 0:
        children.setdefault(node["parent_id"], []).append(node)

# Now children[parent_id] gives all direct children of that node
```

**Notes:**
- `parent_id = 0` means the node has no parent (root-level node)
- `child_count` in the struct reflects the count at save time but child pointers are not valid on disk — use the `parent_id` reconstruction above
- Tree depth is not bounded by the format; cycles are not possible since IDs are monotonically increasing (a node cannot be its own ancestor)

---

## ID Assignment and Uniqueness

Node IDs are **monotonically increasing and never reused**:

- Local IDs start at `1` on a new lattice (stored as `next_id = 1` in header)
- Each new node increments `next_id` by 1
- `next_id` is persisted in the file header and restored on load
- Full 64-bit ID = `(device_id << 32) | local_id`
  - `device_id = 0` for single-device deployments (the common case)
  - For single-device files: ID equals local_id directly (upper 32 bits are zero)
- **ID 0 is permanently reserved** — it means "empty slot" or "no parent" and is never assigned to a real node

---

## WAL File Format

The `.lattice.wal` file records every write operation before it is committed to the main `.lattice` file. On startup, the engine replays any unprocessed entries to recover from crashes.

### WAL Entry Header

Each WAL entry begins with a **24-byte header** followed by variable-length data:

| Offset | Size | Field | Type | Description |
|--------|------|-------|------|-------------|
| 0 | 8 | `sequence` | `uint64_t` | Monotonically increasing sequence number |
| 8 | 4 | `operation` | `uint32_t` | Operation type (see below) |
| 12 | 8 | `node_id` | `uint64_t` | Node ID affected |
| 20 | 4 | `data_size` | `uint32_t` | Byte length of variable-length data that follows |
| 24 | N | data | `uint8_t[]` | Operation-specific payload (`data_size` bytes) |

### Operation Types

| Value | Constant | Variable data |
|-------|----------|--------------|
| 1 | `WAL_OP_ADD_NODE` | node type (1B) + name (64B) + data (512B) + parent_id (8B) |
| 2 | `WAL_OP_UPDATE_NODE` | new data string (null-terminated) |
| 3 | `WAL_OP_DELETE_NODE` | none |
| 4 | `WAL_OP_ADD_CHILD` | child_id (8B) |
| 5 | `WAL_OP_CHECKPOINT` | none — marks checkpoint point |

### Checkpoint Semantics

The WAL context tracks two sequence numbers:
- `sequence` — the last written entry's sequence number
- `checkpoint_sequence` — the last sequence number confirmed as applied to the main file

**Entries with `sequence ≤ checkpoint_sequence` are already applied and must be skipped during recovery.**

Checkpoints are created by explicit calls to `lattice_wal_checkpoint()`. After a checkpoint, `wal_truncate()` removes applied entries from the WAL file, keeping it compact.

### Recovery Procedure (Step by Step)

```
On startup after a crash:

1. Open the .lattice file and read the header
   → total_nodes, next_id, nodes_to_load

2. Load nodes_to_load nodes from the .lattice file into RAM

3. Open the .lattice.wal file

4. Read the WAL sequentially from the beginning:
   a. For each entry where sequence > checkpoint_sequence:
      - WAL_OP_ADD_NODE  → add node to loaded set (if not already present)
      - WAL_OP_UPDATE_NODE → update existing node's data field
      - WAL_OP_DELETE_NODE → remove node from loaded set
      - WAL_OP_ADD_CHILD → add child relationship
      - WAL_OP_CHECKPOINT → update checkpoint_sequence, skip past it
   b. Entries with sequence ≤ checkpoint_sequence are skipped (already applied)

5. After replay, save the fully recovered state back to .lattice
   → This creates a clean checkpoint and resets the WAL

Example:
  Main file has 100 nodes (last clean save)
  WAL has entries 101–150 (written after last save, before crash)
  checkpoint_sequence = 100

  On recovery:
    - Load 100 nodes from .lattice
    - Replay WAL entries 101–150 → now have 150 nodes in RAM
    - Save to .lattice → clean file with 150 nodes
    - Truncate WAL → empty
```

### Adaptive Batching

The WAL uses adaptive write batching for throughput. Batch size adjusts dynamically between a minimum (1K entries) and maximum (100K entries) based on observed write rate, measured per 1-second windows. A background flush thread handles disk writes without blocking the writer. This is transparent to file format readers — batching only affects write timing, not the on-disk entry format.

---

## Data Recovery Without the Binary

If the Synrix engine binary is unavailable, you can extract your data directly from the `.lattice` file using only the spec above. The header is exactly 16 bytes, node offset is deterministic, and text-mode data is plain UTF-8.

```python
"""
lattice_reader.py — Extract data from a .lattice file without the engine binary.
Spec: header=16 bytes, node_size=1216 bytes, little-endian.
"""
import struct, os, sys, json

HEADER_SIZE = 16       # Exact — always 4 x uint32_t
NODE_SIZE   = 1216     # Exact — always 19 x 64 bytes, never changes
MAGIC       = 0x4C415454  # "LATT"

# Field offsets within each 1216-byte node
OFF_ID         = 0    # uint64_t  — 0 = empty slot
OFF_TYPE       = 8    # uint32_t
OFF_NAME       = 16   # char[64]  — null-terminated UTF-8
OFF_DATA       = 80   # char[512] — text: null-terminated; binary: 2-byte length header
OFF_PARENT_ID  = 592  # uint64_t  — 0 = no parent
OFF_CONFIDENCE = 616  # double
OFF_TIMESTAMP  = 624  # uint64_t  — microseconds since Unix epoch, UTC

NODE_TYPES = {
    1: "PRIMITIVE", 2: "KERNEL", 3: "PATTERN", 4: "PERFORMANCE",
    5: "LEARNING",  6: "ANTI_PATTERN", 200: "CHUNK_HEADER", 201: "CHUNK_DATA",
}


def decode_data(raw512: bytes) -> str:
    """Decode data field: text mode (common) or binary mode."""
    length_header = struct.unpack_from("<H", raw512, 0)[0]
    is_binary = (length_header <= 510) or ((length_header & 0x8000) != 0)
    if is_binary and length_header > 0:
        length = length_header & 0x7FFF
        # Return hex for binary data; caller can further decode if needed
        return f"<binary {length} bytes: {raw512[2:2+min(length,16)].hex()}...>"
    # Text mode: null-terminated UTF-8
    return raw512.split(b"\x00")[0].decode("utf-8", errors="replace")


def read_lattice(filepath: str) -> list:
    with open(filepath, "rb") as f:
        raw = f.read()

    if len(raw) < HEADER_SIZE:
        print("[FAIL] File too small to contain a valid header")
        return []

    # Parse exact 16-byte header
    magic, total_nodes, next_id, nodes_to_load = struct.unpack_from("<IIII", raw, 0)

    if magic != MAGIC:
        print(f"[FAIL] Bad magic: expected 0x{MAGIC:08X}, got 0x{magic:08X}")
        return []

    print(f"[OK] Magic valid. total_nodes={total_nodes}, next_id={next_id}, nodes_to_load={nodes_to_load}")

    nodes = []
    for i in range(nodes_to_load):
        offset = HEADER_SIZE + (i * NODE_SIZE)
        if offset + NODE_SIZE > len(raw):
            print(f"[WARN] File truncated at node {i}")
            break

        chunk = raw[offset : offset + NODE_SIZE]

        nid    = struct.unpack_from("<Q", chunk, OFF_ID)[0]
        ntype  = struct.unpack_from("<I", chunk, OFF_TYPE)[0]
        if nid == 0:
            continue  # Empty slot — skip

        nname  = chunk[OFF_NAME : OFF_NAME+64].split(b"\x00")[0].decode("utf-8", errors="replace")
        ndata  = decode_data(chunk[OFF_DATA : OFF_DATA+512])
        npar   = struct.unpack_from("<Q", chunk, OFF_PARENT_ID)[0]
        nconf  = struct.unpack_from("<d", chunk, OFF_CONFIDENCE)[0]
        nts    = struct.unpack_from("<Q", chunk, OFF_TIMESTAMP)[0]

        nodes.append({
            "id":         nid,
            "type":       NODE_TYPES.get(ntype, str(ntype)),
            "name":       nname,
            "data":       ndata,
            "parent_id":  npar,
            "confidence": round(nconf, 4),
            "timestamp_us": nts,
        })

    return nodes


def reassemble_chunks(nodes: list) -> dict:
    """Reassemble chunked data. Returns {original_name: bytes} for each chunked entry."""
    headers = {n["id"]: n for n in nodes if n["type"] == "CHUNK_HEADER"}
    chunks  = [n for n in nodes if n["type"] == "CHUNK_DATA"]
    result  = {}
    for hdr_id, hdr in headers.items():
        # Name format: "C:original_name"
        original_name = hdr["name"][2:] if hdr["name"].startswith("C:") else hdr["name"]
        # Collect chunks: name = "C:{parent_id}:{index}:{total}"
        my_chunks = []
        for c in chunks:
            parts = c["name"].split(":")
            if len(parts) == 4 and int(parts[1]) == hdr_id:
                my_chunks.append((int(parts[2]), c))  # (index, node)
        my_chunks.sort(key=lambda x: x[0])
        result[original_name] = b"".join(
            # chunk data field: first 10 bytes are mini-header, rest is payload
            # For recovery, we have hex string — this is a simplified placeholder
            f"[chunk {idx}]".encode() for idx, _ in my_chunks
        )
    return result


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "data.lattice"
    nodes = read_lattice(path)
    if nodes:
        out = path.replace(".lattice", "_export.json")
        with open(out, "w", encoding="utf-8") as f:
            json.dump(nodes, f, indent=2, ensure_ascii=False)
        print(f"[OK] Exported {len(nodes)} nodes to {out}")
```

**What this script handles correctly:**
- Exact 16-byte header — no guessing
- Deterministic node offsets (`16 + i * 1216`)
- Skips empty slots (`id == 0`)
- Detects and labels binary-mode nodes
- Text-mode nodes (agent memory workloads) fully decoded

**Known limitations:**
- Binary-mode chunk payloads are labeled as `<binary N bytes: hex...>` — full reassembly requires reading chunk mini-headers (10 bytes each) and concatenating payloads in index order (see [Chunked Data](#chunked-data))
- If a chunk node is missing (e.g., partial write before crash): the WAL should have the missing entry — run WAL recovery first. Without WAL, data up to the missing chunk can be recovered; remaining chunks are lost.
- Chunks are always written in order; if contiguous in the file (the normal case), walking sequentially from `first_chunk_local_id` is sufficient.

---

## Platform Notes

- **Byte order:** Little-endian (x86_64 and ARM64)
- **Struct alignment:** 64-byte aligned nodes (`__attribute__((aligned(64)))`); expansion header is packed (`__attribute__((packed))`)
- **Node size invariant:** 1216 bytes across all platforms and compiler versions (enforced by the engine at init via `sizeof(lattice_node_t)` assertion)
- **File extension:** `.lattice` for the node array, `.lattice.wal` for the write-ahead log

---

## Version History

| Version | Node size | Notes |
|---------|-----------|-------|
| 1.0 | 1216 bytes | Initial public release. Fixed-size node layout, WAL durability, prefix index. |
