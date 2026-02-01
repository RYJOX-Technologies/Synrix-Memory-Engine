#!/usr/bin/env python3
"""
Populate SYNRIX lattice with comprehensive knowledge about SYNRIX itself.
This makes the AI an "expert" on SYNRIX architecture, usage, and implementation.
"""

import sys
import os
sys.path.insert(0, 'synrix_unlimited')

from synrix.ai_memory import get_ai_memory

def populate_knowledge():
    """Populate SYNRIX with comprehensive knowledge about itself"""
    
    memory = get_ai_memory()
    
    print("=" * 60)
    print("Populating SYNRIX Knowledge Base")
    print("=" * 60)
    print()
    
    # ========================================================================
    # CORE ARCHITECTURE
    # ========================================================================
    
    print("Storing core architecture knowledge...")
    
    memory.add("ARCHITECTURE:core", """
SYNRIX is a high-performance, persistent knowledge graph system designed for AI agents.
Core principles:
- Knowledge graph/lattice as single source of truth
- Assembly-first design using atomic pattern recombination
- No hard-coding - solutions emerge from pattern recombination
- Execution validation (validate by running, not string comparison)
- Tokenless semantic reasoning
- 300-line limit per source file for bare metal OS
- ARM64 optimization for Jetson Orin Nano (~0.99 TOPS on DP4A kernels)
""")
    
    memory.add("ARCHITECTURE:data_structure", """
SYNRIX uses a persistent lattice (knowledge graph) stored in .lattice files.
- Nodes: Fixed-size structures (512 bytes) containing name, data, type, children
- Prefix-based indexing: O(k) queries scale with results, not total data
- O(1) lookups: Instant access by node ID
- Data-agnostic: Stores text, binary, and chunked data
- Memory-mapped files: Fast access, kernel-managed flushing
""")
    
    memory.add("ARCHITECTURE:persistence", """
SYNRIX persistence model:
- WAL (Write-Ahead Logging): All writes go to WAL first for crash safety
- Atomic file replacement: MoveFileExW with MOVEFILE_REPLACE_EXISTING
- Checkpoint system: Periodically applies WAL entries to main file
- Auto-save: Configurable intervals (node count or time-based)
- Recovery: Automatically replays WAL on startup if needed
""")
    
    memory.add("ARCHITECTURE:performance", """
SYNRIX performance characteristics:
- Lookups: O(1) by node ID
- Queries: O(k) by prefix (scales with results, not data size)
- Writes: Batched for performance (12.5k-50k entries per checkpoint)
- Memory: Configurable RAM cache (default 100k nodes)
- Throughput: Optimized for NVMe sequential writes
- Target: ~0.99 TOPS on DP4A kernels for Jetson Orin Nano
""")
    
    # ========================================================================
    # WINDOWS-NATIVE IMPLEMENTATION
    # ========================================================================
    
    print("Storing Windows-native implementation details...")
    
    memory.add("WINDOWS:approach", """
Windows-native SYNRIX approach:
- Python SDK uses subprocess.Popen with CREATE_NO_WINDOW flag
- No shell scripts (.bat, .ps1) required
- Python owns process lifecycle (not shell)
- Direct DLL access via ctypes (faster than HTTP/subprocess)
- os.add_dll_directory() for deterministic DLL loading
- MinGW runtime DLLs bundled with package
""")
    
    memory.add("WINDOWS:dll_loading", """
Windows DLL loading strategy:
- Use _native.py module with os.add_dll_directory()
- Search for libsynrix.dll (or libsynrix_free_tier.dll) in package directory
- Bundle MinGW runtime DLLs (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll)
- All DLLs co-located in synrix/ package directory
- No PATH manipulation needed
""")
    
    memory.add("WINDOWS:process_spawning", """
Windows process spawning (for CLI/server):
- subprocess.Popen with creationflags=subprocess.CREATE_NO_WINDOW
- close_fds=True for proper cleanup
- List of arguments (not shell=True)
- No cmd.exe, powershell.exe, or execution policy issues
- Python owns process lifecycle
""")
    
    # ========================================================================
    # API REFERENCE
    # ========================================================================
    
    print("Storing API reference...")
    
    memory.add("API:ai_memory", """
AI Memory Interface (recommended for AI agents):
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("KEY", "value")
results = memory.query("KEY")
count = memory.count()
node = memory.get(node_id)

Default lattice: ~/.synrix_ai_memory.lattice
""")
    
    memory.add("API:raw_backend", """
Raw Backend (advanced, direct DLL access):
from synrix.raw_backend import RawSynrixBackend

backend = RawSynrixBackend(
    lattice_path="data.lattice",
    max_nodes=100000,  # RAM cache size
    evaluation_mode=False  # Unlimited mode
)

node_id = backend.add_node("name", "data", node_type=3)
node = backend.get_node(node_id)
results = backend.find_by_prefix("prefix", limit=100)
backend.save()  # Manual save
backend.checkpoint()  # Full durability
""")
    
    memory.add("API:node_types", """
SYNRIX node types:
- LATTICE_NODE_PRIMITIVE (0): Basic data node
- LATTICE_NODE_TASK (1): Task/work item
- LATTICE_NODE_CONSTRAINT (2): Constraint/rule
- LATTICE_NODE_LEARNING (3): Learning/pattern
- LATTICE_NODE_PATTERN (4): Code pattern
- LATTICE_NODE_ANTI_PATTERN (5): Anti-pattern (what to avoid)
""")
    
    # ========================================================================
    # BUILD SYSTEM
    # ========================================================================
    
    print("Storing build system knowledge...")
    
    memory.add("BUILD:windows_msys2", """
Windows build with MSYS2/MinGW-w64:
1. Install MSYS2 from https://www.msys2.org/
2. Install tools: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make
3. Build: cd build/windows && bash build_msys2.sh
4. Output: build_msys2/bin/libsynrix.dll

For free tier: bash build_free_tier_50k.sh or build_free_tier_100k.sh
""")
    
    memory.add("BUILD:free_tier", """
Free tier build configuration:
- CMake flag: -DSYNRIX_FREE_TIER_50K=ON
- Limit define: -DSYNRIX_FREE_TIER_LIMIT=50000 (or 100000)
- Evaluation mode: -DSYNRIX_EVALUATION_MODE_ALWAYS_ENABLED=ON
- Output: libsynrix_free_tier.dll
- Hard-coded limit in C code via SYNRIX_FREE_TIER_LIMIT define
""")
    
    memory.add("BUILD:cmake_config", """
CMakeLists.txt configuration:
- SYNRIX_FREE_TIER_50K: BOOL flag to enable free tier build
- SYNRIX_FREE_TIER_LIMIT: STRING for node limit (50000, 100000, etc.)
- SYNRIX_EVALUATION_MODE_ALWAYS_ENABLED: BOOL for always-on evaluation mode
- Output name changes based on tier: libsynrix_free_tier.dll vs libsynrix.dll
""")
    
    # ========================================================================
    # WAL SYSTEM
    # ========================================================================
    
    print("Storing WAL system knowledge...")
    
    memory.add("WAL:architecture", """
WAL (Write-Ahead Logging) architecture:
- All writes go to WAL first (fast sequential write)
- WAL entries batched in memory (12.5k-50k entries)
- Checkpoint: Flushes WAL, applies entries to main file, marks as applied
- Recovery: Replays WAL entries on startup if needed
- Location: {lattice_path}.wal
- Format: Binary with operation type, node ID, packed data
""")
    
    memory.add("WAL:usage", """
WAL usage patterns:
- flush(): Forces immediate flush of WAL buffer to disk (fast)
- save(): Writes memory state to main file (WAL entries remain uncheckpointed)
- checkpoint(): Flushes WAL, applies entries, saves main file (full durability)

Best practice:
- flush() for immediate durability
- save() regularly for fast persistence
- checkpoint() periodically or before closing for full durability
""")
    
    memory.add("WAL:batch_sizing", """
WAL batch size configuration:
- Free tier (25k): 12.5k batch size (half of limit)
- Free tier (50k): 25k batch size
- Free tier (100k): 50k batch size
- Production: 50k batch size (optimized for NVMe)
- Ensures checkpoint happens before hitting node limit
""")
    
    # ========================================================================
    # PACKAGING
    # ========================================================================
    
    print("Storing packaging knowledge...")
    
    memory.add("PACKAGE:structure", """
SYNRIX package structure:
synrix_free_tier_100k/
├── synrix/              # Python SDK
│   ├── __init__.py
│   ├── ai_memory.py     # AI memory interface
│   ├── raw_backend.py   # Raw backend
│   ├── _native.py       # Windows DLL loader
│   ├── libsynrix_free_tier.dll  # Engine DLL
│   └── *.dll            # MinGW runtime DLLs
├── setup.py             # Package setup
├── installer.bat        # Windows installer
└── README.md           # Documentation
""")
    
    memory.add("PACKAGE:installation", """
SYNRIX installation options:
1. Double-click installer.bat (easiest)
2. pip install -e . (manual)
3. Use directly: sys.path.insert(0, 'path/to/package')

After installation:
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
# Ready to use!
""")
    
    memory.add("PACKAGE:versions", """
SYNRIX package versions:
- synrix_free_tier_50k: 50,000 node limit (hard-coded)
- synrix_free_tier_100k: 100,000 node limit (hard-coded)
- synrix_unlimited: No node limits, full production features

All versions include:
- Python SDK
- Windows DLL (or Linux .so)
- Runtime dependencies
- Installer
- Documentation
""")
    
    # ========================================================================
    # USAGE PATTERNS
    # ========================================================================
    
    print("Storing usage patterns...")
    
    memory.add("USAGE:basic", """
Basic SYNRIX usage:
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("PROJECT:name", "My Project")
memory.add("TASK:fix_bug", "Fixed null pointer")

results = memory.query("TASK:")
for r in results:
    print(f"{r['name']}: {r['data']}")

count = memory.count()
""")
    
    memory.add("USAGE:ai_agent", """
AI Agent usage pattern:
1. Before code generation: Query for constraints/patterns
2. During execution: Store results, errors, patterns
3. After completion: Store success/failure, learnings

Example:
constraints = memory.query("CONSTRAINT:")
patterns = memory.query("PATTERN:")
memory.add("RESULT:task_123", "Completed successfully")
memory.add("ERROR:bug_456", "Avoid this pattern")
""")
    
    memory.add("USAGE:prefix_convention", """
SYNRIX prefix conventions (for O(k) queries):
- PROJECT: Project-related data
- TASK: Tasks/work items
- CONSTRAINT: Constraints/rules
- PATTERN: Code patterns
- ERROR: Errors/failures
- LEARNING: Learnings/insights
- USER: User preferences
- DATA: General data

Query by prefix: memory.query("TASK:") returns all tasks
""")
    
    # ========================================================================
    # TROUBLESHOOTING
    # ========================================================================
    
    print("Storing troubleshooting knowledge...")
    
    memory.add("TROUBLESHOOTING:dll_not_found", """
DLL not found on Windows:
- Ensure libsynrix.dll is in synrix/ package directory
- Check that MinGW runtime DLLs are present
- Verify _native.py is using os.add_dll_directory()
- Check DLL search paths in _native.py
""")
    
    memory.add("TROUBLESHOOTING:limit_reached", """
Free tier limit reached:
- Error: "Free Tier limit reached (X nodes)"
- Options: Delete existing nodes, upgrade to unlimited version
- Limit is hard-coded in DLL (cannot be changed at runtime)
- Check limit: DLL error message shows actual limit
""")
    
    memory.add("TROUBLESHOOTING:data_not_persisting", """
Data not persisting:
- Check that save() or checkpoint() is called
- Verify WAL is enabled (look for "[LATTICE-WAL] ✅ WAL enabled" in logs)
- Check file permissions in home directory
- Verify .lattice file is being created
- Use checkpoint() for full durability guarantees
""")
    
    # ========================================================================
    # DEVELOPMENT WORKFLOW
    # ========================================================================
    
    print("Storing development workflow knowledge...")
    
    memory.add("DEV:workflow", """
SYNRIX development workflow:
1. Study existing code before proposing changes
2. Maintain backward compatibility (10/10)
3. Keep files under 300 lines
4. Test with actual execution (not string comparison)
5. Use assembly-first patterns
6. No hard-coding - derive from knowledge graph
""")
    
    memory.add("DEV:testing", """
SYNRIX testing approach:
- Use vitest for JS/TS, pytest for Python, Catch2 for C++
- Unit & integration tests for every feature
- Test edge cases, errors, limits
- Mock dependencies when needed
- Validate by execution, not string comparison
""")
    
    memory.add("DEV:code_standards", """
SYNRIX code standards:
- C++17 for core engine
- Python 3.8+ for SDK
- Idiomatic style for each language
- Readability > cleverness
- Robust error handling
- Structured logging
- Optimize after profiling
""")
    
    # ========================================================================
    # KEY CONCEPTS
    # ========================================================================
    
    print("Storing key concepts...")
    
    memory.add("CONCEPT:tokenless", """
SYNRIX is tokenless:
- No token-based processing
- Semantic reasoning over knowledge graph
- Concept discovery without regex
- Binary-packed concept storage (.lattice files)
- Hash map index for fast semantic reasoning
- Avoids tokenization overhead
""")
    
    memory.add("CONCEPT:assembly_first", """
Assembly-first design:
- Use assembly patterns as universal building blocks
- Solutions emerge from atomic pattern recombination
- No hard-coding of fundamentals
- Knowledge graph stores fundamentals
- Code generator reasons over graph structure
- Patterns stored in knowledge graph, not in generator
""")
    
    memory.add("CONCEPT:execution_validation", """
Execution validation principle:
- Validate by actual execution, not string comparison
- Run code to verify correctness
- Test with real data and scenarios
- Measure compile-rate and time-to-green
- Ground in toolchains with error catalogs
- Prefer executable specs over examples
""")
    
    # ========================================================================
    # PERFORMANCE OPTIMIZATION
    # ========================================================================
    
    print("Storing performance optimization knowledge...")
    
    memory.add("PERF:jetson", """
Jetson Orin Nano optimization:
- Target: ~0.99 TOPS on DP4A kernels
- ARM64 optimization
- Manages 1.4M+ node knowledge graphs
- Batched persistence
- Optimized for NVMe sequential writes
- INT8 GPU kernels
""")
    
    memory.add("PERF:scaling", """
SYNRIX scaling characteristics:
- O(1) lookups: Constant time by node ID
- O(k) queries: Linear with results, not total data
- Memory: Configurable RAM cache
- Disk: Memory-mapped files for fast access
- WAL: Batched writes for throughput
- Checkpoint: Periodic for durability
""")
    
    # ========================================================================
    # FILE FORMATS
    # ========================================================================
    
    print("Storing file format knowledge...")
    
    memory.add("FORMAT:lattice", """
.lattice file format:
- Header: 16 bytes (4 uint32_t: magic, total_nodes, next_id, nodes_to_load)
- Nodes: Array of fixed-size lattice_node_t structures (512 bytes each)
- Magic number: 0x4C415454 ("LATT")
- Memory-mapped for fast access
- Atomic replacement for crash safety
""")
    
    memory.add("FORMAT:wal", """
.wal file format:
- Binary format with operation type, node ID, packed data
- Pre-allocated (1MB default)
- State ledger for checkpoint tracking
- Mmap'd for fast access
- Recovery: Replays entries from last checkpoint
""")
    
    # ========================================================================
    # INTEGRATION PATTERNS
    # ========================================================================
    
    print("Storing integration patterns...")
    
    memory.add("INTEGRATION:cursor", """
Cursor AI integration:
- Use synrix.ai_memory for persistent memory
- Store constraints before code generation
- Store patterns and anti-patterns
- Track successes and failures
- Query for relevant context before tasks
- Auto-save agent actions
""")
    
    memory.add("INTEGRATION:devin", """
DevinAI integration:
- Check SYNRIX for past errors before attempting tasks
- Apply fixes based on stored patterns
- Store execution results
- Learn from failures
- Avoid known error patterns
""")
    
    # ========================================================================
    # SECURITY & LICENSING
    # ========================================================================
    
    print("Storing security knowledge...")
    
    memory.add("SECURITY:assessment", """
SYNRIX security assessment:
- Low-medium reverse engineering risk (data storage library)
- No secrets or proprietary algorithms
- Open source C code (can be inspected)
- Binary contains no sensitive data
- Free tier: Evaluation mode always enabled
- Production: Full features available
""")
    
    # ========================================================================
    # IMPLEMENTATION DETAILS
    # ========================================================================
    
    print("Storing implementation details...")
    
    memory.add("IMPLEMENTATION:indexing", """
SYNRIX indexing system:
- Dynamic prefix index: O(k) queries by prefix (scales with results)
- Reverse index (id_to_index_map): O(1) lookups by node ID
- Prefix extraction: Extracts semantic prefix from node names (e.g., "TASK:" from "TASK:fix_bug")
- Index built on-demand or during initialization
- Hierarchical structure to avoid O(n^2) complexity
""")
    
    memory.add("IMPLEMENTATION:memory_layout", """
SYNRIX memory layout:
- RAM mode: nodes[] array in RAM, loaded from file on startup
- Disk mode: memory-mapped file (mmap), nodes[] points to mmap'd memory
- Metadata arrays: node_id_map, id_to_index_map, access_count, last_access
- Prefix index: Separate structure for O(k) prefix queries
- WAL buffer: In-memory batch buffer before flush to disk
""")
    
    memory.add("IMPLEMENTATION:node_structure", """
lattice_node_t structure (512 bytes):
- id: uint64_t (64-bit node ID with device prefix)
- type: lattice_node_type_t (PRIMITIVE, TASK, CONSTRAINT, etc.)
- name: char[64] (node name with semantic prefix)
- data: char[512] (text or binary data, max 510 bytes for binary)
- parent_id: uint64_t (parent node for hierarchy)
- child_count: uint32_t (number of children)
- children: uint64_t[8] (direct children array)
- timestamp: uint64_t (creation/modification time)
- payload: union (type-specific data)
""")
    
    memory.add("IMPLEMENTATION:disk_mode", """
Disk mode (memory-mapped files):
- File pre-allocated to full size (header + nodes)
- Memory-mapped with MAP_SHARED (kernel-managed flushing)
- Nodes array points directly to mmap'd memory
- O(1) access by file index (local_id - 1)
- No RAM cache needed (kernel handles page management)
- Optimized for large datasets (>100k nodes)
""")
    
    memory.add("IMPLEMENTATION:ram_mode", """
RAM mode (in-memory cache):
- Nodes loaded into RAM array on startup
- Configurable cache size (max_nodes parameter)
- O(1) lookups via reverse index
- Auto-save to disk periodically
- Optimized for small datasets (<100k nodes)
- Faster for frequent access patterns
""")
    
    memory.add("IMPLEMENTATION:node_id_format", """
SYNRIX node ID format:
- 64-bit ID: device_id (upper 32 bits) + local_id (lower 32 bits)
- device_id: Unique per device/system (auto-assigned from timestamp)
- local_id: Sequential within device (1-indexed)
- Enables distributed systems with unique IDs across devices
- O(1) lookup: Extract local_id, use id_to_index_map[local_id]
""")
    
    memory.add("IMPLEMENTATION:prefix_extraction", """
Prefix extraction algorithm:
- Scans node name for ':' separator
- Extracts prefix up to ':' (e.g., "TASK:" from "TASK:fix_bug")
- Used for O(k) semantic queries
- Prefixes stored in dynamic prefix index
- Hierarchical queries: "TASK:" returns all tasks, "TASK:fix" returns fix tasks
""")
    
    memory.add("IMPLEMENTATION:atomic_operations", """
SYNRIX atomic operations:
- File replacement: MoveFileExW with MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
- WAL checkpoint: Atomic mark entries as applied
- Node addition: Atomic ID reservation and node creation
- Memory barriers: __sync_synchronize() for thread safety
- Crash-safe: WAL ensures no data loss on crash
""")
    
    memory.add("IMPLEMENTATION:error_handling", """
SYNRIX error handling:
- LATTICE_ERROR_NONE (0): Success
- LATTICE_ERROR_FREE_TIER_LIMIT (-100): Free tier limit reached
- LATTICE_ERROR_NOT_FOUND (-1): Node not found
- LATTICE_ERROR_INVALID_INPUT (-2): Invalid parameters
- last_error field tracks last error code
- Python SDK raises FreeTierLimitError for limit errors
""")
    
    # ========================================================================
    # CODE PATTERNS
    # ========================================================================
    
    print("Storing code patterns...")
    
    memory.add("PATTERN:prefix_convention", """
SYNRIX prefix naming convention:
- Use semantic prefixes for O(k) queries
- Format: "PREFIX:name" (e.g., "TASK:fix_bug")
- Common prefixes: PROJECT, TASK, CONSTRAINT, PATTERN, ERROR, LEARNING, USER, DATA
- Query by prefix: memory.query("TASK:") returns all tasks
- Hierarchical: "TASK:FIX:" for fix tasks, "TASK:FEATURE:" for features
""")
    
    memory.add("PATTERN:ai_agent_workflow", """
AI Agent workflow pattern:
1. Before task: Query for constraints and patterns
   constraints = memory.query("CONSTRAINT:")
   patterns = memory.query("PATTERN:")
2. During task: Store intermediate results
   memory.add("TASK:step_1", "Completed")
3. After task: Store final result and learnings
   memory.add("RESULT:task_123", "Success")
   memory.add("LEARNING:insight", "What was learned")
4. On error: Store error and anti-pattern
   memory.add("ERROR:bug_456", "What failed")
   memory.add("ANTI_PATTERN:avoid_this", "What to avoid")
""")
    
    memory.add("PATTERN:data_organization", """
SYNRIX data organization patterns:
- Hierarchical: Use parent_id to create tree structures
- Tagged: Use prefixes for semantic grouping
- Temporal: Use timestamps for chronological queries
- Typed: Use node_type for different data categories
- Namespaced: Use prefixes like "PROJECT:name:" for namespaces
""")
    
    # ========================================================================
    # ADVANCED FEATURES
    # ========================================================================
    
    print("Storing advanced features...")
    
    memory.add("FEATURE:chunked_data", """
SYNRIX chunked data support:
- For data > 510 bytes, split into chunks
- Parent node: "C:parent_id:0:N" (chunk header)
- Chunk nodes: "C:parent_id:1:N", "C:parent_id:2:N", etc.
- Automatic chunking: add_node_chunked() handles splitting
- Automatic reassembly: get_node_chunked() handles merging
- Each chunk is a separate node (counts toward limit)
""")
    
    memory.add("FEATURE:binary_data", """
SYNRIX binary data support:
- add_node_binary(): Stores arbitrary binary data
- First 2 bytes: uint16_t length header
- Max 510 bytes of binary data (512 - 2 for length)
- Binary-safe: Handles null bytes correctly
- Use lattice_is_node_binary() to check if node is binary
- get_node_chunked_to_buffer() for large binary data
""")
    
    memory.add("FEATURE:auto_save", """
SYNRIX auto-save system:
- Configurable intervals: node count or time-based
- Default: Every 5000 nodes or time interval
- Triggers: node_count >= interval, time >= interval, memory pressure
- Non-blocking: Saves in background
- WAL checkpoint: Applies WAL entries during auto-save
- Prevents data loss on unexpected shutdown
""")
    
    memory.add("FEATURE:access_tracking", """
SYNRIX access tracking:
- access_count[]: Tracks how many times each node is accessed
- last_access[]: Tracks last access timestamp
- Used for cache optimization and analytics
- O(1) updates on node access
- Can be used for LRU eviction (if implemented)
""")
    
    # ========================================================================
    # COMPLETION
    # ========================================================================
    
    print()
    print("=" * 60)
    print("Knowledge Base Population Complete")
    print("=" * 60)
    
    total = memory.count()
    print(f"Total nodes in knowledge base: {total:,}")
    print()
    print("Knowledge categories stored:")
    print("  - Architecture & Design")
    print("  - Windows Implementation")
    print("  - API Reference")
    print("  - Build System")
    print("  - WAL System")
    print("  - Packaging")
    print("  - Usage Patterns")
    print("  - Troubleshooting")
    print("  - Development Workflow")
    print("  - Key Concepts")
    print("  - Performance")
    print("  - File Formats")
    print("  - Integration Patterns")
    print("  - Security")
    print("  - Implementation Details")
    print("  - Code Patterns")
    print("  - Advanced Features")
    print()
    print("SYNRIX is now an expert on itself!")

if __name__ == "__main__":
    try:
        populate_knowledge()
    except KeyboardInterrupt:
        print("\n\nPopulation cancelled.")
        sys.exit(1)
    except Exception as e:
        print(f"\nERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)
