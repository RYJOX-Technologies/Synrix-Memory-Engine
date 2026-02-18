"""
SYNRIX Raw C Backend Interface
===============================
Direct access to the raw C SYNRIX lattice implementation via ctypes.
This provides sub-microsecond performance by bypassing all Python overhead.

Usage:
    from synrix.raw_backend import RawSynrixBackend
    
    backend = RawSynrixBackend("/path/to/lattice.lattice")
    node_id = backend.add_node("TASK:write_function", "code here", node_type=5)
    node = backend.get_node(node_id)  # O(1) lookup
    results = backend.find_by_prefix("TASK:")  # O(k) semantic search
"""

import ctypes
import os
import sys
from typing import Optional, List, Dict, Any
from ctypes import Structure, Union, c_uint64, c_uint32, c_char, c_double, c_bool, POINTER, c_int, c_char_p, byref

# Import native loader (handles Windows DLL directory registration)
try:
    from ._native import load_synrix
    _NATIVE_LOADER_AVAILABLE = True
except ImportError:
    _NATIVE_LOADER_AVAILABLE = False


class FreeTierLimitError(Exception):
    """
    Raised when the free tier limit (25k nodes) is reached.
    
    AI agents should catch this exception and naturally inform the user with options:
    - No new nodes can be added until space is freed or tier is upgraded
    - Delete existing nodes to free up space
    - Upgrade to Pro tier for unlimited nodes
    - Contact support for assistance
    """
    pass

# Try to find the SYNRIX shared library
def _find_synrix_lib():
    """Find the SYNRIX shared library (.so/.dll) - auto-detects common locations"""
    import platform
    
    # Determine library extension and names based on platform
    if platform.system() == "Windows":
        lib_ext = ".dll"
        # Try free tier DLL first, then standard
        lib_names = ["libsynrix_free_tier.dll", "libsynrix.dll"]
    else:
        lib_ext = ".so"
        lib_names = ["libsynrix_free_tier.so", "libsynrix.so"]
    
    # Try each library name
    for lib_name in lib_names:
        search_paths = []
        
        # 1. Check SYNRIX_LIB_PATH environment variable (highest priority)
    synrix_lib_path = os.environ.get("SYNRIX_LIB_PATH")
    if synrix_lib_path:
        search_paths.append(synrix_lib_path)
    
    # 2. Auto-detect relative to project root (smoothest for development)
    # Try to find project root by looking for common markers
    current_file = os.path.abspath(__file__)
    python_sdk_dir = os.path.dirname(os.path.dirname(current_file))  # python-sdk/
    project_root = os.path.dirname(python_sdk_dir)  # NebulOS-Scaffolding/
    
    # Check if we're in the project structure (Linux build location)
    if os.path.exists(os.path.join(project_root, "src", "storage", "lattice")):
        lib_path = os.path.join(project_root, "src", "storage", "lattice", lib_name)
        if os.path.exists(lib_path):
            search_paths.append(lib_path)
    
    # Check Windows build location (relative to python-sdk)
    if platform.system() == "Windows":
        # Check in build directory (where CMake outputs the DLL)
        build_paths = [
                os.path.join(python_sdk_dir, "..", "bin", lib_name),  # Package bin/ directory (for free tier package)
                os.path.join(python_sdk_dir, "..", "build", "windows", "build_free_tier", "bin", lib_name),
            os.path.join(python_sdk_dir, "..", "build", "windows", "build", "Release", lib_name),
            os.path.join(python_sdk_dir, "..", "build", "windows", "build", "Debug", lib_name),
            os.path.join(python_sdk_dir, "..", "build", "windows", "build", lib_name),
                os.path.join(python_sdk_dir, "bin", lib_name),  # Package bin directory
            os.path.join(python_sdk_dir, lib_name),  # Same directory as SDK
        ]
        search_paths.extend(build_paths)
    
    # 3. Check library path environment variables
    if platform.system() == "Windows":
        # Windows: Check PATH and current directory
        path_env = os.environ.get("PATH", "")
        if path_env:
            for path in path_env.split(os.pathsep):
                lib_in_path = os.path.join(path, lib_name)
                if os.path.exists(lib_in_path):
                    search_paths.append(lib_in_path)
    else:
        # Linux: Check LD_LIBRARY_PATH
        ld_path = os.environ.get("LD_LIBRARY_PATH", "")
        if ld_path:
            for path in ld_path.split(":"):
                lib_in_path = os.path.join(path, lib_name)
                if os.path.exists(lib_in_path):
                    search_paths.append(lib_in_path)
    
    # 4. Standard system locations
    if platform.system() == "Windows":
        search_paths.extend([
            f"./{lib_name}",
            lib_name,
                os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "synrix", "bin", lib_name),
        ])
    else:
        search_paths.extend([
            f"./{lib_name}",
            lib_name,
            "/usr/local/lib/" + lib_name,
            "/usr/lib/" + lib_name,
        ])
        
        # Try all paths for this library name
        for path in search_paths:
            if os.path.exists(path):
                return path
    
    # None of the library names found
    return None

