#!/usr/bin/env python3
"""
SYNRIX Performance Benchmark
=============================

Comprehensive benchmark suite for SYNRIX vector database:
- Indexing throughput (vectors/second)
- Search latency (p50, p95, p99)
- Concurrent query performance
- Memory usage
- Accuracy validation
"""

import sys
import os
import time
import subprocess
import statistics
import threading
from pathlib import Path
from typing import List, Optional
from concurrent.futures import ThreadPoolExecutor, as_completed
try:
    import psutil
    PSUTIL_AVAILABLE = True
except ImportError:
    PSUTIL_AVAILABLE = False

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

# Try to use NVME Python environment if available
nvme_env = "/mnt/nvme/aion-omega/python-env"
if os.path.exists(nvme_env):
    sys.path.insert(0, os.path.join(nvme_env, "lib", "python3.10", "site-packages"))

try:
    from langchain_community.vectorstores import Qdrant
    from langchain_core.embeddings import Embeddings
    from sentence_transformers import SentenceTransformer
    from qdrant_client import QdrantClient
    IMPORTS_AVAILABLE = True
except ImportError as e:
    print(f"❌ Missing dependencies: {e}")
    print("   Install with: pip install langchain langchain-community qdrant-client sentence-transformers")
    sys.exit(1)


def check_server_running(port: int, timeout: float = 1.0) -> bool:
    """Check if a server is running on the given port"""
    try:
        import socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        result = sock.connect_ex(('localhost', port))
        sock.close()
        return result == 0
    except:
        return False


def start_synrix_server(port: int = 6334) -> Optional[subprocess.Popen]:
    """Start SYNRIX server if not already running"""
    if check_server_running(port):
        return None  # Server already running
    
    project_root = Path(__file__).parent.parent.parent
    server_path = project_root / "integrations" / "qdrant_mimic" / "synrix-server-evaluation"
    
    if not server_path.exists():
        print(f"❌ SYNRIX server not found at: {server_path}")
        return None
    
    lattice_path = os.path.expanduser("~/.synrix_benchmark.lattice")
    
    try:
        log_file_path = f"/tmp/synrix_server_{port}.log"
        log_file = open(log_file_path, "w", buffering=1)  # Overwrite for clean logs
        process = subprocess.Popen(
            [str(server_path), "--port", str(port), "--lattice-path", lattice_path, "--verbose"],
            stdout=log_file,
            stderr=subprocess.STDOUT,
            text=True
        )
        process._log_file = log_file
        time.sleep(2)  # Give server time to start
        
        if process.poll() is None and check_server_running(port):
            return process
        else:
            process.terminate()
            process.wait()
            return None
    except Exception as e:
        print(f"❌ Failed to start server: {e}")
        return None


class BenchmarkEmbeddings(Embeddings):
    """Wrapper for sentence-transformers with dimension tracking"""
    def __init__(self, model_name='all-MiniLM-L6-v2'):
        super().__init__()
        print(f"   Loading model: {model_name}...")
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


def benchmark_indexing(vectorstore, documents: List[str], batch_size: int = 10) -> dict:
    """Benchmark indexing performance"""
    print("\n" + "="*70)
    print("BENCHMARK 1: Indexing Performance")
    print("="*70)
    
    total_docs = len(documents)
    print(f"📊 Indexing {total_docs} documents in batches of {batch_size}...")
    
    start_time = time.time()
    vectorstore.add_texts(texts=documents, batch_size=batch_size)
    elapsed = time.time() - start_time
    
    vectors_per_sec = total_docs / elapsed if elapsed > 0 else 0
    
    result = {
        'total_documents': total_docs,
        'batch_size': batch_size,
        'total_time_sec': elapsed,
        'vectors_per_second': vectors_per_sec,
        'avg_time_per_vector_ms': (elapsed / total_docs * 1000) if total_docs > 0 else 0
    }
    
    print(f"✅ Indexing complete:")
    print(f"   Total time: {elapsed:.2f}s")
    print(f"   Throughput: {vectors_per_sec:.1f} vectors/second")
    print(f"   Avg per vector: {result['avg_time_per_vector_ms']:.2f}ms")
    
    return result


