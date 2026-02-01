# Windows Build Setup

This directory contains everything needed to build the SYNRIX binary (`libsynrix.dll`) for Windows.

## Quick Start

1. **Transfer from Jetson**: Run `create-transfer-package.sh` on Jetson
2. **Transfer to Windows**: Copy the tar.gz file to your Windows PC
3. **Extract**: Extract the archive
4. **Build**: Run `.\build.ps1` or `build.bat`
5. **Output**: `libsynrix.dll` will be in `build/Release/` or `build/Debug/`

## Directory Structure

```
build/windows/
├── src/                      # All .c source files (copied from lattice/)
├── include/                  # All .h header files (copied from lattice/)
├── CMakeLists.txt           # CMake build configuration
├── build.ps1                 # PowerShell build script (recommended)
├── build.bat                 # Batch file build script
├── create-transfer-package.sh  # Script to create transfer package
├── BUILD.md                 # Detailed build instructions
└── README.md                # This file
```

## Files Included

- **Source Files**: All `.c` files from `src/storage/lattice/`
- **Header Files**: All `.h` files from `src/storage/lattice/`
- **Build Scripts**: PowerShell and Batch file options
- **CMake Configuration**: Ready-to-use CMakeLists.txt

## Transfer Process

**On Jetson:**
```bash
cd /mnt/nvme/aion-omega/NebulOS-Scaffolding/build/windows
bash create-transfer-package.sh
```

**On Windows:**
```powershell
# Transfer (replace with your Jetson IP)
scp astro@192.168.1.185:/tmp/synrix-windows-build.tar.gz C:\synrix-windows-build.tar.gz

# Extract (use 7-Zip or PowerShell)
Expand-Archive -Path C:\synrix-windows-build.tar.gz -DestinationPath C:\
# OR use 7-Zip GUI

# Build
cd C:\synrix-windows-build
.\build.ps1
```

## Prerequisites

- **CMake 3.15+** - https://cmake.org/download/
- **Visual Studio 2019/2022** (with C++ workload) OR **MinGW-w64**

See `BUILD.md` for detailed instructions.
