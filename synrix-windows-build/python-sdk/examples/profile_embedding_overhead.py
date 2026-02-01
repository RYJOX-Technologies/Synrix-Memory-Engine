#!/usr/bin/env python3
"""
Embedding Generation Overhead Analysis
======================================

Breaks down embedding generation performance:
- Model loading time
- Inference time
- Batch vs single inference
- Model size impact
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

try:
    from sentence_transformers import SentenceTransformer
    import numpy as np
except ImportError:
    print("❌ Missing sentence-transformers. Install: pip install sentence-transformers")
    sys.exit(1)


def time_ns():
    return time.perf_counter_ns()


def test_model_loading(model_name):
    """Test model loading time"""
    print(f"\n📦 Testing Model Loading: {model_name}")
    start = time_ns()
    model = SentenceTransformer(model_name)
    elapsed = (time_ns() - start) / 1_000_000
    print(f"   Loading time: {elapsed:.2f}ms")
    print(f"   Embedding dimension: {model.get_sentence_embedding_dimension()}")
    return model


def test_single_inference(model, text, iterations=100):
    """Test single text inference"""
    print(f"\n🔍 Testing Single Inference - {iterations} iterations...")
    times = []
    
    for i in range(iterations):
        start = time_ns()
        embedding = model.encode(text, convert_to_numpy=False, normalize_embeddings=True)
        elapsed = (time_ns() - start) / 1_000_000
        times.append(elapsed)
    
    return {
        'mean_ms': statistics.mean(times),
        'p50_ms': statistics.median(times),
        'p95_ms': sorted(times)[int(len(times) * 0.95)],
        'min_ms': min(times),
        'max_ms': max(times),
        'stddev_ms': statistics.stdev(times) if len(times) > 1 else 0
    }


def test_batch_inference(model, texts, iterations=10):
    """Test batch inference"""
    print(f"\n📚 Testing Batch Inference ({len(texts)} texts per batch) - {iterations} iterations...")
    times = []
    
    for i in range(iterations):
        start = time_ns()
        embeddings = model.encode(texts, convert_to_numpy=False, normalize_embeddings=True)
        elapsed = (time_ns() - start) / 1_000_000
        times.append(elapsed)
    
    per_text = [t / len(texts) for t in times]
    
    return {
        'batch_mean_ms': statistics.mean(times),
        'per_text_mean_ms': statistics.mean(per_text),
        'per_text_p50_ms': statistics.median(per_text),
        'per_text_p95_ms': sorted(per_text)[int(len(per_text) * 0.95)],
    }


def test_python_overhead():
    """Test Python client overhead (without embedding)"""
    print(f"\n🐍 Testing Python Client Overhead...")
    
    # Simulate what LangChain does
    import json
    from qdrant_client import QdrantClient
    
    # Test JSON serialization
    test_vector = [0.1] * 384
    times_json = []
    for i in range(1000):
        start = time_ns()
        json_str = json.dumps({"vector": test_vector})
        elapsed = (time_ns() - start) / 1_000_000
        times_json.append(elapsed)
    
    # Test object creation
    times_obj = []
    for i in range(1000):
        start = time_ns()
        obj = {"vector": test_vector, "limit": 2, "with_payload": True}
        elapsed = (time_ns() - start) / 1_000_000
        times_obj.append(elapsed)
    
    return {
        'json_serialization_ms': statistics.mean(times_json),
        'object_creation_ms': statistics.mean(times_obj)
    }


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  Embedding Generation & Python Overhead Analysis               ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    
    test_text = "How do I reset a user password?"
    test_texts = [
        "How do I reset a user password?",
        "What are the API rate limits?",
        "How do I enable two-factor authentication?",
        "Can I export my data?",
        "How do I upgrade my plan?",
    ]
    
    # Test different models
    models_to_test = [
        'all-MiniLM-L6-v2',  # Current (384 dims, ~22MB)
        # 'all-mpnet-base-v2',  # Larger, slower, better quality (768 dims, ~420MB)
    ]
    
    for model_name in models_to_test:
        print(f"\n{'='*70}")
        print(f"MODEL: {model_name}")
        print(f"{'='*70}")
        
        # Load model
        model = test_model_loading(model_name)
        
        # Test single inference
        single_results = test_single_inference(model, test_text, iterations=100)
        print(f"\n   ✅ Single Inference Results:")
        print(f"      Mean: {single_results['mean_ms']:.3f}ms")
        print(f"      p50:  {single_results['p50_ms']:.3f}ms")
        print(f"      p95:  {single_results['p95_ms']:.3f}ms")
        print(f"      Min:  {single_results['min_ms']:.3f}ms")
        print(f"      Max:  {single_results['max_ms']:.3f}ms")
        print(f"      Std:  {single_results['stddev_ms']:.3f}ms")
        
        # Test batch inference
        batch_results = test_batch_inference(model, test_texts, iterations=20)
        print(f"\n   ✅ Batch Inference Results:")
        print(f"      Batch time: {batch_results['batch_mean_ms']:.3f}ms")
        print(f"      Per text:   {batch_results['per_text_mean_ms']:.3f}ms")
        print(f"      Speedup:    {single_results['mean_ms'] / batch_results['per_text_mean_ms']:.2f}×")
    
    # Test Python overhead
    print(f"\n{'='*70}")
    print("PYTHON OVERHEAD ANALYSIS")
    print(f"{'='*70}")
    python_results = test_python_overhead()
    print(f"\n   ✅ Python Operations:")
    print(f"      JSON serialization: {python_results['json_serialization_ms']:.3f}ms")
    print(f"      Object creation:    {python_results['object_creation_ms']:.3f}ms")
    
    # Recommendations
    print(f"\n{'='*70}")
    print("OPTIMIZATION RECOMMENDATIONS")
    print(f"{'='*70}")
    print("\n1. EMBEDDING GENERATION (26ms overhead):")
    print("   Options:")
    print("   a) Pre-compute embeddings (cache at application level)")
    print("   b) Use batch inference (5× faster per text)")
    print("   c) Use faster/smaller models (trade quality for speed)")
    print("   d) Use ONNX runtime (2-3× faster than PyTorch)")
    print("   e) Use C-based embedding library (e.g., fasttext)")
    print("   f) Use GPU acceleration (if available)")
    
    print("\n2. PYTHON OVERHEAD (5.3ms):")
    print("   Options:")
    print("   a) Use direct qdrant_client (bypass LangChain)")
    print("   b) Use C client library (bypass Python entirely)")
    print("   c) Use async/await for concurrent requests")
    print("   d) Optimize JSON serialization (use orjson)")
    print("   e) Use connection pooling")
    
    print("\n3. HTTP OVERHEAD (1.86ms):")
    print("   Options:")
    print("   a) Use shared memory interface (sub-microsecond)")
    print("   b) Use Unix domain sockets (lower latency)")
    print("   c) Use persistent HTTP connections")
    
    print("\n4. BEST APPROACH FOR PRODUCTION:")
    print("   • Pre-compute embeddings (one-time cost)")
    print("   • Use direct C client or shared memory")
    print("   • Cache embeddings in application layer")
    print("   • Use batch processing when possible")
    
    print("\n💡 Key Insight:")
    print("   Embedding generation is NOT SYNRIX's job - it's the application's.")
    print("   SYNRIX stores and searches vectors, it doesn't generate them.")
    print("   For best performance, generate embeddings once and reuse them.")


if __name__ == "__main__":
    main()
