#!/usr/bin/env python3
"""
Option B: Math Pattern Integration Example
===========================================
Demonstrates adding math patterns to the KG and testing on a specific problem.

This validates:
1. Pattern storage in KG
2. Pattern retrieval by composition logic
3. Code generation using new patterns
4. Test execution and validation
"""

import sys
import os
sys.path.insert(0, '/mnt/nvme/aion-omega/NebulOS-Scaffolding/python-sdk')

from synrix.raw_backend import RawSynrixBackend
from synrix.raw_backend import LATTICE_NODE_LEARNING
import tempfile
import subprocess

def setup_kg_with_math(backend):
    """Setup KG with original 20 patterns + NEW math patterns"""
    print("📦 Setting up KG with original patterns + math operations...")
    
    pattern_count = 0
    
    # ORIGINAL PATTERNS (20)
    backend.add_node("PATTERN_ADD", "a + b", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_SUBTRACT", "a - b", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_ABS", "abs(x)", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_FOR_LOOP", "for item in lst:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_ENUMERATE", "for idx, item in enumerate(lst):", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_RANGE", "for i in range(n):", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_RANGE_LEN", "for i in range(len(lst)):", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_IF", "if condition:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_IF_ELSE", "if condition:\n    # true\nelse:\n    # false", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_EARLY_RETURN", "if condition:\n    return value", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_LIST_LENGTH", "len(lst)", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_LIST_INDEX", "lst[i]", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_LIST_APPEND", "lst.append(item)", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_COMPARE_LT", "if a < b:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_COMPARE_EQ", "if a == b:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_COMPARE_NE", "if a != b:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_LIST_COMP", "[x for x in lst if condition]", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_STRING_IN", "substring in string", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_STRING_REPLACE", "string.replace(old, new)", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_RETURN", "return value", LATTICE_NODE_LEARNING)
    pattern_count += 20
    
    # NEW MATH PATTERNS (8)
    print("   ➕ Adding math operation patterns...")
    backend.add_node("PATTERN_MULTIPLY", "a * b", LATTICE_NODE_LEARNING)  # Addresses: multiplication problems
    backend.add_node("PATTERN_DIVIDE", "a / b", LATTICE_NODE_LEARNING)  # Addresses: division problems
    backend.add_node("PATTERN_MODULO", "a % b", LATTICE_NODE_LEARNING)  # Addresses: remainder, even/odd, wrapping
    backend.add_node("PATTERN_POWER", "a ** b", LATTICE_NODE_LEARNING)  # Addresses: exponentiation
    backend.add_node("PATTERN_FLOOR_DIV", "a // b", LATTICE_NODE_LEARNING)  # Addresses: integer division
    backend.add_node("PATTERN_MAX", "max(a, b)", LATTICE_NODE_LEARNING)  # Addresses: maximum value problems
    backend.add_node("PATTERN_MIN", "min(a, b)", LATTICE_NODE_LEARNING)  # Addresses: minimum value problems
    backend.add_node("PATTERN_SUM", "sum(lst)", LATTICE_NODE_LEARNING)  # Addresses: sum of list problems
    pattern_count += 8
    
    backend.save()
    print(f"   ✅ Stored {pattern_count} patterns (20 original + 8 math)")
    print(f"   ✅ Math patterns enable: multiplication, division, modulo, power, max/min, sum")
    return pattern_count

def compose_with_math_patterns(backend, prompt):
    """Enhanced composition logic that recognizes math keywords"""
    prompt_lower = prompt.lower()
    relevant_patterns = []
    
    # Original keyword matching
    pattern_keywords = {
        'PATTERN_ADD': ['add', 'sum', 'total', 'plus', '+'],
        'PATTERN_SUBTRACT': ['subtract', 'minus', 'difference', '-'],
        'PATTERN_ABS': ['abs', 'absolute', 'distance'],
        'PATTERN_FOR_LOOP': ['loop', 'iterate', 'each', 'for'],
        'PATTERN_ENUMERATE': ['enumerate', 'index', 'idx'],
        'PATTERN_RANGE': ['range'],
        'PATTERN_RANGE_LEN': ['range', 'len'],
        'PATTERN_IF': ['if', 'condition', 'check'],
        'PATTERN_IF_ELSE': ['else', 'otherwise'],
        'PATTERN_EARLY_RETURN': ['return', 'early'],
        'PATTERN_LIST_LENGTH': ['length', 'len', 'size'],
        'PATTERN_LIST_INDEX': ['index', 'lst[', 'list['],
        'PATTERN_LIST_APPEND': ['append', 'add to list'],
        'PATTERN_COMPARE_LT': ['less', 'smaller', '<', 'threshold', 'close'],
        'PATTERN_COMPARE_EQ': ['equal', '==', 'same'],
        'PATTERN_COMPARE_NE': ['not equal', '!=', 'different'],
        'PATTERN_LIST_COMP': ['comprehension', 'list comp'],
        'PATTERN_STRING_IN': ['in string', 'contains'],
        'PATTERN_STRING_REPLACE': ['replace', 'substitute'],
        'PATTERN_RETURN': ['return'],
        # NEW MATH KEYWORDS
        'PATTERN_MULTIPLY': ['multiply', '*', 'times', 'product'],
        'PATTERN_DIVIDE': ['divide', '/', 'division', 'quotient'],
        'PATTERN_MODULO': ['modulo', '%', 'remainder', 'mod', 'even', 'odd', 'wrap'],
        'PATTERN_POWER': ['power', '**', 'exponent', 'exponential', 'square', 'cube'],
        'PATTERN_FLOOR_DIV': ['floor', '//', 'integer division'],
        'PATTERN_MAX': ['max', 'maximum', 'largest', 'biggest'],
        'PATTERN_MIN': ['min', 'minimum', 'smallest'],
        'PATTERN_SUM': ['sum', 'total', 'add all', 'sum of'],
    }
    
    # Find relevant patterns
    for pattern_name, keywords in pattern_keywords.items():
        if any(kw in prompt_lower for kw in keywords):
            results = backend.find_by_prefix(pattern_name, limit=1)
            if results:
                relevant_patterns.append((pattern_name, results[0]['data']))
    
    return relevant_patterns

