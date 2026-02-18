# SYNRIX Robotics SDK

Python SDK for robotics memory on SYNRIX: sensor data, robot state, action logging, crash recovery.

## What this is
This SDK is the robotics memory client. It requires the SYNRIX engine DLL (same as the Agent Memory SDK).
For internal distribution we use the free tier (25k node limit) and provide upgrade instructions when the limit is reached.

## Requirements
- Windows 10+
- Python 3.8+
- SYNRIX free tier package (DLL + runtime deps)

## Quick start (Windows)

1) Install the SDK
```bash
cd robotics-sdk
pip install -e .
```

2) Provide the engine DLL
Option A: copy into `robotics-sdk/synrix/`
- `libsynrix.dll` (single engine; tier set by SYNRIX_LICENSE_KEY)
- `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `zlib1.dll` (runtime deps)

Option B: set an explicit DLL path
```powershell
set SYNRIX_LIB_PATH=C:\path\to\libsynrix.dll
```

Use `download_zlib.ps1` if you need zlib1.dll. Run `install_dependencies.bat` if required for your system.

3) Use the SDK
```python
from synrix.robotics import RoboticsNexus

robot = RoboticsNexus(robot_id="my_robot")
robot.store_sensor("camera", {"frame": 1, "data": "..."})
state = robot.get_state()
```

## Free tier limit
When the 25k node limit is reached, the SDK raises an error with upgrade instructions. Use a paid tier to remove the limit.

## Examples
See `examples/` for quickstart and usage.

## License
See [LICENSE](LICENSE). SDK code is MIT; engine binaries are proprietary.
