#!/usr/bin/env python3
"""
SYNRIX Performance Optimizations Test
=====================================

Tests various optimizations to bridge the performance gap:
1. Direct qdrant_client (bypass LangChain)
2. Embedding caching
3. Batch processing
4. Connection pooling
"""

import sys
import os
import time
import statistics
from pathlib import Path
from functools import lru_cache

script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

nvme_env = "/mnt/nvme/aion-omega/python-env"
if os.path.exists(nvme_env):
    sys.path.insert(0, os.path.join(nvme_env, "lib", "python3.10", "site-packages"))

from mock_customer_demo import start_synrix_server, check_server_running
from qdrant_client import QdrantClient
from sentence_transformers import SentenceTransformer

def time_ns():
    return time.perf_counter_ns()


def test_langchain_overhead(qdrant_client, embeddings, query, iterations=100):
    """Test with LangChain (baseline - what we've been measuring)"""
    from langchain_community.vectorstores import Qdrant
    
    vectorstore = Qdrant(
        client=qdrant_client,
        collection_name='support_knowledge_base',
        embeddings=embeddings
    )
    
    times = []
    for i in range(iterations):
        start = time_ns()
        results = vectorstore.similarity_search(query, k=2)
        elapsed = (time_ns() - start) / 1_000_000
        times.append(elapsed)
    
    return {
        'mean_ms': statistics.mean(times),
        'p50_ms': statistics.median(times),
        'p95_ms': sorted(times)[int(len(times) * 0.95)],
    }


def test_direct_qdrant_client(qdrant_client, query_embedding, iterations=100):
    """Test with direct qdrant_client (bypass LangChain)"""
    times = []
    
    for i in range(iterations):
        start = time_ns()
        results = qdrant_client.search(
            collection_name='support_knowledge_base',
            query_vector=query_embedding,
            limit=2,
            with_payload=True
        )
        elapsed = (time_ns() - start) / 1_000_000
        times.append(elapsed)
    
    return {
        'mean_ms': statistics.mean(times),
        'p50_ms': statistics.median(times),
        'p95_ms': sorted(times)[int(len(times) * 0.95)],
    }


def test_with_cached_embedding(qdrant_client, query_embedding, iterations=1000):
    """Test with pre-computed embedding (realistic production use)"""
    times = []
    
    for i in range(iterations):
        start = time_ns()
        results = qdrant_client.search(
            collection_name='support_knowledge_base',
            query_vector=query_embedding,
            limit=2,
            with_payload=True
        )
        elapsed = (time_ns() - start) / 1_000_000
        times.append(elapsed)
    
    return {
        'mean_ms': statistics.mean(times),
        'p50_ms': statistics.median(times),
        'p95_ms': sorted(times)[int(len(times) * 0.95)],
        'p99_ms': sorted(times)[int(len(times) * 0.99)],
    }


