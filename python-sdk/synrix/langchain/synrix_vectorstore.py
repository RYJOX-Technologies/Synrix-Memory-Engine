"""
SynrixVectorStore - native LangChain VectorStore adapter for SYNRIX.

Uses a local Synrix engine process for vector operations and a local
metadata store for document text/metadata.
"""

from __future__ import annotations

import json
import uuid
from typing import Any, Dict, Iterable, List, Optional, Tuple

from langchain_core.documents import Document
from langchain_core.embeddings import Embeddings
from langchain_core.vectorstores import VectorStore

from synrix.client import SynrixClient

try:
    from synrix.direct_client import SynrixDirectClient
    DIRECT_AVAILABLE = True
except Exception:
    SynrixDirectClient = None
    DIRECT_AVAILABLE = False


class SynrixVectorStore(VectorStore):
    """
    LangChain VectorStore backed by a local Synrix engine process.

    - Vector ops go through SynrixClient (local HTTP).
    - Document text/metadata stored locally via Synrix memory nodes.
    """

    def __init__(
        self,
        embedding: Embeddings,
        collection_name: str = "langchain_vectors",
        client: Optional[SynrixClient] = None,
        host: str = "localhost",
        port: int = 6334,
        distance: str = "Cosine",
        vector_dim: Optional[int] = None,
        metadata_collection: str = "langchain_meta",
        use_direct: bool = True,
    ):
        self.embedding = embedding
        self.collection_name = collection_name
        self.client = client or SynrixClient(host=host, port=port)
        self.distance = distance
        self.vector_dim = vector_dim
        self.metadata_collection = metadata_collection
        self._collection_ready = False

        if use_direct and DIRECT_AVAILABLE:
            try:
                self.meta_client = SynrixDirectClient()
            except Exception:
                self.meta_client = SynrixClient(host=host, port=port)
        else:
            self.meta_client = SynrixClient(host=host, port=port)

    @property
    def embeddings(self) -> Embeddings:
        return self.embedding

    def _ensure_collection(self) -> None:
        if self._collection_ready:
            return
        try:
            self.client.get_collection(self.collection_name)
        except Exception:
            self.client.create_collection(
                self.collection_name,
                vector_dim=self.vector_dim,
                distance=self.distance,
            )
        self._collection_ready = True

    def _ensure_meta_collection(self) -> None:
        if isinstance(self.meta_client, SynrixClient):
            try:
                self.meta_client.get_collection(self.metadata_collection)
            except Exception:
                self.meta_client.create_collection(self.metadata_collection)

    def _new_id(self) -> int:
        return uuid.uuid4().int & ((1 << 63) - 1)

    def _store_meta(self, point_id: int, text: str, metadata: Optional[Dict[str, Any]]) -> None:
        self._ensure_meta_collection()
        payload = {"text": text, "metadata": metadata or {}}
        data = json.dumps(payload, separators=(",", ":"))
        key = f"VECTOR_DOC:{self.collection_name}:{point_id}"
        try:
            self.meta_client.add_node(key, data, collection=self.metadata_collection)
        except Exception:
            # Best-effort metadata; search can still return IDs
            pass

    def _load_meta(self, point_id: int) -> Optional[Document]:
        key = f"VECTOR_DOC:{self.collection_name}:{point_id}"
        try:
            results = self.meta_client.query_prefix(
                key,
                collection=self.metadata_collection,
                limit=1,
            )
        except Exception:
            return None
        if not results:
            return None
        payload = results[0].get("payload", {})
        data = payload.get("data", "")
        try:
            decoded = json.loads(data)
        except json.JSONDecodeError:
            decoded = {"text": data, "metadata": {}}
        return Document(page_content=decoded.get("text", ""), metadata=decoded.get("metadata", {}))

    @classmethod
    def from_texts(
        cls,
        texts: List[str],
        embedding: Embeddings,
        metadatas: Optional[List[Dict[str, Any]]] = None,
        ids: Optional[List[str]] = None,
        **kwargs: Any,
    ) -> "SynrixVectorStore":
        store = cls(embedding=embedding, **kwargs)
        store.add_texts(texts=texts, metadatas=metadatas, ids=ids)
        return store

    @classmethod
    def from_documents(
        cls,
        documents: List[Document],
        embedding: Embeddings,
        ids: Optional[List[str]] = None,
        **kwargs: Any,
    ) -> "SynrixVectorStore":
        texts = [doc.page_content for doc in documents]
        metadatas = [doc.metadata for doc in documents]
        return cls.from_texts(texts, embedding, metadatas=metadatas, ids=ids, **kwargs)

    def add_texts(
        self,
        texts: Iterable[str],
        metadatas: Optional[List[Dict[str, Any]]] = None,
        ids: Optional[List[str]] = None,
        **kwargs: Any,
    ) -> List[str]:
        self._ensure_collection()
        text_list = list(texts)
        metadata_list = metadatas or [{} for _ in text_list]

        vectors = self.embedding.embed_documents(text_list)
        if self.vector_dim is None and vectors:
            self.vector_dim = len(vectors[0])

        point_ids: List[int] = []
        for idx in range(len(text_list)):
            if ids and idx < len(ids):
                point_id = int(ids[idx])
            else:
                point_id = self._new_id()
            point_ids.append(point_id)

        points: List[Dict[str, Any]] = []
        for point_id, vector in zip(point_ids, vectors):
            points.append({"id": point_id, "vector": vector})

        self.client.upsert_points(self.collection_name, points)

        for point_id, text, metadata in zip(point_ids, text_list, metadata_list):
            self._store_meta(point_id, text, metadata)

        return [str(pid) for pid in point_ids]

    def similarity_search_with_score(
        self,
        query: str,
        k: int = 4,
        **kwargs: Any,
    ) -> List[Tuple[Document, float]]:
        self._ensure_collection()
        query_vector = self.embedding.embed_query(query)
        results = self.client.search_points(
            self.collection_name,
            vector=query_vector,
            limit=k,
            score_threshold=kwargs.get("score_threshold"),
        )
        docs_with_scores: List[Tuple[Document, float]] = []
        for result in results:
            point_id = result.get("id")
            score = result.get("score", 0.0)
            doc = self._load_meta(point_id)
            if doc is None:
                doc = Document(page_content="", metadata={"id": point_id})
            docs_with_scores.append((doc, score))
        return docs_with_scores

    def similarity_search(
        self,
        query: str,
        k: int = 4,
        **kwargs: Any,
    ) -> List[Document]:
        return [doc for doc, _ in self.similarity_search_with_score(query, k=k, **kwargs)]

    def delete(self, ids: Optional[List[str]] = None, **kwargs: Any) -> bool:
        # Point deletion is not currently exposed by SynrixClient.
        return False
