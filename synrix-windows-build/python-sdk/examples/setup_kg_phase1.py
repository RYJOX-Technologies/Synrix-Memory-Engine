"""
Enhanced setup_kg() - Phase 1 Patterns
========================================
Complete pattern set for HumanEval v2 testing.

This adds ~35-40 new atomic patterns to the original 20, organized by category.
Each pattern is documented with what problem types it addresses.

Total: ~55-60 patterns (all atomic fundamentals, no HumanEval solutions)

Usage:
    from setup_kg_phase1 import setup_kg_phase1
    backend = RawSynrixBackend("lattice.lattice")
    setup_kg_phase1(backend)
"""

from synrix.raw_backend import LATTICE_NODE_LEARNING

def setup_kg_phase1(backend):
    """
    Setup KG with Phase 1 enhanced patterns.
    
    Original 20 patterns + ~35-40 new atomic patterns covering:
    - Math operations (8 patterns)
    - List operations (10 patterns)
    - String operations (8 patterns)
    - Type conversions (6 patterns)
    - Control flow (4 patterns)
    - Advanced iteration (3 patterns)
    
    Expected impact: 73.2% → 82-85% pass rate
    """
    print("📦 Setting up KG with Phase 1 enhanced patterns...")
    print("   🎯 Original 20 + ~40 new atomic patterns")
    print()
    
    pattern_count = 0
    
    # ============================================================
    # ORIGINAL PATTERNS (20) - Keep for backward compatibility
    # ============================================================
    print("   📚 Original patterns (20)...")
    
    # 1. BASIC OPERATIONS
    backend.add_node("PATTERN_ADD", "a + b", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_SUBTRACT", "a - b", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_ABS", "abs(x)", LATTICE_NODE_LEARNING)
    pattern_count += 3
    
    # 2. LOOPS
    backend.add_node("PATTERN_FOR_LOOP", "for item in lst:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_ENUMERATE", "for idx, item in enumerate(lst):", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_RANGE", "for i in range(n):", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_RANGE_LEN", "for i in range(len(lst)):", LATTICE_NODE_LEARNING)
    pattern_count += 4
    
    # 3. CONDITIONALS
    backend.add_node("PATTERN_IF", "if condition:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_IF_ELSE", "if condition:\n    # true\nelse:\n    # false", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_EARLY_RETURN", "if condition:\n    return value", LATTICE_NODE_LEARNING)
    pattern_count += 3
    
    # 4. LIST OPERATIONS
    backend.add_node("PATTERN_LIST_LENGTH", "len(lst)", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_LIST_INDEX", "lst[i]", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_LIST_APPEND", "lst.append(item)", LATTICE_NODE_LEARNING)
    pattern_count += 3
    
    # 5. COMPARISONS
    backend.add_node("PATTERN_COMPARE_LT", "if a < b:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_COMPARE_EQ", "if a == b:", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_COMPARE_NE", "if a != b:", LATTICE_NODE_LEARNING)
    pattern_count += 3
    
    # 6. LIST COMPREHENSIONS
    backend.add_node("PATTERN_LIST_COMP", "[x for x in lst if condition]", LATTICE_NODE_LEARNING)
    pattern_count += 1
    
    # 7. STRING OPERATIONS
    backend.add_node("PATTERN_STRING_IN", "substring in string", LATTICE_NODE_LEARNING)
    backend.add_node("PATTERN_STRING_REPLACE", "string.replace(old, new)", LATTICE_NODE_LEARNING)
    pattern_count += 2
    
    # 8. RETURN
    backend.add_node("PATTERN_RETURN", "return value", LATTICE_NODE_LEARNING)
    pattern_count += 1
    
    # ============================================================
    # PHASE 1: MATH OPERATIONS (8 patterns)
    # Addresses: multiplication, division, modulo, power, max/min, sum
    # ============================================================
    print("   ➕ Math operations (8)...")
    backend.add_node("PATTERN_MULTIPLY", "a * b", LATTICE_NODE_LEARNING)  # Multiplication problems
    backend.add_node("PATTERN_DIVIDE", "a / b", LATTICE_NODE_LEARNING)  # Division problems
    backend.add_node("PATTERN_MODULO", "a % b", LATTICE_NODE_LEARNING)  # Remainder, even/odd, wrapping
    backend.add_node("PATTERN_POWER", "a ** b", LATTICE_NODE_LEARNING)  # Exponentiation
    backend.add_node("PATTERN_FLOOR_DIV", "a // b", LATTICE_NODE_LEARNING)  # Integer division
    backend.add_node("PATTERN_MAX", "max(a, b)", LATTICE_NODE_LEARNING)  # Maximum value
    backend.add_node("PATTERN_MIN", "min(a, b)", LATTICE_NODE_LEARNING)  # Minimum value
    backend.add_node("PATTERN_SUM", "sum(lst)", LATTICE_NODE_LEARNING)  # Sum of list
    pattern_count += 8
    
    # ============================================================
    # PHASE 1: LIST OPERATIONS (10 patterns)
    # Addresses: slicing, sorting, reversing, extending, inserting, removing
    # ============================================================
    print("   📋 List operations (10)...")
    backend.add_node("PATTERN_LIST_SLICE", "lst[start:end]", LATTICE_NODE_LEARNING)  # Slicing
    backend.add_node("PATTERN_LIST_SLICE_STEP", "lst[start:end:step]", LATTICE_NODE_LEARNING)  # Slicing with step
    backend.add_node("PATTERN_LIST_REVERSE", "lst[::-1]", LATTICE_NODE_LEARNING)  # Reverse list
    backend.add_node("PATTERN_LIST_SORT", "sorted(lst)", LATTICE_NODE_LEARNING)  # Sort list
    backend.add_node("PATTERN_LIST_SORT_REVERSE", "sorted(lst, reverse=True)", LATTICE_NODE_LEARNING)  # Sort descending
    backend.add_node("PATTERN_LIST_EXTEND", "lst.extend(other_lst)", LATTICE_NODE_LEARNING)  # Extend list
    backend.add_node("PATTERN_LIST_INSERT", "lst.insert(index, item)", LATTICE_NODE_LEARNING)  # Insert at index
    backend.add_node("PATTERN_LIST_REMOVE", "lst.remove(item)", LATTICE_NODE_LEARNING)  # Remove item
    backend.add_node("PATTERN_LIST_POP", "lst.pop(index)", LATTICE_NODE_LEARNING)  # Pop item
    backend.add_node("PATTERN_LIST_COUNT", "lst.count(item)", LATTICE_NODE_LEARNING)  # Count occurrences
    pattern_count += 10
    
    # ============================================================
    # PHASE 1: STRING OPERATIONS (8 patterns)
    # Addresses: splitting, joining, stripping, case conversion, finding
    # ============================================================
    print("   📝 String operations (8)...")
    backend.add_node("PATTERN_STRING_SPLIT", "string.split(sep)", LATTICE_NODE_LEARNING)  # Split string
    backend.add_node("PATTERN_STRING_JOIN", "sep.join(lst)", LATTICE_NODE_LEARNING)  # Join list
    backend.add_node("PATTERN_STRING_STRIP", "string.strip()", LATTICE_NODE_LEARNING)  # Strip whitespace
    backend.add_node("PATTERN_STRING_UPPER", "string.upper()", LATTICE_NODE_LEARNING)  # Uppercase
    backend.add_node("PATTERN_STRING_LOWER", "string.lower()", LATTICE_NODE_LEARNING)  # Lowercase
    backend.add_node("PATTERN_STRING_STARTSWITH", "string.startswith(prefix)", LATTICE_NODE_LEARNING)  # Starts with
    backend.add_node("PATTERN_STRING_ENDSWITH", "string.endswith(suffix)", LATTICE_NODE_LEARNING)  # Ends with
    backend.add_node("PATTERN_STRING_FIND", "string.find(substring)", LATTICE_NODE_LEARNING)  # Find substring
    pattern_count += 8
    
    # ============================================================
    # PHASE 1: TYPE CONVERSIONS (6 patterns)
    # Addresses: int, float, str, list, set, dict conversions
    # ============================================================
    print("   🔄 Type conversions (6)...")
    backend.add_node("PATTERN_INT", "int(x)", LATTICE_NODE_LEARNING)  # Convert to int
    backend.add_node("PATTERN_FLOAT", "float(x)", LATTICE_NODE_LEARNING)  # Convert to float
    backend.add_node("PATTERN_STR", "str(x)", LATTICE_NODE_LEARNING)  # Convert to string
    backend.add_node("PATTERN_LIST", "list(iterable)", LATTICE_NODE_LEARNING)  # Convert to list
    backend.add_node("PATTERN_SET", "set(iterable)", LATTICE_NODE_LEARNING)  # Convert to set
    backend.add_node("PATTERN_DICT", "dict(key=value)", LATTICE_NODE_LEARNING)  # Convert to dict
    pattern_count += 6
    
    # ============================================================
    # PHASE 1: CONTROL FLOW (4 patterns)
    # Addresses: while loops, break, continue, elif
    # ============================================================
    print("   🔀 Control flow (4)...")
    backend.add_node("PATTERN_WHILE", "while condition:", LATTICE_NODE_LEARNING)  # While loop
    backend.add_node("PATTERN_BREAK", "break", LATTICE_NODE_LEARNING)  # Break from loop
    backend.add_node("PATTERN_CONTINUE", "continue", LATTICE_NODE_LEARNING)  # Continue loop
    backend.add_node("PATTERN_ELIF", "elif condition:", LATTICE_NODE_LEARNING)  # Else-if
    pattern_count += 4
    
    # ============================================================
    # PHASE 1: ADVANCED ITERATION (3 patterns)
    # Addresses: zip, enumerate with start, range with step
    # ============================================================
    print("   🔁 Advanced iteration (3)...")
    backend.add_node("PATTERN_ZIP", "zip(lst1, lst2)", LATTICE_NODE_LEARNING)  # Zip two lists
    backend.add_node("PATTERN_ENUMERATE_START", "enumerate(lst, start=1)", LATTICE_NODE_LEARNING)  # Enumerate with start
    backend.add_node("PATTERN_RANGE_STEP", "range(start, end, step)", LATTICE_NODE_LEARNING)  # Range with step
    pattern_count += 3
    
    backend.save()
    print()
    print(f"   ✅ Stored {pattern_count} patterns total")
    print(f"      • Original: 20 patterns")
    print(f"      • Phase 1: {pattern_count - 20} new patterns")
    print(f"      • Total: {pattern_count} atomic fundamentals")
    print()
    print("   🎯 Expected impact: 73.2% → 82-85% pass rate")
    print("   ✅ All patterns are atomic - no HumanEval solutions")
    
    return pattern_count