# C Structure definitions (must match persistent_lattice.h)

# Payload union members (largest is lattice_learning_t at 288 bytes)
class LatticeLearning(Structure):
    """lattice_learning_t structure"""
    _fields_ = [
        ("pattern_sequence", c_char * 256),
        ("frequency", c_uint32),
        ("success_rate", c_double),
        ("performance_gain", c_double),
        ("last_used", c_uint64),
        ("evolution_generation", c_uint32),
    ]

# Payload union - use largest member (lattice_learning_t)
class LatticePayload(Union):
    """payload union from lattice_node_t"""
    _fields_ = [
        ("learning", LatticeLearning),  # Largest member (288 bytes)
        # Other members are smaller, so we only need to define the largest
        # for proper structure size alignment
    ]

class LatticeNode(Structure):
    """C lattice_node_t structure - MUST match C struct exactly"""
    _fields_ = [
        ("id", c_uint64),
        ("type", c_uint32),  # lattice_node_type_t
        ("name", c_char * 64),
        ("data", c_char * 512),
        ("parent_id", c_uint64),
        ("child_count", c_uint32),
        ("children", POINTER(c_uint64)),
        ("confidence", c_double),
        ("timestamp", c_uint64),
        ("payload", LatticePayload),  # CRITICAL: Must include payload union
    ]

# Node types (from persistent_lattice.h)
LATTICE_NODE_PRIMITIVE = 1
LATTICE_NODE_KERNEL = 2
LATTICE_NODE_PATTERN = 3
LATTICE_NODE_PERFORMANCE = 4
LATTICE_NODE_LEARNING = 5
LATTICE_NODE_ANTI_PATTERN = 6

