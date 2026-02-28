#!/usr/bin/env python3
"""
Synrix Demo: Local RAG Without Embeddings
==========================================
Retrieves relevant documents using prefix-semantic naming.
No embedding model. No vector DB. No API keys. No server.

Run:
  pip install synrix
  python3 hello_memory.py
"""

import sys, os, time, tempfile, shutil
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from synrix.raw_backend import RawSynrixBackend

DOCS = [
    ("DOC_PYTHON_ASYNCIO_1",   "asyncio is Python's built-in library for writing concurrent code using async/await syntax. It provides event loops, coroutines, and tasks for non-blocking I/O."),
    ("DOC_PYTHON_ASYNCIO_2",   "To run an async function: asyncio.run(main()). Use 'await' inside async functions to yield control. asyncio.gather() runs multiple coroutines concurrently."),
    ("DOC_PYTHON_TYPING_1",    "Python type hints use the typing module. Common types: List[int], Dict[str, Any], Optional[str], Tuple[int, ...]. Use mypy for static checking."),
    ("DOC_PYTHON_DATACLASS_1", "dataclasses provide a decorator to auto-generate __init__, __repr__, __eq__. Use @dataclass on a class with annotated fields. Supports default values and field()."),
    ("DOC_RUST_OWNERSHIP_1",   "Rust's ownership model ensures memory safety without garbage collection. Each value has one owner. When the owner goes out of scope, the value is dropped."),
    ("DOC_RUST_BORROWING_1",   "Borrowing lets you reference data without taking ownership. Immutable borrows (&T) allow multiple readers. Mutable borrows (&mut T) allow one writer, no readers."),
    ("DOC_RUST_LIFETIMES_1",   "Lifetimes annotate how long references are valid. Written as 'a. The borrow checker ensures references don't outlive their referents. Prevents dangling pointers."),
    ("DOC_K8S_PODS_1",         "A Kubernetes Pod is the smallest deployable unit. It contains one or more containers sharing network and storage. Pods are ephemeral; use Deployments for persistence."),
    ("DOC_K8S_SERVICES_1",     "A Kubernetes Service exposes Pods to the network. Types: ClusterIP (internal), NodePort (external port), LoadBalancer (cloud LB). Services use label selectors."),
    ("DOC_K8S_DEPLOY_1",       "A Deployment manages ReplicaSets and provides declarative updates. spec.replicas controls scaling. Rolling updates replace pods gradually with zero downtime."),
    ("DOC_DB_POSTGRES_1",      "PostgreSQL is an open-source relational database. Supports JSONB, full-text search, CTEs, window functions, and extensions like PostGIS and pg_trgm."),
    ("DOC_DB_REDIS_1",         "Redis is an in-memory data store used as cache, message broker, and database. Supports strings, lists, sets, hashes, sorted sets. Persistence via RDB and AOF."),
    ("DOC_NET_HTTP_1",         "HTTP/2 uses binary framing, multiplexing (multiple streams over one connection), header compression (HPACK), and server push. Reduces latency vs HTTP/1.1."),
    ("DOC_NET_GRPC_1",         "gRPC uses Protocol Buffers for serialization and HTTP/2 for transport. Supports unary, server streaming, client streaming, and bidirectional streaming RPCs."),
    ("DOC_SEC_TLS_1",          "TLS 1.3 reduces handshake to 1-RTT (0-RTT with resumption). Removes insecure ciphers. Uses AEAD (AES-GCM, ChaCha20-Poly1305). Certificate-based authentication."),
    ("DOC_SEC_OAUTH_1",        "OAuth 2.0 provides delegated authorization. Flows: Authorization Code (web apps), PKCE (mobile/SPA), Client Credentials (machine-to-machine). Never expose client secrets."),
    ("DOC_ML_TRANSFORMER_1",   "Transformers use self-attention to process sequences in parallel. Key components: multi-head attention, positional encoding, feed-forward layers, layer normalization."),
    ("DOC_ML_FINETUNE_1",      "Fine-tuning adapts a pre-trained model to a specific task. Techniques: full fine-tuning, LoRA (low-rank adaptation), QLoRA (quantized LoRA), prefix tuning."),
    ("DOC_FRONT_REACT_1",      "React uses a virtual DOM for efficient updates. Components are functions returning JSX. useState for state, useEffect for side effects, useContext for shared state."),
    ("DOC_DEVOPS_DOCKER_1",    "Docker containers package applications with dependencies. Dockerfile defines the image. docker-compose orchestrates multi-container apps. Use multi-stage builds for smaller images."),
]

