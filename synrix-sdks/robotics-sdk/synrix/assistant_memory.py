"""
SYNRIX Memory Integration for AI Assistants

This module provides persistent memory for AI assistants (like this one) using SYNRIX.
It enables:
1. Learning from past conversations
2. Remembering corrections and preferences
3. Avoiding repeated mistakes
4. Building knowledge over time
"""

import json
import time
from typing import Optional, Dict, List, Any
from .agent_memory import SynrixMemory


class AssistantMemory:
    """
    Persistent memory for AI assistants using SYNRIX.
    
    Stores:
    - Conversation patterns
    - User preferences
    - Corrections and feedback
    - Successful solutions
    - Common mistakes to avoid
    """
    
    def __init__(self, memory: Optional[SynrixMemory] = None, use_direct: bool = True):
        """
        Initialize assistant memory.
        
        Args:
            memory: Optional SynrixMemory instance
            use_direct: Use direct shared memory if available
        """
        if memory is None:
            self.memory = SynrixMemory(use_direct=use_direct, collection="assistant_memory")
        else:
            self.memory = memory
            self.memory.collection = "assistant_memory"
    
    def store_conversation(
        self,
        user_query: str,
        assistant_response: str,
        success: bool = True,
        feedback: Optional[str] = None,
        metadata: Optional[Dict] = None
    ):
        """
        Store a conversation turn in SYNRIX.
        
        Args:
            user_query: What the user asked
            assistant_response: What the assistant responded
            success: Whether the response was successful
            feedback: Optional user feedback
            metadata: Additional metadata (task_type, code_changes, etc.)
        """
        # Extract task type from query
        task_type = self._classify_query(user_query)
        
        # Create conversation key
        conversation_id = f"conv_{int(time.time() * 1000)}"
        
        result_value = "success" if success else "failed"
        if feedback:
            result_value = feedback.lower()
        
        store_metadata = {
            "user_query": user_query[:500],
            "assistant_response": assistant_response[:1000],
            "success": success,
            "feedback": feedback,
            "task_type": task_type,
            "timestamp": time.time(),
            **(metadata or {})
        }
        
        self.memory.write(
            f"conversation:{task_type}:{conversation_id}",
            result_value,
            metadata=store_metadata
        )
    
    def store_correction(
        self,
        original_response: str,
        corrected_response: str,
        error_type: str,
        context: Optional[str] = None
    ):
        """
        Store a correction when the assistant makes a mistake.
        
        Args:
            original_response: What the assistant said (incorrect)
            corrected_response: What it should have said
            error_type: Type of error (wrong_approach, incorrect_code, etc.)
            context: Additional context about the mistake
        """
        task_type = "correction"
        
        metadata = {
            "original": original_response[:500],
            "corrected": corrected_response[:500],
            "error_type": error_type,
            "context": context,
            "timestamp": time.time()
        }
        
        self.memory.write(
            f"correction:{error_type}:{int(time.time() * 1000)}",
            "correction_applied",
            metadata=metadata
        )
    
    def store_preference(
        self,
        preference_key: str,
        preference_value: str,
        context: Optional[str] = None
    ):
        """
        Store user preferences.
        
        Args:
            preference_key: What the preference is about (e.g., "code_style", "response_format")
            preference_value: The preference value
            context: When/why this preference applies
        """
        metadata = {
            "preference_value": preference_value,
            "context": context,
            "timestamp": time.time()
        }
        
        self.memory.write(
            f"preference:{preference_key}",
            preference_value,
            metadata=metadata
        )
    
    def query_similar_conversations(
        self,
        user_query: str,
        limit: int = 5
    ) -> List[Dict[str, Any]]:
        """
        Query SYNRIX for similar past conversations.
        
        Args:
            user_query: Current user query
            limit: Maximum number of results
            
        Returns:
            List of similar past conversations with responses
        """
        task_type = self._classify_query(user_query)
        
        # Query for similar conversations
        memory_data = self.memory.get_task_memory_summary(f"conversation:{task_type}", limit=limit)
        
        similar_conversations = []
        for attempt in memory_data.get("last_attempts", []):
            similar_conversations.append({
                "user_query": attempt.get("metadata", {}).get("user_query"),
                "assistant_response": attempt.get("metadata", {}).get("assistant_response"),
                "success": attempt.get("metadata", {}).get("success", True),
                "timestamp": attempt.get("timestamp", 0)
            })
        
        return similar_conversations
    
    def get_corrections(self, error_type: Optional[str] = None) -> List[Dict[str, Any]]:
        """
        Get past corrections to avoid repeating mistakes.
        
        Args:
            error_type: Optional filter by error type
            
        Returns:
            List of corrections
        """
        if error_type:
            prefix = f"correction:{error_type}:"
        else:
            prefix = "correction:"
        
        results = self.memory.client.query_prefix(prefix, collection=self.memory.collection, limit=20)
        
        corrections = []
        for result in results:
            payload = result.get("payload", {})
            metadata = payload.get("metadata", {})
            corrections.append({
                "original": metadata.get("original"),
                "corrected": metadata.get("corrected"),
                "error_type": metadata.get("error_type"),
                "context": metadata.get("context")
            })
        
        return corrections
    
    def get_preferences(self, preference_key: Optional[str] = None) -> Dict[str, Any]:
        """
        Get user preferences.
        
        Args:
            preference_key: Optional specific preference to get
            
        Returns:
            Dictionary of preferences
        """
        if preference_key:
            prefix = f"preference:{preference_key}"
        else:
            prefix = "preference:"
        
        results = self.memory.client.query_prefix(prefix, collection=self.memory.collection, limit=50)
        
        preferences = {}
        for result in results:
            payload = result.get("payload", {})
            name = payload.get("name", "")
            value = payload.get("data", "{}")
            
            try:
                data = json.loads(value)
                pref_key = name.split(":")[-1] if ":" in name else name
                preferences[pref_key] = {
                    "value": data.get("value", ""),
                    "context": data.get("metadata", {}).get("context"),
                    "timestamp": data.get("timestamp", 0)
                }
            except:
                pass
        
        return preferences
    
    def get_learning_summary(self) -> Dict[str, Any]:
        """
        Get summary of what the assistant has learned.
        
        Returns:
            Learning statistics and patterns
        """
        # Get conversation stats
        conv_data = self.memory.get_task_memory_summary("conversation", limit=100)
        
        # Get correction stats
        corrections = self.get_corrections()
        
        # Get preference stats
        preferences = self.get_preferences()
        
        return {
            "total_conversations": len(conv_data.get("last_attempts", [])),
            "successful_conversations": len(conv_data.get("successes", [])),
            "failed_conversations": len(conv_data.get("failures", [])),
            "total_corrections": len(corrections),
            "total_preferences": len(preferences),
            "common_mistakes": list(conv_data.get("failure_patterns", set())),
            "success_rate": len(conv_data.get("successes", [])) / max(len(conv_data.get("last_attempts", [])), 1)
        }
    
    def _classify_query(self, query: str) -> str:
        """Classify query type for better organization"""
        query_lower = query.lower()
        
        if any(word in query_lower for word in ["write", "create", "generate", "code"]):
            return "code_generation"
        elif any(word in query_lower for word in ["fix", "bug", "error", "debug"]):
            return "debugging"
        elif any(word in query_lower for word in ["explain", "how", "what", "why"]):
            return "explanation"
        elif any(word in query_lower for word in ["test", "run", "execute"]):
            return "testing"
        elif any(word in query_lower for word in ["refactor", "improve", "optimize"]):
            return "refactoring"
        else:
            return "general"
    
    def close(self):
        """Close memory connection"""
        if hasattr(self.memory, 'close'):
            self.memory.close()


