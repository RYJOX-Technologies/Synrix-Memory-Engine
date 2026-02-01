#!/usr/bin/env python3
"""
SYNRIX Repeat Performance Test
==============================

Tests performance when embeddings are pre-computed (realistic use case):
- Pre-compute embedding once
- Run many searches with same embedding
- Measures pure search overhead (no embedding generation)
"""

import sys
import os
import time
import statistics
from pathlib import Path

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

nvme_env = "/mnt/nvme/aion-omega/python-env"
if os.path.exists(nvme_env):
    sys.path.insert(0, os.path.join(nvme_env, "lib", "python3.10", "site-packages"))

from mock_customer_demo import start_synrix_server, check_server_running
from qdrant_client import QdrantClient
from langchain_community.vectorstores import Qdrant
from sentence_transformers import SentenceTransformer
from langchain_core.embeddings import Embeddings

class SentenceTransformerEmbeddings(Embeddings):
    def __init__(self, model_name='all-MiniLM-L6-v2'):
        super().__init__()
        self.model = SentenceTransformer(model_name)
        self.embedding_dimension = self.model.get_sentence_embedding_dimension()
    
    def embed_documents(self, texts):
        if not texts:
            return []
        return self.model.encode(texts, convert_to_numpy=False, normalize_embeddings=True)
    
    def embed_query(self, text):
        if not text:
            return []
        embeddings = self.model.encode([text], convert_to_numpy=False, normalize_embeddings=True)
        return embeddings[0] if embeddings else []


def time_ns():
    """Get current time in nanoseconds"""
    return time.perf_counter_ns()


