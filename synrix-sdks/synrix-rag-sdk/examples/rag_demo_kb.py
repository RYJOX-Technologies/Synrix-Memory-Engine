#!/usr/bin/env python3
"""
SYNRIX RAG Demo – Knowledge base style (what people use RAG for).

Typical RAG use: ingest a corpus (support articles, docs, wiki), then answer
questions by retrieving relevant chunks and feeding them to an LLM.

This demo uses a built-in set of ~30 support/kb-style documents (many >512 bytes → chunked).
Many docs exceed 512 bytes so the engine uses chunked storage; ingestion is
one call per doc into the C engine (no Python chunk loop).

Run (from synrix-rag-sdk):  pip install -e . && python examples/rag_demo_kb.py
"""

import os
import sys
import time

_script_dir = os.path.dirname(os.path.abspath(__file__))
_sdk_root = os.path.dirname(_script_dir)
if _sdk_root not in sys.path:
    sys.path.insert(0, _sdk_root)

# Built-in knowledge-base style documents (support / product docs).
# Each is long enough to exceed 512 bytes → chunked storage in engine.
KB_DOCS = [
    ("Password reset", "How to reset your password. If you forgot your password, go to the login page and click Forgot password. Enter your email and we will send a secure link valid for 24 hours. Do not share this link. If you use SSO, contact your IT admin to reset your password. For security we lock accounts after 5 failed attempts for 15 minutes. To unlock earlier, use the same Forgot password flow or contact support. If you do not receive the email within a few minutes, check your spam folder and ensure you entered the correct address. Password requirements: at least 8 characters, including one number and one symbol. You can change your password anytime from Account -> Security."),
    ("Refund policy", "Refunds are available within 30 days of purchase for annual plans. Monthly plans can be cancelled anytime; no refund for the current month. To request a refund go to Billing in the dashboard, select the invoice, and click Request refund. Refunds are processed within 5–10 business days to the original payment method. Enterprise contracts are governed by the signed agreement; contact your account manager for cancellation terms."),
    ("API rate limits", "API rate limits depend on your plan. Free: 100 requests per minute. Pro: 1000 per minute. Enterprise: configurable. When you exceed the limit you receive HTTP 429; the Retry-After header indicates when to retry. We recommend exponential backoff. For bulk operations use our batch endpoints which count as a single request. Rate limits are per API key and per endpoint; contact us for higher limits."),
    ("Two-factor authentication", "We support 2FA via TOTP (authenticator apps like Google Authenticator or Authy). Enable 2FA in Account → Security. Scan the QR code with your app and enter the code to confirm. Save your backup codes in a safe place; you will need one if you lose your device. Recovery can take 24–48 hours if you lose both device and backup codes. SSO users may have 2FA enforced by their identity provider."),
    ("Exporting your data", "You can export all your data from Settings → Data export. Exports include projects, history, and assets in a ZIP. Large accounts may take up to 24 hours; we email you when the export is ready. Exports are available for 7 days. For GDPR or legal hold we can provide a full backup; contact support with your request and we will respond within 5 business days."),
    ("Billing and invoices", "Invoices are generated at the start of each billing cycle. You can download them from Billing → Invoices. We accept major credit cards and ACH (US). Enterprise customers can pay by invoice (NET 30). Update your payment method anytime in Billing. Failed payments trigger 3 retry attempts; we notify you by email. If payment continues to fail, your account may be restricted until the balance is paid."),
    ("SSO and SAML", "Enterprise plans can enable SAML 2.0 SSO. Configure your IdP with our SAML metadata URL (found in Settings → SSO). We support Okta, Azure AD, Google Workspace, and any SAML 2.0–compliant provider. After SSO is enabled, users sign in through your IdP. Just-in-time provisioning is supported; we create user records on first sign-in. Contact your account manager to enable SSO."),
    ("Webhook setup", "Webhooks notify your server when events occur (e.g. run completed, model trained). Create a webhook in Settings → Integrations: add the URL and choose events. We sign payloads with HMAC-SHA256; verify the X-Signature header. We retry failed deliveries with exponential backoff for up to 24 hours. Respond with 2xx quickly to avoid retries. For high volume we recommend our event stream API instead."),
    ("Database connection limits", "Each plan has a maximum number of concurrent database connections. Free: 5, Pro: 25, Enterprise: 100 or custom. Connections are pooled; idle connections are released after 10 minutes. If you hit the limit you will see Too many connections. Reduce connection lifetime in your client or upgrade your plan. For read replicas, connections are counted per replica."),
    ("Troubleshooting slow queries", "Slow queries are usually due to missing indexes or large scans. Use the Query analyzer in the dashboard to see execution plans. Add indexes on frequently filtered or joined columns. Avoid SELECT *; fetch only needed columns. For analytics, use our batch query API which runs during off-peak hours. If the issue persists, our support team can review your schema and suggest optimizations."),
    ("Backup and recovery", "We take automated daily backups of your data. Backups are retained for 30 days on Pro and 90 days on Enterprise. Point-in-time recovery is available for the last 7 days. To restore, contact support with the desired timestamp; we will restore to a new environment for you to verify before switching. Self-service restore is available on Enterprise via the dashboard."),
    ("Compliance and SOC 2", "We are SOC 2 Type II certified. Our compliance documentation and penetration test summaries are available under NDA for Enterprise customers. We support data residency in US and EU. For HIPAA we offer a BAA on Enterprise; for GDPR we provide DPA and process data per our privacy policy. Audit logs are available in the dashboard for 90 days (longer on request)."),
    ("Mobile app usage", "Our mobile app is available for iOS and Android. Sign in with the same credentials as the web app. Push notifications can be enabled in app settings. Offline mode caches recent data for viewing; edits sync when you are back online. For security, the app requires a PIN or biometric after 5 minutes of inactivity. SSO is supported via device browser or embedded web view depending on your IdP."),
    ("Keyboard shortcuts", "Quick reference: Ctrl+K or Cmd+K for command palette. Ctrl+/ for keyboard shortcuts help. In the editor: Ctrl+S to save, Ctrl+Enter to run. In the file tree: Ctrl+N new file, Ctrl+Shift+N new folder. We support custom shortcuts in Settings → Shortcuts. Most actions are available from the command palette; type to search."),
    ("Integrations overview", "We integrate with Slack, Teams, Jira, GitHub, and 50+ tools. Go to Settings → Integrations to connect. Most integrations use OAuth; you authorize once and we store the token securely. For GitHub we support both cloud and enterprise server. Webhooks are available for custom integrations. Need something we do not support? Request it in our community forum or contact sales for custom work."),
    ("Deployment and regions", "You can deploy in US East, US West, EU West, and Asia Pacific. Choose a region when creating a project; it cannot be changed later. Data stays in the selected region for compliance. For low latency, pick the region closest to your users. Enterprise can use private regions or on-premise deployment; contact sales for details."),
    ("Audit logs", "Audit logs record who did what and when: logins, config changes, data access. View them in Settings → Audit log. Filters include user, action, and date. Logs are retained for 90 days by default; Enterprise can extend retention. Export to CSV or send to your SIEM via our log forwarder. For compliance we can provide attestation letters."),
    ("Getting started guide", "Welcome. After signup, create a project and invite your team from Settings → Members. Install the CLI with the one-liner from the dashboard; use it to run jobs and sync config. Our docs and tutorials are in the Help menu. For a guided tour, click your profile and select Product tour. Need help? Use the in-app chat or email support@example.com. Community forum and status page are linked in the footer."),
    ("CLI installation", "Install the CLI: Windows (PowerShell) run the install script from the dashboard. macOS and Linux use curl. After install, run our login command to authenticate. The CLI stores credentials in your home directory. For CI/CD use a service account key or environment variables. See our docs for advanced options like custom config paths and proxies."),
    ("Service status", "We publish real-time status at status.example.com. Subscribe to get email or SMS for incidents. Planned maintenance is announced at least 48 hours ahead. During incidents we post updates every 30 minutes until resolved. You can also check status from the in-app banner. Enterprise customers have a dedicated status channel and SLA credits for downtime."),
    ("Data retention", "By default we retain your data for the life of your account. You can set retention policies in Settings → Data: for example delete runs older than 90 days. Deleted data is purged within 30 days from our backups. For GDPR right to erasure we process requests within 30 days. Export your data before deletion if you need a copy."),
    ("Team roles and permissions", "Roles: Viewer (read only), Editor (read and write), Admin (full access including billing and members). Owners have full control and can transfer ownership. Invite members by email; they get a link to join. You can restrict access by project or by feature. SSO and SCIM are available on Enterprise for automatic provisioning and deprovisioning."),
    ("Notification preferences", "Configure notifications in Settings → Notifications. Choose email, Slack, or in-app for: run failures, billing, security alerts, and product updates. You can set quiet hours to avoid after-hours pings. Digest mode sends a daily or weekly summary instead of per-event. Enterprise can route alerts to a webhook or PagerDuty."),
    ("Security best practices", "Use a strong password and enable 2FA. Rotate API keys periodically and use separate keys per environment. Restrict member access to the minimum needed. Review audit logs for anomalies. Do not commit API keys to source control; use environment variables or a secrets manager. We support IP allowlisting and VPC peering on Enterprise."),
    ("Pricing tiers", "Free: for individuals and small projects; 100 API calls/min and 5GB storage. Pro: for teams; 1000 API calls/min, 50GB storage, and email support. Enterprise: unlimited usage, SLA, SSO, and dedicated support. Prices are on our website. Annual billing saves 20%. Education and nonprofit discounts are available; apply from the pricing page."),
    ("Cancelling your subscription", "You can cancel anytime from Billing → Subscription. Cancellation takes effect at the end of the current billing period; you keep access until then. No refund for the unused portion of the current period. After cancellation your data remains available for 30 days; export it if you need to keep it. To reactivate, resubscribe before the 30-day grace period ends."),
    ("Uploading large files", "The web app supports files up to 500 MB. For larger files use the CLI or the resumable upload API. We recommend chunked upload for files over 100 MB. Uploads are encrypted in transit and at rest. For bulk uploads use our batch API to avoid rate limits. Storage counts toward your plan limit; check usage in Settings → Usage."),
    ("Version history", "We keep version history for configs and key assets. View history in the item menu and restore a previous version if needed. History is retained for 30 days on Pro and 90 days on Enterprise. Deleted items can be restored from trash within the retention window. Version history is not available for free tier; upgrade to enable it."),
    ("Custom domains", "Enterprise customers can use a custom domain for the app and API. Add your domain in Settings → Domains and follow the DNS instructions. We provide a certificate; you must add the CNAME record. Propagation can take up to 48 hours. Custom domains are optional; our default domain always works."),
    ("SLA and uptime", "We guarantee 99.9% uptime for Pro and 99.95% for Enterprise, excluding planned maintenance. If we fall short, you get service credits: 10% for 99.0–99.9%, 25% for 95–99%, 50% for under 95%. Request credits within 30 days of the incident. Credits are applied to the next invoice. See the full SLA in our terms."),
    ("Contacting support", "Free users can use the community forum and docs. Pro and Enterprise get email support; response time is within 24 hours for Pro and 4 hours for Enterprise. Include your account email and a clear description. For outages use the in-app priority channel. Enterprise has a dedicated success manager and optional premium support with a guaranteed response time."),
    ("Glossary", "API key: secret used to authenticate API requests. SSO: single sign-on. 2FA: two-factor authentication. TOTP: time-based one-time password. Webhook: HTTP callback for events. Rate limit: maximum requests per time window. IdP: identity provider. SAML: Security Assertion Markup Language. BAA: business associate agreement. DPA: data processing agreement."),
]

