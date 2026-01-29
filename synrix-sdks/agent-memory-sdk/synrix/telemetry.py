"""
SYNRIX Telemetry and Feedback

Optional, privacy-respecting telemetry for understanding real-world usage
and performance across different hardware configurations.

All telemetry is OPT-IN and can be disabled at any time.
"""

import platform
import sys
import time
import json
import hashlib
from typing import Optional, Dict, Any
from datetime import datetime

try:
    import psutil
    PSUTIL_AVAILABLE = True
except ImportError:
    PSUTIL_AVAILABLE = False


class TelemetryCollector:
    """
    Collects hardware and performance telemetry (opt-in only).
    
    Usage:
        collector = TelemetryCollector(enabled=True)
        collector.record_operation("add_node", latency_ms=1.5)
        collector.submit_feedback("Great performance on Jetson!")
    """
    
    def __init__(self, enabled: bool = False):
        """
        Initialize telemetry collector.
        
        Args:
            enabled: Whether telemetry is enabled (default: False, opt-in)
        """
        self.enabled = enabled
        self.session_id = self._generate_session_id()
        self.operations: list = []
        self.start_time = time.time()
    
    def _generate_session_id(self) -> str:
        """Generate anonymous session ID"""
        seed = f"{platform.node()}{time.time()}"
        return hashlib.sha256(seed.encode()).hexdigest()[:16]
    
    def get_hardware_info(self) -> Dict[str, Any]:
        """
        Collect hardware information (anonymous, no PII).
        
        Returns:
            Dictionary with hardware information
        """
        info = {
            "platform": platform.system(),
            "platform_release": platform.release(),
            "platform_version": platform.version(),
            "architecture": platform.machine(),
            "processor": platform.processor(),
            "python_version": f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}",
        }
        
        if PSUTIL_AVAILABLE:
            try:
                info["cpu_count"] = psutil.cpu_count(logical=True)
                info["cpu_freq_mhz"] = psutil.cpu_freq().current if psutil.cpu_freq() else None
                info["ram_total_gb"] = round(psutil.virtual_memory().total / (1024**3), 2)
                info["ram_available_gb"] = round(psutil.virtual_memory().available / (1024**3), 2)
            except Exception:
                pass  # Gracefully handle any psutil errors
        
        return info
    
    def record_operation(
        self,
        operation: str,
        latency_ms: Optional[float] = None,
        success: bool = True,
        error_type: Optional[str] = None,
        metadata: Optional[Dict[str, Any]] = None
    ):
        """
        Record an operation for performance analysis.
        
        Args:
            operation: Operation name (e.g., "add_node", "query_prefix")
            latency_ms: Operation latency in milliseconds
            success: Whether operation succeeded
            error_type: Error type if failed
            metadata: Additional metadata
        """
        if not self.enabled:
            return
        
        record = {
            "operation": operation,
            "timestamp": time.time(),
            "success": success,
        }
        
        if latency_ms is not None:
            record["latency_ms"] = latency_ms
        
        if error_type:
            record["error_type"] = error_type
        
        if metadata:
            record["metadata"] = metadata
        
        self.operations.append(record)
    
    def get_telemetry_summary(self) -> Dict[str, Any]:
        """
        Get summary of collected telemetry.
        
        Returns:
            Dictionary with telemetry summary
        """
        if not self.enabled:
            return {}
        
        summary = {
            "session_id": self.session_id,
            "timestamp": datetime.utcnow().isoformat(),
            "hardware": self.get_hardware_info(),
            "operations": {
                "total": len(self.operations),
                "by_type": {},
                "latency_stats": {},
            },
            "session_duration_seconds": time.time() - self.start_time,
        }
        
        # Aggregate operations
        for op in self.operations:
            op_type = op["operation"]
            if op_type not in summary["operations"]["by_type"]:
                summary["operations"]["by_type"][op_type] = {
                    "count": 0,
                    "success_count": 0,
                    "error_count": 0,
                    "latencies": [],
                }
            
            stats = summary["operations"]["by_type"][op_type]
            stats["count"] += 1
            if op["success"]:
                stats["success_count"] += 1
            else:
                stats["error_count"] += 1
            
            if "latency_ms" in op:
                stats["latencies"].append(op["latency_ms"])
        
        # Calculate latency statistics
        for op_type, stats in summary["operations"]["by_type"].items():
            if stats["latencies"]:
                latencies = stats["latencies"]
                summary["operations"]["latency_stats"][op_type] = {
                    "min_ms": min(latencies),
                    "max_ms": max(latencies),
                    "avg_ms": sum(latencies) / len(latencies),
                    "p50_ms": sorted(latencies)[len(latencies) // 2],
                    "p95_ms": sorted(latencies)[int(len(latencies) * 0.95)] if len(latencies) > 1 else latencies[0],
                    "p99_ms": sorted(latencies)[int(len(latencies) * 0.99)] if len(latencies) > 1 else latencies[0],
                }
        
        return summary
    
    def submit_feedback(
        self,
        feedback: str,
        email: Optional[str] = None,
        include_telemetry: bool = True
    ) -> Dict[str, Any]:
        """
        Submit feedback with optional telemetry.
        
        Args:
            feedback: User feedback text
            email: Optional email for follow-up (not required)
            include_telemetry: Whether to include telemetry data
        
        Returns:
            Dictionary with submission details (for manual submission)
        """
        payload = {
            "feedback": feedback,
            "timestamp": datetime.utcnow().isoformat(),
        }
        
        if email:
            payload["email"] = email
        
        if include_telemetry and self.enabled:
            payload["telemetry"] = self.get_telemetry_summary()
        elif include_telemetry:
            # Include hardware info even if telemetry disabled
            payload["hardware"] = self.get_hardware_info()
        
        return payload
    
    def export_telemetry(self, filepath: Optional[str] = None) -> str:
        """
        Export telemetry data to JSON file.
        
        Args:
            filepath: Optional file path (default: timestamped filename)
        
        Returns:
            Path to exported file
        """
        if not self.enabled:
            raise ValueError("Telemetry is disabled")
        
        if filepath is None:
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            filepath = f"synrix_telemetry_{timestamp}.json"
        
        data = self.get_telemetry_summary()
        
        with open(filepath, "w") as f:
            json.dump(data, f, indent=2)
        
        return filepath


# Global telemetry instance (disabled by default)
_global_telemetry: Optional[TelemetryCollector] = None


def enable_telemetry():
    """Enable global telemetry collection"""
    global _global_telemetry
    _global_telemetry = TelemetryCollector(enabled=True)


def disable_telemetry():
    """Disable global telemetry collection"""
    global _global_telemetry
    _global_telemetry = None


def get_telemetry() -> Optional[TelemetryCollector]:
    """Get global telemetry instance"""
    return _global_telemetry


def record_operation(operation: str, latency_ms: Optional[float] = None, **kwargs):
    """Record operation using global telemetry"""
    if _global_telemetry:
        _global_telemetry.record_operation(operation, latency_ms, **kwargs)

