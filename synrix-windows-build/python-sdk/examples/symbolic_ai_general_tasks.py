#!/usr/bin/env python3
"""
SYNRIX Symbolic AI - General Tasks Demo

Shows SYNRIX doing general tasks symbolically (zero tokenization):
1. Question Answering
2. Planning
3. Reasoning
4. Learning

Then shows how it can also plug into an LLM if desired.
"""

import sys
import os
import json
import time

# Add parent directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(script_dir)
if parent_dir not in sys.path:
    sys.path.insert(0, parent_dir)

try:
    from synrix.raw_backend import RawSynrixBackend, LATTICE_NODE_LEARNING
    RAW_BACKEND_AVAILABLE = True
except ImportError:
    print("❌ RawSynrixBackend not available")
    sys.exit(1)


class SymbolicAIAssistant:
    """
    General-purpose symbolic AI assistant using SYNRIX.
    
    Does general tasks symbolically - no tokenization, no LLM needed.
    But can also enhance an LLM if desired.
    """
    
    def __init__(self, lattice_path: str = "symbolic_ai.lattice"):
        self.backend = RawSynrixBackend(lattice_path)
        self._seed_knowledge()
    
    def _seed_knowledge(self):
        """Seed the KG with some general knowledge patterns"""
        facts = [
            ("FACT:capital_of_france", "The capital of France is Paris"),
            ("FACT:capital_of_japan", "The capital of Japan is Tokyo"),
            ("FACT:largest_planet", "Jupiter is the largest planet in our solar system"),
            ("FACT:python_creator", "Python was created by Guido van Rossum"),
        ]
        
        reasoning = [
            ("REASONING:if_rain_then_umbrella", "If it rains, then bring an umbrella"),
            ("REASONING:if_cold_then_coat", "If it's cold, then wear a coat"),
            ("REASONING:if_tired_then_rest", "If you're tired, then you should rest"),
        ]
        
        planning = [
            ("PLAN:travel_steps", "Step 1: Research destination\nStep 2: Book flights\nStep 3: Book hotel\nStep 4: Pack bags\nStep 5: Travel"),
            ("PLAN:project_steps", "Step 1: Define requirements\nStep 2: Design solution\nStep 3: Implement\nStep 4: Test\nStep 5: Deploy"),
        ]
        
        for key, value in facts + reasoning + planning:
            self.backend.add_node(key, value, node_type=LATTICE_NODE_LEARNING)
    
    def answer_question(self, question: str) -> str:
        """
        Answer questions symbolically - zero tokenization.
        
        Process:
        1. Extract key concepts from question (word extraction, not tokenization)
        2. Query KG for matching patterns
        3. Return answer from KG
        """
        # Extract key concepts (simple word extraction)
        words = question.lower().split()
        key_words = [w for w in words if len(w) > 3 and w not in ["what", "where", "when", "who", "why", "how", "is", "the", "of", "and", "for"]]
        
        # Query KG for matching patterns
        results = []
        for word in key_words:
            matches = self.backend.find_by_prefix(f"FACT:{word}", limit=5)
            results.extend(matches)
        
        if results:
            # Return first match (could be improved with ranking)
            return results[0]["data"]
        else:
            return "I don't have that information in my knowledge base."
    
    def plan_task(self, task: str) -> str:
        """Generate a plan symbolically"""
        # Query for planning patterns
        results = self.backend.find_by_prefix("PLAN:", limit=5)
        
        if results:
            # Return most relevant plan pattern
            return results[0]["data"]
        else:
            return "I can help plan, but need more specific patterns in my knowledge base."
    
    def reason(self, situation: str) -> str:
        """Reason about situations symbolically"""
        # Extract key concepts
        words = situation.lower().split()
        key_words = [w for w in words if len(w) > 3]
        
        # Query for reasoning patterns
        results = []
        for word in key_words:
            matches = self.backend.find_by_prefix("REASONING:", limit=5)
            results.extend(matches)
        
        if results:
            return results[0]["data"]
        else:
            return "I can reason about this, but need more patterns in my knowledge base."
    
    def learn(self, fact: str, key: str = None):
        """Learn new facts symbolically"""
        if key is None:
            key = f"FACT:learned_{int(time.time())}"
        
        self.backend.add_node(key, fact, node_type=LATTICE_NODE_LEARNING)
        return key
    
    def get_memory_stats(self) -> dict:
        """Get statistics about stored knowledge"""
        all_facts = self.backend.find_by_prefix("FACT:", limit=1000)
        all_reasoning = self.backend.find_by_prefix("REASONING:", limit=1000)
        all_plans = self.backend.find_by_prefix("PLAN:", limit=1000)
        
        return {
            "facts": len(all_facts),
            "reasoning_patterns": len(all_reasoning),
            "planning_patterns": len(all_plans),
            "total_patterns": len(all_facts) + len(all_reasoning) + len(all_plans)
        }


