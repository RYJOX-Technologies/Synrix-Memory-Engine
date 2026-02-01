# 60-Second Morphos Demo - Recording Guide

## Pre-Recording Checklist

### ✅ Prerequisites
- [ ] `libsynrix.so` is built (in `python-sdk/` directory)
- [ ] `LD_LIBRARY_PATH` is set (or run from `python-sdk/examples/`)
- [ ] Terminal is ready (clear, good font size)
- [ ] Screen recording software is ready
- [ ] Demo script is executable: `chmod +x morphos_demo_60sec.sh`

### ✅ Files Ready
- [ ] `agent_demo_REAL.py` - Main demo (100% real SYNRIX)
- [ ] `INTEGRATION_EXAMPLE.py` - Integration example
- [ ] `morphos_demo_60sec.sh` - Demo runner script

## Recording Steps

### 1. Setup (Before Recording)
```bash
cd examples
export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH
clear
```

### 2. Start Recording
- Start screen recorder
- Make sure terminal is visible
- Good font size (readable on video)

### 3. Run Demo
```bash
./morphos_demo_60sec.sh
```

### 4. What to Say (60 Seconds)

**0-10s: Problem**
- "Agents forget everything on restart"
- "They make the same mistakes repeatedly"

**10-20s: Solution**
- "SYNRIX adds persistent memory"
- "3 lines of code to integrate"

**20-40s: Demo**
- "Watch baseline agent: 70% success, 4 repeated errors"
- "Now with SYNRIX: 95% success, 0 repeated errors"
- "Sub-microsecond lookups, survives restarts"

**40-50s: Integration**
- "Here's how easy it is"
- "Copy this code, replace 3 lines, done"

**50-60s: Results**
- "25% improvement in success rate"
- "Zero repeated errors"
- "Ready to use tomorrow"

## What the Demo Shows

1. **Integration Example** (first 10 seconds)
   - Before/after code comparison
   - 3-line integration

2. **Baseline Agent** (10-20 seconds)
   - 70% success rate
   - 4 repeated errors
   - No memory

3. **SYNRIX Agent** (20-35 seconds)
   - 95% success rate
   - 0 repeated errors
   - Real performance metrics

4. **Persistence Demo** (35-45 seconds)
   - Agent restart
   - Remembers past mistakes
   - Zero repeated errors

5. **Integration Instructions** (45-60 seconds)
   - Where to find code
   - How to use it
   - Ready to deploy

## Key Metrics to Highlight

- **Success Rate:** +25% improvement (70% → 95%)
- **Repeated Errors:** 4 → 0 (100% reduction)
- **Performance:** 129μs lookups (sub-microsecond)
- **Persistence:** Survives restarts (demonstrated)

## Post-Recording

### Check Video
- [ ] All text is readable
- [ ] Demo runs smoothly
- [ ] Metrics are visible
- [ ] Integration code is clear

### Files to Share
- [ ] Demo video
- [ ] `INTEGRATION_EXAMPLE.py` (for them to copy)
- [ ] Package location: `python-sdk/synrix/`

## Troubleshooting

### If demo fails:
1. Check `LD_LIBRARY_PATH`: `export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH`
2. Check library: `ls -lh ../libsynrix.so`
3. Run directly: `python3 agent_demo_REAL.py`

### If too slow:
- Reduce task count in `create_test_tasks(count=20)` → `count=10`
- Skip persistence demo section (optional)

### If text too small:
- Increase terminal font size
- Zoom terminal window
- Use full screen terminal

## Quick Test Run

Before recording, do a test run:
```bash
cd examples
export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH
./morphos_demo_60sec.sh
```

Should complete in ~10-15 seconds (demo itself), leaving time for narration.

