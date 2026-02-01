# Mock Customer Demo

## Overview

This demo shows **real-world value** by simulating a customer support team using SYNRIX for their knowledge base.

**Scenario**: Acme Corp Customer Support Team
- 10 support agents
- 60 queries per hour per agent
- Need instant access to product documentation

## What It Demonstrates

1. **Real-World Use Case**: Customer support knowledge base
2. **Performance**: 40-50× faster than cloud (2-4ms vs 100-200ms)
3. **Business Impact**: Time saved, cost predictability, data privacy
4. **ROI Analysis**: Quantified benefits (agent hours saved, cost savings)

## Running the Demo

```bash
cd NebulOS-Scaffolding/python-sdk/examples
source /mnt/nvme/aion-omega/python-env/bin/activate
export LD_LIBRARY_PATH=/mnt/nvme/aion-omega/NebulOS-Scaffolding/python-sdk:$LD_LIBRARY_PATH
python3 mock_customer_demo.py
```

## What You'll See

1. **Scenario Setup**: Problem statement and solution
2. **Knowledge Base Indexing**: 18 support documents loaded
3. **Agent Queries**: 5 realistic support queries with timing
4. **Performance Comparison**: p50/p95 latencies vs cloud
5. **Business Impact**: Time saved, cost analysis, ROI

## Key Metrics Shown

- **Response Time**: 2-4ms (vs 100-200ms cloud)
- **Speedup**: 40-50× faster
- **Time Saved**: ~1 hour per day for 10-agent team
- **Cost**: Fixed (no per-query pricing)
- **Privacy**: Data stays local

## Use Cases This Applies To

1. **Customer Support Teams** (primary demo)
2. **Internal Knowledge Bases**
3. **Document Search Systems**
4. **FAQ/Help Systems**
5. **Compliance/Regulatory Search**

## Customization

To use with your own data:

1. Replace `knowledge_base` list with your documents
2. Adjust `agent_queries` to match your use case
3. Update business metrics (agents, queries/hour) in the ROI section

## Next Steps

After running this demo:
- See `MIGRATION_GUIDE.md` for how to migrate your app
- Try `example_real_world_migration.py` for a working example
- Use `quick_start_synrix.sh` to start your own server