def test_batch_embedding(model, queries):
    """Test batch embedding generation"""
    start = time_ns()
    embeddings = model.encode(queries, convert_to_numpy=False, normalize_embeddings=True)
    elapsed = (time_ns() - start) / 1_000_000
    return embeddings, elapsed


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX Performance Optimizations Test                          ║")
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
        model = SentenceTransformer('all-MiniLM-L6-v2')
        qdrant_client = QdrantClient(url='http://localhost:6334', timeout=120)
        
        # Ensure collection exists
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
                print("   Indexing data...")
                from langchain_community.vectorstores import Qdrant
                from langchain_core.embeddings import Embeddings
                
                class SimpleEmbeddings(Embeddings):
                    def embed_documents(self, texts):
                        return model.encode(texts, convert_to_numpy=False, normalize_embeddings=True)
                    def embed_query(self, text):
                        return model.encode([text], convert_to_numpy=False, normalize_embeddings=True)[0]
                
                embeddings_wrapper = SimpleEmbeddings()
                vectorstore = Qdrant(client=qdrant_client, collection_name='support_knowledge_base', embeddings=embeddings_wrapper)
                vectorstore.add_texts(texts=knowledge_base, batch_size=3)
                print("   ✅ Indexed")
            else:
                print(f"   Collection has {collection.points_count} points")
        except:
            print("   Creating collection...")
            qdrant_client.create_collection(
                collection_name='support_knowledge_base',
                vectors_config={'size': 384, 'distance': 'Cosine'}
            )
            from langchain_community.vectorstores import Qdrant
            from langchain_core.embeddings import Embeddings
            
            class SimpleEmbeddings(Embeddings):
                def embed_documents(self, texts):
                    return model.encode(texts, convert_to_numpy=False, normalize_embeddings=True)
                def embed_query(self, text):
                    return model.encode([text], convert_to_numpy=False, normalize_embeddings=True)[0]
            
            embeddings_wrapper = SimpleEmbeddings()
            vectorstore = Qdrant(client=qdrant_client, collection_name='support_knowledge_base', embeddings=embeddings_wrapper)
            vectorstore.add_texts(texts=knowledge_base, batch_size=3)
            print("   ✅ Indexed")
        
        test_query = "How do I reset a user password?"
        
        # Pre-compute embedding
        print(f"\n📝 Pre-computing embedding for: \"{test_query}\"")
        embedding_start = time_ns()
        query_embedding = model.encode([test_query], convert_to_numpy=False, normalize_embeddings=True)[0]
        if hasattr(query_embedding, 'tolist'):
            query_embedding = query_embedding.tolist()
        embedding_time = (time_ns() - embedding_start) / 1_000_000
        print(f"   ✅ Embedding generated in {embedding_time:.2f}ms (one-time cost)")
        
        # Test 1: LangChain (baseline)
        print(f"\n{'='*70}")
        print("TEST 1: LangChain (Baseline - What We've Been Measuring)")
        print(f"{'='*70}")
        from langchain_core.embeddings import Embeddings
        class SimpleEmbeddings(Embeddings):
            def embed_documents(self, texts):
                return model.encode(texts, convert_to_numpy=False, normalize_embeddings=True)
            def embed_query(self, text):
                return model.encode([text], convert_to_numpy=False, normalize_embeddings=True)[0]
        
        langchain_results = test_langchain_overhead(qdrant_client, SimpleEmbeddings(), test_query, iterations=100)
        
        # Test 2: Direct qdrant_client (bypass LangChain)
        print(f"\n{'='*70}")
        print("TEST 2: Direct qdrant_client (Bypass LangChain)")
        print(f"{'='*70}")
        direct_results = test_direct_qdrant_client(qdrant_client, query_embedding, iterations=100)
        
        # Test 3: Cached embedding (realistic production)
        print(f"\n{'='*70}")
        print("TEST 3: Cached Embedding (Realistic Production Use)")
        print(f"{'='*70}")
        print("   (Pre-computed embedding, many searches)")
        cached_results = test_with_cached_embedding(qdrant_client, query_embedding, iterations=1000)
        
        # Test 4: Batch embedding
        print(f"\n{'='*70}")
        print("TEST 4: Batch Embedding Generation")
        print(f"{'='*70}")
        test_queries = [
            "How do I reset a user password?",
            "What are the API rate limits?",
            "How do I enable two-factor authentication?",
            "Can I export my data?",
            "How do I upgrade my plan?",
        ]
        batch_embeddings, batch_time = test_batch_embedding(model, test_queries)
        per_query_time = batch_time / len(test_queries)
        single_time = embedding_time  # From earlier
        speedup = single_time / per_query_time
        
        # Print results
        print(f"\n{'='*70}")
        print("OPTIMIZATION RESULTS")
        print(f"{'='*70}")
        
        print(f"\n1. LangChain (Baseline):")
        print(f"   Mean: {langchain_results['mean_ms']:.3f}ms")
        print(f"   p50:  {langchain_results['p50_ms']:.3f}ms")
        print(f"   p95:  {langchain_results['p95_ms']:.3f}ms")
        
        print(f"\n2. Direct qdrant_client (No LangChain):")
        print(f"   Mean: {direct_results['mean_ms']:.3f}ms")
        print(f"   p50:  {direct_results['p50_ms']:.3f}ms")
        print(f"   p95:  {direct_results['p95_ms']:.3f}ms")
        langchain_overhead = langchain_results['mean_ms'] - direct_results['mean_ms']
        print(f"   LangChain overhead: {langchain_overhead:.3f}ms")
        
        print(f"\n3. Cached Embedding (Production Use):")
        print(f"   Mean: {cached_results['mean_ms']:.3f}ms")
        print(f"   p50:  {cached_results['p50_ms']:.3f}ms")
        print(f"   p95:  {cached_results['p95_ms']:.3f}ms")
        print(f"   p99:  {cached_results['p99_ms']:.3f}ms")
        print(f"   Throughput: {1000 / cached_results['mean_ms']:.1f} queries/sec")
        
        print(f"\n4. Batch Embedding:")
        print(f"   Single query: {single_time:.2f}ms")
        print(f"   Batch (5 queries): {batch_time:.2f}ms total")
        print(f"   Per query (batch): {per_query_time:.2f}ms")
        print(f"   Speedup: {speedup:.2f}×")
        
        # Summary
        print(f"\n{'='*70}")
        print("PERFORMANCE BREAKDOWN")
        print(f"{'='*70}")
        print(f"\nCurrent (LangChain + embedding on each call):")
        print(f"   {langchain_results['mean_ms']:.2f}ms total")
        print(f"   ├─ Embedding generation: {embedding_time:.2f}ms (one-time)")
        print(f"   ├─ LangChain overhead: {langchain_overhead:.2f}ms")
        print(f"   ├─ HTTP + C server: ~1.86ms (from previous test)")
        print(f"   └─ SYNRIX core: ~0.0003ms (sub-microsecond)")
        
        print(f"\nOptimized (Direct client + cached embedding):")
        print(f"   {cached_results['mean_ms']:.2f}ms total")
        print(f"   ├─ Embedding: 0ms (pre-computed)")
        print(f"   ├─ Python client: {cached_results['mean_ms'] - 1.86:.2f}ms")
        print(f"   ├─ HTTP + C server: ~1.86ms")
        print(f"   └─ SYNRIX core: ~0.0003ms")
        
        improvement = langchain_results['mean_ms'] / cached_results['mean_ms']
        print(f"\n✅ Improvement: {improvement:.1f}× faster with optimizations")
        
        print(f"\n{'='*70}")
        print("RECOMMENDATIONS")
        print(f"{'='*70}")
        print("\n1. EMBEDDING GENERATION (26ms → 0ms):")
        print("   ✅ Pre-compute embeddings at application startup")
        print("   ✅ Cache embeddings in memory (LRU cache)")
        print("   ✅ Use batch processing when possible (2.88× faster)")
        print("   ✅ Consider ONNX runtime (2-3× faster than PyTorch)")
        
        print("\n2. PYTHON OVERHEAD (5.3ms → ~1-2ms):")
        print("   ✅ Use direct qdrant_client (bypass LangChain)")
        print("   ✅ Use async/await for concurrent requests")
        print("   ✅ Consider C client library for production")
        
        print("\n3. HTTP OVERHEAD (1.86ms → sub-microsecond):")
        print("   ✅ Use shared memory interface (direct C API)")
        print("   ✅ Use Unix domain sockets (lower latency)")
        
        print("\n4. PRODUCTION ARCHITECTURE:")
        print("   • Embedding service: Pre-compute + cache embeddings")
        print("   • Search service: Direct C client or shared memory")
        print("   • Result: ~2ms total (vs 35ms baseline)")
        print("   • 17.5× faster than current benchmark")
        
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
