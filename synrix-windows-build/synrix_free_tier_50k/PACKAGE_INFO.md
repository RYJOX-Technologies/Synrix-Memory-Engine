# SYNRIX Free Tier Package - Information

## Package Contents

This package contains:
- ✅ **Python SDK** - Complete SYNRIX Python interface
- ✅ **Windows DLLs** - All required libraries (no external dependencies)
- ✅ **Documentation** - Comprehensive guides for AI agents
- ✅ **Examples** - Usage examples and patterns

## Version Information

- **Package Version**: 0.1.0
- **Free Tier Limit**: 50,000 nodes (documented)
- **Platform**: Windows (Linux/macOS support via shared libraries)
- **Python**: 3.8+ required

## What's Included

### Core Files
- `synrix/` - Python package directory
  - `__init__.py` - Package initialization
  - `_native.py` - Windows-native DLL loader
  - `raw_backend.py` - Direct C backend interface
  - `ai_memory.py` - AI agent memory interface
  - `*.dll` - Windows DLLs (libsynrix.dll + dependencies)

### Documentation
- `README.md` - Quick start and API reference
- `AI_AGENT_GUIDE.md` - Comprehensive AI agent guide
- `INSTALL.md` - Installation instructions
- `PACKAGE_INFO.md` - This file

### Setup
- `setup.py` - Python package setup script

## Key Features

1. **Windows-Native**: No scripts, no environment variables needed
2. **Self-Contained**: All DLLs included, no external dependencies
3. **Fast**: O(1) lookups (~131.5 ns), O(k) queries (~10-100 μs)
4. **Persistent**: Survives crashes, reboots, updates
5. **Data-Agnostic**: Text, binary, structured, unstructured

## Installation

See `INSTALL.md` for detailed installation instructions.

Quick start:
```python
import sys
sys.path.insert(0, '/path/to/synrix_free_tier_50k')
from synrix.ai_memory import get_ai_memory
```

## Usage

See `README.md` for API reference and `AI_AGENT_GUIDE.md` for comprehensive examples.

## Notes

- **Node Limit**: The package is configured for 50k nodes. The actual C-level enforcement may vary based on build configuration.
- **Windows DLLs**: All required MinGW runtime DLLs are included.
- **Cross-Platform**: While this package includes Windows DLLs, the Python code works on Linux/macOS with appropriate shared libraries.

## Support

For questions or issues:
1. Check `AI_AGENT_GUIDE.md` for examples
2. Review `README.md` for API reference
3. Contact support for assistance

## License

See LICENSE file for details.
