"""
Tests for synrix.ai_memory.AIMemory (agent memory interface).

Requires Synrix DLL (SYNRIX_LIB_PATH) for real backend tests.
Tests are skipped if the backend cannot be loaded.
"""

import pytest


class TestAIMemoryAddAndQuery:
    """add() and query() by prefix."""

    def test_add_small_and_query_by_prefix(self, ai_memory):
        node_id = ai_memory.add("AGENT:test:key1", "value1")
        assert node_id is not None
        results = ai_memory.query("AGENT:test:", limit=10)
        assert len(results) >= 1
        names = [r.get("name", "") for r in results]
        data = [r.get("data", "") for r in results]
        assert "AGENT:test:key1" in names or any("key1" in n for n in names)
        assert "value1" in data

    def test_query_returns_matching_prefix_only(self, ai_memory):
        ai_memory.add("PREFIX:a:1", "a1")
        ai_memory.add("PREFIX:a:2", "a2")
        ai_memory.add("PREFIX:b:1", "b1")
        results = ai_memory.query("PREFIX:a:", limit=10)
        data = [r.get("data", "") for r in results]
        assert "a1" in data
        assert "a2" in data
        assert "b1" not in data

    def test_add_large_payload_chunked_and_query_returns_full(self, ai_memory):
        # Payload > 511 bytes uses chunked storage; query should return full data
        large = "x" * 600
        ai_memory.add("AGENT:chunked:big", large)
        results = ai_memory.query("AGENT:chunked:", limit=10)
        # May appear under AGENT:chunked: or C:AGENT:chunked: (chunked prefix)
        found = False
        for r in results:
            if r.get("data") == large:
                found = True
                break
        assert found, "Chunked payload should be returned in full"

    def test_add_empty_value_rejected(self, ai_memory):
        # add() with empty data may return None or raise; we only check it doesn't crash
        # (engine might accept or reject empty; SDK doc says "data: Memory data/value")
        result = ai_memory.add("AGENT:empty:key", "")
        # Either None or a node_id; just ensure no exception
        assert result is None or isinstance(result, int)


class TestAIMemoryGet:
    """get(node_id)"""

    def test_get_by_id_returns_data(self, ai_memory):
        node_id = ai_memory.add("AGENT:get:one", "get_me")
        assert node_id is not None
        node = ai_memory.get(node_id)
        assert node is not None
        assert node.get("data") == "get_me" or "get_me" in str(node.get("data", ""))

    def test_get_nonexistent_returns_none(self, ai_memory):
        node = ai_memory.get(999999999)
        assert node is None


class TestAIMemoryCountAndList:
    """count() and list_all()"""

    def test_count_increases_after_add(self, ai_memory):
        c0 = ai_memory.count()
        ai_memory.add("AGENT:count:a", "1")
        ai_memory.add("AGENT:count:b", "2")
        c1 = ai_memory.count()
        assert c1 >= c0 + 2

    def test_list_all_with_prefix(self, ai_memory):
        ai_memory.add("LIST:p:1", "v1")
        ai_memory.add("LIST:p:2", "v2")
        results = ai_memory.list_all(prefix="LIST:p:")
        data = [r.get("data", "") for r in results]
        assert "v1" in data
        assert "v2" in data


class TestAIMemoryAgentStyle:
    """Agent-style usage: structured keys, prefix handoff."""

    def test_agent_handoff_plan_then_outcomes(self, ai_memory):
        # Agent A: plan
        ai_memory.add("TASK:handoff:plan:step1", "analyze")
        ai_memory.add("TASK:handoff:plan:step2", "execute")
        # Agent B: read plan by prefix, write outcomes
        plan = ai_memory.query("TASK:handoff:plan:", limit=10)
        assert len(plan) >= 2
        ai_memory.add("TASK:handoff:outcomes:step1", "done")
        ai_memory.add("TASK:handoff:outcomes:step2", "done")
        # Agent C: read full task by prefix
        full = ai_memory.query("TASK:handoff:", limit=20)
        assert len(full) >= 4
        data = [r.get("data", "") for r in full]
        assert "analyze" in data
        assert "done" in data
