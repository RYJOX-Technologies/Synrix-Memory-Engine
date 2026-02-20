#!/usr/bin/env python3
"""
SYNRIX Agent-to-Agent Demo

Shows Synrix as shared memory for agents (and agent-to-future-self). No human
at the storage layer, no embeddings: agents use a structured key/prefix
namespace and retrieve by prefix.

Run from agent-memory-sdk root:
  pip install -e .
  set SYNRIX_LIB_PATH=<path to dir containing libsynrix.dll>
  python -m synrix.examples.agent_to_agent_demo

Or: python synrix/examples/agent_to_agent_demo.py
"""

import json
import os
import sys

# Ensure synrix is importable
if __name__ == "__main__" and "__file__" in dir():
    _root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if _root not in sys.path:
        sys.path.insert(0, _root)

# Shared namespace: all agents use the same prefix convention
# TASK:{id}:plan:     - planner writes steps
# TASK:{id}:outcomes: - executor writes results
# TASK:{id}:summary:  - summarizer writes final report
TASK_ID = "demo_001"
PREFIX_PLAN = f"TASK:{TASK_ID}:plan:"
PREFIX_OUTCOMES = f"TASK:{TASK_ID}:outcomes:"
PREFIX_SUMMARY = f"TASK:{TASK_ID}:summary:"
PREFIX_TASK = f"TASK:{TASK_ID}:"


def main():
    demo_lattice = os.path.join(os.path.dirname(__file__), "agent_demo.lattice")
    if os.path.exists(demo_lattice):
        try:
            os.remove(demo_lattice)
        except Exception:
            pass

    print("SYNRIX Agent-to-Agent Demo")
    print("==========================")
    print("Agents share one lattice. No human queries, no embeddings.")
    print("Lookup is by prefix (structured keys).\n")

    try:
        from synrix.ai_memory import AIMemory
        memory = AIMemory(lattice_path=demo_lattice)
        backend = memory.backend
    except ImportError:
        print("ERROR: synrix not installed. pip install -e . from agent-memory-sdk")
        return 1
    except Exception as e:
        print(f"ERROR: Synrix init failed: {e}")
        print("Set SYNRIX_LIB_PATH to the directory containing libsynrix.dll")
        return 1

    # --- Agent A: Planner ---
    print("[Agent A - Planner] Writing plan (prefix: {})".format(PREFIX_PLAN))
    steps = ["Analyze request", "Fetch context", "Generate response", "Log outcome"]
    for i, step in enumerate(steps):
        key = "{}step_{}".format(PREFIX_PLAN, i + 1)
        memory.add(key, json.dumps({"order": i + 1, "action": step}))
    memory.add("{}meta".format(PREFIX_PLAN), json.dumps({"task_id": TASK_ID, "steps_count": len(steps)}))
    print("   Wrote {} plan nodes.\n".format(len(steps) + 1))

    # --- Agent B: Executor ---
    print("[Agent B - Executor] Reading plan by prefix, then writing outcomes.")
    plan_nodes = memory.query(PREFIX_PLAN, limit=20)
    print("   Read {} nodes under {}.".format(len(plan_nodes), PREFIX_PLAN))
    for node in plan_nodes:
        name = node.get("name", "")
        if name.startswith(PREFIX_OUTCOMES) or "outcomes" in name:
            continue
        if "step_" in name:
            # Simulate doing the step
            step_id = name.split("step_")[-1].rstrip(":")
            outcome_key = "{}step_{}".format(PREFIX_OUTCOMES, step_id)
            memory.add(outcome_key, json.dumps({"status": "done", "step": step_id}))
    memory.add("{}meta".format(PREFIX_OUTCOMES), json.dumps({"task_id": TASK_ID, "completed": True}))
    print("   Wrote outcome nodes under {}.\n".format(PREFIX_OUTCOMES))

    # --- Agent C: Summarizer (or same agent, later) ---
    print("[Agent C - Summarizer] Reading full task context by prefix: {}.".format(PREFIX_TASK))
    all_nodes = memory.query(PREFIX_TASK, limit=50)
    print("   Retrieved {} nodes (plan + outcomes).".format(len(all_nodes)))
    plan_data = [n for n in all_nodes if PREFIX_PLAN in n.get("name", "")]
    outcome_data = [n for n in all_nodes if PREFIX_OUTCOMES in n.get("name", "")]
    summary_parts = [
        "Task {}: {} plan steps, {} outcomes.".format(
            TASK_ID, len(plan_data), len(outcome_data)
        ),
        "Plan steps: " + ", ".join(
            json.loads(n.get("data", "{}")).get("action", "?") for n in sorted(plan_data, key=lambda x: x.get("name", "")) if "step_" in n.get("name", "")
        ),
        "All outcomes completed.",
    ]
    summary_text = " ".join(summary_parts)
    memory.add("{}report".format(PREFIX_SUMMARY), summary_text)
    print("   Wrote summary under {} (single node).\n".format(PREFIX_SUMMARY))

    # --- Show what "the human" might see (but never touched Synrix) ---
    print("--- Result (assembled by agents; human never queried the store) ---")
    summary_nodes = memory.query(PREFIX_SUMMARY, limit=5)
    for n in summary_nodes:
        print("  ", n.get("data", ""))
    print()

    # Optional: one large payload to show chunked storage
    print("[Agent A - Planner] Writing a large payload (triggers chunked storage in engine).")
    large_report = "LONG_REPORT: " + ("Lorem ipsum context and findings. " * 80)
    memory.add("TASK:{}:plan:large_attachment".format(TASK_ID), large_report)
    print("   Done (payload >512 bytes -> engine uses chunked storage).\n")

    print("Demo complete. All reads/writes were by prefix; no embeddings, no human at the store.")
    print("Lattice: {}".format(demo_lattice))
    return 0


if __name__ == "__main__":
    sys.exit(main())
