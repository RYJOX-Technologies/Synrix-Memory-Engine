# Robotics SDK – Demos

## Public demos

- **`robotics_quick_demo.py`** – Minimal RoboticsNexus: store_sensor, set_state, get_state, log_action. Shows persistence for robot state and sensor data.

  ```bash
  pip install -e .
  set SYNRIX_LIB_PATH=<path to dir with libsynrix.dll>
  python -m synrix.examples.robotics_quick_demo
  ```

- **`tour.py`** – Guided knowledge-graph tour (run `python -m synrix` from robotics-sdk). Uses the same engine; adds concepts and runs prefix queries.
