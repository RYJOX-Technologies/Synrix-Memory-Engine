# Monday Demo - The Crazy Interactive Demo 🚀

## What This Demo Shows

This interactive demo showcases the full power of SYNRIX's KG-driven architecture:

1. **Natural Language → Assembly Generation**
   - Type requests in plain English
   - System queries knowledge graph
   - Generates ARM64 assembly code in real-time

2. **Self-Improving Code Generation**
   - Tests generated code
   - Learns from failures
   - Stores fixes in KG
   - Regenerates with improvements

3. **Live Performance Metrics**
   - Real-time latency measurements
   - Comparison with traditional approaches
   - Speedup calculations

4. **Knowledge Graph Growth**
   - Patterns learned
   - Fixes stored
   - Statistics visualization

## Quick Start

```bash
cd python-sdk
make -f Makefile.synrix_cli  # Build CLI if needed
python3 examples/monday_demo_crazy.py
```

## Demo Flow

1. **Initialization**
   - Creates lattice at `~/.synrix/demo.lattice`
   - Pre-seeds with common patterns

2. **Automated Scenarios**
   - Runs 3 pre-defined scenarios
   - Shows generation → testing → learning cycle

3. **Interactive Mode**
   - Enter your own requests
   - See real-time code generation
   - Watch the system learn

## Example Requests

- `generate a memory copy kernel for ARM64`
- `create an arithmetic operation kernel`
- `generate a sorting algorithm`
- `create a search function`

## Features

- ✅ Colorful terminal UI
- ✅ Real-time metrics
- ✅ Self-improvement loop
- ✅ KG statistics
- ✅ Performance comparisons

## What Makes It "Crazy"

1. **Live Learning**: System improves as you use it
2. **Real-time Generation**: Assembly code in milliseconds
3. **Visual Feedback**: See the KG grow and learn
4. **Performance**: 8-12µs lookups vs 200-1000ms traditional
5. **Interactive**: Type anything, get code

## Tips for the Demo

- Start with simple requests
- Watch the metrics update in real-time
- Try making the same request twice (see improvement)
- Check the KG stats to see growth

Enjoy the demo! 🎉