# Example queries users might ask (semantic search should find the right doc)
QUERIES = [
    "How do I reset my password?",
    "What is your refund policy?",
    "API rate limits and 429 errors",
    "How to set up two-factor authentication",
    "Export my data or get a backup",
    "Where do I find my invoices?",
    "Set up SSO with Okta or Azure",
    "Webhook retries and signature verification",
    "Too many database connections",
    "Slow query performance and indexes",
    "SOC 2 and compliance",
    "Keyboard shortcuts in the editor",
    "Which regions can I deploy in?",
    "How do I cancel my subscription?",
]


def main():
    demo_lattice = os.path.join(_script_dir, "demo_kb.lattice")
    if os.path.exists(demo_lattice):
        try:
            os.remove(demo_lattice)
        except Exception:
            pass

    print("SYNRIX RAG Demo – Knowledge base")
    print("=================================\n")
    print("Use case: support/docs corpus -> answer questions via semantic search.\n")

    synrix_client = None
    try:
        from synrix.ai_memory import AIMemory
        synrix_client = AIMemory(lattice_path=demo_lattice)
        print("[OK] SYNRIX backend (C engine does chunking; one call per doc).\n")
    except ImportError:
        print("[WARN] synrix not installed; using in-memory fallback.\n")
    except Exception as e:
        print(f"[WARN] SYNRIX init failed: {e}; using in-memory fallback.\n")

    from synrix_rag import RAGMemory

    rag = RAGMemory(
        collection_name="support_kb",
        embedding_model="local",
        synrix_client=synrix_client,
    )

    # Ingest
    total_chars = 0
    chunked_count = 0
    t0 = time.perf_counter()
    print(f"Ingesting {len(KB_DOCS)} documents...")
    for title, text in KB_DOCS:
        doc_id = rag.add_document(text=text, metadata={"title": title})
        size = len(text.encode("utf-8"))
        total_chars += size
        # Stored payload = JSON(text + metadata + embedding); almost always > 512 so engine uses chunked
        if size > 511:
            chunked_count += 1
    ingest_time = time.perf_counter() - t0
    print(f"   Done in {ingest_time:.2f}s ({len(KB_DOCS)} docs, {total_chars:,} chars, {chunked_count} stored via chunked storage).\n")

    # Search
    print("Sample queries (semantic search):\n")
    for q in QUERIES[:6]:
        t0 = time.perf_counter()
        results = rag.search(q, top_k=2)
        elapsed = time.perf_counter() - t0
        top = results[0] if results else {}
        title = top.get("metadata", {}).get("title", "-")
        score = top.get("score", 0)
        print(f"   Q: {q}")
        print(f"   -> Top: \"{title}\" (score={score:.3f}, {elapsed*1000:.0f}ms)\n")

    # Context for LLM
    print("Context for LLM (single query):")
    ctx = rag.get_context("How do I get a refund?", top_k=2)
    print(f"   {ctx[:400]}...\n")

    print("Demo complete. RAG with a decent-sized kb works; large docs use C-engine chunked storage.")
    if synrix_client and os.path.exists(demo_lattice):
        print(f"Lattice: {demo_lattice}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