class RawSynrixBackend:
    """
    Direct interface to raw C SYNRIX backend.
    
    This provides the fastest possible access by calling C functions directly,
    bypassing HTTP, JSON, and Python object overhead.
    
    Performance:
        - O(1) lookups: ~0.1-1.0μs (raw C)
        - O(k) semantic queries: ~10-100μs (raw C)
        - No Python overhead, no serialization
    """
    
    def __init__(self, lattice_path: str, max_nodes: int = 100000, device_id: int = 0, evaluation_mode: bool = True):
        """
        Initialize raw C backend.

        Args:
            lattice_path: Path to .lattice file (will create if doesn't exist)
            max_nodes: Maximum nodes in RAM cache (default: 100k)
            device_id: Device ID for distributed systems (0 = auto-assign)
            evaluation_mode: If False, disables 50k node limit (unlimited nodes)
        """
        # One-time activation: if backend says key already used, clear it before engine reads it
        try:
            from .license_activate import ensure_license_activated
            ensure_license_activated()
        except Exception:
            pass
        # Suppress debug output from DLL during initialization
        import os
        import sys
        _original_stderr = sys.stderr
        try:
            if sys.platform == 'win32':
                devnull = open(os.devnull, 'w')
            else:
                devnull = open('/dev/null', 'w')
            sys.stderr = devnull
        except:
            pass  # If redirection fails, continue anyway
        
        self.lattice_path = lattice_path
        self.lib = None
        self.lattice_ptr = None
        self._load_library()
        self._init_lattice(max_nodes, device_id, evaluation_mode)
        
        # Restore stderr after initialization
        try:
            sys.stderr = _original_stderr
            devnull.close()
        except:
            pass
    
    def _load_library(self):
        """Load the SYNRIX C shared library using native loader (Windows-native)"""
        # Use native loader if available (handles Windows DLL directory registration)
        if _NATIVE_LOADER_AVAILABLE:
            try:
                self.lib = load_synrix()
                # Define function signatures
                self._setup_function_signatures()
                return
            except OSError as e:
                # Fall back to old method if native loader fails
                pass
        
        # Fallback to old method (for backwards compatibility)
        lib_path = _find_synrix_lib()
        
        if not lib_path:
            import platform
            lib_name = "libsynrix.dll" if platform.system() == "Windows" else "libsynrix.so"
            path_hint = "PATH" if platform.system() == "Windows" else "LD_LIBRARY_PATH"
            raise RuntimeError(
                f"SYNRIX C library ({lib_name}) not found. "
                "Please ensure libsynrix.dll is in the synrix package directory, "
                f"set SYNRIX_LIB_PATH environment variable, "
                f"or add the library directory to {path_hint}.\n"
                f"Example: set SYNRIX_LIB_PATH=C:\\path\\to\\{lib_name}" if platform.system() == "Windows" 
                else f"Example: export SYNRIX_LIB_PATH=/path/to/{lib_name}"
            )
        
        try:
            self.lib = ctypes.CDLL(lib_path)
        except OSError as e:
            raise RuntimeError(f"Failed to load SYNRIX library from {lib_path}: {e}")
        
        # Define function signatures
        self._setup_function_signatures()
    
    def _setup_function_signatures(self):
        """Setup ctypes function signatures for C functions"""
        # lattice_init(lattice, storage_path, max_nodes, device_id) -> int
        self.lib.lattice_init.argtypes = [
            POINTER(ctypes.c_void_p),  # persistent_lattice_t*
            c_char_p,                  # storage_path
            c_uint32,                  # max_nodes
            c_uint32                   # device_id
        ]
        self.lib.lattice_init.restype = c_int
        
        # lattice_add_node(lattice, type, name, data, parent_id) -> uint64_t
        self.lib.lattice_add_node.argtypes = [
            POINTER(ctypes.c_void_p),  # persistent_lattice_t*
            c_uint32,                  # type
            c_char_p,                  # name
            c_char_p,                  # data
            c_uint64                   # parent_id
        ]
        self.lib.lattice_add_node.restype = c_uint64
        
        # lattice_get_node_data(lattice, id, out_node) -> int
        self.lib.lattice_get_node_data.argtypes = [
            POINTER(ctypes.c_void_p),  # persistent_lattice_t*
            c_uint64,                  # id
            POINTER(LatticeNode)       # out_node
        ]
        self.lib.lattice_get_node_data.restype = c_int
        
        # lattice_get_node_copy(lattice, id) -> POINTER(LatticeNode)
        if hasattr(self.lib, 'lattice_get_node_copy'):
            self.lib.lattice_get_node_copy.argtypes = [
                POINTER(ctypes.c_void_p),  # persistent_lattice_t*
                c_uint64                    # id
            ]
            self.lib.lattice_get_node_copy.restype = POINTER(LatticeNode)
        
        # lattice_free_node_copy(node) -> void
        if hasattr(self.lib, 'lattice_free_node_copy'):
            self.lib.lattice_free_node_copy.argtypes = [POINTER(LatticeNode)]
            self.lib.lattice_free_node_copy.restype = None
        
        # lattice_find_nodes_by_name(lattice, name, node_ids, max_ids) -> uint32_t
        self.lib.lattice_find_nodes_by_name.argtypes = [
            POINTER(ctypes.c_void_p),  # persistent_lattice_t*
            c_char_p,                  # name (prefix)
            POINTER(c_uint64),         # node_ids (output)
            c_uint32                   # max_ids
        ]
        self.lib.lattice_find_nodes_by_name.restype = c_uint32
        
        # lattice_get_node_data(lattice, id, out_node) -> int (0 = success)
        if hasattr(self.lib, 'lattice_get_node_data'):
            self.lib.lattice_get_node_data.argtypes = [
                POINTER(ctypes.c_void_p),  # persistent_lattice_t*
                c_uint64,                  # id
                POINTER(LatticeNode)       # out_node
            ]
            self.lib.lattice_get_node_data.restype = c_int
        
        # lattice_save(lattice) -> int
        self.lib.lattice_save.argtypes = [POINTER(ctypes.c_void_p)]
        self.lib.lattice_save.restype = c_int
        
        # lattice_cleanup(lattice) -> void
        if hasattr(self.lib, 'lattice_cleanup'):
            self.lib.lattice_cleanup.argtypes = [POINTER(ctypes.c_void_p)]
            self.lib.lattice_cleanup.restype = None
        
        # lattice_add_node_binary(lattice, type, name, data, data_len, parent_id) -> uint64_t
        if hasattr(self.lib, 'lattice_add_node_binary'):
            self.lib.lattice_add_node_binary.argtypes = [
                POINTER(ctypes.c_void_p),  # persistent_lattice_t*
                c_uint32,                  # type
                c_char_p,                  # name
                ctypes.c_void_p,           # data (void*)
                ctypes.c_size_t,           # data_len
                c_uint64                   # parent_id
            ]
            self.lib.lattice_add_node_binary.restype = c_uint64
        
        # lattice_add_node_chunked(lattice, type, name, data, data_len, parent_id) -> uint64_t
        if hasattr(self.lib, 'lattice_add_node_chunked'):
            self.lib.lattice_add_node_chunked.argtypes = [
                POINTER(ctypes.c_void_p),  # persistent_lattice_t*
                c_uint32,                  # type
                c_char_p,                  # name
                ctypes.c_void_p,           # data (void*)
                ctypes.c_size_t,           # data_len
                c_uint64                   # parent_id
            ]
            self.lib.lattice_add_node_chunked.restype = c_uint64
        
        # Python-friendly wrappers (avoid void** issues)
        # lattice_get_node_chunked_size(lattice, parent_id) -> ssize_t
        # Note: ssize_t is typically c_long or c_int64 depending on platform
        if hasattr(self.lib, 'lattice_get_node_chunked_size'):
            self.lib.lattice_get_node_chunked_size.argtypes = [
                POINTER(ctypes.c_void_p),  # lattice
                c_uint64                   # parent_id
            ]
            # ssize_t is typically the same as c_long (signed size_t)
            self.lib.lattice_get_node_chunked_size.restype = ctypes.c_long
        
        # lattice_get_node_chunked_to_buffer(lattice, parent_id, buffer, buffer_size) -> ssize_t
        if hasattr(self.lib, 'lattice_get_node_chunked_to_buffer'):
            self.lib.lattice_get_node_chunked_to_buffer.argtypes = [
                POINTER(ctypes.c_void_p),  # lattice
                c_uint64,                   # parent_id
                ctypes.c_void_p,            # buffer (pre-allocated, void*)
                ctypes.c_size_t            # buffer_size
            ]
            self.lib.lattice_get_node_chunked_to_buffer.restype = ctypes.c_long
        
        # lattice_get_last_error(lattice) -> int (error code)
        if hasattr(self.lib, 'lattice_get_last_error'):
            self.lib.lattice_get_last_error.argtypes = [POINTER(ctypes.c_void_p)]
            self.lib.lattice_get_last_error.restype = c_int
        
        # lattice_disable_evaluation_mode(lattice) -> int
        if hasattr(self.lib, 'lattice_disable_evaluation_mode'):
            self.lib.lattice_disable_evaluation_mode.argtypes = [POINTER(ctypes.c_void_p)]
            self.lib.lattice_disable_evaluation_mode.restype = c_int
        
        # lattice_get_hardware_id(hwid_out, hwid_size) -> int
        if hasattr(self.lib, 'lattice_get_hardware_id'):
            self.lib.lattice_get_hardware_id.argtypes = [c_char_p, ctypes.c_size_t]
            self.lib.lattice_get_hardware_id.restype = c_int
        
        # lattice_configure_persistence(lattice, auto_save_enabled, interval_nodes, interval_seconds, save_on_pressure) -> void
        if hasattr(self.lib, 'lattice_configure_persistence'):
            self.lib.lattice_configure_persistence.argtypes = [
                POINTER(ctypes.c_void_p),  # persistent_lattice_t*
                ctypes.c_bool,             # auto_save_enabled
                c_uint32,                  # interval_nodes
                c_uint32,                  # interval_seconds
                ctypes.c_bool              # save_on_pressure
            ]
            self.lib.lattice_configure_persistence.restype = None
    
    def _init_lattice(self, max_nodes: int, device_id: int, evaluation_mode: bool = True):
        """Initialize the C lattice structure"""
        # Allocate memory for persistent_lattice_t
        # We need to know the size - estimate or use a large buffer
        # For now, we'll use a pointer and let C manage it
        lattice_size = 1024 * 1024  # 1MB should be enough for the struct
        self.lattice_ptr = (ctypes.c_void_p * (lattice_size // 8))()
        
        # Convert path to bytes
        path_bytes = self.lattice_path.encode('utf-8')
        
        # Call lattice_init
        result = self.lib.lattice_init(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            path_bytes,
            max_nodes,
            device_id
        )
        
        if result != 0:
            raise RuntimeError(f"Failed to initialize lattice: error code {result}")
        
        # Disable evaluation mode if requested (unlimited nodes)
        if not evaluation_mode and hasattr(self.lib, 'lattice_disable_evaluation_mode'):
            self.lib.lattice_disable_evaluation_mode(ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)))
        
        # Note: WAL is already enabled by lattice_init() by default
        # If you need to re-enable or change WAL settings, use lattice_enable_wal()
        # but it only takes the lattice pointer (no wal_path parameter)
        # WAL path is auto-generated from storage_path + ".wal"
        
        # Note: lattice_init() already calls lattice_load() internally, so we don't need to call it again
        
        # Setup lattice_build_prefix_index if available
        if hasattr(self.lib, 'lattice_build_prefix_index'):
            self.lib.lattice_build_prefix_index.argtypes = [POINTER(ctypes.c_void_p)]
            self.lib.lattice_build_prefix_index.restype = None
    
    def add_node(self, name: str, data: str, node_type: int = LATTICE_NODE_LEARNING, 
                 parent_id: int = 0) -> int:
        """
        Add a node to the lattice.
        
        Args:
            name: Node name (e.g., "TASK:write_function")
            data: Node data (string, max 512 bytes)
            node_type: Node type (default: LATTICE_NODE_LEARNING)
            parent_id: Parent node ID (0 = no parent)
        
        Returns:
            Node ID (uint64), or 0 on failure
        
        Raises:
            FreeTierLimitError: If free tier limit (25k nodes) is reached
        """
        name_bytes = name.encode('utf-8')[:63]  # Max 64 chars
        data_bytes = data.encode('utf-8')[:511]  # Max 512 chars
        
        node_id = self.lib.lattice_add_node(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            node_type,
            name_bytes,
            data_bytes,
            parent_id
        )
        
        # Check for free tier limit error
        if node_id == 0 and hasattr(self.lib, 'lattice_get_last_error'):
            error_code = self.lib.lattice_get_last_error(
                ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
            )
            if error_code == -100:  # LATTICE_ERROR_FREE_TIER_LIMIT
                raise FreeTierLimitError(
                    "SYNRIX: Free Tier limit reached (25,000 nodes). "
                    "No new nodes can be added to the lattice. "
                    "Options: Delete existing nodes to free space, upgrade to Pro tier for unlimited nodes at synrix.io, or contact support for assistance."
                )
        
        return node_id
    
    def add_node_binary(self, name: str, data: bytes, node_type: int = LATTICE_NODE_PRIMITIVE,
                       parent_id: int = 0) -> int:
        """
        Add a node with binary data (handles arbitrary binary data including null bytes).
        
        Args:
            name: Node name (e.g., "BINARY:test_data")
            data: Binary data (bytes, max 510 bytes)
            node_type: Node type (default: LATTICE_NODE_PRIMITIVE)
            parent_id: Parent node ID (0 = no parent)
        
        Returns:
            Node ID (uint64), or 0 on failure
        """
        if not hasattr(self.lib, 'lattice_add_node_binary'):
            raise RuntimeError("lattice_add_node_binary not available in library")
        
        if len(data) > 510:
            raise ValueError(f"Binary data too large: {len(data)} bytes (max 510)")
        
        name_bytes = name.encode('utf-8')[:63]  # Max 64 chars
        
        # Create ctypes array from bytes
        data_array = (ctypes.c_uint8 * len(data)).from_buffer_copy(data)
        
        node_id = self.lib.lattice_add_node_binary(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            node_type,
            name_bytes,
            ctypes.cast(data_array, ctypes.c_void_p),
            len(data),
            parent_id
        )
        
        # Check for free tier limit error
        if node_id == 0 and hasattr(self.lib, 'lattice_get_last_error'):
            error_code = self.lib.lattice_get_last_error(
                ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
            )
            if error_code == -100:  # LATTICE_ERROR_FREE_TIER_LIMIT
                raise FreeTierLimitError(
                    "SYNRIX: Free Tier limit reached (25,000 nodes). "
                    "No new nodes can be added to the lattice. "
                    "Options: Delete existing nodes to free space, upgrade to Pro tier for unlimited nodes at synrix.io, or contact support for assistance."
                )
        
        return node_id
    
    def add_node_chunked(self, name: str, data: bytes, node_type: int = LATTICE_NODE_PRIMITIVE,
                        parent_id: int = 0) -> int:
        """
        Add a node with chunked data (for data > 510 bytes).
        Automatically chunks data into 500-byte chunks.
        
        Args:
            name: Node name (e.g., "BINARY:large_file")
            data: Binary data (bytes, any size)
            node_type: Node type (default: LATTICE_NODE_PRIMITIVE)
            parent_id: Parent node ID (0 = no parent)
        
        Returns:
            Parent node ID (uint64), or 0 on failure
        """
        if not hasattr(self.lib, 'lattice_add_node_chunked'):
            raise RuntimeError("lattice_add_node_chunked not available in library")
        
        name_bytes = name.encode('utf-8')[:63]  # Max 64 chars
        
        # Create ctypes array from bytes
        data_array = (ctypes.c_uint8 * len(data)).from_buffer_copy(data)
        
        parent_id = self.lib.lattice_add_node_chunked(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            node_type,
            name_bytes,
            ctypes.cast(data_array, ctypes.c_void_p),
            len(data),
            parent_id
        )
        
        # Check for free tier limit error
        if parent_id == 0 and hasattr(self.lib, 'lattice_get_last_error'):
            error_code = self.lib.lattice_get_last_error(
                ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
            )
            if error_code == -100:  # LATTICE_ERROR_FREE_TIER_LIMIT
                raise FreeTierLimitError(
                    "SYNRIX: Free Tier limit reached (25,000 nodes). "
                    "No new nodes can be added to the lattice. "
                    "Options: Delete existing nodes to free space, upgrade to Pro tier for unlimited nodes at synrix.io, or contact support for assistance."
                )
        
        return parent_id
    
    def get_node_chunked(self, parent_id: int) -> Optional[bytes]:
        """
        Reassemble chunked data from a chunked node.
        
        Args:
            parent_id: Node ID of the chunked data header (node with "C:" prefix)
        
        Returns:
            Reassembled data as bytes, or None if error
        
        Note:
            Uses Python-friendly wrapper functions that avoid void** pointer issues.
        """
        if not hasattr(self.lib, 'lattice_get_node_chunked_size'):
            return None
        
        # Step 1: Get the size
        size = self.lib.lattice_get_node_chunked_size(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            parent_id
        )
        
        if size < 0:
            return None
        
        # Step 2: Allocate buffer in Python using create_string_buffer
        # This creates a mutable buffer that can be passed directly to C
        buffer_size = size
        buffer = ctypes.create_string_buffer(buffer_size)
        
        # Step 3: Copy data to buffer
        # create_string_buffer returns a pointer-like object that can be passed as void*
        result = self.lib.lattice_get_node_chunked_to_buffer(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            parent_id,
            buffer,  # create_string_buffer can be passed directly as void*
            buffer_size
        )
        
        if result < 0:
            if result == -2:
                # Buffer too small (shouldn't happen, but handle gracefully)
                return None
            return None
        
        # Convert to Python bytes (result is the actual size written)
        return bytes(buffer[:result])
    
    def get_node(self, node_id: int) -> Optional[Dict[str, Any]]:
        """
        Get a node by ID (O(1) lookup).
        
        Args:
            node_id: Node ID to retrieve
        
        Returns:
            Dict with node data, or None if not found
        """
        # Use lattice_get_node_copy which properly allocates and we free with lattice_free_node_copy
        # This is the SAFE API designed for this use case
        node_ptr = self.lib.lattice_get_node_copy(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            node_id
        )
        
        if not node_ptr:
            return None
        
        node = node_ptr.contents  # Dereference the pointer
        
        # Extract children array before freeing (C code allocates it with malloc)
        children = None
        if node.child_count > 0 and node.children:
            # Copy children to Python list
            children = [node.children[i] for i in range(node.child_count)]
        
        # Convert C struct to Python dict
        result_dict = {
            "id": node.id,
            "type": node.type,
            "name": node.name.decode('utf-8', errors='ignore').rstrip('\x00'),
            "data": node.data.decode('utf-8', errors='ignore').rstrip('\x00'),
            "parent_id": node.parent_id,
            "child_count": node.child_count,
            "children": children,
            "confidence": node.confidence,
            "timestamp": node.timestamp,
        }
        
        # Free the node copy (this properly frees both the struct and children array)
        self.lib.lattice_free_node_copy(node_ptr)
        
        return result_dict
    
    def find_by_prefix(self, prefix: str, limit: int = 100, raw: bool = True) -> List[Dict[str, Any]]:
        """
        Find nodes by name prefix (O(k) semantic search).
        
        AI agent-first design: Returns bytes by default (raw=True) for maximum performance.
        String decoding adds ~500-600μs overhead per 100 nodes and is only needed for human display.
        
        Args:
            prefix: Name prefix to search for (e.g., "TASK:")
            limit: Maximum number of results
            raw: If True (default), return bytes (fast, AI-optimized)
                 If False, decode to Python strings (slower, human-friendly)
        
        Returns:
            List of node dicts (bytes if raw=True, decoded strings if raw=False)
            
        Note:
            For AI agents: Use raw=True (default) - decode only when needed for display/logging.
            For human users: Use raw=False if you need Python strings immediately.
            The C struct provides bytes (c_char arrays) - decoding is optional convenience.
        """
        # Allocate buffer for node IDs
        node_ids = (c_uint64 * limit)()
        prefix_bytes = prefix.encode('utf-8')
        
        count = self.lib.lattice_find_nodes_by_name(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
            prefix_bytes,
            node_ids,
            limit
        )
        
        # Use get_node() for each result (safer, handles errors)
        results = []
        
        for i in range(count):
            node_id = node_ids[i]
            if node_id == 0:
                continue  # Skip invalid node IDs
            
            # Use get_node() which handles errors gracefully
            try:
                node = self.get_node(node_id)
                if node is None:
                    continue  # Node not found or error
                
                # get_node() already returns a dict, but we need to handle raw mode
                if raw:
                    # Ensure name and data are bytes
                    if isinstance(node.get('name'), str):
                        node['name'] = node['name'].encode('utf-8')
                    if isinstance(node.get('data'), str):
                        node['data'] = node['data'].encode('utf-8')
                
                results.append(node)
            except Exception as e:
                # Skip nodes that cause errors (corrupted or deleted)
                continue
        
        return results
    
    @staticmethod
    def decode_node_strings(node: Dict[str, Any]) -> Dict[str, Any]:
        """
        Helper to decode bytes to strings for a node (lazy decoding for AI agents).
        
        AI agent-first: Decode only when needed, not during query.
        This allows AI agents to work with bytes for fast operations, then decode
        only the fields they actually use.
        
        Args:
            node: Node dict with bytes fields (from find_by_prefix with raw=True)
        
        Returns:
            Node dict with decoded string fields
        """
        decoded = node.copy()
        if isinstance(node.get('name'), bytes):
            decoded['name'] = node['name'].decode('utf-8', errors='ignore').rstrip('\x00')
        if isinstance(node.get('data'), bytes):
            decoded['data'] = node['data'].decode('utf-8', errors='ignore').rstrip('\x00')
        return decoded
    
    def save(self) -> bool:
        """Save lattice to disk"""
        result = self.lib.lattice_save(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
        )
        return result == 0
    
    def flush(self) -> bool:
        """
        Manually flush WAL buffer to disk (for durability guarantees).
        
        WAL uses batching for performance - entries are buffered in memory
        and only flushed when the batch is full (12.5k-50k entries) or on checkpoint.
        This function forces an immediate flush of any pending WAL entries.
        
        Returns:
            True if flush was successful, False otherwise
        """
        if not hasattr(self.lib, 'lattice_wal_flush'):
            # Try to find wal_flush directly
            if hasattr(self.lib, 'wal_flush'):
                if not hasattr(self.lib.wal_flush, 'argtypes'):
                    self.lib.wal_flush.argtypes = [POINTER(ctypes.c_void_p)]
                    self.lib.wal_flush.restype = c_int
                result = self.lib.wal_flush(
                    ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
                )
                return result == 0
            return False
        
        if not hasattr(self.lib.lattice_wal_flush, 'argtypes'):
            self.lib.lattice_wal_flush.argtypes = [POINTER(ctypes.c_void_p)]
            self.lib.lattice_wal_flush.restype = c_int
        result = self.lib.lattice_wal_flush(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
        )
        return result == 0
    
    def checkpoint(self) -> bool:
        """
        Force WAL checkpoint (apply WAL entries to main file, then mark as applied).
        
        This function:
        1. Flushes any pending WAL entries to disk
        2. Recovers/replays WAL entries to memory
        3. Saves the lattice to main file
        4. Marks WAL entries as checkpointed
        
        Use this for durability guarantees - ensures all WAL entries are persisted.
        """
        if not hasattr(self.lib, 'lattice_wal_checkpoint'):
            return False
        if not hasattr(self.lib.lattice_wal_checkpoint, 'argtypes'):
            self.lib.lattice_wal_checkpoint.argtypes = [POINTER(ctypes.c_void_p)]
            self.lib.lattice_wal_checkpoint.restype = c_int
        result = self.lib.lattice_wal_checkpoint(
            ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
        )
        return result == 0
    
    def get_hardware_id(self) -> Optional[str]:
        """
        Get hardware ID (stable, unique identifier for license tracking).
        
        Returns:
            Hardware ID string (64 hex characters), or None on error
        """
        if not hasattr(self.lib, 'lattice_get_hardware_id'):
            return None
        
        hwid_buffer = ctypes.create_string_buffer(65)  # 64 chars + null terminator
        result = self.lib.lattice_get_hardware_id(hwid_buffer, 65)
        
        if result == 0:
            return hwid_buffer.value.decode('utf-8')
        return None
    
    def get_usage_info(self) -> Dict[str, Any]:
        """
        Get current usage information for AI agents to report to users.
        
        Returns:
            Dict with keys: 'current', 'limit', 'percentage', 'remaining'
            Example: {'current': 25000, 'limit': 25000, 'percentage': 100, 'remaining': 0}
        """
        # Count nodes by querying all (empty prefix gets all)
        all_nodes = self.find_by_prefix("", limit=30000)
        current = len(all_nodes)
        limit = 25000  # Free tier limit
        percentage = round((current / limit) * 100) if limit > 0 else 0
        remaining = max(0, limit - current)
        
        return {
            'current': current,
            'limit': limit,
            'percentage': percentage,
            'remaining': remaining
        }
    
    def build_prefix_index(self):
        """Build prefix index for O(k) queries (call after adding many nodes)"""
        if hasattr(self.lib, 'lattice_build_prefix_index'):
            self.lib.lattice_build_prefix_index(ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)))
        else:
            # Index will be built automatically on first query, but explicit build is faster for large datasets
            pass
    
    def configure_persistence(self, auto_save_enabled: bool = True, 
                             interval_nodes: int = 5000, 
                             interval_seconds: int = 300, 
                             save_on_pressure: bool = True):
        """
        Configure persistence settings (auto-save behavior).
        
        Args:
            auto_save_enabled: Enable/disable automatic periodic saves
            interval_nodes: Save every N nodes (0 = disabled)
            interval_seconds: Save every T seconds (0 = disabled)
            save_on_pressure: Save snapshot when RAM fills (90% capacity)
        """
        if hasattr(self.lib, 'lattice_configure_persistence'):
            self.lib.lattice_configure_persistence(
                ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p)),
                auto_save_enabled,
                interval_nodes,
                interval_seconds,
                save_on_pressure
            )
    
    def close(self):
        """Cleanup and close the lattice"""
        if self.lattice_ptr and hasattr(self, 'lib') and self.lib:
            try:
                if hasattr(self.lib, 'lattice_cleanup'):
                    self.lib.lattice_cleanup(
                        ctypes.cast(self.lattice_ptr, POINTER(ctypes.c_void_p))
                    )
            except Exception as e:
                # Ignore errors during cleanup - library may already be unloaded
                pass
        self.lattice_ptr = None
        if hasattr(self, 'lib'):
            self.lib = None
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


