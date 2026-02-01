#!/usr/bin/env python3
"""
Live DevinAI + SYNRIX Integration Demo

This demonstrates how to actually connect DevinAI (or any coding agent) to SYNRIX.
Shows real-time learning from coding mistakes.
"""

import sys
import os

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

from synrix.devin_integration import DevinSynrixIntegration, DevinAISynrixWrapper


def demo_live_integration():
    """
    Live demo showing DevinAI + SYNRIX integration.
    
    This shows:
    1. How to initialize the integration
    2. How DevinAI would query SYNRIX before coding
    3. How SYNRIX learns from DevinAI's mistakes
    4. How DevinAI improves over time
    """
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║     DevinAI + SYNRIX Live Integration Demo                     ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    # Initialize integration
    print("1. Initializing SYNRIX integration...")
    integration = DevinSynrixIntegration(use_direct=True)
    print("   ✅ SYNRIX memory ready")
    print()
    
    # Simulate DevinAI tasks
    print("2. Simulating DevinAI coding tasks...")
    print()
    
    tasks = [
        {
            "description": "Write a function to process data",
            "code": "def process_data()\n    return 'done'",
            "type": "write_function"
        },
        {
            "description": "Fix import error in numpy code",
            "code": "import numpy as np\narr = np.array([1,2,3])",
            "type": "fix_bug"
        },
        {
            "description": "Write async function",
            "code": "async def process()\n    await sleep(1)",
            "type": "write_function"
        },
        {
            "description": "Test division",
            "code": "result = 10 / 0",
            "type": "run_tests"
        },
    ]
    
    for i, task in enumerate(tasks, 1):
        print(f"   Task {i}: {task['description']}")
        print(f"   Code: {task['code'].strip()}")
        
        # Step 1: DevinAI queries SYNRIX before coding
        past_errors = integration.check_past_errors(task['type'], task['code'])
        print(f"   📚 SYNRIX found {len(past_errors)} past errors for this task type")
        
        # Step 2: Apply fixes based on SYNRIX memory
        fixed_code, fixes = integration.apply_fixes(task['code'], past_errors)
        
        if fixes:
            print(f"   🔧 SYNRIX applied fixes: {', '.join(fixes)}")
            print(f"   Fixed code: {fixed_code.strip()}")
        else:
            print(f"   ✅ No fixes needed (no known errors)")
        
        # Step 3: Execute (simulate DevinAI execution)
        # In real integration, this would be DevinAI's execution
        from agent_demo_devin_real import execute_python_code
        success, error, error_type = execute_python_code(fixed_code)
        
        # Step 4: Store result in SYNRIX
        integration.store_result(
            task_type=task['type'],
            task_id=f"task_{i}",
            code=fixed_code,
            success=success,
            error=error,
            error_type=error_type
        )
        
        if success:
            print(f"   ✅ Task succeeded")
        else:
            print(f"   ❌ Task failed: {error_type}")
            if error:
                print(f"      Error: {error[:100]}")
        
        print()
    
    # Show learning summary
    print("3. SYNRIX Learning Summary:")
    print()
    for task_type in ["write_function", "fix_bug", "run_tests"]:
        summary = integration.get_learning_summary(task_type)
        if summary["total_attempts"] > 0:
            print(f"   {task_type}:")
            print(f"      Attempts: {summary['total_attempts']}")
            print(f"      Success Rate: {summary['success_rate']:.1%}")
            print(f"      Failure Patterns Learned: {len(summary['failure_patterns'])}")
            if summary['most_common_failure']:
                print(f"      Most Common Error: {summary['most_common_failure'].get('metadata', {}).get('error_type', 'N/A')}")
            print()
    
    print("═══════════════════════════════════════════════════════════════")
    print("  KEY BENEFITS FOR DEVINAI")
    print("═══════════════════════════════════════════════════════════════")
    print()
    print("  ✅ Automatic Learning:")
    print("     • DevinAI queries SYNRIX before each coding task")
    print("     • SYNRIX returns past errors and patterns")
    print("     • DevinAI applies fixes automatically")
    print()
    print("  ✅ Persistent Memory:")
    print("     • All coding attempts stored in SYNRIX")
    print("     • Error patterns learned across sessions")
    print("     • Remembers mistakes even after restart")
    print()
    print("  ✅ Fast Semantic Queries:")
    print("     • Find similar past coding tasks")
    print("     • Retrieve error patterns")
    print("     • Learn from successful patterns")
    print()
    print("  ✅ Integration is Simple:")
    print("     • Drop-in memory layer")
    print("     • Works with any coding agent")
    print("     • No changes to DevinAI core code needed")
    print()
    
    integration.close()


if __name__ == "__main__":
    demo_live_integration()

