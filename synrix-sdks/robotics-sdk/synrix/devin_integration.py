"""
SYNRIX Integration for DevinAI (and other coding agents)

This module provides a drop-in memory layer for coding agents like DevinAI.
It hooks into the agent's execution flow to:
1. Store coding attempts and results in SYNRIX
2. Query SYNRIX before attempting tasks to avoid known errors
3. Learn from past mistakes automatically
"""

import json
import time
import sys
import os
import subprocess
import tempfile
from typing import Optional, Dict, List, Any, Callable
from .agent_memory import SynrixMemory


class DevinSynrixIntegration:
    """
    Integration layer between DevinAI (or any coding agent) and SYNRIX.
    
    Usage:
        # Initialize
        integration = DevinSynrixIntegration()
        
        # Before DevinAI attempts a task
        past_errors = integration.check_past_errors(task_description, code_context)
        if past_errors:
            # Modify code to avoid known errors
            code = integration.apply_fixes(code, past_errors)
        
        # Execute task (via DevinAI or directly)
        result = execute_code(code)
        
        # Store result in SYNRIX
        integration.store_result(task_description, code, result)
    """
    
    def __init__(self, memory: Optional[SynrixMemory] = None, use_direct: bool = True):
        """
        Initialize DevinAI-SYNRIX integration.
        
        Args:
            memory: Optional SynrixMemory instance. If None, creates a new one.
            use_direct: Use direct shared memory client if available.
        """
        if memory is None:
            self.memory = SynrixMemory(use_direct=use_direct)
        else:
            self.memory = memory
        
        self.known_error_patterns = {}
        self.fix_functions = self._initialize_fix_functions()
    
    def _initialize_fix_functions(self) -> Dict[str, Callable]:
        """Initialize code fix functions for different error types"""
        return {
            "syntax_error": self._fix_syntax_error,
            "import_error": self._fix_import_error,
            "name_error": self._fix_name_error,
            "type_error": self._fix_type_error,
            "runtime_error": self._fix_runtime_error,
            "indentation_error": self._fix_indentation_error,
        }
    
    def check_past_errors(self, task_type: str, code_context: str = "") -> List[Dict[str, Any]]:
        """
        Query SYNRIX for past errors related to this task.
        
        Args:
            task_type: Type of task (e.g., "write_function", "fix_bug")
            code_context: Optional code snippet for context-aware search
            
        Returns:
            List of past error patterns that match this task
        """
        # Query SYNRIX for past failures
        memory_data = self.memory.get_task_memory_summary(task_type, limit=20)
        
        failures = memory_data.get("failures", [])
        failure_patterns = memory_data.get("failure_patterns", set())
        
        # Filter failures that might be relevant to current code context
        relevant_failures = []
        if code_context:
            code_lower = code_context.lower()
            for failure in failures:
                error_type = failure.get("metadata", {}).get("error_type", "")
                code_snippet = failure.get("metadata", {}).get("code_snippet", "").lower()
                
                # Check if code context matches past failure
                if code_snippet and any(keyword in code_lower for keyword in code_snippet.split()[:5]):
                    relevant_failures.append(failure)
                elif error_type in failure_patterns:
                    relevant_failures.append(failure)
        else:
            relevant_failures = failures
        
        return relevant_failures
    
    def apply_fixes(self, code: str, past_errors: List[Dict[str, Any]]) -> tuple[str, List[str]]:
        """
        Apply fixes to code based on past errors stored in SYNRIX.
        
        Args:
            code: Original code
            past_errors: List of past error patterns from SYNRIX
            
        Returns:
            (fixed_code, list_of_fixes_applied)
        """
        fixed_code = code
        fixes_applied = []
        
        for error in past_errors:
            error_type = error.get("metadata", {}).get("error_type", "")
            error_msg = error.get("metadata", {}).get("error", "")
            
            if error_type in self.fix_functions:
                fixed_code, fix_desc = self.fix_functions[error_type](fixed_code, error_msg)
                if fix_desc:
                    fixes_applied.append(fix_desc)
        
        return fixed_code, fixes_applied
    
    def store_result(
        self,
        task_type: str,
        task_id: str,
        code: str,
        success: bool,
        error: Optional[str] = None,
        error_type: Optional[str] = None,
        metadata: Optional[Dict] = None
    ):
        """
        Store coding task result in SYNRIX memory.
        
        Args:
            task_type: Type of task
            task_id: Unique task identifier
            code: Code that was executed
            success: Whether execution succeeded
            error: Error message if failed
            error_type: Type of error (syntax_error, import_error, etc.)
            metadata: Additional metadata
        """
        result_value = "success" if success else f"failed_{error_type or 'unknown'}"
        
        store_metadata = {
            "task_id": task_id,
            "task_type": task_type,
            "error": error[:200] if error else None,
            "error_type": error_type,
            "code_snippet": code[:500],  # Store code snippet
            "timestamp": time.time(),
            **(metadata or {})
        }
        
        self.memory.write(
            f"task:{task_type}:{task_id}:attempt",
            result_value,
            metadata=store_metadata
        )
    
    # Fix functions for different error types
    def _fix_syntax_error(self, code: str, error_msg: str) -> tuple[str, Optional[str]]:
        """Fix syntax errors (missing colons, brackets, etc.)"""
        fixes = []
        
        # Fix missing colon after function definition
        if "expected ':'" in error_msg.lower() or "invalid syntax" in error_msg.lower():
            lines = code.split("\n")
            for i, line in enumerate(lines):
                if line.strip().startswith("def ") and not line.strip().endswith(":"):
                    if ":" not in line:
                        lines[i] = line.rstrip() + ":"
                        fixes.append("Added missing colon")
                elif line.strip().startswith("class ") and not line.strip().endswith(":"):
                    if ":" not in line:
                        lines[i] = line.rstrip() + ":"
                        fixes.append("Added missing colon")
            
            if fixes:
                return "\n".join(lines), fixes[0]
        
        return code, None
    
    def _fix_import_error(self, code: str, error_msg: str) -> tuple[str, Optional[str]]:
        """Fix import errors (missing imports)"""
        code_lower = code.lower()
        
        # Check for common missing imports
        if "numpy" in code_lower or "np." in code:
            if "import numpy" not in code and "import np" not in code:
                code = "import numpy as np\n" + code
                return code, "Added numpy import"
        
        if "pandas" in code_lower or "pd." in code:
            if "import pandas" not in code and "import pd" not in code:
                code = "import pandas as pd\n" + code
                return code, "Added pandas import"
        
        if "sleep" in code_lower and "await sleep" in code_lower:
            if "from asyncio import sleep" not in code and "from time import sleep" not in code:
                code = "from asyncio import sleep\n" + code
                return code, "Added asyncio.sleep import"
        elif "sleep(" in code:
            if "from time import sleep" not in code and "import sleep" not in code:
                code = "from time import sleep\n" + code
                return code, "Added time.sleep import"
        
        return code, None
    
    def _fix_name_error(self, code: str, error_msg: str) -> tuple[str, Optional[str]]:
        """Fix name errors (undefined variables)"""
        # Extract variable name from error message
        if "name '" in error_msg and "' is not defined" in error_msg:
            var_name = error_msg.split("name '")[1].split("'")[0]
            if var_name not in code.split("=")[0]:  # Variable not defined
                # Simple fix: add variable definition
                code = f"{var_name} = 0\n" + code
                return code, f"Added definition for {var_name}"
        
        return code, None
    
    def _fix_type_error(self, code: str, error_msg: str) -> tuple[str, Optional[str]]:
        """Fix type errors (string + int, etc.)"""
        # Fix string + int concatenation
        if "unsupported operand" in error_msg.lower() and "+" in code:
            # Simple heuristic: if we see 'string' + number, convert number to string
            import re
            # This is a simplified fix - in production, would use AST parsing
            if "'" in code or '"' in code:
                # Try to fix common patterns
                code = re.sub(r"(['\"].*?['\"]) \+ (\d+)", r"\1 + str(\2)", code)
                if code != code:  # If pattern was found and replaced
                    return code, "Fixed string concatenation type error"
        
        return code, None
    
    def _fix_runtime_error(self, code: str, error_msg: str) -> tuple[str, Optional[str]]:
        """Fix runtime errors (division by zero, etc.)"""
        # Fix division by zero
        if "ZeroDivisionError" in error_msg or "division by zero" in error_msg.lower():
            if "/ 0" in code:
                code = code.replace("/ 0", "/ 1")
                return code, "Fixed division by zero"
            elif " / 0" in code:
                code = code.replace(" / 0", " / 1")
                return code, "Fixed division by zero"
        
        return code, None
    
    def _fix_indentation_error(self, code: str, error_msg: str) -> tuple[str, Optional[str]]:
        """Fix indentation errors"""
        # Simple fix: ensure consistent indentation
        lines = code.split("\n")
        fixed_lines = []
        indent_level = 0
        
        for line in lines:
            stripped = line.lstrip()
            if not stripped:
                fixed_lines.append("")
                continue
            
            # Decrease indent after certain keywords
            if stripped.startswith(("return", "break", "continue", "pass")):
                # Check if previous line was a block ender
                pass
            
            # Increase indent after certain keywords
            if stripped.endswith(":"):
                fixed_lines.append("    " * indent_level + stripped)
                indent_level += 1
            else:
                fixed_lines.append("    " * indent_level + stripped)
                # Decrease if this line doesn't continue the block
                if indent_level > 0 and not stripped.startswith(("    ", "\t")):
                    # This is a heuristic - in production would use AST
                    pass
        
        fixed_code = "\n".join(fixed_lines)
        if fixed_code != code:
            return fixed_code, "Fixed indentation"
        
        return code, None
    
    def get_learning_summary(self, task_type: str) -> Dict[str, Any]:
        """
        Get a summary of what the agent has learned for a task type.
        
        Returns:
            Dictionary with learning statistics
        """
        memory_data = self.memory.get_task_memory_summary(task_type, limit=50)
        
        return {
            "total_attempts": len(memory_data.get("last_attempts", [])),
            "total_failures": len(memory_data.get("failures", [])),
            "total_successes": len(memory_data.get("successes", [])),
            "failure_patterns": list(memory_data.get("failure_patterns", set())),
            "most_common_failure": memory_data.get("most_common_failure"),
            "success_rate": len(memory_data.get("successes", [])) / max(len(memory_data.get("last_attempts", [])), 1)
        }
    
    def close(self):
        """Close the memory connection"""
        if hasattr(self.memory, 'close'):
            self.memory.close()


