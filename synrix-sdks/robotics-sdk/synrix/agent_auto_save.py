#!/usr/bin/env python3
"""
SYNRIX Agent Auto-Save Helper
==============================
Automatically saves agent actions to SYNRIX without manual calls.

This can be used by:
1. AI agents directly (via Python)
2. VSCode extension (via subprocess calls)
3. Any automation that wants to track agent behavior
"""

import sys
import os
import json
import time
from typing import Optional, Dict, Any
from pathlib import Path

# Add parent to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from synrix.raw_backend import RawSynrixBackend
    SYNRIX_AVAILABLE = True
except ImportError:
    SYNRIX_AVAILABLE = False


class AgentAutoSave:
    """Automatically saves agent actions to SYNRIX"""
    
    def __init__(self, lattice_path: Optional[str] = None, max_nodes: int = 1000000):
        """
        Initialize auto-save helper.
        
        Args:
            lattice_path: Path to lattice file (default: ~/.cursor_ai_memory.lattice)
            max_nodes: Max nodes (unlimited for creator)
        """
        if not SYNRIX_AVAILABLE:
            self.memory = None
            return
        
        if lattice_path is None:
            lattice_path = os.path.expanduser("~/.cursor_ai_memory.lattice")
        
        try:
            self.memory = RawSynrixBackend(lattice_path, max_nodes=max_nodes)
        except Exception as e:
            print(f"Warning: Could not initialize SYNRIX: {e}", file=sys.stderr)
            self.memory = None
    
    def save_file_created(self, file_path: str, content_preview: str = "", context: str = ""):
        """Save when agent creates a new file"""
        if not self.memory:
            return
        
        try:
            file_name = Path(file_path).name
            node_name = f"AGENT:file_created:{file_name}:{int(time.time())}"
            data = json.dumps({
                "file": file_path,
                "preview": content_preview[:200],
                "context": context,
                "timestamp": time.time()
            })
            self.memory.add_node(node_name, data, node_type=5)  # LATTICE_NODE_LEARNING
        except Exception:
            pass  # Fail silently
    
    def save_file_modified(self, file_path: str, change_type: str, context: str = ""):
        """Save when agent modifies a file"""
        if not self.memory:
            return
        
        try:
            file_name = Path(file_path).name
            node_name = f"AGENT:file_modified:{file_name}:{int(time.time())}"
            data = json.dumps({
                "file": file_path,
                "change_type": change_type,  # "search_replace", "write", etc.
                "context": context,
                "timestamp": time.time()
            })
            self.memory.add_node(node_name, data, node_type=5)
        except Exception:
            pass
    
    def save_pattern(self, pattern_name: str, code: str, context: str = "", success_rate: float = 1.0,
                     description: str = "", files: list = None, functions: list = None, 
                     related_patterns: list = None, date: str = None):
        """Save a successful code pattern with complete metadata
        
        Args:
            pattern_name: Name of the pattern
            code: The code/implementation
            context: Context where it was used
            success_rate: Success rate (0.0-1.0)
            description: Description of what the pattern does
            files: List of files where this pattern is used
            functions: List of function names related to this pattern
            related_patterns: List of related pattern names
            date: Date string (default: current date)
        """
        if not self.memory:
            return
        
        try:
            node_name = f"PATTERN:{pattern_name}"
            
            # Build complete pattern data
            pattern_data = {
                "code": code if code else "",
                "context": context if context else "",
                "description": description if description else (context if context else ""),
                "success_rate": success_rate,
                "timestamp": time.time(),
                "date": date if date else time.strftime("%Y-%m-%d"),
                "files": files if files else [],
                "functions": functions if functions else [],
                "related_patterns": related_patterns if related_patterns else []
            }
            
            # Validate that we have at least code or description
            if not pattern_data["code"] and not pattern_data["description"]:
                print(f"WARNING: Pattern {pattern_name} has no code or description, skipping")
                return
            
            data = json.dumps(pattern_data, separators=(',', ':'))
            
            # Use chunked storage if data exceeds 511 bytes
            if len(data) > 511:
                data_bytes = data.encode('utf-8')
                node_id = self.memory.add_node_chunked(node_name, data_bytes, node_type=3)  # LATTICE_NODE_PATTERN
                if node_id == 0:
                    print(f"ERROR: Failed to store chunked pattern: {pattern_name}")
            else:
                self.memory.add_node(node_name, data, node_type=3)  # LATTICE_NODE_PATTERN
        except Exception as e:
            print(f"ERROR: Failed to save pattern {pattern_name}: {e}")
    
    def save_constraint(self, constraint_name: str, description: str):
        """Save a project constraint"""
        if not self.memory:
            return
        
        try:
            node_name = f"CONSTRAINT:{constraint_name}"
            self.memory.add_node(node_name, description, node_type=6)  # LATTICE_NODE_ANTI_PATTERN
        except Exception:
            pass
    
    def save_failure(self, error_type: str, error: str, context: str = "", avoid: str = ""):
        """Save a failure to avoid repeating"""
        if not self.memory:
            return
        
        try:
            node_name = f"FAILURE:{error_type}"
            data = json.dumps({
                "error": error,
                "context": context,
                "avoid": avoid,
                "timestamp": time.time()
            })
            self.memory.add_node(node_name, data, node_type=6)  # LATTICE_NODE_ANTI_PATTERN
        except Exception:
            pass
    
    def save_task(self, task_id: str, task_description: str, result: str, success: bool = True):
        """Save a task attempt"""
        if not self.memory:
            return
        
        try:
            node_name = f"TASK:{task_id}"
            data = json.dumps({
                "description": task_description,
                "result": result,
                "success": success,
                "timestamp": time.time()
            })
            self.memory.add_node(node_name, data, node_type=5)  # LATTICE_NODE_LEARNING
        except Exception:
            pass
    
    def close(self):
        """Close and save memory"""
        if self.memory:
            try:
                self.memory.save()
                self.memory.close()
            except Exception:
                pass


