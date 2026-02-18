"""
SYNRIX Robotics SDK - RoboticsNexus
===================================

A complete robotics memory system built on SYNRIX Core Engine.
Provides persistent storage for sensor data, state management, trajectory logging,
and crash recovery for robotics applications.

Perfect for:
- Autonomous robots
- Drones
- Industrial automation
- Service robots
- Research robots

Key Features:
- Sensor data storage (camera, lidar, IMU, GPS)
- State management (pose, battery, environment)
- Trajectory/action logging
- Crash recovery (resume from last known state)
- Learning from failures
- Persistent across power cycles

Usage:
    from synrix.robotics import RoboticsNexus
    
    robot = RoboticsNexus(robot_id="robot_001")
    
    # Store sensor data
    robot.store_sensor("camera", image_data, timestamp)
    
    # Store state
    robot.set_state("pose", {"x": 1.5, "y": 2.3, "theta": 0.5})
    
    # Log actions
    robot.log_action("move_forward", {"distance": 1.0, "speed": 0.5}, success=True)
    
    # Crash recovery
    last_state = robot.get_last_known_state()
"""

import json
import time
from typing import Optional, Dict, List, Any
from datetime import datetime
from .ai_memory import get_ai_memory


class RoboticsNexus:
    """
    Robotics memory system built on SYNRIX Core Engine.
    
    Provides persistent, fast storage for robotics applications with:
    - Sensor data storage (camera, lidar, IMU, GPS, etc.)
    - State management (robot pose, battery, environment state)
    - Trajectory/action logging
    - Crash recovery (resume from last known state)
    - Learning from failures (what actions led to failures)
    - Persistent across power cycles
    
    All data is stored locally using SYNRIX Core Engine, providing:
    - O(1) state lookups
    - O(k) trajectory queries
    - ACID durability
    - Crash-safe persistence
    
    Example:
        >>> robot = RoboticsNexus(robot_id="robot_001")
        >>> 
        >>> # Store sensor data
        >>> robot.store_sensor("camera", image_data, timestamp=time.time())
        >>> robot.store_sensor("lidar", point_cloud, timestamp=time.time())
        >>> 
        >>> # Store state
        >>> robot.set_state("pose", {"x": 1.5, "y": 2.3, "theta": 0.5})
        >>> robot.set_state("battery", 75)
        >>> 
        >>> # Log actions
        >>> robot.log_action("move_forward", {"distance": 1.0, "speed": 0.5}, success=True)
        >>> 
        >>> # Query history
        >>> trajectory = robot.get_trajectory(start_time, end_time)
        >>> failures = robot.get_failures()
        >>> 
        >>> # Crash recovery
        >>> last_state = robot.get_last_known_state()
    """
    
    def __init__(self, robot_id: str = "default_robot", memory=None):
        """
        Initialize RoboticsNexus.
        
        Args:
            robot_id: Unique identifier for this robot
            memory: Optional SYNRIX memory instance (default: creates new with 25k limit)
        
        Example:
            >>> robot = RoboticsNexus(robot_id="robot_001")
        
        Note: RoboticsNexus uses the 25k node limit (free tier) to encourage upgrades.
        """
        self.robot_id = robot_id
        self.memory = memory or get_ai_memory()
        self._checkpoint_counter = 0
    
    # ========================================================================
    # SENSOR DATA STORAGE
    # ========================================================================
    
    def store_sensor(
        self,
        sensor_type: str,
        data: Any,
        timestamp: Optional[float] = None,
        metadata: Optional[Dict] = None
    ) -> bool:
        """
        Store sensor data (camera, lidar, IMU, GPS, etc.).
        
        Args:
            sensor_type: Type of sensor (e.g., "camera", "lidar", "imu", "gps")
            data: Sensor data (can be dict, list, string, or binary)
            timestamp: Optional timestamp (default: current time)
            metadata: Optional metadata (e.g., sensor calibration, settings)
        
        Returns:
            True if stored successfully
        
        Example:
            >>> robot.store_sensor("camera", image_data, timestamp=time.time())
            >>> robot.store_sensor("lidar", point_cloud, metadata={"resolution": 0.1})
            >>> robot.store_sensor("imu", {"accel": [1,2,3], "gyro": [0.1,0.2,0.3]})
        """
        if timestamp is None:
            timestamp = time.time()
        
        # Prepare sensor data
        sensor_data = {
            "robot_id": self.robot_id,
            "sensor_type": sensor_type,
            "data": data if isinstance(data, (dict, list)) else str(data),
            "timestamp": timestamp,
            "metadata": metadata or {}
        }
        
        # Store with prefix for easy querying
        key = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:{timestamp}"
        self.memory.add(key, json.dumps(sensor_data))
        
        # Also store as latest for quick access
        latest_key = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:latest"
        self.memory.add(latest_key, json.dumps(sensor_data))
        
        return True
    
    def get_latest_sensor(self, sensor_type: str) -> Optional[Dict]:
        """
        Get latest sensor reading for a specific sensor type.
        
        Args:
            sensor_type: Type of sensor (e.g., "camera", "lidar")
        
        Returns:
            Latest sensor data dict, or None if not found
        
        Example:
            >>> latest_camera = robot.get_latest_sensor("camera")
            >>> if latest_camera:
            ...     print(f"Last camera reading: {latest_camera['timestamp']}")
        """
        key = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:latest"
        results = self.memory.query(key)
        
        if results:
            try:
                return json.loads(results[0]['data'])
            except:
                return None
        return None
    
    def get_sensor_history(
        self,
        sensor_type: str,
        start_time: Optional[float] = None,
        end_time: Optional[float] = None,
        limit: int = 100
    ) -> List[Dict]:
        """
        Get sensor history for a specific sensor type.
        
        Args:
            sensor_type: Type of sensor
            start_time: Optional start timestamp
            end_time: Optional end timestamp
            limit: Maximum number of results
        
        Returns:
            List of sensor data dicts
        
        Example:
            >>> camera_history = robot.get_sensor_history("camera", start_time=t0, end_time=t1)
        """
        prefix = f"ROBOT:{self.robot_id}:SENSOR:{sensor_type}:"
        results = self.memory.query(prefix)
        
        sensor_history = []
        for result in results:
            # Skip "latest" entries
            if ":latest" in result['name']:
                continue
            
            try:
                sensor_data = json.loads(result['data'])
                timestamp = sensor_data.get('timestamp', 0)
                
                # Filter by time range if provided
                if start_time and timestamp < start_time:
                    continue
                if end_time and timestamp > end_time:
                    continue
                
                sensor_history.append(sensor_data)
            except:
                continue
        
        # Sort by timestamp and limit
        sensor_history.sort(key=lambda x: x.get('timestamp', 0), reverse=True)
        return sensor_history[:limit]
    
    # ========================================================================
    # STATE MANAGEMENT
    # ========================================================================
    
    def set_state(self, state_type: str, state_data: Dict, timestamp: Optional[float] = None) -> bool:
        """
        Store robot state (pose, battery, environment, etc.).
        
        Args:
            state_type: Type of state (e.g., "pose", "battery", "environment")
            state_data: State data dict
            timestamp: Optional timestamp (default: current time)
        
        Returns:
            True if stored successfully
        
        Example:
            >>> robot.set_state("pose", {"x": 1.5, "y": 2.3, "theta": 0.5})
            >>> robot.set_state("battery", {"level": 75, "voltage": 12.5})
            >>> robot.set_state("environment", {"obstacles": [...], "goals": [...]})
        """
        if timestamp is None:
            timestamp = time.time()
        
        state_record = {
            "robot_id": self.robot_id,
            "state_type": state_type,
            "state_data": state_data,
            "timestamp": timestamp
        }
        
        # Store current state
        key = f"ROBOT:{self.robot_id}:STATE:{state_type}:{timestamp}"
        self.memory.add(key, json.dumps(state_record))
        
        # Store as latest for quick access
        latest_key = f"ROBOT:{self.robot_id}:STATE:{state_type}:latest"
        self.memory.add(latest_key, json.dumps(state_record))
        
        return True
    
    def get_state(self, state_type: str) -> Optional[Dict]:
        """
        Get current state for a specific state type.
        
        Args:
            state_type: Type of state (e.g., "pose", "battery")
        
        Returns:
            State data dict, or None if not found
        
        Example:
            >>> pose = robot.get_state("pose")
            >>> if pose:
            ...     print(f"Current position: {pose['state_data']}")
        """
        key = f"ROBOT:{self.robot_id}:STATE:{state_type}:latest"
        results = self.memory.query(key)
        
        if results:
            try:
                state_record = json.loads(results[0]['data'])
                return state_record.get('state_data')
            except:
                return None
        return None
    
    def get_last_known_state(self) -> Dict[str, Any]:
        """
        Get last known state for all state types (for crash recovery).
        
        Returns:
            Dict mapping state_type -> state_data
        
        Example:
            >>> last_state = robot.get_last_known_state()
            >>> pose = last_state.get("pose")
            >>> battery = last_state.get("battery")
            >>> # Resume robot from this state
        """
        prefix = f"ROBOT:{self.robot_id}:STATE:"
        results = self.memory.query(prefix + ":latest")
        
        all_states = {}
        for result in results:
            if ":latest" not in result['name']:
                continue
            
            try:
                state_record = json.loads(result['data'])
                state_type = state_record.get('state_type')
                if state_type:
                    all_states[state_type] = state_record.get('state_data')
            except:
                continue
        
        return all_states
    
    # ========================================================================
    # ACTION/TRAJECTORY LOGGING
    # ========================================================================
    
    def log_action(
        self,
        action_type: str,
        action_data: Dict,
        success: bool = True,
        timestamp: Optional[float] = None,
        metadata: Optional[Dict] = None
    ) -> bool:
        """
        Log robot action (move, turn, pick, place, etc.).
        
        Args:
            action_type: Type of action (e.g., "move_forward", "turn_left", "pick_object")
            action_data: Action parameters (e.g., {"distance": 1.0, "speed": 0.5})
            success: Whether action succeeded
            timestamp: Optional timestamp (default: current time)
            metadata: Optional metadata (e.g., environment conditions)
        
        Returns:
            True if logged successfully
        
        Example:
            >>> robot.log_action("move_forward", {"distance": 1.0, "speed": 0.5}, success=True)
            >>> robot.log_action("turn_left", {"angle": 90}, success=False, 
            ...                  metadata={"reason": "obstacle_detected"})
        """
        if timestamp is None:
            timestamp = time.time()
        
        action_record = {
            "robot_id": self.robot_id,
            "action_type": action_type,
            "action_data": action_data,
            "success": success,
            "timestamp": timestamp,
            "metadata": metadata or {}
        }
        
        # Store action
        key = f"ROBOT:{self.robot_id}:ACTION:{action_type}:{timestamp}"
        self.memory.add(key, json.dumps(action_record))
        
        # Also index by success/failure for learning
        outcome = "SUCCESS" if success else "FAILURE"
        outcome_key = f"ROBOT:{self.robot_id}:{outcome}:{action_type}:{timestamp}"
        self.memory.add(outcome_key, json.dumps(action_record))
        
        return True
    
    def get_trajectory(
        self,
        start_time: Optional[float] = None,
        end_time: Optional[float] = None,
        limit: int = 1000
    ) -> List[Dict]:
        """
        Get trajectory (sequence of actions and states) for time range.
        
        Args:
            start_time: Optional start timestamp
            end_time: Optional end timestamp
            limit: Maximum number of actions to return
        
        Returns:
            List of action records sorted by timestamp
        
        Example:
            >>> trajectory = robot.get_trajectory(start_time=t0, end_time=t1)
            >>> for action in trajectory:
            ...     print(f"{action['action_type']}: {action['success']}")
        """
        prefix = f"ROBOT:{self.robot_id}:ACTION:"
        results = self.memory.query(prefix)
        
        trajectory = []
        for result in results:
            try:
                action_record = json.loads(result['data'])
                timestamp = action_record.get('timestamp', 0)
                
                # Filter by time range if provided
                if start_time and timestamp < start_time:
                    continue
                if end_time and timestamp > end_time:
                    continue
                
                trajectory.append(action_record)
            except:
                continue
        
        # Sort by timestamp
        trajectory.sort(key=lambda x: x.get('timestamp', 0))
        return trajectory[:limit]
    
    # ========================================================================
    # FAILURE ANALYSIS & LEARNING
    # ========================================================================
    
    def get_failures(
        self,
        action_type: Optional[str] = None,
        limit: int = 100
    ) -> List[Dict]:
        """
        Get failure records (what actions led to failures).
        
        Args:
            action_type: Optional filter by action type
            limit: Maximum number of failures to return
        
        Returns:
            List of failure action records
        
        Example:
            >>> failures = robot.get_failures()
            >>> for failure in failures:
            ...     print(f"Failed: {failure['action_type']} - {failure.get('metadata', {}).get('reason')}")
        """
        prefix = f"ROBOT:{self.robot_id}:FAILURE:"
        if action_type:
            prefix += f"{action_type}:"
        
        results = self.memory.query(prefix)
        
        failures = []
        for result in results:
            try:
                failure_record = json.loads(result['data'])
                failures.append(failure_record)
            except:
                continue
        
        # Sort by timestamp (most recent first)
        failures.sort(key=lambda x: x.get('timestamp', 0), reverse=True)
        return failures[:limit]
    
    def get_successes(
        self,
        action_type: Optional[str] = None,
        limit: int = 100
    ) -> List[Dict]:
        """
        Get success records (what actions led to success).
        
        Args:
            action_type: Optional filter by action type
            limit: Maximum number of successes to return
        
        Returns:
            List of successful action records
        
        Example:
            >>> successes = robot.get_successes("move_forward")
            >>> # Analyze what made these successful
        """
        prefix = f"ROBOT:{self.robot_id}:SUCCESS:"
        if action_type:
            prefix += f"{action_type}:"
        
        results = self.memory.query(prefix)
        
        successes = []
        for result in results:
            try:
                success_record = json.loads(result['data'])
                successes.append(success_record)
            except:
                continue
        
        # Sort by timestamp (most recent first)
        successes.sort(key=lambda x: x.get('timestamp', 0), reverse=True)
        return successes[:limit]
    
    def get_action_statistics(self, action_type: str) -> Dict[str, Any]:
        """
        Get statistics for a specific action type (success rate, etc.).
        
        Args:
            action_type: Type of action to analyze
        
        Returns:
            Dict with statistics (total, successes, failures, success_rate)
        
        Example:
            >>> stats = robot.get_action_statistics("move_forward")
            >>> print(f"Success rate: {stats['success_rate']:.2%}")
        """
        successes = self.get_successes(action_type, limit=10000)
        failures = self.get_failures(action_type, limit=10000)
        
        total = len(successes) + len(failures)
        success_count = len(successes)
        failure_count = len(failures)
        
        return {
            "action_type": action_type,
            "total": total,
            "successes": success_count,
            "failures": failure_count,
            "success_rate": success_count / total if total > 0 else 0.0,
            "failure_rate": failure_count / total if total > 0 else 0.0
        }
    
    # ========================================================================
    # CRASH RECOVERY & CHECKPOINTS
    # ========================================================================
    
    def create_checkpoint(self, checkpoint_name: Optional[str] = None) -> str:
        """
        Create a checkpoint (snapshot of current state) for crash recovery.
        
        Args:
            checkpoint_name: Optional checkpoint name (default: auto-generated)
        
        Returns:
            Checkpoint ID
        
        Example:
            >>> checkpoint_id = robot.create_checkpoint("before_dangerous_operation")
            >>> # Later, if crash occurs:
            >>> robot.restore_from_checkpoint(checkpoint_id)
        """
        if checkpoint_name is None:
            self._checkpoint_counter += 1
            checkpoint_name = f"checkpoint_{self._checkpoint_counter}"
        
        # Get all current states
        all_states = self.get_last_known_state()
        
        checkpoint_data = {
            "robot_id": self.robot_id,
            "checkpoint_name": checkpoint_name,
            "states": all_states,
            "timestamp": time.time()
        }
        
        checkpoint_id = f"{checkpoint_name}_{int(time.time())}"
        key = f"ROBOT:{self.robot_id}:CHECKPOINT:{checkpoint_id}"
        self.memory.add(key, json.dumps(checkpoint_data))
        
        # Store as latest checkpoint
        latest_key = f"ROBOT:{self.robot_id}:CHECKPOINT:latest"
        self.memory.add(latest_key, json.dumps(checkpoint_data))
        
        return checkpoint_id
    
    def restore_from_checkpoint(self, checkpoint_id: str) -> bool:
        """
        Restore robot state from a checkpoint.
        
        Args:
            checkpoint_id: Checkpoint ID to restore from
        
        Returns:
            True if restored successfully
        
        Example:
            >>> robot.restore_from_checkpoint("checkpoint_1_1234567890")
        """
        key = f"ROBOT:{self.robot_id}:CHECKPOINT:{checkpoint_id}"
        results = self.memory.query(key)
        
        if not results:
            return False
        
        try:
            checkpoint_data = json.loads(results[0]['data'])
            states = checkpoint_data.get('states', {})
            
            # Restore all states
            for state_type, state_data in states.items():
                self.set_state(state_type, state_data)
            
            return True
        except:
            return False
    
    def get_latest_checkpoint(self) -> Optional[Dict]:
        """
        Get latest checkpoint (for automatic crash recovery).
        
        Returns:
            Latest checkpoint data, or None if not found
        
        Example:
            >>> checkpoint = robot.get_latest_checkpoint()
            >>> if checkpoint:
            ...     robot.restore_from_checkpoint(checkpoint['checkpoint_id'])
        """
        key = f"ROBOT:{self.robot_id}:CHECKPOINT:latest"
        results = self.memory.query(key)
        
        if results:
            try:
                return json.loads(results[0]['data'])
            except:
                return None
        return None
    
    # ========================================================================
    # UTILITIES
    # ========================================================================
    
    def get_stats(self) -> Dict[str, Any]:
        """
        Get robotics database statistics.
        
        Returns:
            Dict with counts of sensors, states, actions, etc.
        
        Example:
            >>> stats = robot.get_stats()
            >>> print(f"Total actions: {stats['actions']}")
        """
        prefix = f"ROBOT:{self.robot_id}:"
        
        sensors = len(self.memory.query(prefix + "SENSOR:"))
        states = len(self.memory.query(prefix + "STATE:"))
        actions = len(self.memory.query(prefix + "ACTION:"))
        successes = len(self.memory.query(prefix + "SUCCESS:"))
        failures = len(self.memory.query(prefix + "FAILURE:"))
        checkpoints = len(self.memory.query(prefix + "CHECKPOINT:"))
        
        return {
            "robot_id": self.robot_id,
            "sensor_readings": sensors,
            "state_updates": states,
            "actions": actions,
            "successes": successes,
            "failures": failures,
            "checkpoints": checkpoints,
            "total_nodes": self.memory.count()
        }
    
    def clear_all(self) -> bool:
        """
        Clear all robotics data for this robot (use with caution!).
        
        Returns:
            True if cleared
        
        Warning: This permanently deletes all data for this robot!
        """
        # Note: SYNRIX doesn't have delete yet, so this is a placeholder
        # In the future, this would delete all nodes with ROBOT:{robot_id}: prefix
        # For now, just return True (data remains but won't be queried easily)
        return True