# ============================================================================
# DevinAI API Integration (if available)
# ============================================================================

class DevinAISynrixWrapper:
    """
    Wrapper for DevinAI API that automatically integrates with SYNRIX.
    
    This would hook into DevinAI's execution flow if they provide:
    - Webhooks/callbacks
    - Plugin system
    - API hooks
    """
    
    def __init__(self, devin_api_key: Optional[str] = None, use_direct: bool = True):
        """
        Initialize DevinAI-SYNRIX wrapper.
        
        Args:
            devin_api_key: DevinAI API key (if available)
            use_direct: Use direct shared memory for SYNRIX
        """
        self.integration = DevinSynrixIntegration(use_direct=use_direct)
        self.devin_api_key = devin_api_key
        # TODO: Initialize DevinAI client when API is available
    
    def execute_with_memory(self, task_description: str, code: str) -> Dict[str, Any]:
        """
        Execute a coding task with SYNRIX memory integration.
        
        This is the main entry point that:
        1. Checks SYNRIX for past errors
        2. Applies fixes to code
        3. Executes via DevinAI (or directly)
        4. Stores results in SYNRIX
        
        Args:
            task_description: Description of the task
            code: Code to execute
            
        Returns:
            Execution result with fixes applied
        """
        task_type = self._classify_task(task_description)
        task_id = f"devin_{int(time.time() * 1000)}"
        
        # Step 1: Check SYNRIX for past errors
        past_errors = self.integration.check_past_errors(task_type, code)
        
        # Step 2: Apply fixes based on past errors
        fixed_code, fixes_applied = self.integration.apply_fixes(code, past_errors)
        
        # Step 3: Execute code (via DevinAI API or directly)
        # TODO: Replace with actual DevinAI API call when available
        success, error, error_type = self._execute_code(fixed_code)
        
        # Step 4: Store result in SYNRIX
        self.integration.store_result(
            task_type=task_type,
            task_id=task_id,
            code=fixed_code,
            success=success,
            error=error,
            error_type=error_type,
            metadata={
                "fixes_applied": fixes_applied,
                "original_code": code[:200],
                "task_description": task_description
            }
        )
        
        return {
            "success": success,
            "error": error,
            "error_type": error_type,
            "fixes_applied": fixes_applied,
            "code_modified": fixed_code != code
        }
    
    def _classify_task(self, description: str) -> str:
        """Classify task type from description"""
        desc_lower = description.lower()
        if "function" in desc_lower or "write" in desc_lower:
            return "write_function"
        elif "bug" in desc_lower or "fix" in desc_lower:
            return "fix_bug"
        elif "test" in desc_lower:
            return "run_tests"
        elif "refactor" in desc_lower:
            return "refactor_code"
        elif "feature" in desc_lower or "add" in desc_lower:
            return "add_feature"
        else:
            return "general"
    
    def _execute_code(self, code: str) -> tuple[bool, Optional[str], Optional[str]]:
        """
        Execute code (placeholder for DevinAI API integration).
        
        In production, this would call DevinAI's API.
        For now, we'll use direct execution as a fallback.
        """
        # TODO: Replace with DevinAI API call
        # For now, use direct execution
        import subprocess
        import tempfile
        
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False) as f:
            f.write(code)
            temp_file = f.name
        
        try:
            result = subprocess.run(
                [sys.executable, temp_file],
                capture_output=True,
                text=True,
                timeout=5
            )
            
            if result.returncode == 0:
                return (True, None, None)
            else:
                error_output = result.stderr.strip()
                
                # Classify error
                if "SyntaxError" in error_output:
                    error_type = "syntax_error"
                elif "ImportError" in error_output or "ModuleNotFoundError" in error_output:
                    error_type = "import_error"
                elif "NameError" in error_output:
                    error_type = "name_error"
                elif "TypeError" in error_output:
                    error_type = "type_error"
                elif "ZeroDivisionError" in error_output:
                    error_type = "runtime_error"
                else:
                    error_type = "unknown_error"
                
                return (False, error_output, error_type)
        except Exception as e:
            return (False, str(e), "execution_error")
        finally:
            try:
                os.unlink(temp_file)
            except:
                pass
    
    def close(self):
        """Close connections"""
        self.integration.close()


# ============================================================================
# Example Usage
# ============================================================================

if __name__ == "__main__":
    import sys
    
    # Example: Using SYNRIX integration with coding agent
    integration = DevinSynrixIntegration()
    
    # Example task
    task_type = "write_function"
    code = """
def hello_world()
    print('Hello')
"""
    
    # Check for past errors
    past_errors = integration.check_past_errors(task_type, code)
    print(f"Found {len(past_errors)} past errors")
    
    # Apply fixes
    fixed_code, fixes = integration.apply_fixes(code, past_errors)
    print(f"Fixes applied: {fixes}")
    print(f"Fixed code:\n{fixed_code}")
    
    # Store result (after execution)
    # integration.store_result(task_type, "task_1", fixed_code, success=True)
    
    integration.close()

