# SYNRIX - Super Simple Guide

## What is SYNRIX?

SYNRIX is like a notebook for your computer. You can write things in it, and it remembers them forever.

## Step 1: Install It

1. Find the file called `installer.bat`
2. Double-click it (click it twice fast)
3. Wait for it to finish (you'll see some text scroll by)
4. When it says "Done!" or "Press Enter to exit", press Enter
5. **That's it!** You're done installing.

## Step 2: Use It in Your Python Code

Open any Python file (like `my_script.py`) and type this:

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
memory.add("My Note", "This is what I want to remember")
```

**That's it!** You just saved something to memory.

## Step 3: Get It Back Later

To get your note back:

```python
from synrix.ai_memory import get_ai_memory

memory = get_ai_memory()
results = memory.query("My")
print(results[0]['data'])  # Prints: "This is what I want to remember"
```

## Real Example

Here's a complete example you can copy and paste:

```python
from synrix.ai_memory import get_ai_memory

# Get your memory notebook
memory = get_ai_memory()

# Write something down
memory.add("Shopping List", "Buy milk")
memory.add("Shopping List", "Buy eggs")
memory.add("Shopping List", "Buy bread")

# Read it back
shopping = memory.query("Shopping List")
for item in shopping:
    print(item['data'])

# This prints:
# Buy milk
# Buy eggs
# Buy bread
```

## What Happens?

1. **First time you use it**: SYNRIX creates a file in your home folder to store your notes
2. **Every time after**: SYNRIX opens that file and adds your new notes
3. **Your notes stay there forever**: Even if you close Python and come back later, your notes are still there!

## Common Things to Do

### Save a Note
```python
memory.add("Note Name", "What you want to remember")
```

### Find All Notes Starting With "Shopping"
```python
results = memory.query("Shopping")
for r in results:
    print(r['data'])
```

### Count How Many Notes You Have
```python
count = memory.count()
print(f"I have {count} notes")
```

### Get One Specific Note by Number
```python
note = memory.get(1)  # Get note number 1
print(note['data'])
```

## Where Are My Notes Stored?

Your notes are saved in a file on your computer:
- **Windows**: `C:\Users\YourName\.synrix_ai_memory.lattice`

You don't need to worry about this file - SYNRIX handles it for you! It's like a magic notebook that never gets lost.

## Troubleshooting

### "I get an error when I try to use it"

**Problem**: You see `ModuleNotFoundError: No module named 'synrix'`

**Solution**: You need to install it first! Go back to Step 1 and double-click `installer.bat`

### "It says DLL not found"

**Problem**: The installer didn't copy all the files

**Solution**: Make sure you ran `installer.bat` completely. Try running it again.

### "My notes disappeared"

**Problem**: You might be looking in the wrong place

**Solution**: Your notes are saved automatically. Make sure you're using the same Python script, or check the file at `C:\Users\YourName\.synrix_ai_memory.lattice`

## Quick Reference Card

```python
# 1. Always start with these two lines:
from synrix.ai_memory import get_ai_memory
memory = get_ai_memory()

# 2. Save something:
memory.add("Name", "Data")

# 3. Find things:
results = memory.query("Name")

# 4. Count things:
count = memory.count()

# 5. Get one thing:
thing = memory.get(1)
```

## That's It!

You now know how to use SYNRIX! It's like a notebook that:
- [OK] Never gets lost
- [OK] Remembers everything
- [OK] Works in any Python script
- [OK] No setup needed after installation

Just remember:
1. Install once (double-click `installer.bat`)
2. Use `get_ai_memory()` in your code
3. Use `memory.add()` to save things
4. Use `memory.query()` to find things

**Happy coding.**
