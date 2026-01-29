"""
SYNRIX Direct Client (Shared Memory)

Ultra-low latency access via mmap shared memory.
Bypasses HTTP overhead for sub-millisecond queries.
"""

import mmap
import os
import struct
import json
import time
from typing import Optional, Dict, List, Any
from .exceptions import SynrixError, SynrixConnectionError


# Shared memory constants (must match synrix_shared_memory.h)
SYNRIX_SHM_NAME = "/synrix_qdrant_shm"
SYNRIX_MAX_QUERY_SIZE = 4096
SYNRIX_MAX_RESPONSE_SIZE = 65536

# Structure sizes (from actual C structures)
CONTROL_SIZE = 56  # synrix_shm_control_t
COMMAND_SIZE = 4184  # synrix_shm_command_t
RESPONSE_SIZE = 65552  # synrix_shm_response_t
SEGMENT_SIZE = CONTROL_SIZE + COMMAND_SIZE + RESPONSE_SIZE  # Total segment size

# Exact offsets (calculated from C structure layout)
# Control structure offsets (0-55)
OFFSET_CONTROL_SERVER_READY = 0
OFFSET_CONTROL_CLIENT_READY = 1
OFFSET_CONTROL_REQUEST_COUNT = 8  # After 2 bools, aligned to 8 bytes
OFFSET_CONTROL_TOTAL_LATENCY_NS = 16
OFFSET_CONTROL_SERVER_VERSION = 24

# Command structure offsets (56-4239)
OFFSET_COMMAND_START = CONTROL_SIZE  # 56
OFFSET_COMMAND_COMMAND = OFFSET_COMMAND_START + 0  # 56
OFFSET_COMMAND_QUERY = OFFSET_COMMAND_START + 64  # 120
OFFSET_COMMAND_QUERY_LEN = OFFSET_COMMAND_START + 4160  # 4216
OFFSET_COMMAND_RESPONSE_LEN = OFFSET_COMMAND_START + 4164  # 4220
OFFSET_COMMAND_STATUS_CODE = OFFSET_COMMAND_START + 4168  # 4224
OFFSET_COMMAND_READY = OFFSET_COMMAND_START + 4172  # 4228
OFFSET_COMMAND_TIMESTAMP_NS = OFFSET_COMMAND_START + 4180  # 4236

# Response structure offsets (4240-69791)
OFFSET_RESPONSE_START = CONTROL_SIZE + COMMAND_SIZE  # 4240
OFFSET_RESPONSE_RESPONSE = OFFSET_RESPONSE_START + 0  # 4240
OFFSET_RESPONSE_RESPONSE_LEN = OFFSET_RESPONSE_START + 65536  # 69776
OFFSET_RESPONSE_STATUS_CODE = OFFSET_RESPONSE_START + 65540  # 69780
OFFSET_RESPONSE_LATENCY_NS = OFFSET_RESPONSE_START + 65548  # 69788


