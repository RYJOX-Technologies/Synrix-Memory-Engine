# Recording Checklist - 60-Second Morphos Demo

## Pre-Recording Setup

### Environment
- [ ] Terminal window is clear and ready
- [ ] Font size is large enough (readable on video)
- [ ] Terminal is full-screen or appropriately sized
- [ ] Screen recorder is ready (OBS, QuickTime, etc.)

### Files
- [ ] `libsynrix.so` exists in `python-sdk/` directory
- [ ] `agent_demo_REAL.py` is ready
- [ ] `INTEGRATION_EXAMPLE.py` is ready
- [ ] `morphos_demo_60sec.sh` is executable

### Test Run
- [ ] Run `./QUICK_START_RECORDING.sh` or `./morphos_demo_60sec.sh`
- [ ] Demo completes successfully
- [ ] All metrics are visible
- [ ] Timing is acceptable (~10-15 seconds for demo)

## Recording

### Start
- [ ] Start screen recorder
- [ ] Clear terminal: `clear`
- [ ] Run: `./morphos_demo_60sec.sh`
- [ ] Narrate using `PRESENTER_SCRIPT.md`

### During Recording
- [ ] Speak clearly and at good pace
- [ ] Highlight key metrics as they appear
- [ ] Point out integration code
- [ ] Emphasize "3 lines of code"
- [ ] Show persistence demo clearly

### End
- [ ] Stop screen recorder
- [ ] Check video quality

## Post-Recording

### Video Check
- [ ] All text is readable
- [ ] Demo runs smoothly (no lag)
- [ ] Metrics are visible and clear
- [ ] Integration code is readable
- [ ] Timing is ~60 seconds total

### Files to Include
- [ ] Demo video (main deliverable)
- [ ] `INTEGRATION_EXAMPLE.py` (for Morphos to copy)
- [ ] Package location: `python-sdk/synrix/`

## Quick Commands

### Setup
```bash
cd examples
export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH
chmod +x morphos_demo_60sec.sh
```

### Test Run
```bash
./morphos_demo_60sec.sh
```

### Record
```bash
# Option 1: Use quick start script
./QUICK_START_RECORDING.sh

# Option 2: Manual
clear
./morphos_demo_60sec.sh
```

## Troubleshooting

### Demo fails to start
- Check: `ls -lh ../libsynrix.so`
- Fix: `export LD_LIBRARY_PATH=..:$LD_LIBRARY_PATH`

### Demo too slow
- Reduce task count in `agent_demo_REAL.py`: `create_test_tasks(count=10)`

### Text too small
- Increase terminal font size
- Zoom terminal window
- Use full-screen terminal

### Video quality issues
- Use high resolution (1080p or higher)
- Ensure good contrast
- Use readable font (monospace, large size)

## Success Criteria

✅ Demo runs smoothly  
✅ All metrics visible  
✅ Integration code clear  
✅ Persistence demonstrated  
✅ Total time ~60 seconds  
✅ Ready to share with Morphos

