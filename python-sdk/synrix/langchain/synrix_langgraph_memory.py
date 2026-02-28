"""
SynrixLangGraphMemory - thin LangGraph memory node using Synrix prefix retriever.
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional, TypedDict

from langchain_core.documents import Document

try:
    from langgraph.graph import StateGraph
    from langgraph.graph.message import add_messages
    LANGGRAPH_AVAILABLE = True
except ImportError:
    StateGraph = None
    add_messages = None
    LANGGRAPH_AVAILABLE = False

from synrix.agent_memory import SynrixMemory
from synrix.langchain.synrix_prefix_retriever import SynrixPrefixRetriever


class MemoryState(TypedDict):
    """State for LangGraph memory node."""
    messages: List[Any]
    memory_context: Optional[str]


class SynrixLangGraphMemory:
    """
    Thin LangGraph memory node that uses Synrix for deterministic prefix recall.
    
    Provides:
    - read_memory: Retrieve relevant context by prefix
    - write_memory: Store agent memory entries
    """

    def __init__(
        self,
        prefix: str = "TASK:",
        collection: str = "agent_memory",
        limit: int = 20,
        host: str = "localhost",
        port: int = 6334,
        use_direct: bool = True,
    ):
        self.retriever = SynrixPrefixRetriever(
            prefix=prefix,
            collection=collection,
            limit=limit,
            host=host,
            port=port,
            use_direct=use_direct,
        )
        self.memory = SynrixMemory(
            collection=collection,
            use_direct=use_direct,
        )

    def read_memory(self, query: Optional[str] = None) -> List[Document]:
        """
        Read memory entries matching the prefix.
        
        Args:
            query: Optional query string (uses prefix if None)
            
        Returns:
            List of Document objects with memory content
        """
        return self.retriever.invoke(query or self.retriever.prefix)

    def write_memory(
        self,
        key: str,
        value: str,
        metadata: Optional[Dict[str, Any]] = None,
    ) -> Optional[int]:
        """
        Write a memory entry.
        
        Args:
            key: Memory key (e.g., "TASK:build_demo")
            value: Memory value
            metadata: Optional metadata
            
        Returns:
            Node ID if successful, None otherwise
        """
        try:
            return self.memory.write(key, value, metadata=metadata)
        except Exception:
            return None

    def memory_node(self, state: MemoryState) -> MemoryState:
        """
        LangGraph node function that reads memory and adds context to state.
        
        Args:
            state: Current graph state
            
        Returns:
            Updated state with memory_context
        """
        docs = self.read_memory()
        context_parts = [doc.page_content for doc in docs]
        memory_context = "\n".join(context_parts) if context_parts else None
        
        return {
            **state,
            "memory_context": memory_context,
        }
