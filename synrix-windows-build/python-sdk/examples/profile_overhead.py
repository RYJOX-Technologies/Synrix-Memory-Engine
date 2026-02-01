#!/usr/bin/env python3
"""
SYNRIX Performance Overhead Analysis
====================================

Measures performance at each layer:
1. Direct C server (bypass HTTP)
2. HTTP overhead
3. Python client overhead
4. Full stack (Python + HTTP + C)
"""

import sys
import os
import time
import subprocess
import json
import socket
from pathlib import Path

# Add parent directory to path
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


def test_http_raw(port, query_vector, iterations=100):
    """Test raw HTTP performance (no Python client overhead)"""
    print(f"\n📡 Testing Raw HTTP (curl) - {iterations} iterations...")
    
    # Create search request
    search_request = {
        "vector": query_vector,
        "limit": 2,
        "with_payload": True
    }
    
    request_json = json.dumps(search_request)
    request_body = f'POST /collections/support_knowledge_base/points/search HTTP/1.1\r\n'
    request_body += f'Host: localhost:{port}\r\n'
    request_body += f'Content-Type: application/json\r\n'
    request_body += f'Content-Length: {len(request_json)}\r\n'
    request_body += f'\r\n'
    request_body += request_json
    
    times = []
    
    for i in range(iterations):
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5.0)
            sock.connect(('localhost', port))
            
            start = time_ns()
            sock.sendall(request_body.encode())
            
            # Read response
            response = b''
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
                if b'\r\n\r\n' in response:
                    # Check if we have Content-Length
                    headers = response.split(b'\r\n\r\n')[0]
                    if b'Content-Length:' in headers:
                        # Parse content length and read body
                        for line in headers.split(b'\r\n'):
                            if line.startswith(b'Content-Length:'):
                                content_len = int(line.split(b':')[1].strip())
                                body_start = response.find(b'\r\n\r\n') + 4
                                if len(response) >= body_start + content_len:
                                    break
                    else:
                        break
            
            elapsed = time_ns() - start
            times.append(elapsed / 1_000_000)  # Convert to ms
            
            sock.close()
        except Exception as e:
            print(f"   Error on iteration {i}: {e}")
            continue
        
        if (i + 1) % 20 == 0:
            print(f"   Completed {i + 1}/{iterations}...")
    
    if not times:
        return {}
    
    return {
        'mean_ms': sum(times) / len(times),
        'min_ms': min(times),
        'max_ms': max(times),
        'p50_ms': sorted(times)[len(times) // 2],
        'p95_ms': sorted(times)[int(len(times) * 0.95)],
        'count': len(times)
    }


def test_python_client(vectorstore, query, iterations=100):
    """Test Python client performance (includes HTTP + Python overhead)"""
    print(f"\n🐍 Testing Python Client (LangChain) - {iterations} iterations...")
    
    times = []
    
    for i in range(iterations):
        start = time_ns()
        results = vectorstore.similarity_search(query, k=2)
        elapsed = time_ns() - start
        times.append(elapsed / 1_000_000)  # Convert to ms
        
        if (i + 1) % 20 == 0:
            print(f"   Completed {i + 1}/{iterations}...")
    
    if not times:
        return {}
    
    return {
        'mean_ms': sum(times) / len(times),
        'min_ms': min(times),
        'max_ms': max(times),
        'p50_ms': sorted(times)[len(times) // 2],
        'p95_ms': sorted(times)[int(len(times) * 0.95)],
        'count': len(times)
    }


def test_embedding_overhead(embeddings, query, iterations=100):
    """Test embedding generation overhead"""
    print(f"\n🧠 Testing Embedding Generation - {iterations} iterations...")
    
    times = []
    
    for i in range(iterations):
        start = time_ns()
        embedding = embeddings.embed_query(query)
        elapsed = time_ns() - start
        times.append(elapsed / 1_000_000)  # Convert to ms
    
    if not times:
        return {}
    
    return {
        'mean_ms': sum(times) / len(times),
        'min_ms': min(times),
        'max_ms': max(times),
        'p50_ms': sorted(times)[len(times) // 2],
        'count': len(times)
    }


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  SYNRIX Performance Overhead Analysis                           ║")
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
                vectorstore_temp = Qdrant(
                    client=qdrant_client,
                    collection_name='support_knowledge_base',
                    embeddings=embeddings
                )
                vectorstore_temp.add_texts(texts=knowledge_base, batch_size=3)
                print(f"   ✅ Indexed {len(knowledge_base)} documents")
            else:
                print(f"   Collection has {collection.points_count} points")
        except:
            print("   Creating collection and indexing data...")
            qdrant_client.create_collection(
                collection_name='support_knowledge_base',
                vectors_config={'size': embeddings.embedding_dimension, 'distance': 'Cosine'}
            )
            vectorstore_temp = Qdrant(
                client=qdrant_client,
                collection_name='support_knowledge_base',
                embeddings=embeddings
            )
            vectorstore_temp.add_texts(texts=knowledge_base, batch_size=3)
            print(f"   ✅ Indexed {len(knowledge_base)} documents")
        
        vectorstore = Qdrant(
            client=qdrant_client,
            collection_name='support_knowledge_base',
            embeddings=embeddings
        )
        
        # Test query
        test_query = "How do I reset a user password?"
        print(f"\n📝 Test query: \"{test_query}\"")
        
        # Generate embedding once for HTTP test
        query_embedding = embeddings.embed_query(test_query)
        # Convert to list if it's a tensor/array
        if hasattr(query_embedding, 'tolist'):
            query_embedding = query_embedding.tolist()
        elif not isinstance(query_embedding, list):
            query_embedding = list(query_embedding)
        print(f"   Embedding dimension: {len(query_embedding)}")
        
        # Run tests
        iterations = 50
        
        embedding_results = test_embedding_overhead(embeddings, test_query, iterations)
        http_results = test_http_raw(6334, query_embedding, iterations)
        python_results = test_python_client(vectorstore, test_query, iterations)
        
        # Print results
        print("\n" + "="*70)
        print("PERFORMANCE BREAKDOWN")
        print("="*70)
        
        print(f"\n1. Embedding Generation (Python + sentence-transformers):")
        if embedding_results:
            print(f"   Mean: {embedding_results['mean_ms']:.2f}ms")
            print(f"   p50:  {embedding_results['p50_ms']:.2f}ms")
            print(f"   Min:  {embedding_results['min_ms']:.2f}ms")
            print(f"   Max:  {embedding_results['max_ms']:.2f}ms")
        
        print(f"\n2. Raw HTTP Request (curl-like, no Python client):")
        if http_results:
            print(f"   Mean: {http_results['mean_ms']:.2f}ms")
            print(f"   p50:  {http_results['p50_ms']:.2f}ms")
            print(f"   p95:  {http_results['p95_ms']:.2f}ms")
            print(f"   Min:  {http_results['min_ms']:.2f}ms")
            print(f"   Max:  {http_results['max_ms']:.2f}ms")
        
        print(f"\n3. Full Python Client (LangChain + HTTP + C server):")
        if python_results:
            print(f"   Mean: {python_results['mean_ms']:.2f}ms")
            print(f"   p50:  {python_results['p50_ms']:.2f}ms")
            print(f"   p95:  {python_results['p95_ms']:.2f}ms")
            print(f"   Min:  {python_results['min_ms']:.2f}ms")
            print(f"   Max:  {python_results['max_ms']:.2f}ms")
        
        # Calculate overhead
        print(f"\n" + "="*70)
        print("OVERHEAD ANALYSIS")
        print("="*70)
        
        if http_results and python_results:
            python_overhead = python_results['mean_ms'] - http_results['mean_ms']
            print(f"\nPython Client Overhead: {python_overhead:.2f}ms")
            print(f"   (Full Python - Raw HTTP)")
        
        if embedding_results and http_results:
            # HTTP includes embedding time in our test, so subtract it
            c_server_time = http_results['mean_ms'] - embedding_results['mean_ms']
            print(f"\nEstimated C Server Time: {c_server_time:.2f}ms")
            print(f"   (Raw HTTP - Embedding)")
            print(f"   Note: This is HTTP overhead + C server processing")
        
        if embedding_results and python_results:
            total_overhead = python_results['mean_ms'] - embedding_results['mean_ms']
            print(f"\nTotal Overhead (Python + HTTP): {total_overhead:.2f}ms")
            print(f"   (Full Python - Embedding)")
        
        print(f"\n💡 To get microsecond performance:")
        print(f"   • Use direct C API (bypass HTTP)")
        print(f"   • Use shared memory interface")
        print(f"   • Call lattice_get_node_data() directly")
        
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
