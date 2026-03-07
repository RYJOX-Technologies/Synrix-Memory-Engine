"""
SYNRIX Robotics - RoboticsNexus
================================

Persistent memory for robotics: sensor data, state, trajectory, crash recovery.
Built on Synrix core (ai_memory add/query API).

Usage:
    from synrix.robotics import RoboticsNexus

    robot = RoboticsNexus(robot_id="robot_001")
    robot.store_sensor("camera", image_data, timestamp)
    robot.set_state("pose", {"x": 1.5, "y": 2.3, "theta": 0.5})
    robot.log_action("move_forward", {"distance": 1.0}, success=True)
    last_state = robot.get_last_known_state()
"""

import json
import time
from typing import Optional, Dict, List, Any
from .ai_memory import get_ai_memory


class RoboticsNexus:
    """Robotics memory: sensors, state, actions, checkpoints, crash recovery."""

    def __init__(self, robot_id: str = "default_robot", memory=None):
        self.robot_id = robot_id
        self.memory = memory or get_ai_memory()
        self._checkpoint_counter = 0

    def store_sensor(self, sensor_type: str, data: Any, timestamp: Optional[float] = None, metadata: Optional[Dict] = None) -> bool:
        if timestamp is None:
            timestamp = time.time()
        sensor_data = {
            "robot_id": self.robot_id,
            "sensor_type": sensor_type,
            "data": data if isinstance(data, (dict, list)) else str(data),
            "timestamp": timestamp,
            "metadata": metadata or {},
        }
        key = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:{timestamp}"
        self.memory.add(key, json.dumps(sensor_data))
        latest_key = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:latest"
        self.memory.add(latest_key, json.dumps(sensor_data))
        return True

    def get_latest_sensor(self, sensor_type: str) -> Optional[Dict]:
        key = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:latest"
        results = self.memory.query(key, limit=1)
        if results:
            try:
                return json.loads(results[0]["data"])
            except Exception:
                return None
        return None

    def set_state(self, state_type: str, state_data: Dict, timestamp: Optional[float] = None) -> bool:
        if timestamp is None:
            timestamp = time.time()
        state_record = {"robot_id": self.robot_id, "state_type": state_type, "state_data": state_data, "timestamp": timestamp}
        key = f"ROBOT:{self.robot_id}:STATE:{state_type}:{timestamp}"
        self.memory.add(key, json.dumps(state_record))
        latest_key = f"ROBOT:{self.robot_id}:STATE:{state_type}:latest"
        self.memory.add(latest_key, json.dumps(state_record))
        return True

    def get_state(self, state_type: str) -> Optional[Dict]:
        key = f"ROBOT:{self.robot_id}:STATE:{state_type}:latest"
        results = self.memory.query(key, limit=1)
        if results:
            try:
                return json.loads(results[0]["data"]).get("state_data")
            except Exception:
                return None
        return None

    def get_last_known_state(self) -> Dict[str, Any]:
        prefix = f"ROBOT:{self.robot_id}:STATE:"
        results = self.memory.query(prefix, limit=200)
        all_states = {}
        for r in results:
            name = r.get("name") or ""
            if ":latest" not in name:
                continue
            try:
                rec = json.loads(r["data"])
                st = rec.get("state_type")
                if not st and "STATE:" in name and ":latest" in name:
                    st = name.split("STATE:")[-1].replace(":latest", "").strip(":")
                if st:
                    all_states[st] = rec.get("state_data")
            except Exception:
                continue
        return all_states

    def log_action(self, action_type: str, action_data: Dict, success: bool = True, timestamp: Optional[float] = None, metadata: Optional[Dict] = None) -> bool:
        if timestamp is None:
            timestamp = time.time()
        action_record = {
            "robot_id": self.robot_id,
            "action_type": action_type,
            "action_data": action_data,
            "success": success,
            "timestamp": timestamp,
            "metadata": metadata or {},
        }
        key = f"ROBOT:{self.robot_id}:ACTION:{action_type}:{timestamp}"
        self.memory.add(key, json.dumps(action_record))
        outcome = "SUCCESS" if success else "FAILURE"
        outcome_key = f"ROBOT:{self.robot_id}:{outcome}:{action_type}:{timestamp}"
        self.memory.add(outcome_key, json.dumps(action_record))
        return True

    def get_trajectory(self, start_time: Optional[float] = None, end_time: Optional[float] = None, limit: int = 1000) -> List[Dict]:
        prefix = f"ROBOT:{self.robot_id}:ACTION:"
        results = self.memory.query(prefix, limit=limit * 2)
        trajectory = []
        for r in results:
            try:
                rec = json.loads(r["data"])
                ts = rec.get("timestamp", 0)
                if start_time and ts < start_time:
                    continue
                if end_time and ts > end_time:
                    continue
                trajectory.append(rec)
            except Exception:
                continue
        trajectory.sort(key=lambda x: x.get("timestamp", 0))
        return trajectory[:limit]

    def get_failures(self, action_type: Optional[str] = None, limit: int = 100) -> List[Dict]:
        prefix = f"ROBOT:{self.robot_id}:FAILURE:"
        if action_type:
            prefix += f"{action_type}:"
        results = self.memory.query(prefix, limit=limit)
        out = []
        for r in results:
            try:
                out.append(json.loads(r["data"]))
            except Exception:
                continue
        out.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        return out[:limit]

    def get_successes(self, action_type: Optional[str] = None, limit: int = 100) -> List[Dict]:
        prefix = f"ROBOT:{self.robot_id}:SUCCESS:"
        if action_type:
            prefix += f"{action_type}:"
        results = self.memory.query(prefix, limit=limit)
        out = []
        for r in results:
            try:
                out.append(json.loads(r["data"]))
            except Exception:
                continue
        out.sort(key=lambda x: x.get("timestamp", 0), reverse=True)
        return out[:limit]

    def create_checkpoint(self, checkpoint_name: Optional[str] = None) -> str:
        if checkpoint_name is None:
            self._checkpoint_counter += 1
            checkpoint_name = f"checkpoint_{self._checkpoint_counter}"
        all_states = self.get_last_known_state()
        checkpoint_data = {"robot_id": self.robot_id, "checkpoint_name": checkpoint_name, "states": all_states, "timestamp": time.time()}
        checkpoint_id = f"{checkpoint_name}_{int(time.time())}"
        key = f"ROBOT:{self.robot_id}:CHECKPOINT:{checkpoint_id}"
        self.memory.add(key, json.dumps(checkpoint_data))
        latest_key = f"ROBOT:{self.robot_id}:CHECKPOINT:latest"
        self.memory.add(latest_key, json.dumps(checkpoint_data))
        return checkpoint_id

    def restore_from_checkpoint(self, checkpoint_id: str) -> bool:
        key = f"ROBOT:{self.robot_id}:CHECKPOINT:{checkpoint_id}"
        results = self.memory.query(key, limit=1)
        if not results:
            return False
        try:
            data = json.loads(results[0]["data"])
            for state_type, state_data in (data.get("states") or {}).items():
                self.set_state(state_type, state_data)
            return True
        except Exception:
            return False

    def get_stats(self) -> Dict[str, Any]:
        prefix = f"ROBOT:{self.robot_id}:"
        return {
            "robot_id": self.robot_id,
            "sensor_readings": len(self.memory.query(prefix + "SENSOR:", limit=10000)),
            "state_updates": len(self.memory.query(prefix + "STATE:", limit=10000)),
            "actions": len(self.memory.query(prefix + "ACTION:", limit=10000)),
            "successes": len(self.memory.query(prefix + "SUCCESS:", limit=10000)),
            "failures": len(self.memory.query(prefix + "FAILURE:", limit=10000)),
            "checkpoints": len(self.memory.query(prefix + "CHECKPOINT:", limit=1000)),
            "total_nodes": self.memory.count(),
        }

    def clear_all(self) -> bool:
        """
        Not implemented. Synrix does not expose a delete or truncate operation
        in the current SDK. To reset robot state, delete the .lattice file and
        reinitialise. Returns False rather than silently succeeding.
        """
        raise NotImplementedError(
            "clear_all() is not supported. Synrix has no delete operation in the "
            "current SDK. To reset state, delete the .lattice file and reinitialise."
        )
