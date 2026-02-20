#!/usr/bin/env python3
"""
SYNRIX Robotics – public demo

Shows RoboticsNexus: store sensor data, set/get state, log an action.
All data persists in the Synrix lattice (crash-safe, local).

Run (from robotics-sdk root, with SYNRIX_LIB_PATH set):
  pip install -e .
  python -m synrix.examples.robotics_quick_demo
"""

import os
import sys

if __name__ == "__main__" and "__file__" in dir():
    _root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if _root not in sys.path:
        sys.path.insert(0, _root)


def main():
    print("SYNRIX Robotics Quick Demo")
    print("==========================\n")

    try:
        from synrix.robotics import RoboticsNexus
    except ImportError:
        print("ERROR: synrix not installed. From robotics-sdk: pip install -e .")
        return 1
    except Exception as e:
        print("ERROR:", e)
        print("Set SYNRIX_LIB_PATH to the directory containing libsynrix.dll")
        return 1

    robot = RoboticsNexus(robot_id="demo_robot")

    # Store sensor reading
    robot.store_sensor("camera", {"frame": 1, "resolution": "640x480"})
    print("1. Stored camera sensor reading.")

    # Set and get state
    robot.set_state("pose", {"x": 1.0, "y": 2.0, "theta": 0.5})
    pose = robot.get_state("pose")
    if pose is not None:
        print("2. Set pose state; get_state('pose') ->", pose)
    else:
        print("2. Set pose state; get_state('pose') -> (stored; if None, node limit may be reached)")

    # Log an action
    robot.log_action("move_forward", {"distance": 1.0}, success=True)
    print("3. Logged action: move_forward (success=True).")

    print("\nDone. Data is persisted in the Synrix lattice (restart-safe).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
