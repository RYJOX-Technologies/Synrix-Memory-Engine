#!/usr/bin/env python3
"""
SYNRIX Agent Benchmark

Comprehensive benchmarking suite comparing baseline vs Synrix-enhanced agents.
Produces detailed metrics for the Morphos demo.
"""

import sys
import os
import time
import json
from typing import Dict, List, Any

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

from agent_demo import BaselineAgent, SynrixAgent, Task, create_test_tasks
from synrix.agent_memory import SynrixMemory


def run_benchmark(
    num_tasks: int = 50,
    num_runs: int = 3,
    use_mock: bool = True
) -> Dict[str, Any]:
    """
    Run comprehensive benchmark.
    
    Args:
        num_tasks: Number of tasks per run
        num_runs: Number of runs to average
        use_mock: Use mock engine (True) or real server (False)
    
    Returns:
        Benchmark results
    """
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║           SYNRIX Agent Benchmark Suite                         ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    baseline_metrics = {
        "success_rates": [],
        "avg_times": [],
        "repeated_errors": [],
        "total_times": []
    }
    
    synrix_metrics = {
        "success_rates": [],
        "avg_times": [],
        "repeated_errors": [],
        "total_times": [],
        "memory_lookups": []
    }
    
    # Run multiple iterations
    for run in range(num_runs):
        print(f"Run {run + 1}/{num_runs}...")
        
        # Baseline agent
        tasks = create_test_tasks(count=num_tasks)
        baseline_agent = BaselineAgent()
        baseline_results = baseline_agent.run_task_loop(tasks)
        
        baseline_metrics["success_rates"].append(baseline_results["success_rate"])
        baseline_metrics["avg_times"].append(baseline_results["avg_time_ms"])
        baseline_metrics["repeated_errors"].append(baseline_results["repeated_errors"])
        baseline_metrics["total_times"].append(baseline_results["total_time_ms"])
        
        # Synrix agent (fresh memory for each run, but learns within run)
        memory = SynrixMemory(use_mock=use_mock)
        tasks = create_test_tasks(count=num_tasks)
        synrix_agent = SynrixAgent(memory)
        synrix_results = synrix_agent.run_task_loop(tasks)
        
        synrix_metrics["success_rates"].append(synrix_results["success_rate"])
        synrix_metrics["avg_times"].append(synrix_results["avg_time_ms"])
        synrix_metrics["repeated_errors"].append(synrix_results["repeated_errors"])
        synrix_metrics["total_times"].append(synrix_results["total_time_ms"])
        synrix_metrics["memory_lookups"].append(synrix_results.get("memory_lookups", 0))
    
    # Calculate averages
    def avg(lst):
        return sum(lst) / len(lst) if lst else 0
    
    baseline_avg = {
        "success_rate": avg(baseline_metrics["success_rates"]),
        "avg_time_ms": avg(baseline_metrics["avg_times"]),
        "repeated_errors": avg(baseline_metrics["repeated_errors"]),
        "total_time_ms": avg(baseline_metrics["total_times"])
    }
    
    synrix_avg = {
        "success_rate": avg(synrix_metrics["success_rates"]),
        "avg_time_ms": avg(synrix_metrics["avg_times"]),
        "repeated_errors": avg(synrix_metrics["repeated_errors"]),
        "total_time_ms": avg(synrix_metrics["total_times"]),
        "memory_lookups": avg(synrix_metrics["memory_lookups"])
    }
    
    # Calculate improvements
    improvement = {
        "success_rate": synrix_avg["success_rate"] - baseline_avg["success_rate"],
        "speed_ratio": baseline_avg["avg_time_ms"] / synrix_avg["avg_time_ms"] if synrix_avg["avg_time_ms"] > 0 else 1.0,
        "errors_avoided": baseline_avg["repeated_errors"] - synrix_avg["repeated_errors"]
    }
    
    # Print results
    print()
    print("═══════════════════════════════════════════════════════════════")
    print("  BENCHMARK RESULTS (Averaged over {} runs)".format(num_runs))
    print("═══════════════════════════════════════════════════════════════")
    print()
    
    print("BASELINE AGENT:")
    print(f"  Success Rate:    {baseline_avg['success_rate']:.1%}")
    print(f"  Avg Time:        {baseline_avg['avg_time_ms']:.2f} ms")
    print(f"  Repeated Errors: {baseline_avg['repeated_errors']:.1f}")
    print()
    
    print("SYNRIX-ENHANCED AGENT:")
    print(f"  Success Rate:    {synrix_avg['success_rate']:.1%}")
    print(f"  Avg Time:        {synrix_avg['avg_time_ms']:.2f} ms")
    print(f"  Repeated Errors: {synrix_avg['repeated_errors']:.1f}")
    print(f"  Memory Lookups:  {synrix_avg['memory_lookups']:.0f}")
    print()
    
    print("IMPROVEMENT:")
    print(f"  Success Rate:    {improvement['success_rate']:+.1%}")
    print(f"  Speed:           {improvement['speed_ratio']:.2f}x")
    print(f"  Errors Avoided:  {improvement['errors_avoided']:.1f}")
    print()
    
    return {
        "baseline": baseline_avg,
        "synrix": synrix_avg,
        "improvement": improvement,
        "raw": {
            "baseline": baseline_metrics,
            "synrix": synrix_metrics
        }
    }


if __name__ == "__main__":
    results = run_benchmark(num_tasks=50, num_runs=3)
    
    # Save results
    output_file = "agent_benchmark_results.json"
    with open(output_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"✅ Results saved to {output_file}")

