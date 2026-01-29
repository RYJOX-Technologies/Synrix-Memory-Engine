#!/usr/bin/env python3
"""
SYNRIX Agent Hooks - Deep Integration Helpers
===============================================
Helper scripts that make it easy for AI agents to use SYNRIX
without violating ToS (all execution is via explicit tool calls).

These scripts can be called by the AI agent using run_terminal_cmd tool.
"""

import sys
import json
import os
from typing import Dict, Any

# Add parent to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

try:
    from synrix.auto_memory import get_memory, AIMemoryHelper
    SYNRIX_AVAILABLE = True
except ImportError:
    SYNRIX_AVAILABLE = False
    print(json.dumps({"error": "SYNRIX not available. Install with: pip install -e ."}))
    sys.exit(1)


def check_memory_before_generate() -> Dict[str, Any]:
    """
    Check memory before generating code.
    Returns JSON with constraints, patterns, failures.
    """
    try:
        memory = AIMemoryHelper()
        context = memory.check_before_generate()
        
        # Format for easy consumption
        result = {
            "constraints": [
                {"name": c["name"], "data": c["data"]}
                for c in context["constraints"]
            ],
            "patterns": [
                {
                    "name": p["name"],
                    "context": p.get("data", {}).get("context", ""),
                    "success_rate": p.get("data", {}).get("success_rate", 0.0)
                }
                for p in context["patterns"]
            ],
            "failures": [
                {
                    "name": f["name"],
                    "error": f.get("data", {}).get("error", "")
                }
                for f in context["failures"]
            ]
        }
        return result
    except Exception as e:
        return {"error": str(e)}


def store_pattern_after_success(pattern_name: str, code: str, context: str = "", success_rate: float = 1.0) -> Dict[str, Any]:
    """
    Store a pattern after successful code generation.
    """
    try:
        memory = AIMemoryHelper()
        node_id = memory.store_pattern(pattern_name, code, context, success_rate)
        return {"success": True, "node_id": node_id, "pattern": pattern_name}
    except Exception as e:
        return {"error": str(e)}


def store_failure_after_error(error_type: str, error: str, context: str = "", avoid: str = "") -> Dict[str, Any]:
    """
    Store a failure after an error.
    """
    try:
        memory = AIMemoryHelper()
        node_id = memory.store_failure(error_type, error, context, avoid)
        return {"success": True, "node_id": node_id, "error_type": error_type}
    except Exception as e:
        return {"error": str(e)}


def store_constraint(rule_name: str, description: str) -> Dict[str, Any]:
    """
    Store a project constraint.
    """
    try:
        memory = AIMemoryHelper()
        node_id = memory.store_constraint(rule_name, description)
        return {"success": True, "node_id": node_id, "constraint": rule_name}
    except Exception as e:
        return {"error": str(e)}


def main():
    """CLI interface for agent hooks"""
    if len(sys.argv) < 2:
        print(json.dumps({"error": "Usage: agent_hooks.py <command> [args...]"}))
        sys.exit(1)
    
    command = sys.argv[1]
    
    if command == "check":
        # Check memory before generation
        result = check_memory_before_generate()
        print(json.dumps(result, indent=2))
    
    elif command == "store_pattern":
        # Store pattern: agent_hooks.py store_pattern <name> <code> [context] [success_rate]
        if len(sys.argv) < 4:
            print(json.dumps({"error": "Usage: store_pattern <name> <code> [context] [success_rate]"}))
            sys.exit(1)
        pattern_name = sys.argv[2]
        code = sys.argv[3]
        context = sys.argv[4] if len(sys.argv) > 4 else ""
        success_rate = float(sys.argv[5]) if len(sys.argv) > 5 else 1.0
        result = store_pattern_after_success(pattern_name, code, context, success_rate)
        print(json.dumps(result))
    
    elif command == "store_failure":
        # Store failure: agent_hooks.py store_failure <error_type> <error> [context] [avoid]
        if len(sys.argv) < 4:
            print(json.dumps({"error": "Usage: store_failure <error_type> <error> [context] [avoid]"}))
            sys.exit(1)
        error_type = sys.argv[2]
        error = sys.argv[3]
        context = sys.argv[4] if len(sys.argv) > 4 else ""
        avoid = sys.argv[5] if len(sys.argv) > 5 else ""
        result = store_failure_after_error(error_type, error, context, avoid)
        print(json.dumps(result))
    
    elif command == "store_constraint":
        # Store constraint: agent_hooks.py store_constraint <name> <description>
        if len(sys.argv) < 4:
            print(json.dumps({"error": "Usage: store_constraint <name> <description>"}))
            sys.exit(1)
        rule_name = sys.argv[2]
        description = sys.argv[3]
        result = store_constraint(rule_name, description)
        print(json.dumps(result))
    
    else:
        print(json.dumps({"error": f"Unknown command: {command}"}))
        sys.exit(1)


if __name__ == "__main__":
    main()
