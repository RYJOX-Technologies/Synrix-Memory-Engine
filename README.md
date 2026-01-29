# Synrix Memory Engine

**Synrix is a local-first, Qdrant-compatible memory engine for AI systems.**  
It runs entirely on your machine and provides fast, predictable retrieval without cloud latency or vendor lock-in.

If you already use Qdrant (LangChain), Synrix is a drop-in local replacement.

Think of it as a memory engine optimized for predictable, targeted recall rather than brute-force similarity scans.

[![LangChain Compatible](https://img.shields.io/badge/langchain-compatible-brightgreen)](https://python.langchain.com/)
[![OpenAI API](https://img.shields.io/badge/api-openai--compatible-blue)](https://platform.openai.com/)
[![License](https://img.shields.io/badge/license-MIT%20%2F%20Proprietary-lightgrey)](LICENSE)

## What Problem Does This Solve?

Your AI needs to remember things. Most solutions either:
- Send everything to the cloud (privacy concerns, latency)
- Use vector databases that require heavy indexing and tuning as datasets grow
- Lock you into vendor ecosystems

Synrix is different. It runs locally, queries scale with results (not data size), and you own everything.

## How It Works

```
Your App → SDK / REST → Synrix Engine → Memory-Mapped Local Storage
```

Synrix runs entirely locally. The SDK and evaluation engine are open source (MIT), and the production engine is licensed for scale.

## Quick Start

### 1. Download the Engine

Grab the engine for your platform from the [releases page](https://github.com/YOUR_REPO/releases):
- Linux ARM64 (Jetson): `synrix-server-evaluation-0.1.0-linux-arm64` (free, MIT-licensed, full source available)
- Linux ARM64 (Jetson): `synrix-server-0.1.0-linux-arm64` (production, requires license)

### 2. Run It

```bash
chmod +x synrix-server-evaluation-0.1.0-linux-arm64
./synrix-server-evaluation-0.1.0-linux-arm64 --port 6334
```

You'll see it start up. The evaluation engine is provided with full source code under the MIT license.

**Evaluation vs Production**

- **Evaluation binary** (`synrix-server-evaluation-*`): Free, full source code available (MIT), limited to 100k nodes / ~1GB memory. Perfect for development and testing.
- **Production binary** (`synrix-server-*`): Requires commercial license, unlimited nodes. The production engine removes evaluation limits and is licensed for production deployments.
- The evaluation engine exists so teams can audit behavior and performance before committing to production.
- No signup or license key required to try Synrix.

### 3. Install the Python SDK

```bash
pip install synrix
```

Or if you want to install from source:
```bash
tar -xzf synrix-0.1.0.tar.gz
cd synrix-0.1.0
pip install -e .
```

### 4. Use It

```python
from synrix import SynrixClient

client = SynrixClient(host="localhost", port=6334)

# Create a collection
client.create_collection("my_memories", vector_dim=128)

# Store something
client.add_node("ISA_ADD", "Addition operation", collection="my_memories")

# Query it
results = client.query_prefix("ISA_", collection="my_memories")
print(f"Found {len(results)} nodes")
```

That's it. You're storing and querying AI memory locally.

## What Makes This Different?

**O(k) queries**: Many vector databases rely on approximate global search that degrades as datasets grow or requires heavy indexing and tuning. Synrix queries scale with the number of matching results (O(k)), not total dataset size. If you have 1 million nodes but only 100 match your query, it only checks those 100.

**Local-first**: Everything runs on your machine. No network calls, no cloud dependency, no data leaving your computer.

**Fast and predictable**: 
- Microsecond-scale local lookups
- No network variance
- No cold starts
- No per-query billing latency

In typical local setups, this results in materially lower end-to-end latency than cloud-hosted vector databases.

**Qdrant-compatible**: If you're already using Qdrant, Synrix is a drop-in replacement. Same API, same behavior, just faster and local.

Considering Synrix for production? See "Evaluating for Production" below.

## Use Cases

**RAG (Retrieval Augmented Generation)**: Store document embeddings, search them when users ask questions, feed results to your LLM.

**AI Agent Memory**: Help your AI remember past conversations and learned patterns.

**Semantic Search**: Find similar items in your data without sending everything to a cloud service.

## Installation

### Engine Binary (Easiest)

Download the engine for your platform and run it. That's it. No dependencies, no installation, just download and go.

### Python SDK

```bash
pip install synrix
```

The SDK is MIT licensed - you can read the code, modify it, do whatever you want with it.

### Docker

```bash
docker-compose up -d
```

See `docker-compose.yml` for details.

### Build from Source

See `FRESH_INSTALL_GUIDE.md` for step-by-step instructions.

## Requirements

- Linux ARM64 (currently tested and supported on Jetson Orin Nano and similar devices)
- Python 3.8+ (for the SDK)
- ~1GB RAM (for 100K nodes)
- Disk space (~1GB per 1M nodes)

**Note**: Currently tested and supported on Linux ARM64. Other platforms are in progress.

## Platform Support

| Platform | Status |
|----------|--------|
| Linux ARM64 (Jetson) | Ready |
| Linux x86_64 | In progress |
| macOS | In progress |
| Windows | In progress |

## API Compatibility

Synrix implements the core Qdrant REST API endpoints:

**Working now:**
- `GET /collections` - List collections
- `GET /collections/{name}` - Get collection info
- `PUT /collections/{name}` - Create collection
- `DELETE /collections/{name}` - Delete collection
- `PUT /collections/{name}/points` - Upsert vectors
- `POST /collections/{name}/points/search` - Search vectors

**Coming in v0.1.1:**
- `POST /collections/{name}/points/scroll`
- `GET /collections/{name}/points/{id}`
- `POST /collections/{name}/query`

The core stuff (create, search, upsert) works and covers most real-world use cases.

## Troubleshooting

**Engine won't start?**
- Check if port 6334 is in use: `lsof -i :6334`
- Make sure the engine is executable: `chmod +x synrix-server-*`
- Check you have disk space

**Can't connect from Python?**
- Make sure the engine is running
- Test with: `curl http://localhost:6334/health`
- Check the port matches (default is 6334)

**Out of memory?**
- Reduce `--max-nodes` (default is 1M = ~1GB RAM)
- Use smaller vector dimensions
- Close other apps

## License

- **Evaluation Engine**: MIT License (full source available)
- **Python SDK**: MIT License
- **Production Engine**: Proprietary commercial license

The evaluation engine is free to use for development and testing.
Production deployments require a commercial license.

See [LICENSE](LICENSE) for details.

## Performance

- **O(k) query time** - Scales with results, not data size
- **Microsecond-scale local lookups** - No network variance
- **No cold starts** - Always ready
- **No per-query billing latency** - Predictable performance

Reproducible benchmark scripts are included in the repository.

## Getting Help

- **Installation issues?** See `FRESH_INSTALL_GUIDE.md`
- **Questions?** Open a GitHub issue
- **Found a bug?** Let us know

## Evaluating for Production?

If you're considering Synrix for production use, we offer:
- Pilot support on your hardware
- Benchmark reproduction
- Integration guidance

Open an issue or contact us to discuss a pilot.

## What's Next?

1. Download the engine
2. Install the Python SDK
3. Try the examples
4. Build something cool

**Synrix Memory Engine** - Fast, local AI memory. Your data, your machine.
