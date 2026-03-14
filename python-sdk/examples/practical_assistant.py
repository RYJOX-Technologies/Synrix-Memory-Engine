#!/usr/bin/env python3
"""
Practical Assistant – AI agents store custom memories, instant recall
====================================================================
Runnable assistant that uses the Synrix agent memory layer for context
and optional LLM for replies. Demonstrates the "practical app": a layer
where agents store memories as they see fit and recall them instantly.

Usage:
  # Interactive (Ollama on localhost:11434)
  python practical_assistant.py

  # Single query
  python practical_assistant.py "What's my favorite color?"

  # Use OpenAI-compatible API (e.g. Jetpack / cloud)
  OPENAI_API_BASE=https://... OPENAI_API_KEY=sk-... python practical_assistant.py "Hello"

  # Use MiniMax API (auto-detected when MINIMAX_API_KEY is set)
  MINIMAX_API_KEY=your-key python practical_assistant.py "Hello"

  # Memory-only mode (no LLM): recall and store only
  SYNRIX_ONLY=1 python practical_assistant.py "recall preference:"
"""

import os
import sys
import json
import time
import re

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

try:
    from synrix.agent_memory_layer import get_agent_memory
except ImportError:
    print("Install the Synrix SDK: pip install -e . from python-sdk/", file=sys.stderr)
    sys.exit(1)


def gather_context(memory, limit_conversation: int = 5) -> str:
    """Build context string from Synrix (preferences, facts, last conversations)."""
    lines = []
    for prefix, label in [("preference:", "Preferences"), ("fact:", "Facts"), ("conversation:", "Recent conversation")]:
        items = memory.search(prefix, limit=limit_conversation if "conversation" in prefix else 20)
        if not items:
            continue
        lines.append(f"[{label}]")
        for it in items:
            key = it.get("key", "")
            val = it.get("value")
            if isinstance(val, dict):
                val = val.get("value", str(val))
            lines.append(f"  {key}: {val}")
        lines.append("")
    return "\n".join(lines).strip()


def call_ollama(prompt: str, model: str = "llama2", max_tokens: int = 256) -> str:
    """Call local Ollama API."""
    try:
        import requests
    except ImportError:
        return "(install requests: pip install requests)"
    url = os.environ.get("OLLAMA_HOST", "http://localhost:11434") + "/api/generate"
    payload = {"model": model, "prompt": prompt, "stream": False, "options": {"num_predict": max_tokens}}
    try:
        r = requests.post(url, json=payload, timeout=60)
        r.raise_for_status()
        out = r.json().get("response", "")
        return out.strip()
    except Exception as e:
        return f"(Ollama error: {e})"


def call_minimax(prompt: str, model: str = None, max_tokens: int = 256) -> str:
    """Call MiniMax API (OpenAI-compatible, https://api.minimax.io/v1)."""
    try:
        import requests
    except ImportError:
        return "(install requests: pip install requests)"
    key = os.environ.get("MINIMAX_API_KEY", "")
    if not key:
        return "(Set MINIMAX_API_KEY for MiniMax mode)"
    model = model or os.environ.get("MINIMAX_MODEL", "MiniMax-M2.5")
    url = "https://api.minimax.io/v1/chat/completions"
    headers = {"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
    payload = {
        "model": model,
        "messages": [{"role": "user", "content": prompt}],
        "max_tokens": max_tokens,
        "temperature": 0.7,
    }
    try:
        r = requests.post(url, json=payload, headers=headers, timeout=60)
        r.raise_for_status()
        data = r.json()
        choice = (data.get("choices") or [{}])[0]
        return (choice.get("message") or {}).get("content", "").strip()
    except Exception as e:
        return f"(MiniMax error: {e})"


def call_openai_compatible(prompt: str, model: str = None, max_tokens: int = 256) -> str:
    """Call OpenAI-compatible API (e.g. OPENAI_API_BASE)."""
    try:
        import requests
    except ImportError:
        return "(install requests)"
    base = os.environ.get("OPENAI_API_BASE", "").rstrip("/")
    key = os.environ.get("OPENAI_API_KEY", "")
    if not base or not key:
        return "(Set OPENAI_API_BASE and OPENAI_API_KEY for API mode)"
    model = model or os.environ.get("OPENAI_MODEL", "gpt-3.5-turbo")
    url = f"{base}/chat/completions"
    headers = {"Authorization": f"Bearer {key}", "Content-Type": "application/json"}
    payload = {
        "model": model,
        "messages": [{"role": "user", "content": prompt}],
        "max_tokens": max_tokens,
    }
    try:
        r = requests.post(url, json=payload, headers=headers, timeout=60)
        r.raise_for_status()
        data = r.json()
        choice = (data.get("choices") or [{}])[0]
        return (choice.get("message") or {}).get("content", "").strip()
    except Exception as e:
        return f"(API error: {e})"


def run_turn(memory, user_message: str, use_llm: bool = True) -> str:
    """One turn: gather context, optionally call LLM, store conversation."""
    context = gather_context(memory)
    system = "You are a helpful assistant. Use the following stored context when relevant."
    prompt = f"{system}\n\nContext:\n{context}\n\nUser: {user_message}\n\nAssistant:"

    if not use_llm:
        # Memory-only: echo what we recalled
        if context:
            return f"Recalled context:\n{context}"
        return "No memories found for that query. Use remember key value to store."

    if os.environ.get("MINIMAX_API_KEY"):
        reply = call_minimax(prompt)
    elif os.environ.get("OPENAI_API_BASE") and os.environ.get("OPENAI_API_KEY"):
        reply = call_openai_compatible(prompt)
    else:
        reply = call_ollama(prompt)

    # Store conversation
    key = f"conversation:{int(time.time())}"
    memory.remember(key, {"user": user_message, "assistant": reply})

    # Optional: "remember X" → store as fact
    if "remember" in user_message.lower():
        m = re.search(r"remember\s+(.+?)(?:\.|$)", user_message, re.I | re.S)
        if m:
            fact_val = m.group(1).strip()
            memory.remember("fact:user_stated", fact_val)

    return reply


def main():
    lattice = os.environ.get("SYNRIX_LATTICE", os.path.expanduser("~/.synrix/agent_memory.lattice"))
    memory = get_agent_memory(lattice_path=lattice)
    synrix_only = os.environ.get("SYNRIX_ONLY", "").lower() in ("1", "true", "yes")

    if len(sys.argv) > 1:
        query = " ".join(sys.argv[1:])
        print(run_turn(memory, query, use_llm=not synrix_only))
        return

    print("Practical Assistant (Synrix memory + LLM). Commands: recall <prefix>, or ask anything. Ctrl+D / quit to exit.")
    print("Memory backend:", memory.status())
    while True:
        try:
            line = input("You: ").strip()
        except EOFError:
            break
        if not line:
            continue
        if line.lower() in ("quit", "exit", "q"):
            break
        if line.lower().startswith("recall "):
            prefix = line[7:].strip() or ""
            items = memory.search(prefix, limit=20)
            if not items:
                print("(no memories)")
            else:
                for it in items:
                    print(" ", it.get("key"), "->", it.get("value"))
            continue
        print("Assistant:", run_turn(memory, line, use_llm=not synrix_only))


if __name__ == "__main__":
    main()