class SynrixDirectClient:
    """
    Direct shared memory client for SYNRIX.
    
    Provides sub-millisecond access via mmap, bypassing HTTP overhead.
    
    Example:
        >>> client = SynrixDirectClient()
        >>> node_id = client.add_node("ISA_ADD", "Addition", collection="test")
        >>> results = client.query_prefix("ISA_", collection="test")
    """
    
    def __init__(self, shm_name: str = SYNRIX_SHM_NAME):
        """
        Initialize direct shared memory client.
        
        Args:
            shm_name: Shared memory segment name (default: /synrix_qdrant_shm)
        
        Raises:
            SynrixConnectionError: If shared memory is not available
        """
        self.shm_name = shm_name
        self.shm_fd = None
        self.shm = None
        
        try:
            # POSIX shared memory on Linux is in /dev/shm/
            # shm_open creates it there automatically
            shm_path = f"/dev/shm{shm_name}"
            
            # Try /dev/shm path first
            if not os.path.exists(shm_path):
                # Fallback: try direct path (some systems)
                shm_path = shm_name
            
            self.shm_fd = os.open(shm_path, os.O_RDWR)
            if self.shm_fd < 0:
                raise SynrixConnectionError(
                    f"Failed to open shared memory: {shm_name}. "
                    "Make sure SYNRIX server is running with shared memory enabled."
                )
            
            # Map shared memory
            self.shm = mmap.mmap(self.shm_fd, SEGMENT_SIZE, access=mmap.ACCESS_WRITE)
            
            # Wait for server to be ready
            wait_count = 0
            while wait_count < 50:
                server_ready = struct.unpack('?', self.shm[OFFSET_CONTROL_SERVER_READY:OFFSET_CONTROL_SERVER_READY+1])[0]
                if server_ready:
                    break
                time.sleep(0.1)
                wait_count += 1
            
            if not struct.unpack('?', self.shm[OFFSET_CONTROL_SERVER_READY:OFFSET_CONTROL_SERVER_READY+1])[0]:
                self.close()
                raise SynrixConnectionError("Server not ready after 5 seconds")
            
            # Mark client as ready
            self.shm[OFFSET_CONTROL_CLIENT_READY:OFFSET_CONTROL_CLIENT_READY+1] = struct.pack('?', True)
            
        except FileNotFoundError:
            raise SynrixConnectionError(
                f"Shared memory not found: {shm_name}. "
                "Make sure SYNRIX server is running with shared memory enabled."
            )
        except Exception as e:
            self.close()
            raise SynrixConnectionError(f"Failed to connect to shared memory: {e}")
    
    def _query(self, command: str, query: str) -> Dict[str, Any]:
        """Send command via shared memory"""
        if not self.shm:
            raise SynrixConnectionError("Not connected to shared memory")
        
        # Wait for previous command to complete (check ready flag)
        wait_count = 0
        while wait_count < 100:  # 10ms timeout
            ready = struct.unpack('?', self.shm[OFFSET_COMMAND_READY:OFFSET_COMMAND_READY+1])[0]
            if ready:
                break
            time.sleep(0.0001)  # 100μs
            wait_count += 1
        
        # Write command
        cmd_bytes = command.encode('utf-8')[:63]  # Max 64 bytes, leave room for null
        self.shm[OFFSET_COMMAND_COMMAND:OFFSET_COMMAND_COMMAND+64] = cmd_bytes.ljust(64, b'\0')[:64]
        
        # Write query
        query_bytes = query.encode('utf-8')[:SYNRIX_MAX_QUERY_SIZE-1]
        self.shm[OFFSET_COMMAND_QUERY:OFFSET_COMMAND_QUERY+len(query_bytes)] = query_bytes
        self.shm[OFFSET_COMMAND_QUERY+len(query_bytes):OFFSET_COMMAND_QUERY+len(query_bytes)+1] = b'\0'
        
        # Write query_len
        self.shm[OFFSET_COMMAND_QUERY_LEN:OFFSET_COMMAND_QUERY_LEN+4] = struct.pack('I', len(query_bytes))
        
        # Set ready to false to signal server
        self.shm[OFFSET_COMMAND_READY:OFFSET_COMMAND_READY+1] = struct.pack('?', False)
        
        # Wait for response (busy-wait for first 100 iterations for low latency)
        wait_count = 0
        while wait_count < 10000:  # 1 second timeout
            ready = struct.unpack('?', self.shm[OFFSET_COMMAND_READY:OFFSET_COMMAND_READY+1])[0]
            if ready:
                break
            # Busy-wait for first 100 iterations (~10μs), then sleep
            if wait_count < 100:
                pass  # Busy-wait (check immediately)
            else:
                time.sleep(0.00001)  # 10μs after initial busy-wait
            wait_count += 1
        
        if not ready:
            raise SynrixError("Command timeout")
        
        # Read status code
        status_code = struct.unpack('I', self.shm[OFFSET_RESPONSE_STATUS_CODE:OFFSET_RESPONSE_STATUS_CODE+4])[0]
        
        if status_code != 200:
            raise SynrixError(f"Command failed with status {status_code}")
        
        # Read response length
        response_len = struct.unpack('I', self.shm[OFFSET_RESPONSE_RESPONSE_LEN:OFFSET_RESPONSE_RESPONSE_LEN+4])[0]
        
        # Read response data (mmap slice returns bytes directly)
        response_bytes = self.shm[OFFSET_RESPONSE_RESPONSE:OFFSET_RESPONSE_RESPONSE+response_len]
        response_str = response_bytes.decode('utf-8', errors='ignore')
        
        try:
            return json.loads(response_str)
        except json.JSONDecodeError:
            return {"raw": response_str}
    
    def create_collection(self, collection: str, vector_dim: Optional[int] = None) -> bool:
        """Create a collection (not yet implemented in shared memory)"""
        # Fallback: would need HTTP or implement in server
        return True
    
    def get_collection(self, collection: str) -> Dict[str, Any]:
        """Get collection info"""
        return self._query("GET_COLLECTION", collection)
    
    def list_collections(self) -> List[str]:
        """List all collections (not yet implemented)"""
        return []
    
    def add_node(self, name: str, data: str, collection: str = "default") -> Optional[int]:
        """Add a node to the knowledge graph"""
        # Format: "collection:name|data"
        query = f"{collection}:{name}|{data}"
        result = self._query("ADD_NODE", query)
        # Extract node ID from result
        if "result" in result and "id" in result["result"]:
            return result["result"]["id"]
        return None
    
    def query_prefix(self, prefix: str, collection: str = "default", limit: int = 100) -> List[Dict[str, Any]]:
        """Query nodes by prefix (O(k) where k = results)"""
        # Format: "collection:prefix|limit"
        query = f"{collection}:{prefix}|{limit}"
        result = self._query("QUERY_PREFIX", query)
        # Parse results
        if "result" in result and "points" in result["result"]:
            return result["result"]["points"]
        return []
    
    def get_node_by_id(self, node_id: int) -> Optional[Dict[str, Any]]:
        """
        O(1) direct lookup by node ID.
        
        Args:
            node_id: Node ID to lookup
            
        Returns:
            Node data dictionary or None if not found
            
        Example:
            >>> node = client.get_node_by_id(12345)
            >>> print(node["payload"]["name"])
        """
        result = self._query("GET_NODE_BY_ID", str(node_id))
        if "result" in result:
            return result["result"]
        return None
    
    def close(self):
        """Close shared memory connection"""
        if self.shm:
            self.shm.close()
            self.shm = None
        if self.shm_fd:
            os.close(self.shm_fd)
            self.shm_fd = None
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
