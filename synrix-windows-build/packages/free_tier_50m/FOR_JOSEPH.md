# SYNRIX Unlimited - For Joseph

## What You Have

This is SYNRIX - a simple way to save and remember things in your Python programs. Think of it like a notebook that never gets lost.

## Installation (Super Easy!)

### Step 1: Find the File
Look for a file called `installer.bat` in this folder.

### Step 2: Double-Click It
Just double-click `installer.bat` (click it twice fast).

### Step 3: Wait
You'll see some text scroll by. This is normal! It's installing SYNRIX.

### Step 4: Press Enter
When it says "Press Enter to exit", press Enter.

### Step 5: Done!
That's it! SYNRIX is now installed.

## How to Use It (Super Simple!)

After installation, you can use SYNRIX in **any Python script**. Here's how:

### The Two Lines You Always Need

```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
```

That's it! Now you have a memory notebook.

### Save Something

```python
memory.add("My Note", "This is what I want to remember")
```

### Get It Back

```python
results = memory.query("My")
for r in results:
    print(r['data'])
```

## Complete Example

Here's a complete example you can copy and paste:

```python
from synrix.ai_memory import get_ai_memory

# Get your memory
memory = get_ai_memory()

# Save some things
memory.add("Project Name", "My Cool Project")
memory.add("Project Status", "In Progress")
memory.add("Project Deadline", "December 31")

# Find all project things
project_stuff = memory.query("Project")
for item in project_stuff:
    print(f"{item['name']}: {item['data']}")

# Count how many things you have
total = memory.count()
print(f"\nTotal things saved: {total}")
```

## What Happens Behind the Scenes?

1. **First time**: SYNRIX creates a file at `C:\Users\Joseph\.synrix_ai_memory.lattice`
2. **Every time**: SYNRIX opens that file and saves your new stuff
3. **Your stuff stays there**: Even if you close Python, your data is saved!

You don't need to worry about this file - SYNRIX handles everything for you.

## Quick Reference

```python
# Always start with these two lines:
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()

# Save something:
memory.add("Name", "What you want to remember")

# Find things by name:
results = memory.query("Name")

# Count everything:
count = memory.count()

# Get one thing by its number:
thing = memory.get(1)
```

## Test It Works

After installing, you can test it by running:

```bash
python test_installation.py
```

This will verify that everything is working correctly!

**Or test it manually:**
```python
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()
memory.add("TEST", "Works!")
print("Success!")
```

## What's Different from Free Tier?

| Feature | Free Tier | Unlimited (This) |
|---------|-----------|------------------|
| How many things you can save | 50,000 | **Unlimited!** |
| Use case | Testing | **Production** |

## Need More Help?

- `SIMPLE_GUIDE.md` - Even simpler explanation (like you're 5 years old!)
- `USAGE_GUIDE.md` - More detailed examples
- `example_usage.py` - Working example you can run

## Summary

1. **Install**: Double-click `installer.bat`
2. **Use**: Add these two lines to any Python script:
   ```python
   from synrix.ai_memory import get_ai_memory
   memory = get_ai_memory()
   ```
3. **Save**: `memory.add("Name", "Data")`
4. **Find**: `memory.query("Name")`

**That's it!** No complicated setup, no configuration files, no paths to worry about. Just install once, then use it anywhere!

---

**Ready to use!** Just double-click `installer.bat` and you're done.
