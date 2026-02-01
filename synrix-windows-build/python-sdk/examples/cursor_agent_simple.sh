#!/bin/bash
#
# Cursor AI Agent - Simple SYNRIX Integration
#
# This script shows how Cursor can use SYNRIX as a system tool
# No Python, no wrappers - just direct binary calls
#

SYNRIX_CLI="./synrix_cli"
LATTICE_PATH="$HOME/.cursor/synrix_memory.lattice"

# Initialize if needed
if [ ! -f "$LATTICE_PATH" ]; then
    $SYNRIX_CLI init "$LATTICE_PATH"
fi

# Example: Remember a pattern
remember_pattern() {
    local key="$1"
    local value="$2"
    $SYNRIX_CLI write "$LATTICE_PATH" "$key" "$value" 2>/dev/null | grep '^{' | head -1
}

# Example: Recall a pattern
recall_pattern() {
    local key="$1"
    $SYNRIX_CLI read "$LATTICE_PATH" "$key" 2>/dev/null | grep '^{' | head -1
}

# Example: Search patterns
search_patterns() {
    local prefix="$1"
    local limit="${2:-10}"
    $SYNRIX_CLI search "$LATTICE_PATH" "$prefix" "$limit" 2>/dev/null | grep '^{' | head -1
}

# Usage examples
if [ "$1" = "demo" ]; then
    echo "Cursor AI - SYNRIX Integration Demo"
    echo "===================================="
    
    echo ""
    echo "1. Remembering pattern..."
    remember_pattern "pattern:python:error_handling" "Use try/except with specific exceptions"
    
    echo ""
    echo "2. Recalling pattern..."
    recall_pattern "pattern:python:error_handling"
    
    echo ""
    echo "3. Searching patterns..."
    search_patterns "pattern:python:" 10
fi