KEYWORD_TO_PREFIX = {
    "async": "DOC_PYTHON_ASYNCIO",  "await": "DOC_PYTHON_ASYNCIO",  "asyncio": "DOC_PYTHON_ASYNCIO",
    "type": "DOC_PYTHON_TYPING",    "typing": "DOC_PYTHON_TYPING",  "hints": "DOC_PYTHON_TYPING",
    "dataclass": "DOC_PYTHON_DATACLASS", "class": "DOC_PYTHON_DATACLASS",
    "python": "DOC_PYTHON_",
    "ownership": "DOC_RUST_OWNERSHIP", "borrow": "DOC_RUST_BORROWING", "lifetime": "DOC_RUST_LIFETIMES",
    "rust": "DOC_RUST_",
    "pod": "DOC_K8S_PODS",     "service": "DOC_K8S_SERVICES", "deploy": "DOC_K8S_DEPLOY",
    "kubernetes": "DOC_K8S_",  "k8s": "DOC_K8S_",
    "postgres": "DOC_DB_POSTGRES", "sql": "DOC_DB_POSTGRES", "redis": "DOC_DB_REDIS",
    "database": "DOC_DB_",     "db": "DOC_DB_",
    "http": "DOC_NET_HTTP",    "grpc": "DOC_NET_GRPC",        "network": "DOC_NET_",
    "tls": "DOC_SEC_TLS",      "oauth": "DOC_SEC_OAUTH",      "security": "DOC_SEC_",
    "transformer": "DOC_ML_TRANSFORMER", "finetune": "DOC_ML_FINETUNE", "lora": "DOC_ML_FINETUNE",
    "ml": "DOC_ML_",           "ai": "DOC_ML_",
    "react": "DOC_FRONT_REACT", "frontend": "DOC_FRONT_",
    "docker": "DOC_DEVOPS_DOCKER", "container": "DOC_DEVOPS_DOCKER", "devops": "DOC_DEVOPS_",
}


def query_to_prefixes(query):
    words = query.lower().replace("?", "").replace(",", "").split()
    prefixes = []
    for w in words:
        if w in KEYWORD_TO_PREFIX:
            p = KEYWORD_TO_PREFIX[w]
            if p not in prefixes:
                prefixes.append(p)
    return prefixes if prefixes else ["DOC_"]


def main():
    print("Synrix — Local RAG Without Embeddings")
    print("=" * 50)
    print()

    tmpdir = tempfile.mkdtemp()
    lattice_path = os.path.join(tmpdir, "rag_demo.lattice")

    try:
        backend = RawSynrixBackend(lattice_path, max_nodes=26000)

        print(f"Ingesting {len(DOCS)} documents ...")
        t0 = time.perf_counter()
        for name, data in DOCS:
            backend.add_node(name, data)
        ingest_ms = (time.perf_counter() - t0) * 1000
        print(f"  Done in {ingest_ms:.2f} ms ({ingest_ms/len(DOCS):.2f} ms/doc)\n")

        queries = [
            "How does async work in Python?",
            "Explain Rust ownership and borrowing",
            "What is a Kubernetes pod?",
            "How do I use Redis as a cache?",
            "What is OAuth 2.0?",
            "How do transformers work in ML?",
        ]

        for q in queries:
            prefixes = query_to_prefixes(q)
            print(f"Q: {q}")
            print(f"   Prefixes: {prefixes}")

            all_results = []
            t0 = time.perf_counter()
            for prefix in prefixes:
                results = backend.find_by_prefix(prefix, limit=10, raw=False)
                all_results.extend(results)
            elapsed_us = (time.perf_counter() - t0) * 1e6

            if all_results:
                for r in all_results[:3]:
                    name = r.get("name", "?")
                    data = r.get("data", "")[:80]
                    print(f"   -> {name}: {data}...")
                print(f"   Retrieved {len(all_results)} docs in {elapsed_us:.1f} us"
                      f" — no embeddings, no API calls.")
            else:
                print(f"   (no matches)")
            print()

        print("-" * 50)
        print("Key takeaway: prefix-semantic retrieval is instant,")
        print("deterministic, and requires zero embedding infrastructure.")

    finally:
        backend.close()
        shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == "__main__":
    main()