def test_with_precomputed_embedding(vectorstore, query_embedding, iterations=1000):
    """Test search performance with pre-computed embedding (bypass embedding generation)"""
    print(f"\n🔍 Testing Search with Pre-computed Embedding - {iterations} iterations...")
    print("   (No embedding generation overhead)")
    
    times = []
    
    for i in range(iterations):
        # Use the vectorstore's internal search with pre-computed embedding
        # This bypasses embedding generation
        start = time_ns()
        
        # Direct search using qdrant_client with pre-computed vector
        from qdrant_client.models import SearchRequest
        search_req = SearchRequest(
            vector=query_embedding,
            limit=2,
            with_payload=True
        )
        
        # Get the client from vectorstore
        client = vectorstore.client
        collection_name = vectorstore.collection_name
        
        result = client.search(
            collection_name=collection_name,
            query_vector=query_embedding,
            limit=2,
            with_payload=True
        )
        
        elapsed = time_ns() - start
        times.append(elapsed / 1_000_000)  # Convert to ms
        
        if (i + 1) % 200 == 0:
            print(f"   Completed {i + 1}/{iterations}...")
    
    if not times:
        return {}
    
    sorted_times = sorted(times)
    n = len(sorted_times)
    
    return {
        'mean_ms': sum(times) / len(times),
        'min_ms': min(times),
        'max_ms': max(times),
        'p50_ms': sorted_times[n // 2],
        'p95_ms': sorted_times[int(n * 0.95)] if n > 1 else sorted_times[-1],
        'p99_ms': sorted_times[int(n * 0.99)] if n > 1 else sorted_times[-1],
        'count': len(times)
    }


def test_with_langchain_overhead(vectorstore, query, iterations=1000):
    """Test with LangChain (includes embedding generation on each call)"""
    print(f"\n🐍 Testing with LangChain (includes embedding) - {iterations} iterations...")
    print("   (This is what the benchmark was measuring)")
    
    times = []
    
    for i in range(iterations):
        start = time_ns()
        results = vectorstore.similarity_search(query, k=2)
        elapsed = time_ns() - start
        times.append(elapsed / 1_000_000)  # Convert to ms
        
        if (i + 1) % 200 == 0:
            print(f"   Completed {i + 1}/{iterations}...")
    
    if not times:
        return {}
    
    sorted_times = sorted(times)
    n = len(sorted_times)
    
    return {
        'mean_ms': sum(times) / len(times),
        'min_ms': min(times),
        'max_ms': max(times),
        'p50_ms': sorted_times[n // 2],
        'p95_ms': sorted_times[int(n * 0.95)] if n > 1 else sorted_times[-1],
        'p99_ms': sorted_times[int(n * 0.99)] if n > 1 else sorted_times[-1],
        'count': len(times)
    }


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX Repeat Performance Test (Pre-computed Embeddings)      ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    
    # Start server
    print("\n🚀 Starting SYNRIX server...")
    synrix_process = start_synrix_server(6334)
    if synrix_process is None and not check_server_running(6334):
        print("❌ Failed to start SYNRIX server")
        return
    print("✅ SYNRIX server ready")
    
    try:
        # Setup
        print("\n🔧 Setting up...")
        embeddings = SentenceTransformerEmbeddings('all-MiniLM-L6-v2')
        
        qdrant_client = QdrantClient(url='http://localhost:6334', timeout=120)
        
        # Ensure collection exists with data
        knowledge_base = [
            "Acme Pro is our enterprise product with advanced features including multi-user support, API access, and priority support.",
            "Acme Basic is our starter plan with core features for individual users. It includes 10GB storage and email support.",
            "To reset a user password, go to Settings > Users > Select User > Reset Password. An email will be sent with reset instructions.",
            "API rate limits are 1000 requests per hour for Basic plans and 10000 requests per hour for Pro plans. Limits reset every hour.",
            "Two-factor authentication can be enabled in Settings > Security > Enable 2FA. You'll need a mobile authenticator app.",
            "Export functionality requires Pro plan. Go to Data > Export to download your data in CSV or JSON format.",
            "To upgrade your plan, go to Settings > Billing > Upgrade Plan. Changes take effect immediately and are prorated.",
        ]
        
        try:
            collection = qdrant_client.get_collection('support_knowledge_base')
            if collection.points_count == 0:
                print("   Collection empty, indexing data...")
                vectorstore = Qdrant(
                    client=qdrant_client,
                    collection_name='support_knowledge_base',
                    embeddings=embeddings
                )
                vectorstore.add_texts(texts=knowledge_base, batch_size=3)
                print(f"   ✅ Indexed {len(knowledge_base)} documents")
            else:
                print(f"   Collection has {collection.points_count} points")
                vectorstore = Qdrant(
                    client=qdrant_client,
                    collection_name='support_knowledge_base',
                    embeddings=embeddings
                )
        except:
            print("   Creating collection and indexing data...")
            qdrant_client.create_collection(
                collection_name='support_knowledge_base',
                vectors_config={'size': embeddings.embedding_dimension, 'distance': 'Cosine'}
            )
            vectorstore = Qdrant(
                client=qdrant_client,
                collection_name='support_knowledge_base',
                embeddings=embeddings
            )
            vectorstore.add_texts(texts=knowledge_base, batch_size=3)
            print(f"   ✅ Indexed {len(knowledge_base)} documents")
        
        # Pre-compute embedding once
        test_query = "How do I reset a user password?"
        print(f"\n📝 Test query: \"{test_query}\"")
        print("   Generating embedding once (this is the one-time cost)...")
        
        embedding_start = time_ns()
        query_embedding = embeddings.embed_query(test_query)
        embedding_time = (time_ns() - embedding_start) / 1_000_000
        
        # Convert to list if needed
        if hasattr(query_embedding, 'tolist'):
            query_embedding = query_embedding.tolist()
        elif not isinstance(query_embedding, list):
            query_embedding = list(query_embedding)
        
        print(f"   ✅ Embedding generated in {embedding_time:.2f}ms (one-time cost)")
        print(f"   Embedding dimension: {len(query_embedding)}")
        
        # Test 1: Pre-computed embedding (realistic repeat performance)
        iterations = 1000
        print(f"\n{'='*70}")
        print("TEST 1: Repeat Performance (Pre-computed Embedding)")
        print(f"{'='*70}")
        print("This simulates realistic use: embed once, search many times")
        
        precomputed_results = test_with_precomputed_embedding(vectorstore, query_embedding, iterations)
        
        # Test 2: LangChain with embedding (what benchmark was measuring)
        print(f"\n{'='*70}")
        print("TEST 2: LangChain Performance (Includes Embedding)")
        print(f"{'='*70}")
        print("This is what the benchmark was measuring (embedding on every call)")
        
        langchain_results = test_with_langchain_overhead(vectorstore, test_query, iterations)
        
        # Print comparison
        print(f"\n{'='*70}")
        print("PERFORMANCE COMPARISON")
        print(f"{'='*70}")
        
        if precomputed_results:
            print(f"\n✅ Pre-computed Embedding (Realistic Repeat Performance):")
            print(f"   Mean: {precomputed_results['mean_ms']:.3f}ms")
            print(f"   p50:  {precomputed_results['p50_ms']:.3f}ms")
            print(f"   p95:  {precomputed_results['p95_ms']:.3f}ms")
            print(f"   p99:  {precomputed_results['p99_ms']:.3f}ms")
            print(f"   Min:  {precomputed_results['min_ms']:.3f}ms")
            print(f"   Max:  {precomputed_results['max_ms']:.3f}ms")
            print(f"   Throughput: {1000 / precomputed_results['mean_ms']:.1f} queries/sec")
        
        if langchain_results:
            print(f"\n🐍 LangChain (With Embedding on Each Call):")
            print(f"   Mean: {langchain_results['mean_ms']:.3f}ms")
            print(f"   p50:  {langchain_results['p50_ms']:.3f}ms")
            print(f"   p95:  {langchain_results['p95_ms']:.3f}ms")
            print(f"   p99:  {langchain_results['p99_ms']:.3f}ms")
            print(f"   Min:  {langchain_results['min_ms']:.3f}ms")
            print(f"   Max:  {langchain_results['max_ms']:.3f}ms")
            print(f"   Throughput: {1000 / langchain_results['mean_ms']:.1f} queries/sec")
        
        if precomputed_results and langchain_results:
            overhead = langchain_results['mean_ms'] - precomputed_results['mean_ms']
            speedup = langchain_results['mean_ms'] / precomputed_results['mean_ms']
            print(f"\n📊 Overhead Analysis:")
            print(f"   Embedding overhead: {overhead:.3f}ms per query")
            print(f"   Speedup (pre-computed vs with embedding): {speedup:.2f}×")
            print(f"\n💡 Key Insight:")
            print(f"   • One-time embedding cost: {embedding_time:.2f}ms")
            print(f"   • Repeat search overhead: {precomputed_results['mean_ms']:.3f}ms")
            print(f"   • Python orchestration overhead: ~{precomputed_results['mean_ms'] - 1.86:.3f}ms")
            print(f"     (Total - HTTP/C server time from previous test)")
        
    except Exception as e:
        print(f"\n❌ Test failed: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if synrix_process:
            print("\n🛑 Stopping SYNRIX server...")
            try:
                synrix_process.terminate()
                synrix_process.wait(timeout=5)
            except:
                try:
                    synrix_process.kill()
                except:
                    pass
            if hasattr(synrix_process, '_log_file'):
                try:
                    synrix_process._log_file.close()
                except:
                    pass
            print("✅ SYNRIX server stopped")


if __name__ == "__main__":
    main()
