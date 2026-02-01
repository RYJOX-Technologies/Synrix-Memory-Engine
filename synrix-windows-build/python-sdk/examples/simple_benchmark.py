#!/usr/bin/env python3
"""
Simple SYNRIX Benchmark
========================

Quick benchmark using the existing customer demo setup.
Measures search latency and throughput.
"""

import sys
import os
import time
import statistics
from pathlib import Path

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

nvme_env = "/mnt/nvme/aion-omega/python-env"
if os.path.exists(nvme_env):
    sys.path.insert(0, os.path.join(nvme_env, "lib", "python3.10", "site-packages"))

# Import the demo function
from mock_customer_demo import demo_customer_support_kb, start_synrix_server, check_server_running
from langchain_community.vectorstores import Qdrant
from qdrant_client import QdrantClient
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


def benchmark_search(vectorstore, queries, iterations=50):
    """Benchmark search performance"""
    print("\n" + "="*70)
    print("SEARCH PERFORMANCE BENCHMARK")
    print("="*70)
    print(f"Running {iterations} iterations of {len(queries)} queries...")
    
    all_times = []
    
    for i in range(iterations):
        for query in queries:
            start = time.time()
            results = vectorstore.similarity_search(query, k=2)
            elapsed = (time.time() - start) * 1000
            all_times.append(elapsed)
        
        if (i + 1) % 10 == 0:
            print(f"   Completed {i + 1}/{iterations} iterations...")
    
    if not all_times:
        return {}
    
    sorted_times = sorted(all_times)
    n = len(sorted_times)
    
    results = {
        'total_queries': len(all_times),
        'p50_ms': sorted_times[n // 2],
        'p95_ms': sorted_times[int(n * 0.95)] if n > 1 else sorted_times[-1],
        'p99_ms': sorted_times[int(n * 0.99)] if n > 1 else sorted_times[-1],
        'mean_ms': statistics.mean(all_times),
        'min_ms': min(all_times),
        'max_ms': max(all_times),
        'stddev_ms': statistics.stdev(all_times) if len(all_times) > 1 else 0
    }
    
    print(f"\n✅ Results ({len(all_times)} queries):")
    print(f"   p50 (median): {results['p50_ms']:.2f}ms")
    print(f"   p95:          {results['p95_ms']:.2f}ms")
    print(f"   p99:          {results['p99_ms']:.2f}ms")
    print(f"   Mean:         {results['mean_ms']:.2f}ms")
    print(f"   Min:          {results['min_ms']:.2f}ms")
    print(f"   Max:          {results['max_ms']:.2f}ms")
    print(f"   Std Dev:      {results['stddev_ms']:.2f}ms")
    print(f"   Throughput:   {len(all_times) / (sum(all_times) / 1000):.1f} queries/sec")
    
    return results


def main():
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  Simple SYNRIX Benchmark                                         ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    
    # Start server
    print("\n🚀 Starting SYNRIX server...")
    synrix_process = start_synrix_server(6334)
    if synrix_process is None and not check_server_running(6334):
        print("❌ Failed to start SYNRIX server")
        return
    print("✅ SYNRIX server ready")
    
    try:
        # Use existing demo setup
        print("\n🔧 Setting up vectorstore...")
        embeddings = SentenceTransformerEmbeddings('all-MiniLM-L6-v2')
        
        qdrant_client = QdrantClient(url='http://localhost:6334', timeout=120)
        
        # Import knowledge base from demo
        from mock_customer_demo import demo_customer_support_kb
        # Get the knowledge base content (first 62 docs from the demo)
        knowledge_base = [
            "Acme Pro is our enterprise product with advanced features including multi-user support, API access, and priority support.",
            "Acme Basic is our starter plan with core features for individual users. It includes 10GB storage and email support.",
            "Our pricing starts at $29/month for Basic and $99/month for Pro. Annual plans include 20% discount.",
            "Acme Enterprise is our largest plan designed for organizations with 100+ users. Includes dedicated support, custom SLA, and on-premise deployment options.",
            "Feature comparison: Basic includes core features, Pro adds API access and webhooks, Enterprise includes everything plus custom integrations and dedicated support.",
            "Mobile apps are available for iOS and Android. Download from the App Store or Google Play. Mobile apps support all Basic and Pro features.",
            "Browser extensions are available for Chrome, Firefox, and Safari. Extensions allow quick access to your data without opening the full application.",
            "Desktop applications are available for Windows, macOS, and Linux. Desktop apps provide offline access and better performance for large datasets.",
            "API documentation is available at api.acme.com/docs. All API endpoints support JSON responses and require authentication via API key.",
            "SDKs are available for Python, JavaScript, Java, and Go. SDKs simplify integration and handle authentication automatically.",
            "To reset a user password, go to Settings > Users > Select User > Reset Password. An email will be sent with reset instructions.",
            "If you're experiencing login issues, clear your browser cache and cookies, then try logging in again. If problems persist, contact support.",
            "API rate limits are 1000 requests per hour for Basic plans and 10000 requests per hour for Pro plans. Limits reset every hour.",
            "If you've forgotten your password, use the 'Forgot Password' link on the login page. You'll receive an email with reset instructions within 5 minutes.",
            "Account lockout occurs after 5 failed login attempts. Wait 15 minutes or contact support to unlock your account immediately.",
            "Session timeout is 24 hours of inactivity. You'll be automatically logged out for security. Simply log in again to continue.",
            "Two-factor authentication codes expire after 30 seconds. If your code expires, request a new one from your authenticator app.",
            "Email notifications can be managed in Settings > Notifications > Email. You can enable or disable specific notification types.",
            "Push notifications require the mobile app and can be configured in the app settings. Notifications alert you to important account activity.",
            "Browser compatibility: We support Chrome 90+, Firefox 88+, Safari 14+, and Edge 90+. Older browsers may experience limited functionality.",
            "Mobile app requires iOS 13+ or Android 8+. Older devices may not support all features. Check device compatibility before installing.",
            "If you're seeing error messages, check the error code in our documentation. Common errors include authentication failures and rate limit exceeded.",
            "If data sync is failing, check your internet connection and verify API credentials are correct. Restart the application if issues continue.",
            "Export functionality requires Pro plan. Go to Data > Export to download your data in CSV or JSON format.",
            "Two-factor authentication can be enabled in Settings > Security > Enable 2FA. You'll need a mobile authenticator app.",
            "If files aren't uploading, check file size limits. Basic plans allow 10MB per file, Pro allows 100MB, Enterprise allows 1GB.",
            "Slow performance can be caused by large datasets. Try filtering or paginating results. Pro and Enterprise plans include performance optimizations.",
            "Data import errors usually indicate format issues. Ensure CSV files have proper headers and data types match expected formats.",
            "Connection timeouts may occur with slow internet. Increase timeout settings in Settings > Advanced > Network Timeout.",
            "If webhooks aren't firing, verify the webhook URL is accessible and returns 200 status codes. Check webhook logs in Settings > Integrations.",
            "API authentication errors typically mean your API key is invalid or expired. Generate a new key in Settings > API > Generate Key.",
            "Database errors are rare but can occur. If you see database error messages, contact support immediately with the error code.",
            "Webhooks allow you to receive real-time notifications when events occur. Configure webhooks in Settings > Integrations > Webhooks.",
            "Custom integrations are available for Pro customers. Contact your account manager to set up custom API integrations.",
            "Data backup runs automatically every 24 hours. Manual backups can be triggered from Settings > Backup > Create Backup Now.",
            "Scheduled reports can be configured to run daily, weekly, or monthly. Reports are emailed automatically to specified recipients.",
            "Data visualization tools are available in Pro and Enterprise plans. Create charts and graphs from your data with drag-and-drop interface.",
            "Advanced search supports boolean operators, wildcards, and regex patterns. Use quotes for exact phrases and parentheses for grouping.",
            "Bulk operations allow you to modify multiple records at once. Select records using checkboxes and choose an action from the bulk menu.",
            "Workflow automation lets you create custom workflows triggered by events. Available in Pro and Enterprise plans with visual workflow builder.",
            "Data validation rules ensure data quality. Define rules in Settings > Data > Validation Rules. Invalid data is flagged for review.",
            "Audit logs track all changes to your data. View logs in Settings > Security > Audit Logs. Logs are retained for 90 days on Pro, 1 year on Enterprise.",
            "To upgrade your plan, go to Settings > Billing > Upgrade Plan. Changes take effect immediately and are prorated.",
            "Team members can be added in Settings > Team > Add Member. Pro plans support up to 50 team members.",
            "Billing invoices are available in Settings > Billing > Invoices. You can download PDF copies of all invoices.",
            "Payment methods can be added or updated in Settings > Billing > Payment Methods. We accept credit cards and ACH transfers.",
            "Subscription cancellation can be done in Settings > Billing > Cancel Subscription. Your account remains active until the end of the billing period.",
            "Refund requests must be submitted within 30 days of purchase. Contact billing@acme.com with your account details and reason for refund.",
            "Account deletion permanently removes all data. This action cannot be undone. Export your data first in Settings > Data > Export.",
            "Organization settings allow admins to configure company-wide defaults. Available in Enterprise plans with admin access.",
            "Single sign-on (SSO) is available for Enterprise customers. Configure SSO in Settings > Security > Single Sign-On.",
            "User roles and permissions can be customized in Settings > Team > Roles. Define what each role can access and modify.",
            "Our API uses RESTful endpoints with JSON responses. Authentication requires an API key in the Authorization header.",
            "Webhook payloads include event type, timestamp, and relevant data. Payloads are signed with HMAC-SHA256 for security.",
            "Database queries are optimized for performance. Complex queries may take longer but typically complete within 2-3 seconds.",
            "API versioning: We maintain backward compatibility for 12 months. New API versions are announced 3 months in advance.",
            "Rate limiting uses token bucket algorithm. Burst capacity is 2x your hourly limit. Exceeding limits returns 429 status code.",
            "Data encryption: All data is encrypted at rest using AES-256 and in transit using TLS 1.3. Keys are managed securely.",
            "Data residency options are available for Enterprise customers. Choose where your data is stored to meet compliance requirements.",
            "Backup and disaster recovery: Daily backups are retained for 30 days. Point-in-time recovery available for Enterprise plans.",
            "Monitoring and alerts: Set up custom alerts for API usage, errors, and performance metrics. Alerts can be sent via email or webhook.",
            "Performance optimization: Use pagination for large result sets. Filter data before retrieving to reduce response times and bandwidth.",
        ]
        
        # Use existing collection or create new one
        try:
            collection = qdrant_client.get_collection('support_knowledge_base')
            if collection.points_count == 0:
                print("   Collection exists but is empty, indexing documents...")
                needs_indexing = True
            else:
                print(f"   Using existing collection with {collection.points_count} points")
                needs_indexing = False
        except:
            print("   Creating new collection...")
            qdrant_client.create_collection(
                collection_name='support_knowledge_base',
                vectors_config={'size': embeddings.embedding_dimension, 'distance': 'Cosine'}
            )
            needs_indexing = True
        
        vectorstore = Qdrant(
            client=qdrant_client,
            collection_name='support_knowledge_base',
            embeddings=embeddings
        )
        
        if needs_indexing:
            print(f"   Indexing {len(knowledge_base)} documents...")
            vectorstore.add_texts(texts=knowledge_base, batch_size=3)
            print("   ✅ Indexing complete")
        
        # Test queries
        test_queries = [
            "How do I reset a user password?",
            "What are the API rate limits?",
            "How do I enable two-factor authentication?",
            "Can I export my data?",
            "How do I upgrade my plan?",
        ]
        
        # Run benchmark
        results = benchmark_search(vectorstore, test_queries, iterations=50)
        
        print("\n" + "="*70)
        print("BENCHMARK SUMMARY")
        print("="*70)
        print(f"Total queries: {results.get('total_queries', 0)}")
        print(f"p50 latency: {results.get('p50_ms', 0):.2f}ms")
        print(f"p95 latency: {results.get('p95_ms', 0):.2f}ms")
        print(f"p99 latency: {results.get('p99_ms', 0):.2f}ms")
        print(f"Mean latency: {results.get('mean_ms', 0):.2f}ms")
        print("\n✅ Benchmark complete!")
        
    except Exception as e:
        print(f"\n❌ Benchmark failed: {e}")
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
