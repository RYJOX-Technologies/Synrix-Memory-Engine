"""
Dynamic Semantic Classification - Proof of Concept
===================================================

Learns patterns from the lattice instead of hardcoding keywords.
Uses syntactic patterns and lattice-based learning.
"""

import re
from typing import Dict, Optional, List
from dataclasses import dataclass

from .auto_organizer import AutoOrganizer, ClassificationResult
from .raw_backend import RawSynrixBackend


@dataclass
class SyntacticPattern:
    """Represents a syntactic pattern extracted from text"""
    verb: str
    verb_form: str  # base form: "go", "went", "going" -> "go"
    object_noun: Optional[str] = None
    preposition: Optional[str] = None
    location: Optional[str] = None
    time_expression: Optional[str] = None
    question_type: Optional[str] = None  # "when", "what", "who", "where"


class DynamicAutoOrganizer(AutoOrganizer):
    """
    Dynamic auto-organizer that learns patterns from the lattice
    instead of using hardcoded keyword lists
    """
    
    def __init__(self, backend: Optional[RawSynrixBackend] = None):
        super().__init__()
        self.backend = backend
        
        # Verb classes (can be learned from lattice over time)
        # Start with common patterns, but these can grow dynamically
        self.verb_classes = {
            'movement': ['go', 'went', 'travel', 'move', 'come', 'arrive'],
            'action': ['paint', 'draw', 'write', 'read', 'build', 'make', 'create'],
            'participation': ['attend', 'participate', 'join', 'take part'],
        }
        
        # Pattern templates (syntactic, not keyword-based)
        self.pattern_templates = {
            'EVENT_': [
                r'\b\w+\s+(went|go|going|goes)\s+to\s+\w+',  # "went to [place]"
                r'\b\w+\s+(attend|attended|participate|participated)\s+\w+',  # "attended [event]"
            ],
            'ACTIVITY_': [
                r'\b\w+\s+(paint|paints|painted|draw|draws|drew)\w*\s+\w+',  # "[subject] paint [object]"
                r'\b\w+\s+(camp|camps|camped|hike|hikes|hiked)\w*',  # "[subject] camp/hike"
            ],
            'FACT_': [
                r'\b\w+\s+(is|was|are|were)\s+\w+',  # "[subject] is [attribute]"
                r'\b\w+\s+(from|moved from)\s+\w+',  # "[subject] from [place]"
            ],
            'TEMPORAL_': [
                r'\bwhen\s+\w+\s+\w+',  # "when [subject] [verb]"
                r'\b\w+\s+(birthday|anniversary|born|died)\s+',  # "[subject] birthday/anniversary"
            ],
        }
    
    def extract_syntactic_pattern(self, text: str) -> SyntacticPattern:
        """
        Extract syntactic pattern from text
        This is the key - we extract structure, not keywords
        """
        text_lower = text.lower()
        
        # Extract verb (simplified - in production would use POS tagging)
        verb_match = re.search(r'\b(went|go|going|goes|paint|paints|painted|draw|draws|drew|'
                              r'camp|camps|camped|hike|hikes|hiked|attend|attended|'
                              r'participate|participated|is|was|are|were|from|moved)\w*', text_lower)
        verb = verb_match.group(1) if verb_match else None
        
        # Normalize verb to base form
        verb_form = self._normalize_verb(verb) if verb else None
        
        # Extract question type
        question_type = None
        if text.strip().endswith('?'):
            if 'when' in text_lower:
                question_type = 'when'
            elif 'what' in text_lower:
                question_type = 'what'
            elif 'who' in text_lower:
                question_type = 'who'
            elif 'where' in text_lower:
                question_type = 'where'
        
        # Extract time expression
        time_match = re.search(r'\b(january|february|march|april|may|june|july|august|'
                               r'september|october|november|december|\d{4}|\d{1,2}/\d{1,2})', text_lower)
        time_expression = time_match.group(1) if time_match else None
        
        # Extract location (simplified - after "to" or "from")
        location_match = re.search(r'\b(to|from)\s+(\w+)', text_lower)
        location = location_match.group(2) if location_match else None
        preposition = location_match.group(1) if location_match else None
        
        return SyntacticPattern(
            verb=verb,
            verb_form=verb_form,
            preposition=preposition,
            location=location,
            time_expression=time_expression,
            question_type=question_type
        )
    
    def _normalize_verb(self, verb: str) -> str:
        """Normalize verb to base form (simplified)"""
        if not verb:
            return None
        
        # Simple normalization (in production would use proper lemmatization)
        verb_lower = verb.lower()
        if verb_lower.startswith('went'):
            return 'go'
        if verb_lower.startswith('paint'):
            return 'paint'
        if verb_lower.startswith('draw'):
            return 'draw'
        if verb_lower.startswith('camp'):
            return 'camp'
        if verb_lower.startswith('hike'):
            return 'hike'
        if verb_lower.startswith('attend'):
            return 'attend'
        if verb_lower.startswith('participat'):
            return 'participate'
        
        return verb_lower
    
    def query_lattice_patterns(self, pattern: SyntacticPattern) -> Optional[str]:
        """
        Query the lattice for similar patterns that were already classified
        This is the learning mechanism - we learn from past classifications
        """
        if not self.backend:
            return None
        
        # Build pattern query based on verb form
        if pattern.verb_form:
            # Try each semantic type
            for sem_type in ['EVENT_', 'ACTIVITY_', 'FACT_', 'TEMPORAL_']:
                pattern_prefix = f"PATTERN_{sem_type}:{pattern.verb_form}:"
                results = self.backend.find_by_prefix(pattern_prefix, limit=1)
                if results:
                    # Found a learned pattern - use it
                    return sem_type
        
        return None
    
    def store_pattern_in_lattice(self, pattern: SyntacticPattern, classification: str):
        """
        Store pattern in lattice for future learning
        This is how the system learns - it remembers successful classifications
        """
        if not self.backend or not pattern.verb_form:
            return
        
        pattern_name = f"PATTERN_{classification}:{pattern.verb_form}:"
        pattern_data = f"verb={pattern.verb_form},prep={pattern.preposition or 'none'},location={pattern.location or 'none'}"
        
        # Check if pattern already exists
        existing = self.backend.find_by_prefix(pattern_name, limit=1)
        if not existing:
            # Store new pattern
            self.backend.add_node(
                pattern_name,
                pattern_data,
                node_type=3  # LATTICE_NODE_PATTERN
            )
    
    def classify_by_pattern_template(self, pattern: SyntacticPattern) -> Optional[str]:
        """
        Classify using pattern templates (syntactic patterns, not keywords)
        """
        text = f"{pattern.verb or ''} {pattern.preposition or ''} {pattern.location or ''}"
        text_lower = text.lower()
        
        # Match against pattern templates
        for sem_type, templates in self.pattern_templates.items():
            for template in templates:
                if re.search(template, text_lower):
                    return sem_type
        
        # Use verb classes (semantic, not keyword-based)
        if pattern.verb_form:
            if pattern.verb_form in self.verb_classes.get('movement', []):
                if pattern.preposition == 'to' and pattern.location:
                    return 'EVENT_'  # Movement to location = event
            if pattern.verb_form in self.verb_classes.get('action', []):
                return 'ACTIVITY_'  # Action verb = activity
            if pattern.verb_form in self.verb_classes.get('participation', []):
                return 'EVENT_'  # Participation = event
        
        # Question type detection
        if pattern.question_type == 'when':
            return 'TEMPORAL_'
        if pattern.question_type == 'who':
            return 'PERSON_'
        if pattern.question_type == 'where':
            return 'FACT_'
        
        # Time expression detection
        if pattern.time_expression:
            if 'birthday' in text_lower or 'anniversary' in text_lower:
                return 'TEMPORAL_'
        
        return None
    
    def classify(self, data: str, context: Optional[Dict] = None) -> ClassificationResult:
        """
        Dynamic classification that learns from the lattice
        """
        if not data:
            return ClassificationResult(
                prefix="GENERIC:",
                confidence=0.0,
                reason="Empty data"
            )
        
        # Extract syntactic pattern
        pattern = self.extract_syntactic_pattern(data)
        
        # Step 1: Query lattice for learned patterns
        learned_type = self.query_lattice_patterns(pattern)
        if learned_type:
            # Use learned classification
            if context and context.get("agent_id"):
                agent_id = str(context["agent_id"])
                prefix = f"AGENT_{agent_id}:{learned_type}:"
                name = self._sanitize_name(data[:30])
                return ClassificationResult(
                    prefix=prefix,
                    confidence=0.9,
                    reason=f"Learned pattern from lattice: {pattern.verb_form}",
                    suggested_name=f"{prefix}{name}"
                )
        
        # Step 2: Use pattern templates (if no learned pattern)
        template_type = self.classify_by_pattern_template(pattern)
        if template_type:
            # Store pattern for future learning
            self.store_pattern_in_lattice(pattern, template_type)
            
            if context and context.get("agent_id"):
                agent_id = str(context["agent_id"])
                prefix = f"AGENT_{agent_id}:{template_type}:"
                name = self._sanitize_name(data[:30])
                return ClassificationResult(
                    prefix=prefix,
                    confidence=0.8,
                    reason=f"Pattern template match: {pattern.verb_form}",
                    suggested_name=f"{prefix}{name}"
                )
        
        # Step 3: Fallback to parent class
        return super().classify(data, context)
    
    def _sanitize_name(self, text: str) -> str:
        """Sanitize text for use in node name"""
        sanitized = re.sub(r'[^a-zA-Z0-9_]', '_', text)
        sanitized = re.sub(r'_+', '_', sanitized)
        sanitized = sanitized.strip('_')
        if len(sanitized) > 30:
            sanitized = sanitized[:30]
        return sanitized or "DATA"


def classify_data_dynamic(data: str, context: Optional[Dict] = None, 
                         backend: Optional[RawSynrixBackend] = None) -> ClassificationResult:
    """
    Dynamic classification that learns from the lattice
    
    Example:
        >>> backend = RawSynrixBackend("test.lattice")
        >>> result = classify_data_dynamic("Caroline went to museum", 
        ...                                {"agent_id": "conv-26"}, backend)
        >>> # First time: Uses pattern template
        >>> # Second time: Uses learned pattern from lattice
    """
    organizer = DynamicAutoOrganizer(backend=backend)
    return organizer.classify(data, context)
