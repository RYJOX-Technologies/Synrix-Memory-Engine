#!/bin/bash
# Run benchmark in MSYS2 environment

cd /c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build

export PATH="$PATH:/c/Users/Livew/Desktop/synrix-windows-build/synrix-windows-build/build/windows/build_msys2/bin"

./benchmark_c_raw.exe 10000 1000 100