def benchmark_search_latency(vectorstore, queries: List[str], k: int = 5, iterations: int = 10) -> dict:
    """Benchmark search latency with multiple iterations"""
    print("\n" + "="*70)
    print("BENCHMARK 2: Search Latency")
    print("="*70)
    
    print(f"📊 Running {iterations} iterations of {len(queries)} queries each (k={k})...")
    
    all_times = []
    
    for iteration in range(iterations):
        iteration_times = []
        for query in queries:
            start = time.time()
            results = vectorstore.similarity_search(query, k=k)
            elapsed = (time.time() - start) * 1000  # Convert to ms
            iteration_times.append(elapsed)
            all_times.append(elapsed)
        
        if (iteration + 1) % 5 == 0:
            print(f"   Completed {iteration + 1}/{iterations} iterations...")
    
    if not all_times:
        return {}
    
    sorted_times = sorted(all_times)
    n = len(sorted_times)
    
    result = {
        'total_queries': len(all_times),
        'iterations': iterations,
        'k': k,
        'p50_ms': sorted_times[n // 2],
        'p95_ms': sorted_times[int(n * 0.95)] if n > 1 else sorted_times[-1],
        'p99_ms': sorted_times[int(n * 0.99)] if n > 1 else sorted_times[-1],
        'min_ms': min(all_times),
        'max_ms': max(all_times),
        'mean_ms': statistics.mean(all_times),
        'median_ms': statistics.median(all_times),
        'stddev_ms': statistics.stdev(all_times) if len(all_times) > 1 else 0
    }
    
    print(f"✅ Latency results ({len(all_times)} queries):")
    print(f"   p50 (median): {result['p50_ms']:.2f}ms")
    print(f"   p95:          {result['p95_ms']:.2f}ms")
    print(f"   p99:          {result['p99_ms']:.2f}ms")
    print(f"   Mean:         {result['mean_ms']:.2f}ms")
    print(f"   Min:          {result['min_ms']:.2f}ms")
    print(f"   Max:          {result['max_ms']:.2f}ms")
    print(f"   Std Dev:      {result['stddev_ms']:.2f}ms")
    
    return result


def benchmark_concurrent_queries(vectorstore, queries: List[str], num_threads: int = 10, queries_per_thread: int = 10) -> dict:
    """Benchmark concurrent query performance"""
    print("\n" + "="*70)
    print("BENCHMARK 3: Concurrent Query Performance")
    print("="*70)
    
    total_queries = num_threads * queries_per_thread
    print(f"📊 Running {total_queries} queries across {num_threads} threads ({queries_per_thread} per thread)...")
    
    def run_queries(thread_id: int):
        thread_times = []
        for i in range(queries_per_thread):
            query = queries[i % len(queries)]  # Cycle through queries
            start = time.time()
            results = vectorstore.similarity_search(query, k=5)
            elapsed = (time.time() - start) * 1000
            thread_times.append(elapsed)
        return thread_times
    
    start_time = time.time()
    all_times = []
    
    with ThreadPoolExecutor(max_workers=num_threads) as executor:
        futures = [executor.submit(run_queries, i) for i in range(num_threads)]
        for future in as_completed(futures):
            thread_times = future.result()
            all_times.extend(thread_times)
    
    total_elapsed = time.time() - start_time
    
    if not all_times:
        return {}
    
    sorted_times = sorted(all_times)
    n = len(sorted_times)
    
    result = {
        'total_queries': total_queries,
        'num_threads': num_threads,
        'queries_per_thread': queries_per_thread,
        'total_time_sec': total_elapsed,
        'queries_per_second': total_queries / total_elapsed if total_elapsed > 0 else 0,
        'p50_ms': sorted_times[n // 2],
        'p95_ms': sorted_times[int(n * 0.95)] if n > 1 else sorted_times[-1],
        'p99_ms': sorted_times[int(n * 0.99)] if n > 1 else sorted_times[-1],
        'mean_ms': statistics.mean(all_times),
        'min_ms': min(all_times),
        'max_ms': max(all_times)
    }
    
    print(f"✅ Concurrent query results:")
    print(f"   Total time: {total_elapsed:.2f}s")
    print(f"   Throughput: {result['queries_per_second']:.1f} queries/second")
    print(f"   p50: {result['p50_ms']:.2f}ms")
    print(f"   p95: {result['p95_ms']:.2f}ms")
    print(f"   p99: {result['p99_ms']:.2f}ms")
    print(f"   Mean: {result['mean_ms']:.2f}ms")
    
    return result


def get_memory_usage() -> dict:
    """Get current memory usage"""
    if not PSUTIL_AVAILABLE:
        return {'rss_mb': 0, 'vms_mb': 0}
    try:
        process = psutil.Process()
        mem_info = process.memory_info()
        return {
            'rss_mb': mem_info.rss / (1024 * 1024),
            'vms_mb': mem_info.vms / (1024 * 1024)
        }
    except:
        return {'rss_mb': 0, 'vms_mb': 0}


def benchmark_accuracy(vectorstore, test_queries: List[tuple]) -> dict:
    """Benchmark search accuracy (requires ground truth)"""
    print("\n" + "="*70)
    print("BENCHMARK 4: Search Accuracy")
    print("="*70)
    
    if not test_queries:
        print("⚠️  No accuracy test queries provided, skipping...")
        return {}
    
    print(f"📊 Testing accuracy on {len(test_queries)} query-document pairs...")
    
    correct = 0
    total = len(test_queries)
    
    for query, expected_doc in test_queries:
        results = vectorstore.similarity_search(query, k=5)
        if results:
            # Check if expected document is in top 5 results
            top_contents = [r.page_content for r in results[:5]]
            if any(expected_doc in content for content in top_contents):
                correct += 1
    
    accuracy = (correct / total * 100) if total > 0 else 0
    
    result = {
        'total_queries': total,
        'correct': correct,
        'accuracy_percent': accuracy
    }
    
    print(f"✅ Accuracy results:")
    print(f"   Correct: {correct}/{total}")
    print(f"   Accuracy: {accuracy:.1f}%")
    
    return result


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX Performance Benchmark Suite                             ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    # Start SYNRIX server
    print("🚀 Starting SYNRIX server...")
    synrix_process = start_synrix_server(6334)
    if synrix_process is None and not check_server_running(6334):
        print("❌ Failed to start SYNRIX server")
        return
    print("✅ SYNRIX server ready")
    
    synrix_process_local = synrix_process  # Keep reference for cleanup
    
    try:
        # Initialize embeddings and vectorstore
        print("\n🔧 Initializing embeddings and vectorstore...")
        embeddings = BenchmarkEmbeddings('all-MiniLM-L6-v2')
        
        qdrant_client = QdrantClient(url='http://localhost:6334', timeout=120)
        
        # Clean up and create collection
        try:
            qdrant_client.delete_collection('benchmark_collection')
        except:
            pass
        
        qdrant_client.create_collection(
            collection_name='benchmark_collection',
            vectors_config={'size': embeddings.embedding_dimension, 'distance': 'Cosine'}
        )
        
        vectorstore = Qdrant(
            client=qdrant_client,
            collection_name='benchmark_collection',
            embeddings=embeddings
        )
        
        # Generate test documents
        print("\n📚 Generating test documents...")
        num_docs = 500  # Reduced for stability
        documents = [
            f"Document {i}: This is a test document about topic {i % 10}. "
            f"It contains information about various subjects including technology, science, "
            f"and general knowledge. The content is designed to test vector similarity search "
            f"performance and accuracy. Each document has unique characteristics that make it "
            f"distinguishable from others while maintaining semantic relationships."
            for i in range(num_docs)
        ]
        print(f"   Generated {len(documents)} documents")
        
        # Test queries
        test_queries = [
            "What is technology?",
            "Tell me about science",
            "Explain general knowledge",
            "What are the main topics?",
            "Describe the content",
            "Information about subjects",
            "Technology and science",
            "Knowledge and content",
            "Test performance",
            "Vector similarity"
        ]
        
        # Run benchmarks
        mem_before = get_memory_usage()
        
        indexing_results = benchmark_indexing(vectorstore, documents, batch_size=5)
        
        mem_after_index = get_memory_usage()
        
        search_results = benchmark_search_latency(vectorstore, test_queries, k=5, iterations=10)
        
        concurrent_results = benchmark_concurrent_queries(vectorstore, test_queries, num_threads=5, queries_per_thread=10)
        
        mem_after_all = get_memory_usage()
        
        # Print summary
        print("\n" + "="*70)
        print("BENCHMARK SUMMARY")
        print("="*70)
        print(f"\n📊 Indexing:")
        print(f"   Throughput: {indexing_results.get('vectors_per_second', 0):.1f} vectors/sec")
        print(f"   Avg latency: {indexing_results.get('avg_time_per_vector_ms', 0):.2f}ms per vector")
        
        print(f"\n🔍 Search Latency:")
        print(f"   p50: {search_results.get('p50_ms', 0):.2f}ms")
        print(f"   p95: {search_results.get('p95_ms', 0):.2f}ms")
        print(f"   p99: {search_results.get('p99_ms', 0):.2f}ms")
        print(f"   Mean: {search_results.get('mean_ms', 0):.2f}ms")
        
        print(f"\n⚡ Concurrent Performance:")
        print(f"   Throughput: {concurrent_results.get('queries_per_second', 0):.1f} queries/sec")
        print(f"   p50: {concurrent_results.get('p50_ms', 0):.2f}ms")
        print(f"   p95: {concurrent_results.get('p95_ms', 0):.2f}ms")
        
        print(f"\n💾 Memory Usage:")
        print(f"   Before indexing: {mem_before.get('rss_mb', 0):.1f} MB RSS")
        print(f"   After indexing: {mem_after_index.get('rss_mb', 0):.1f} MB RSS")
        print(f"   After all tests: {mem_after_all.get('rss_mb', 0):.1f} MB RSS")
        print(f"   Indexing overhead: {mem_after_index.get('rss_mb', 0) - mem_before.get('rss_mb', 0):.1f} MB")
        
        print("\n✅ Benchmark complete!")
        
    except Exception as e:
        print(f"\n❌ Benchmark failed: {e}")
        import traceback
        traceback.print_exc()
    finally:
        # Cleanup - only stop server if we started it
        if synrix_process_local:
            print("\n🛑 Stopping SYNRIX server...")
            try:
                synrix_process_local.terminate()
                synrix_process_local.wait(timeout=5)
            except:
                try:
                    synrix_process_local.kill()
                except:
                    pass
            if hasattr(synrix_process_local, '_log_file'):
                try:
                    synrix_process_local._log_file.close()
                except:
                    pass
            print("✅ SYNRIX server stopped")


if __name__ == "__main__":
    main()
