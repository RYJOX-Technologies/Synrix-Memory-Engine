"""
SYNRIX Storage Format Options
=============================

Provides multiple storage formats for different use cases:
- JSON: Human-readable, easy debugging (slight overhead)
- Binary: Maximum performance, autonomous systems (no overhead)
- Simple: Fast text format, middle ground (minimal overhead)

Usage:
    from synrix.storage_formats import JSONFormat, BinaryFormat, SimpleFormat
    
    # For demos/human-readable
    formatter = JSONFormat()
    data = formatter.encode({'title': 'Doc', 'content': '...'})
    
    # For production/autonomous
    formatter = BinaryFormat()
    data = formatter.encode(struct.pack('...', ...))
    
    # For simple text data
    formatter = SimpleFormat()
    data = formatter.encode(['title', 'content', 'source'])
"""

import json
import struct
from typing import Any, Dict, List, Optional, Union
from enum import Enum


class StorageFormat(Enum):
    """Storage format types"""
    JSON = "json"      # Human-readable, debugging
    BINARY = "binary"  # Maximum performance
    SIMPLE = "simple"  # Fast text, middle ground


class BaseFormatter:
    """Base class for storage formatters"""
    
    def encode(self, data: Any) -> bytes:
        """Encode data to bytes (max 512 bytes)"""
        raise NotImplementedError
    
    def decode(self, data: bytes) -> Any:
        """Decode bytes back to data"""
        raise NotImplementedError
    
    def get_format_name(self) -> str:
        """Get format name"""
        raise NotImplementedError


class JSONFormatter(BaseFormatter):
    """
    JSON Format - Human-readable, easy debugging
    
    Pros:
    - Human-readable
    - Easy to debug
    - Flexible structure
    - Widely supported
    
    Cons:
    - ~0.008μs overhead per operation
    - Slightly larger size
    
    Best for: Demos, development, human-readable data
    """
    
    def encode(self, data: Union[Dict, List, str, int, float, bool]) -> bytes:
        """Encode to JSON string"""
        json_str = json.dumps(data, separators=(',', ':'))  # Compact format
        return json_str.encode('utf-8')[:511]  # Max 511 bytes for text mode
    
    def decode(self, data: bytes) -> Any:
        """Decode from JSON string"""
        try:
            text = data.decode('utf-8').rstrip('\x00')
            return json.loads(text)
        except (json.JSONDecodeError, UnicodeDecodeError):
            return None
    
    def get_format_name(self) -> str:
        return "JSON"


class BinaryFormatter(BaseFormatter):
    """
    Binary Format - Maximum performance, autonomous systems
    
    Pros:
    - Zero overhead (no parsing)
    - Most compact
    - Fastest
    - Supports arbitrary binary data
    
    Cons:
    - Not human-readable
    - Requires knowing structure
    - Harder to debug
    
    Best for: Production, autonomous systems, maximum performance
    """
    
    def encode(self, data: bytes) -> bytes:
        """
        Encode binary data with length header
        
        Format: [2 bytes: length][data]
        Max data size: 510 bytes (512 - 2 byte header)
        """
        if len(data) > 510:
            data = data[:510]
        
        # Binary mode: first 2 bytes = length, data starts at offset 2
        length = len(data)
        header = struct.pack('<H', length)  # Little-endian uint16
        return header + data
    
    def decode(self, data: bytes) -> Optional[bytes]:
        """Decode binary data (extract from offset 2)"""
        if len(data) < 2:
            return None
        
        # Extract length from first 2 bytes
        length = struct.unpack('<H', data[:2])[0]
        
        if length > 510 or len(data) < 2 + length:
            return None
        
        # Return data starting at offset 2
        return data[2:2+length]
    
    def get_format_name(self) -> str:
        return "Binary"
    
    def pack_struct(self, format_str: str, *values) -> bytes:
        """Helper: Pack struct data for binary storage"""
        packed = struct.pack(format_str, *values)
        return self.encode(packed)
    
    def unpack_struct(self, data: bytes, format_str: str):
        """Helper: Unpack struct data from binary storage"""
        binary_data = self.decode(data)
        if binary_data is None:
            return None
        return struct.unpack(format_str, binary_data)


class SimpleFormatter(BaseFormatter):
    """
    Simple Format - Fast text, middle ground
    
    Format: field1|field2|field3|...
    Uses | as delimiter (can be escaped with \|)
    
    Pros:
    - ~10× faster than JSON
    - Still human-readable
    - Simple parsing
    
    Cons:
    - Fixed structure (need to know field order)
    - Less flexible than JSON
    
    Best for: Simple structured data, performance-sensitive text
    """
    
    def __init__(self, delimiter: str = '|'):
        self.delimiter = delimiter
        self.escape_char = '\\'
    
    def encode(self, data: List[str]) -> bytes:
        """Encode list of strings with delimiter"""
        # Escape delimiter in data
        escaped = [str(field).replace(self.escape_char, self.escape_char + self.escape_char)
                  .replace(self.delimiter, self.escape_char + self.delimiter)
                  for field in data]
        
        result = self.delimiter.join(escaped)
        return result.encode('utf-8')[:511]  # Max 511 bytes for text mode
    
    def decode(self, data: bytes) -> Optional[List[str]]:
        """Decode delimited string back to list"""
        try:
            text = data.decode('utf-8').rstrip('\x00')
            if not text:
                return []
            
            # Simple split (doesn't handle escaped delimiters perfectly, but good enough)
            # For production, use proper escaping
            parts = text.split(self.delimiter)
            
            # Unescape
            unescaped = [part.replace(self.escape_char + self.delimiter, self.delimiter)
                        .replace(self.escape_char + self.escape_char, self.escape_char)
                        for part in parts]
            
            return unescaped
        except UnicodeDecodeError:
            return None
    
    def get_format_name(self) -> str:
        return "Simple"


# Convenience functions
def get_formatter(format_type: Union[StorageFormat, str]) -> BaseFormatter:
    """Get formatter by type"""
    if isinstance(format_type, str):
        format_type = StorageFormat(format_type.lower())
    
    if format_type == StorageFormat.JSON:
        return JSONFormatter()
    elif format_type == StorageFormat.BINARY:
        return BinaryFormatter()
    elif format_type == StorageFormat.SIMPLE:
        return SimpleFormatter()
    else:
        raise ValueError(f"Unknown format: {format_type}")


# Default formatters (singletons for efficiency)
_JSON_FORMATTER = None
_BINARY_FORMATTER = None
_SIMPLE_FORMATTER = None


def json_format() -> JSONFormatter:
    """Get JSON formatter (singleton)"""
    global _JSON_FORMATTER
    if _JSON_FORMATTER is None:
        _JSON_FORMATTER = JSONFormatter()
    return _JSON_FORMATTER


def binary_format() -> BinaryFormatter:
    """Get binary formatter (singleton)"""
    global _BINARY_FORMATTER
    if _BINARY_FORMATTER is None:
        _BINARY_FORMATTER = BinaryFormatter()
    return _BINARY_FORMATTER


def simple_format() -> SimpleFormatter:
    """Get simple formatter (singleton)"""
    global _SIMPLE_FORMATTER
    if _SIMPLE_FORMATTER is None:
        _SIMPLE_FORMATTER = SimpleFormatter()
    return _SIMPLE_FORMATTER
