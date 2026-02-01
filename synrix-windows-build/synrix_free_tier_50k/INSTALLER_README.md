# SYNRIX Installer - Double-Click Installation

## Quick Install (Easiest)

### Option 1: Double-Click Installer (Recommended)

1. **Double-click `install_synrix.exe`**
2. Follow the on-screen instructions
3. Done!

The installer will:
- Check Python version (3.8+ required)
- Install SYNRIX package
- Test the installation
- Create example script

### Option 2: Batch File Installer

1. **Double-click `installer.bat`**
2. Wait for installation to complete
3. Done!

### Option 3: Python Script Installer

1. **Double-click `installer.py`** (if Python is associated with .py files)
2. Or run: `python installer.py`
3. Follow the prompts

## Creating the .exe Installer

If you need to create `install_synrix.exe` from `installer.py`:

```bash
# Install PyInstaller
pip install pyinstaller

# Create installer
python create_installer_exe.py
```

The installer .exe will be created in the `dist/` directory.

## Manual Installation

If the installer doesn't work, you can install manually:

```bash
cd synrix_free_tier_50k
pip install -e .
```

Then test:

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
```

## Requirements

- **Python 3.8+** (required)
- **pip** (usually comes with Python)

## Troubleshooting

### "Python not found"
- Install Python 3.8+ from https://www.python.org/
- Make sure "Add Python to PATH" is checked during installation

### "pip not found"
- Python should include pip by default
- Try: `python -m ensurepip --upgrade`

### "Installation failed"
- Check that you have write permissions
- Try running as administrator
- Check Python version: `python --version`

### "DLL not found" after installation
- Make sure all DLLs are in the `synrix/` directory
- Check that Python is 3.8+ (required for `os.add_dll_directory()`)

## What Gets Installed

The installer installs SYNRIX as a Python package, making it available system-wide:

```python
# Works from any directory
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

## Uninstalling

To uninstall SYNRIX:

```bash
pip uninstall synrix-free-tier
```

## Next Steps

After installation:

1. Read `QUICK_START.md` for usage examples
2. Check `AI_AGENT_GUIDE.md` for comprehensive examples
3. See `example_usage.py` for a working example

---

**That's it!** Double-click and install - it's that simple.
