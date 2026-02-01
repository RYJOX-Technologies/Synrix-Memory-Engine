#!/usr/bin/env python3
"""
SYNRIX Assistant Memory Demo

Shows how to connect SYNRIX directly to an AI assistant (like this one)
to enable persistent learning from conversations.
"""

import sys
import os

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

from synrix.assistant_memory import (
    AssistantMemory,
    get_assistant_memory,
    query_before_responding,
    store_after_responding
)


def demo_assistant_memory():
    """
    Demo showing how an AI assistant would use SYNRIX memory.
    """
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║     SYNRIX Assistant Memory Integration Demo                     ║")
    print("║     How AI Assistants Learn from Past Conversations             ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()
    
    # Initialize memory
    print("1. Initializing SYNRIX memory for assistant...")
    memory = get_assistant_memory(use_direct=True)
    print("   ✅ Memory ready")
    print()
    
    # Simulate conversation turns
    print("2. Simulating conversation turns...")
    print()
    
    conversations = [
        {
            "user": "Write a function to add two numbers",
            "assistant": "def add(a, b):\n    return a + b",
            "success": True
        },
        {
            "user": "Write a function to add two numbers",
            "assistant": "def add(a, b)\n    return a + b",  # Missing colon
            "success": False,
            "feedback": "Missing colon after function definition"
        },
        {
            "user": "Write a function to add two numbers",
            "assistant": "def add(a, b):\n    return a + b",  # Fixed
            "success": True
        },
        {
            "user": "How do I use numpy?",
            "assistant": "You can import numpy and use it like: import numpy as np",
            "success": True
        },
        {
            "user": "Write code using numpy",
            "assistant": "arr = np.array([1,2,3])",  # Missing import
            "success": False,
            "feedback": "Need to import numpy first"
        },
    ]
    
    for i, conv in enumerate(conversations, 1):
        print(f"   Turn {i}:")
        print(f"   User: {conv['user']}")
        
        # Before responding: Query SYNRIX
        context = query_before_responding(conv['user'], memory)
        
        if context['similar_conversations']:
            print(f"   📚 SYNRIX found {len(context['similar_conversations'])} similar past conversations")
            for similar in context['similar_conversations'][:1]:
                if similar['success']:
                    print(f"      ✅ Past success: {similar['assistant_response'][:50]}...")
                else:
                    print(f"      ❌ Past failure: {similar['assistant_response'][:50]}...")
        
        if context['corrections']:
            print(f"   🔧 SYNRIX found {len(context['corrections'])} relevant corrections")
            for correction in context['corrections'][:1]:
                print(f"      Avoid: {correction['original'][:50]}...")
                print(f"      Use instead: {correction['corrected'][:50]}...")
        
        # Assistant responds
        print(f"   Assistant: {conv['assistant_response']}")
        
        # After responding: Store in SYNRIX
        store_after_responding(
            user_query=conv['user'],
            assistant_response=conv['assistant_response'],
            success=conv['success'],
            memory=memory,
            feedback=conv.get('feedback')
        )
        
        if conv['success']:
            print(f"   ✅ Stored successful response")
        else:
            print(f"   ❌ Stored failed response (will learn from this)")
            # Store correction
            if 'feedback' in conv:
                memory.store_correction(
                    original_response=conv['assistant_response'],
                    corrected_response="def add(a, b):\n    return a + b",  # Corrected version
                    error_type="syntax_error",
                    context=conv['feedback']
                )
        
        print()
    
    # Show learning summary
    print("3. SYNRIX Learning Summary:")
    print()
    summary = memory.get_learning_summary()
    print(f"   Total Conversations: {summary['total_conversations']}")
    print(f"   Successful: {summary['successful_conversations']}")
    print(f"   Failed: {summary['failed_conversations']}")
    print(f"   Success Rate: {summary['success_rate']:.1%}")
    print(f"   Total Corrections: {summary['total_corrections']}")
    print(f"   Common Mistakes: {summary['common_mistakes']}")
    print()
    
    # Show how assistant would use this
    print("4. How This Helps the Assistant:")
    print()
    print("   ✅ Before responding:")
    print("      • Query SYNRIX for similar past conversations")
    print("      • Check for relevant corrections")
    print("      • Review user preferences")
    print("      • Avoid known mistakes")
    print()
    print("   ✅ After responding:")
    print("      • Store conversation in SYNRIX")
    print("      • Learn from feedback")
    print("      • Remember successful patterns")
    print()
    print("   ✅ Over time:")
    print("      • Builds knowledge base")
    print("      • Improves response quality")
    print("      • Avoids repeating mistakes")
    print("      • Remembers user preferences")
    print()
    
    memory.close()


if __name__ == "__main__":
    demo_assistant_memory()


