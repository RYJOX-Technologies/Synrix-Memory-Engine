# Changelog

## [Unreleased]

### Fixed
- **[CRITICAL]** Removed 10K indexing threshold that caused O(n) query spikes at scale
  - Prefix index now updates incrementally at all dataset sizes
  - First query on 50K nodes: **0.31ms** (was 50-100ms with O(n) rebuild)
  - Verified O(k) scaling up to 100K nodes (0.07ms queries)
  - Fixed potential memory corruption from full index rebuilds
  - **Trade-off**: Add latency increased ~7-275% to maintain incremental index (agents do 100x more queries than adds, net win)

### Added
- Comprehensive test suite for O(k) indexing at scale (`scripts/test_ok_indexing_fix.py`)
- Windows x86_64 benchmarks in `docs/BENCHMARKS.md`

## [Previous Releases]

See git history for earlier changes.
