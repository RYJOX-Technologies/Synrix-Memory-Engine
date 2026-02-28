#!/usr/bin/env python3
"""
Synrix Demo: Multi-Session Agent Memory
=========================================
Shows cross-session recall: preferences stored in session 1
are available in session 2, updated in session 3 — all local,
no cloud, no embeddings.

Run:
  pip install synrix
  python3 ai_agent_synrix_demo.py
"""

import sys, os, time, tempfile, shutil
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from synrix.raw_backend import RawSynrixBackend


class AgentMemory:
    """Thin wrapper: stores/retrieves user facts in Synrix."""

    def __init__(self, backend):
        self.b = backend

    def remember(self, key, value):
        name = f"USER_PREF_{key.upper()}"
        existing = self.b.find_by_prefix(name, limit=1, raw=False)
        if existing:
            self.b.add_node(name, value)
        else:
            self.b.add_node(name, value)

    def recall(self, key=None):
        prefix = f"USER_PREF_{key.upper()}" if key else "USER_PREF_"
        results = self.b.find_by_prefix(prefix, limit=50, raw=False)
        facts = {}
        for r in results:
            k = r["name"].replace("USER_PREF_", "").lower()
            facts[k] = r["data"]
        return facts

    def update(self, key, value):
        self.remember(key, value)

    def store_conversation(self, session_id, role, message):
        name = f"CONV_S{session_id}_{role.upper()}"
        self.b.add_node(name, message)

    def get_history(self, session_id=None):
        prefix = f"CONV_S{session_id}_" if session_id else "CONV_"
        results = self.b.find_by_prefix(prefix, limit=100, raw=False)
        return [(r["name"], r["data"]) for r in results]


def session_1(mem):
    print("SESSION 1 — User introduces themselves")
    print("-" * 50)

    msgs = [
        ("user",  "Hi! I'm Alex. I'm a backend engineer at Acme Corp."),
        ("user",  "I mostly write Python and Go. I prefer dark mode everywhere."),
        ("user",  "I use Neovim and deploy to Kubernetes on AWS."),
    ]

    for role, msg in msgs:
        mem.store_conversation(1, role, msg)
        print(f"  [{role}] {msg}")

    mem.remember("name", "Alex")
    mem.remember("role", "Backend engineer")
    mem.remember("company", "Acme Corp")
    mem.remember("languages", "Python, Go")
    mem.remember("theme", "dark mode")
    mem.remember("editor", "Neovim")
    mem.remember("deploy_target", "Kubernetes on AWS")

    t0 = time.perf_counter()
    stored = mem.recall()
    elapsed_us = (time.perf_counter() - t0) * 1e6
    print(f"\n  Stored {len(stored)} facts in {elapsed_us:.1f} us")
    print()


def session_2(mem):
    print("SESSION 2 — New session, agent recalls everything")
    print("-" * 50)

    t0 = time.perf_counter()
    facts = mem.recall()
    elapsed_us = (time.perf_counter() - t0) * 1e6

    print(f"  Agent recalls {len(facts)} user preferences ({elapsed_us:.1f} us):")
    for k, v in sorted(facts.items()):
        print(f"    {k}: {v}")

    mem.store_conversation(2, "assistant",
        f"Welcome back, {facts.get('name', 'there')}! "
        f"I remember you're a {facts.get('role', 'developer')} at {facts.get('company', 'your company')}, "
        f"working with {facts.get('languages', 'your stack')}.")

    print(f"\n  [assistant] Welcome back, {facts.get('name')}! I remember you're a "
          f"{facts.get('role')} at {facts.get('company')}, working with {facts.get('languages')}.")

    mem.store_conversation(2, "user", "Can you help me with a Go HTTP handler?")
    print(f"  [user] Can you help me with a Go HTTP handler?")

    mem.store_conversation(2, "assistant",
        f"Sure! I'll write it in Go since that's one of your languages, "
        f"and I'll keep the style consistent with your {facts.get('editor', 'editor')} setup.")
    print(f"  [assistant] Sure! I'll write it in Go since that's one of your languages.")
    print()


def session_3(mem):
    print("SESSION 3 — User updates a preference")
    print("-" * 50)

    t0 = time.perf_counter()
    old_editor = mem.recall("editor")
    elapsed_us = (time.perf_counter() - t0) * 1e6

    print(f"  Agent recalls editor preference: {old_editor.get('editor', '?')} ({elapsed_us:.1f} us)")

    mem.store_conversation(3, "user", "Actually I switched from Neovim to Zed.")
    print(f"  [user] Actually I switched from Neovim to Zed.")

    mem.update("editor", "Zed")
    print(f"  Agent updated editor preference: Neovim -> Zed")

    t0 = time.perf_counter()
    new_facts = mem.recall()
    elapsed_us = (time.perf_counter() - t0) * 1e6

    print(f"\n  Current memory ({len(new_facts)} facts, {elapsed_us:.1f} us):")
    for k, v in sorted(new_facts.items()):
        marker = " (updated)" if k == "editor" else ""
        print(f"    {k}: {v}{marker}")

    total_history = mem.get_history()
    print(f"\n  Total conversation history: {len(total_history)} messages across 3 sessions")
    print()


def main():
    print("Synrix — Multi-Session Agent Memory")
    print("=" * 50)
    print("No cloud. No embeddings. No Mem0. Just local storage.\n")

    tmpdir = tempfile.mkdtemp()
    lattice_path = os.path.join(tmpdir, "agent_memory.lattice")

    try:
        backend = RawSynrixBackend(lattice_path, max_nodes=26000)
        mem = AgentMemory(backend)

        session_1(mem)
        session_2(mem)
        session_3(mem)

        print("-" * 50)
        print("Key takeaway: cross-session memory with microsecond recall,")
        print("stored locally, no API calls, no embedding model, no cloud sync.")

    finally:
        backend.close()
        shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == "__main__":
    main()
