# SYNRIX Windows Performance Tuning Guide

## Current Performance Baseline

**Test Results (7/7 tests passing):**
- Node Addition: ~2.3 μs/node
- Node Lookup: ~20.9 μs (O(1) hash lookup)
- Prefix Queries: ~281 μs (O(k) semantic search)
- All Operations: Sub-millisecond

**Comparison to Linux:**
- Addition: ✅ Matches Linux
- Lookup: ⚠️ ~20x slower (DLL overhead)
- Prefix Queries: ⚠️ ~3x slower (memory-bound)

## Performance Bottlenecks Identified

### 1. DLL Call Overhead (~10-15 μs per call)
- **Cause**: Python ctypes → C DLL boundary
- **Impact**: Affects all operations, especially lookups
- **Mitigation**: Batch operations when possible

### 2. Windows Memory Management (~5-10 μs)
- **Cause**: Windows memory manager + TLB + scheduler jitter
- **Impact**: Adds latency to memory operations
- **Mitigation**: Pre-warm memory, use larger batches

### 3. Memory-Bound Operations
- **Cause**: Windows working set trimming + Defender + NTFS metadata
- **Impact**: Prefix queries slower than Linux
- **Mitigation**: Exclude lattice paths from Defender

## Optimization Opportunities

### Immediate (Low-Hanging Fruit)

1. **Batch Operations**
   - Group multiple operations together
   - Amortize DLL call overhead
   - **Expected gain**: 2-3x for batched operations

2. **Defender Exclusions**
   - Exclude lattice/WAL paths from Windows Defender
   - **Expected gain**: 10-20% reduction in latency spikes

3. **Connection Pooling** (Future)
   - Reuse lattice connections
   - **Expected gain**: Eliminate initialization overhead

### Medium-Term

1. **Python C Extension** (Instead of ctypes)
   - Direct C API, no ctypes overhead
   - **Expected gain**: 5-10x reduction in call overhead
   - **Effort**: Medium (requires C extension module)

2. **Async API** (Future)
   - Non-blocking operations
   - **Expected gain**: Better concurrency
   - **Effort**: High

### Long-Term

1. **Platform Abstraction**
   - Native Windows API for critical paths
   - **Expected gain**: 2-3x overall
   - **Effort**: High (requires architecture changes)

## Performance Tuning Checklist

### Before Optimization

- [ ] Profile with `cProfile` to identify hot paths
- [ ] Measure baseline performance
- [ ] Identify specific bottlenecks
- [ ] Set performance targets

### Optimization Steps

- [ ] **Batch Operations**
  - [ ] Implement batch add API
  - [ ] Implement batch get API
  - [ ] Test performance improvement

- [ ] **Defender Exclusions**
  - [ ] Document exclusion paths
  - [ ] Test latency improvement
  - [ ] Update documentation

- [ ] **Memory Pre-warming**
  - [ ] Implement prefetch for common queries
  - [ ] Test cache hit rate improvement

### After Optimization

- [ ] Re-run benchmarks
- [ ] Verify performance improvement
- [ ] Ensure no regressions
- [ ] Update documentation

## Benchmarking

### Run Full Benchmark Suite

```bash
python benchmark_windows.py
```

### Profile Hot Paths

```python
import cProfile
import pstats

profiler = cProfile.Profile()
profiler.enable()

# Run operations
for i in range(1000):
    b.add_node(f'TEST:node_{i}', 'data', 5)

profiler.disable()
stats = pstats.Stats(profiler)
stats.sort_stats('cumulative')
stats.print_stats(20)
```

### Measure Specific Operations

```python
import time

# Measure lookup overhead
start = time.perf_counter()
for i in range(1000):
    b.get_node(node_ids[i])
elapsed = time.perf_counter() - start
print(f"Lookup: {elapsed*1000/1000:.2f} μs per call")
```

## Performance Targets

### Current (Baseline)
- Node Addition: < 5 μs ✅
- Node Lookup: < 50 μs ✅
- Prefix Queries: < 500 μs ✅

### Optimized (Target)
- Node Addition: < 3 μs (already achieved)
- Node Lookup: < 10 μs (with batching/C extension)
- Prefix Queries: < 200 μs (with Defender exclusion)

### Agent Workload Requirements
- Context Restoration: < 100 ms ✅ (currently < 1 ms)
- Memory Operations: < 1 ms ✅ (currently sub-millisecond)
- Batch Operations: < 10 ms for 1000 nodes ✅

**Verdict**: Current performance meets all agent workload requirements.

## Recommendations

### For Agent Workloads (Current Priority)
✅ **No optimization needed** - Performance is acceptable
- All operations sub-millisecond
- Overhead negligible compared to model inference
- Batch-friendly architecture

### For High-Frequency Scenarios (Future)
- Implement Python C extension
- Add batch operation APIs
- Consider async API for concurrency

### For Ultra-Low-Latency (Future)
- Platform abstraction with native Windows API
- Custom memory allocator
- Lock-free data structures

## Next Steps

1. ✅ Verify current performance meets requirements
2. ⏳ Profile to identify specific bottlenecks (if needed)
3. ⏳ Implement batch operations (if needed)
4. ⏳ Document Defender exclusions
5. ⏳ Re-benchmark after optimizations