# ============================================================================
# Integration Helper Functions
# ============================================================================

def get_assistant_memory(use_direct: bool = True) -> AssistantMemory:
    """
    Get or create assistant memory instance.
    
    This can be called at the start of a conversation to initialize memory.
    """
    return AssistantMemory(use_direct=use_direct)


def query_before_responding(user_query: str, memory: AssistantMemory) -> Dict[str, Any]:
    """
    Query SYNRIX before generating a response.
    
    Returns context that should influence the response:
    - Similar past conversations
    - Relevant corrections
    - User preferences
    - Common mistakes to avoid
    """
    # Get similar conversations
    similar = memory.query_similar_conversations(user_query, limit=3)
    
    # Get relevant corrections
    corrections = memory.get_corrections()
    
    # Get preferences
    preferences = memory.get_preferences()
    
    return {
        "similar_conversations": similar,
        "corrections": corrections[:5],  # Most recent 5
        "preferences": preferences,
        "should_avoid": [c["original"] for c in corrections[:3]]  # Things to avoid
    }


def store_after_responding(
    user_query: str,
    assistant_response: str,
    success: bool,
    memory: AssistantMemory,
    feedback: Optional[str] = None,
    metadata: Optional[Dict] = None
):
    """
    Store conversation turn after responding.
    
    This should be called after the assistant generates a response.
    """
    memory.store_conversation(
        user_query=user_query,
        assistant_response=assistant_response,
        success=success,
        feedback=feedback,
        metadata=metadata
    )