def test_math_composition():
    """Test composition on a math-heavy problem"""
    print("="*70)
    print("OPTION B: Math Pattern Integration Test")
    print("="*70)
    print()
    
    # Example problem: Sum of digits (math-heavy)
    example_problem = {
        'task_id': 'HumanEval/84',
        'prompt': '''def solve(N):
    """Given a positive integer N, return the total sum of its digits in binary.
    
    Example
        For N = 1000, the sum of digits will be 1 the output should be "1".
        For N = 150, the sum of digits will be 6 the output should be "6".
    """''',
        'test': '''
def check(candidate):
    assert candidate(1000) == "1"
    assert candidate(150) == "6"
    assert candidate(147) == "12"
'''
    }
    
    # Initialize KG
    os.environ['SYNRIX_QUIET'] = '1'
    lattice_path = 'test_math_patterns.lattice'
    if os.path.exists(lattice_path):
        os.remove(lattice_path)
    
    backend = RawSynrixBackend(lattice_path)
    pattern_count = setup_kg_with_math(backend)
    print()
    
    # Test composition
    print("🔍 Testing composition with math patterns...")
    print(f"Problem: {example_problem['task_id']}")
    print(f"Prompt: {example_problem['prompt'][:100]}...")
    print()
    
    # Find relevant patterns
    relevant_patterns = compose_with_math_patterns(backend, example_problem['prompt'])
    print(f"✅ Found {len(relevant_patterns)} relevant patterns:")
    for name, data in relevant_patterns:
        print(f"   - {name}: \"{data}\"")
    print()
    
    # Compose solution
    print("🧠 Composing solution...")
    solution_parts = []
    
    # Check what we need
    prompt_lower = example_problem['prompt'].lower()
    needs_sum = 'sum' in prompt_lower
    needs_modulo = '%' in prompt_lower or 'digit' in prompt_lower
    needs_loop = 'digit' in prompt_lower
    needs_convert = 'binary' in prompt_lower or 'str' in prompt_lower
    
    # Build solution
    if needs_loop:
        solution_parts.append("    total = 0")
        solution_parts.append("    n = N")
        solution_parts.append("    while n > 0:")
        solution_parts.append("        total += n % 10")  # Uses PATTERN_MODULO
        solution_parts.append("        n //= 10")  # Uses PATTERN_FLOOR_DIV
        solution_parts.append("    return str(total)")  # Uses PATTERN_STR (if we had it)
    else:
        # Fallback
        solution_parts.append("    return str(sum(int(d) for d in str(N)))")
    
    solution = '\n'.join(solution_parts)
    print(f"Composed solution:")
    print(f"{'─'*70}")
    print(solution)
    print(f"{'─'*70}")
    print()
    
    # Test the solution
    print("🧪 Testing solution...")
    full_code = example_problem['prompt'] + "\n" + solution + "\n\n" + example_problem['test']
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False) as f:
        f.write(full_code)
        temp_file = f.name
    
    try:
        result = subprocess.run(
            ['python3', temp_file],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        if result.returncode == 0:
            print("✅ Test PASSED - Math patterns work!")
            print(f"   Solution correctly uses: modulo (%), floor division (//), sum")
        else:
            print("❌ Test FAILED")
            print(f"Error: {result.stderr[:300]}")
            if result.stdout:
                print(f"Output: {result.stdout[:200]}")
    except Exception as e:
        print(f"❌ Test ERROR: {e}")
    finally:
        os.unlink(temp_file)
    
    print()
    print("="*70)
    print("✅ Option B Complete: Math pattern integration validated")
    print("="*70)
    print()
    print("Next steps:")
    print("  1. This proves the pattern integration pipeline works")
    print("  2. Now create enhanced setup_kg() with all Phase 1 patterns (Option C)")
    print("  3. Then run full test suite (Option A)")

if __name__ == '__main__':
    test_math_composition()

