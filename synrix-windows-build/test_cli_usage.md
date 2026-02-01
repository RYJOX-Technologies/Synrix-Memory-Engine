# Testing SYNRIX CLI Usage

Once `synrix.exe` is built, test it like this:

## Test Commands

```cmd
cd C:\Users\Livew\Desktop\synrix-windows-build\synrix-windows-build

REM Test add
build\windows\build_msys2\bin\synrix.exe add test.lattice "MEMORY:test1" "Data 1"

REM Test query
build\windows\build_msys2\bin\synrix.exe query test.lattice "MEMORY:" 10

REM Test get (use node_id from add output)
build\windows\build_msys2\bin\synrix.exe get test.lattice <node_id>

REM Test count
build\windows\build_msys2\bin\synrix.exe count test.lattice
```

## Expected Output

**Add:**
```json
{"success":true,"node_id":6308444734549393409}
```

**Query:**
```json
{"success":true,"count":1,"nodes":[{"id":6308444734549393409,"name":"MEMORY:test1","data":"Data 1"}]}
```

**Get:**
```json
{"success":true,"id":6308444734549393409,"name":"MEMORY:test1","data":"Data 1"}
```

**Count:**
```json
{"success":true,"count":1}
```

## No Python Scripts Needed!

Once built, you can use `synrix.exe` directly from command line, just like on Linux:

```cmd
synrix.exe add memory.lattice "key" "value"
synrix.exe query memory.lattice "prefix"
```

No Python scripts, no imports, just direct command-line access!
