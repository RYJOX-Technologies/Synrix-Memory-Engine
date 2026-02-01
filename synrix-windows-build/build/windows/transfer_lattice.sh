#!/bin/bash
# Transfer lattice file separately (useful if you update it later)
# Run this on Jetson to transfer updated lattice to Windows

set -e

LATTICE_FILE="$HOME/.cursor_ai_memory.lattice"
WINDOWS_USER="${1:-Livew}"  # Default Windows username
WINDOWS_HOST="${2:-192.168.1.XXX}"  # Update with your Windows IP

if [ ! -f "$LATTICE_FILE" ]; then
    echo "❌ Lattice file not found: $LATTICE_FILE"
    exit 1
fi

LATTICE_SIZE=$(du -h "$LATTICE_FILE" | cut -f1)
echo "Transferring lattice file to Windows..."
echo "  File: $LATTICE_FILE"
echo "  Size: $LATTICE_SIZE"
echo "  Destination: $WINDOWS_USER@$WINDOWS_HOST:~/cursor_ai_memory.lattice"
echo ""

# Transfer using scp
scp "$LATTICE_FILE" "$WINDOWS_USER@$WINDOWS_HOST:~/cursor_ai_memory.lattice"

echo ""
echo "✅ Lattice file transferred!"
echo ""
echo "On Windows, move it to:"
echo "  copy cursor_ai_memory.lattice \$env:USERPROFILE\\.cursor_ai_memory.lattice"
echo ""
echo "Or use it directly:"
echo "  python -c \"from synrix.raw_backend import RawSynrixBackend; m = RawSynrixBackend('C:\\\\Users\\\\$WINDOWS_USER\\\\cursor_ai_memory.lattice')\""
