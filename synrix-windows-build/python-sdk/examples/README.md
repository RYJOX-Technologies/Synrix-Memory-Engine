# AION Omega Examples

Simple, standalone examples showing how to use AION Omega.

## Simple RAG Example

**File**: `simple_rag.py`

Standalone RAG pipeline without LangChain dependencies.

```bash
# Start SYNRIX server first
# Then run:
python examples/simple_rag.py
```

**What it does**:
1. Stores documents in SYNRIX
2. Queries SYNRIX for relevant documents
3. Shows how to generate answers (mock for demo)

**Use case**: Understanding the basic RAG workflow with AION Omega.

## LangChain Integration

**Location**: `../integrations/langchain/`

See `integrations/langchain/README.md` for LangChain integration examples.

## OpenAI-Compatible API

**Location**: `../integrations/openai_compatible/`

See `integrations/openai_compatible/README.md` for OpenAI-compatible API usage.

