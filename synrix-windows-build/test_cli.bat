@echo off
REM Test SYNRIX CLI
REM Run from MSYS2 terminal for best results

echo Testing SYNRIX CLI...
echo.

cd /d "%~dp0"

REM Add DLL directory to PATH
set PATH=%~dp0build\windows\build_msys2\bin;%PATH%

REM Test add
echo 1. Adding node...
build\windows\build_msys2\bin\synrix.exe add test_cli.lattice "MEMORY:test1" "This is test data"
echo.

REM Test query
echo 2. Querying nodes...
build\windows\build_msys2\bin\synrix.exe query test_cli.lattice "MEMORY:" 10
echo.

REM Test count
echo 3. Counting nodes...
build\windows\build_msys2\bin\synrix.exe count test_cli.lattice
echo.

echo Done!
