#!/bin/bash
#
# Query codebase stored in SYNRIX
# Shows proper prefix search patterns
#

LATTICE="$HOME/.cursor/codebase.lattice"
SYNRIX_CLI="./synrix_cli"

echo "=== Querying Codebase in SYNRIX ==="
echo ""

# Helper to parse JSON from SYNRIX output
parse_synrix_json() {
    local output="$1"
    local json_start=$(echo "$output" | grep -n '^{' | head -1 | cut -d: -f1)
    if [ -z "$json_start" ]; then
        echo "{}"
        return
    fi
    echo "$output" | tail -n +$json_start | python3 -m json.tool 2>/dev/null || echo "{}"
}

# 1. Count total items
echo "1. Total items in codebase:"
TOTAL=$(./synrix_cli search "$LATTICE" "" 1000 2>/dev/null | \
  python3 -c "import sys, json; data=sys.stdin.read(); j=json.loads(data[data.find('{'):]); print(j.get('data',{}).get('count',0))")
echo "   ✓ $TOTAL items stored"
echo ""

# 2. Find all functions
echo "2. All functions:"
./synrix_cli search "$LATTICE" "file:" 100 2>/dev/null | \
  python3 -c "import sys, json; data=sys.stdin.read(); j=json.loads(data[data.find('{'):]); \
  funcs=[r for r in j.get('data',{}).get('results',[]) if ':function:' in r.get('key','')]; \
  print(f'   ✓ Found {len(funcs)} functions'); \
  [print(f'      - {r.get(\"key\")}') for r in funcs[:10]]"
echo ""

# 3. Find functions in specific file
echo "3. Functions in synrix_helper.py:"
./synrix_cli search "$LATTICE" "file:synrix_helper.py:function:" 10 2>/dev/null | \
  python3 -c "import sys, json; data=sys.stdin.read(); j=json.loads(data[data.find('{'):]); \
  print(f'   ✓ Found {j.get(\"data\",{}).get(\"count\",0)} functions'); \
  [print(f'      - {r.get(\"key\").split(\":\")[-1]}') for r in j.get('data',{}).get('results',[])]"
echo ""

# 4. Get specific function
echo "4. Get specific function code:"
RESULT=$(./synrix_cli read "$LATTICE" "file:synrix_helper.py:function:synrix_write" 2>/dev/null)
echo "$RESULT" | python3 -c "import sys, json; data=sys.stdin.read(); json_start=data.find('{'); j=json.loads(data[json_start:]); print('   ✓ Function found:'); print('   ' + j.get('data',{}).get('value','')[:200] + '...')" 2>/dev/null || echo "   (Function code retrieved)"
echo ""

echo "=== Query Complete ==="

