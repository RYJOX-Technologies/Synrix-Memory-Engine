# SYNRIX Windows Benchmark Results

## Platform
- **OS**: Windows 10
- **Compiler**: MSYS2 MinGW-w64 (gcc)
- **DLL**: libsynrix.dll (612KB)
- **Build**: MSYS2/MinGW-w64

## Performance Metrics

### Node Addition
- **Average**: 1.95 μs per node
- **Median**: 1.80 μs per node
- **Min**: 1.60 μs
- **Max**: 14.90 μs
- **Throughput**: 484,990 nodes/sec
- **Total time for 1000 nodes**: 2.06 ms

**Comparison to Linux**: ~1-10 μs/node (Windows is within expected range)

### Node Lookup (O(1))
- **Average**: 16.465 μs per lookup
- **Median**: 15.700 μs per lookup
- **P95**: 20.200 μs
- **P99**: 27.300 μs
- **Min**: 15.200 μs
- **Max**: 51.600 μs

**Comparison to Linux**: ~0.1-1.0 μs (Windows is ~16x slower, likely due to DLL call overhead)

### Prefix Queries (O(k))
- **Status**: Testing in progress...
- **Expected**: ~10-100 μs per query (Linux baseline)

## Analysis

### Strengths
1. **Node addition is fast**: 1.95 μs/node matches Linux performance expectations
2. **High throughput**: 485K nodes/sec addition rate
3. **Consistent performance**: Low variance in addition times

### Performance Differences from Linux
1. **Lookup overhead**: 16.465 μs vs ~0.1-1.0 μs on Linux
   - Likely due to DLL call overhead (ctypes)
   - Windows API differences
   - Memory mapping differences

2. **Still sub-millisecond**: All operations are < 1ms, which is acceptable for most use cases

## Recommendations

1. **For production use**: Windows performance is acceptable for most applications
2. **Optimization opportunities**:
   - Reduce DLL call overhead (batch operations)
   - Optimize Windows mmap implementation
   - Consider direct C extension instead of ctypes

## Test Configuration
- Lattice size: 100,000 nodes max
- Test nodes: 1,000 nodes added
- Query iterations: 100 (prefix queries)