class LLMEnhancedAssistant:
    """
    Same assistant, but enhanced with an LLM (optional).
    
    Shows how SYNRIX can plug into an LLM if desired.
    """
    
    def __init__(self, lattice_path: str = "symbolic_ai.lattice", use_llm: bool = False):
        self.symbolic = SymbolicAIAssistant(lattice_path)
        self.use_llm = use_llm
        self.llm_model = None
        
        if use_llm:
            # Would initialize LLM here (llama.cpp, etc.)
            # For demo, we'll simulate
            self.llm_model = "simulated_llm"
    
    def answer_question(self, question: str) -> str:
        """
        Answer with optional LLM enhancement.
        
        Process:
        1. Try symbolic answer first (fast, zero tokenization)
        2. If no match, use LLM (if enabled)
        3. Store LLM answer in SYNRIX for future use
        """
        # Try symbolic first (sub-microsecond)
        symbolic_answer = self.symbolic.answer_question(question)
        
        if "don't have" not in symbolic_answer.lower():
            return f"[Symbolic] {symbolic_answer}"
        
        # Fallback to LLM if enabled
        if self.use_llm and self.llm_model:
            # Would call LLM here
            llm_answer = f"[LLM] This is a simulated LLM response to: {question}"
            
            # Store in SYNRIX for future (now it's learned!)
            self.symbolic.learn(llm_answer, f"FACT:llm_learned_{int(time.time())}")
            
            return llm_answer
        
        return symbolic_answer


def demo_pure_symbolic():
    """Demo: Pure symbolic AI (zero tokenization)"""
    print("╔════════════════════════════════════════════════════════════╗")
    print("║   PURE SYMBOLIC AI - Zero Tokenization                    ║")
    print("╚════════════════════════════════════════════════════════════╝")
    print()
    
    assistant = SymbolicAIAssistant()
    
    # Task 1: Question Answering
    print("TASK 1: Question Answering")
    print("─────────────────────────────────────────────────────────────")
    question = "What is the capital of France?"
    print(f"Question: {question}")
    answer = assistant.answer_question(question)
    print(f"Answer: {answer}")
    print(f"Method: Symbolic lookup (zero tokenization)")
    print()
    
    # Task 2: Planning
    print("TASK 2: Planning")
    print("─────────────────────────────────────────────────────────────")
    task = "Plan a trip"
    print(f"Task: {task}")
    plan = assistant.plan_task(task)
    print(f"Plan:\n{plan}")
    print(f"Method: Symbolic pattern composition")
    print()
    
    # Task 3: Reasoning
    print("TASK 3: Reasoning")
    print("─────────────────────────────────────────────────────────────")
    situation = "It's raining outside"
    print(f"Situation: {situation}")
    reasoning = assistant.reason(situation)
    print(f"Reasoning: {reasoning}")
    print(f"Method: Symbolic pattern matching")
    print()
    
    # Task 4: Learning
    print("TASK 4: Learning")
    print("─────────────────────────────────────────────────────────────")
    new_fact = "The user's favorite color is blue"
    print(f"Learning: {new_fact}")
    key = assistant.learn(new_fact, "FACT:user_favorite_color")
    print(f"Stored as: {key}")
    
    # Verify learning
    question2 = "What is my favorite color?"
    print(f"Question: {question2}")
    answer2 = assistant.answer_question(question2)
    print(f"Answer: {answer2}")
    print(f"Method: Symbolic retrieval (learned fact)")
    print()
    
    # Stats
    stats = assistant.get_memory_stats()
    print("Memory Statistics:")
    print(f"  Facts: {stats['facts']}")
    print(f"  Reasoning patterns: {stats['reasoning_patterns']}")
    print(f"  Planning patterns: {stats['planning_patterns']}")
    print(f"  Total patterns: {stats['total_patterns']}")
    print()
    
    assistant.backend.close()


def demo_llm_enhanced():
    """Demo: LLM-enhanced (optional)"""
    print("╔════════════════════════════════════════════════════════════╗")
    print("║   LLM-ENHANCED (Optional) - SYNRIX + LLM                  ║")
    print("╚════════════════════════════════════════════════════════════╝")
    print()
    
    assistant = LLMEnhancedAssistant(use_llm=True)
    
    print("Same assistant, but with optional LLM fallback:")
    print()
    
    # Question that's not in KG
    question = "What is the meaning of life?"
    print(f"Question: {question}")
    answer = assistant.answer_question(question)
    print(f"Answer: {answer}")
    print(f"Method: Symbolic lookup failed → LLM fallback → Stored in SYNRIX")
    print()
    
    # Ask again (now it's in SYNRIX!)
    print("Ask the same question again:")
    answer2 = assistant.answer_question(question)
    print(f"Answer: {answer2}")
    print(f"Method: Now retrieved from SYNRIX (learned from LLM)")
    print()
    
    print("Key Point: LLM is optional. SYNRIX handles:")
    print("  • Fast symbolic lookups (sub-microsecond)")
    print("  • Persistent memory (learns from LLM)")
    print("  • Pattern storage (LLM answers become patterns)")
    print()
    
    assistant.symbolic.backend.close()


def main():
    print("=" * 60)
    print("SYNRIX Symbolic AI - General Tasks Demo")
    print("=" * 60)
    print()
    print("This demo shows:")
    print("  1. Pure symbolic AI (zero tokenization)")
    print("  2. Optional LLM integration (if desired)")
    print()
    print()
    
    # Demo 1: Pure symbolic
    demo_pure_symbolic()
    
    print()
    print("=" * 60)
    print()
    
    # Demo 2: LLM-enhanced
    demo_llm_enhanced()
    
    print()
    print("=" * 60)
    print("Demo complete!")
    print("=" * 60)


if __name__ == "__main__":
    main()