# Global instance for easy access
_auto_save_instance: Optional[AgentAutoSave] = None

def get_auto_save() -> AgentAutoSave:
    """Get or create global auto-save instance"""
    global _auto_save_instance
    if _auto_save_instance is None:
        _auto_save_instance = AgentAutoSave()
    return _auto_save_instance


# CLI interface for extension to call
if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description="SYNRIX Agent Auto-Save")
    parser.add_argument("action", choices=["file_created", "file_modified", "pattern", "constraint", "failure", "task"])
    parser.add_argument("--file", help="File path (for file_created/file_modified)")
    parser.add_argument("--name", help="Pattern/constraint/failure name")
    parser.add_argument("--data", help="JSON data or description")
    parser.add_argument("--context", default="", help="Context description")
    parser.add_argument("--change-type", help="Change type (for file_modified)")
    
    args = parser.parse_args()
    
    auto_save = AgentAutoSave()
    
    if args.action == "file_created":
        auto_save.save_file_created(args.file or "", args.data or "", args.context)
    elif args.action == "file_modified":
        auto_save.save_file_modified(args.file or "", args.change_type or "unknown", args.context)
    elif args.action == "pattern":
        data = json.loads(args.data) if args.data else {}
        auto_save.save_pattern(args.name or "", data.get("code", ""), args.context, data.get("success_rate", 1.0))
    elif args.action == "constraint":
        auto_save.save_constraint(args.name or "", args.data or "")
    elif args.action == "failure":
        data = json.loads(args.data) if args.data else {}
        auto_save.save_failure(args.name or "", data.get("error", ""), args.context, data.get("avoid", ""))
    elif args.action == "task":
        data = json.loads(args.data) if args.data else {}
        auto_save.save_task(args.name or "", data.get("description", ""), data.get("result", ""), data.get("success", True))
    
    auto_save.close()
    print("✅ Saved to SYNRIX")
