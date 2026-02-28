"""
SynrixPrefixRetriever - deterministic prefix-based retriever for LangChain.
"""

from __future__ import annotations

from typing import Any, Dict, List, Optional

from langchain_core.documents import Document
from langchain_core.retrievers import BaseRetriever
from pydantic import ConfigDict

from synrix.client import SynrixClient

try:
    from synrix.direct_client import SynrixDirectClient
    DIRECT_AVAILABLE = True
except Exception:
    SynrixDirectClient = None
    DIRECT_AVAILABLE = False


class SynrixPrefixRetriever(BaseRetriever):
    """
    Deterministic prefix-based retriever backed by a local Synrix engine process.
    """

    model_config = ConfigDict(arbitrary_types_allowed=True)

    prefix: str
    collection: str = "agent_memory"
    limit: int = 20
    client: Any

    def __init__(
        self,
        prefix: str,
        collection: str = "agent_memory",
        limit: int = 20,
        client: Optional[Any] = None,
        host: str = "localhost",
        port: int = 6334,
        use_direct: bool = True,
    ):
        if client is not None:
            resolved_client = client
        elif use_direct and DIRECT_AVAILABLE:
            try:
                resolved_client = SynrixDirectClient()
            except Exception:
                resolved_client = SynrixClient(host=host, port=port)
        else:
            resolved_client = SynrixClient(host=host, port=port)

        super().__init__(
            prefix=prefix,
            collection=collection,
            limit=limit,
            client=resolved_client,
        )

    def _get_relevant_documents(self, query: str) -> List[Document]:
        prefix = query or self.prefix
        results = self.client.query_prefix(prefix, collection=self.collection, limit=self.limit)
        documents: List[Document] = []
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "")
            data = payload.get("data", "")
            documents.append(Document(page_content=data, metadata={"key": name}))
        return documents
