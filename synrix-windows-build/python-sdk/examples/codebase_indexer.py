#!/usr/bin/env python3
"""
Codebase Indexer - Store your codebase in SYNRIX
TOS-compliant: Explicitly stores YOUR codebase, not Cursor's
"""

import subprocess
import json
import os
from pathlib import Path

SYNRIX_CLI = "./synrix_cli"
LATTICE = os.path.expanduser("~/.cursor/codebase.lattice")

def synrix_write(key, value):
    """Write to SYNRIX"""
    result = subprocess.run(
        [SYNRIX_CLI, "write", LATTICE, key, value],
        capture_output=True,
        text=True
    )
    for line in result.stdout.split('\n'):
        if line.strip().startswith('{'):
            return json.loads(line.strip())
    return {"success": False}

def index_file(file_path):
    """Index a single file - extract functions and classes"""
    if not os.path.exists(file_path):
        return
    
    with open(file_path, 'r') as f:
        content = f.read()
        lines = content.split('\n')
    
    # Simple extraction (in production, use AST parser like Tree-sitter)
    in_function = False
    function_name = None
    function_lines = []
    indent_level = 0
    
    for i, line in enumerate(lines):
        stripped = line.strip()
        
        # Detect function definition
        if stripped.startswith('def '):
            # Save previous function if any
            if function_name and function_lines:
                func_code = '\n'.join(function_lines)
                if len(func_code) < 500:  # Within node size limit
                    key = f"file:{file_path}:function:{function_name}"
                    synrix_write(key, func_code)
                    print(f"  ✓ Stored function: {function_name}")
            
            # Start new function
            function_name = stripped[4:].split('(')[0].strip()
            function_lines = [line]
            indent_level = len(line) - len(line.lstrip())
            in_function = True
        
        # Detect class definition
        elif stripped.startswith('class '):
            class_name = stripped[6:].split('(')[0].split(':')[0].strip()
            # Store class (simplified - just first few lines)
            class_code = '\n'.join(lines[i:min(i+20, len(lines))])
            key = f"file:{file_path}:class:{class_name}"
            if len(class_code) < 500:
                synrix_write(key, class_code)
                print(f"  ✓ Stored class: {class_name}")
        
        # Continue function
        elif in_function:
            current_indent = len(line) - len(line.lstrip()) if line.strip() else indent_level + 1
            if line.strip() and current_indent > indent_level:
                function_lines.append(line)
            elif line.strip() and current_indent <= indent_level:
                # Function ended
                if function_name and function_lines:
                    func_code = '\n'.join(function_lines)
                    if len(func_code) < 500:
                        key = f"file:{file_path}:function:{function_name}"
                        synrix_write(key, func_code)
                        print(f"  ✓ Stored function: {function_name}")
                in_function = False
                function_name = None
                function_lines = []

def index_directory(directory, extensions=None):
    """Index all files in a directory"""
    if extensions is None:
        extensions = ['.py', '.cpp', '.c', '.h', '.hpp']
    
    directory = Path(directory)
    files_indexed = 0
    
    for ext in extensions:
        for file_path in directory.rglob(f'*{ext}'):
            if file_path.is_file():
                print(f"Indexing: {file_path}")
                index_file(str(file_path))
                files_indexed += 1
    
    return files_indexed

if __name__ == "__main__":
    print("=== Codebase Indexer ===\n")
    print("This stores YOUR codebase in SYNRIX for semantic querying.")
    print("TOS-compliant: Explicitly stores your code, not Cursor's.\n")
    
    # Initialize
    if not os.path.exists(LATTICE):
        subprocess.run([SYNRIX_CLI, "init", LATTICE], check=True, capture_output=True)
        print("✓ Lattice initialized\n")
    
    # Index current directory
    print("Indexing current directory...")
    files_indexed = index_directory(".", ['.py'])
    
    print(f"\n✓ Indexed {files_indexed} files")
    print(f"\nYour codebase is stored in: {LATTICE}")
    print("\nQuery it with:")
    print("  ./synrix_cli search ~/.cursor/codebase.lattice 'file::function:' 100")

