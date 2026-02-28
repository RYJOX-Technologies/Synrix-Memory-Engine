"""
Automatic Organization System for SYNRIX
========================================

Automatically classifies and organizes data on ingestion without requiring
manual prefix assignment.
"""

import re
from typing import Dict, Optional, Tuple, List
from dataclasses import dataclass


@dataclass
class ClassificationResult:
    """Result of automatic classification"""
    prefix: str
    confidence: float
    reason: str
    suggested_name: Optional[str] = None


class AutoOrganizer:
    """
    Automatically organizes data by classifying it and assigning appropriate prefixes
    """
    
    def __init__(self):
        # Code pattern indicators
        self.code_indicators = [
            'function', 'def ', 'class ', 'return ', 'if ', 'for ', 'while ',
            'import ', 'from ', '{', '}', '(', ')', ';', '->', 'async ',
            'const ', 'let ', 'var ', 'public ', 'private ', 'protected '
        ]
        
        # ISA/Instruction indicators
        self.isa_indicators = [
            'add', 'mov', 'sub', 'mul', 'div', 'jmp', 'call', 'ret',
            'register', 'instruction', 'opcode', 'operand', 'assembly',
            'mnemonic', 'cpu', 'instruction set'
        ]
        
        # Learning pattern indicators
        self.learning_indicators = [
            'learned', 'pattern', 'success', 'failure', 'error',
            'memory', 'recall', 'remember', 'experience', 'learned from',
            'best practice', 'anti-pattern'
        ]
        
        # Constraint indicators
        self.constraint_indicators = [
            'constraint', 'rule', 'must', 'cannot', 'should not',
            'forbidden', 'required', 'mandatory', 'prohibited'
        ]
        
        # Domain keywords
        self.domain_keywords = {
            'physics': ['energy', 'force', 'field', 'wave', 'particle', 'quantum',
                       'thermal', 'magnetic', 'electric', 'momentum', 'acceleration',
                       'velocity', 'mass', 'charge', 'potential', 'kinetic'],
            'chemistry': ['reaction', 'molecule', 'compound', 'bond', 'equilibrium',
                         'ph', 'solubility', 'oxidation', 'reduction', 'catalyst'],
            'biology': ['protein', 'enzyme', 'cell', 'genetic', 'dna', 'rna',
                       'amino', 'acid', 'peptide', 'metabolic'],
            'computing': ['algorithm', 'data structure', 'function', 'class', 'api',
                         'database', 'network', 'protocol', 'framework'],
            'mathematics': ['equation', 'formula', 'theorem', 'proof', 'calculate',
                          'integral', 'derivative', 'matrix', 'vector']
        }
    
    def classify(self, data: str, context: Optional[Dict] = None) -> ClassificationResult:
        """
        Automatically classify data and return appropriate prefix
        
        Args:
            data: The data to classify
            context: Optional context (agent_id, user_id, session_id, domain, type)
        
        Returns:
            ClassificationResult with prefix, confidence, and reason
        """
        if not data:
            return ClassificationResult(
                prefix="GENERIC:",
                confidence=0.0,
                reason="Empty data"
            )
        
        data_lower = data.lower()
        data_stripped = data.strip()
        
        # 1. Check context first (highest priority)
        if context:
            result = self._classify_from_context(context, data)
            if result:
                return result
        
        # 2. Check for code patterns
        if self._is_code_pattern(data):
            name = self._extract_code_name(data)
            return ClassificationResult(
                prefix="PATTERN_",
                confidence=0.8,
                reason="Code pattern detected",
                suggested_name=f"PATTERN_{name}" if name else None
            )
        
        # 3. Check for ISA/Instruction patterns
        if self._is_isa_pattern(data_lower):
            name = self._extract_isa_name(data)
            return ClassificationResult(
                prefix="ISA_",
                confidence=0.85,
                reason="ISA/Instruction pattern detected",
                suggested_name=f"ISA_{name}" if name else None
            )
        
        # 4. Check for learning patterns
        if self._is_learning_pattern(data_lower):
            name = self._extract_learning_name(data)
            return ClassificationResult(
                prefix="LEARNING_",
                confidence=0.75,
                reason="Learning/memory pattern detected",
                suggested_name=f"LEARNING_{name}" if name else None
            )
        
        # 5. Check for constraint patterns
        if self._is_constraint_pattern(data_lower):
            name = self._extract_constraint_name(data)
            return ClassificationResult(
                prefix="CONSTRAINT_",
                confidence=0.8,
                reason="Constraint pattern detected",
                suggested_name=f"CONSTRAINT_{name}" if name else None
            )
        
        # 6. Check for domain classification
        domain = self._classify_domain(data_lower)
        if domain:
            name = self._extract_domain_name(data, domain)
            return ClassificationResult(
                prefix=f"DOMAIN_{domain.upper()}:",
                confidence=0.7,
                reason=f"Domain classification: {domain}",
                suggested_name=f"DOMAIN_{domain.upper()}:{name}" if name else None
            )
        
        # 7. Fallback to generic
        return ClassificationResult(
            prefix="GENERIC:",
            confidence=0.5,
            reason="No specific pattern detected, using generic namespace"
        )
    
    def _classify_from_context(self, context: Dict, data: str) -> Optional[ClassificationResult]:
        """Classify based on context (agent, user, session)"""
        # Agent context
        if context.get("agent_id"):
            agent_id = str(context["agent_id"])
            name = self._sanitize_name(data[:50])  # First 50 chars
            return ClassificationResult(
                prefix=f"AGENT_{agent_id}:",
                confidence=0.9,
                reason=f"Agent context: agent_id={agent_id}",
                suggested_name=f"AGENT_{agent_id}:{name}"
            )
        
        # User context
        if context.get("user_id"):
            user_id = str(context["user_id"])
            name = self._sanitize_name(data[:50])
            return ClassificationResult(
                prefix=f"USER_{user_id}:",
                confidence=0.9,
                reason=f"User context: user_id={user_id}",
                suggested_name=f"USER_{user_id}:{name}"
            )
        
        # Session context
        if context.get("session_id"):
            session_id = str(context["session_id"])
            name = self._sanitize_name(data[:50])
            return ClassificationResult(
                prefix=f"SESSION_{session_id}:",
                confidence=0.9,
                reason=f"Session context: session_id={session_id}",
                suggested_name=f"SESSION_{session_id}:{name}"
            )
        
        return None
    
    def _is_code_pattern(self, data: str) -> bool:
        """Detect if data is a code pattern"""
        indicator_count = sum(1 for ind in self.code_indicators if ind in data)
        return indicator_count >= 3
    
    def _is_isa_pattern(self, data_lower: str) -> bool:
        """Detect if data is an ISA/instruction pattern"""
        return any(ind in data_lower for ind in self.isa_indicators)
    
    def _is_learning_pattern(self, data_lower: str) -> bool:
        """Detect if data is a learning/memory pattern"""
        return any(ind in data_lower for ind in self.learning_indicators)
    
    def _is_constraint_pattern(self, data_lower: str) -> bool:
        """Detect if data is a constraint pattern"""
        return any(ind in data_lower for ind in self.constraint_indicators)
    
    def _classify_domain(self, data_lower: str) -> Optional[str]:
        """Classify data into domain"""
        scores = {}
        
        for domain, keywords in self.domain_keywords.items():
            score = sum(1 for kw in keywords if kw in data_lower)
            if score > 0:
                scores[domain] = score
        
        if scores:
            return max(scores, key=scores.get)
        
        return None
    
    def _extract_code_name(self, data: str) -> str:
        """Extract name from code pattern"""
        # Try to extract function/class name
        patterns = [
            r'def\s+(\w+)',
            r'class\s+(\w+)',
            r'function\s+(\w+)',
            r'const\s+(\w+)',
            r'let\s+(\w+)',
            r'var\s+(\w+)'
        ]
        
        for pattern in patterns:
            match = re.search(pattern, data, re.IGNORECASE)
            if match:
                return match.group(1).upper()
        
        # Fallback: use first meaningful word
        words = re.findall(r'\b\w+\b', data)
        if words:
            return words[0].upper()
        
        return "CODE"
    
    def _extract_isa_name(self, data: str) -> str:
        """Extract name from ISA pattern"""
        # Look for instruction names
        words = re.findall(r'\b\w+\b', data.lower())
        for word in words:
            if word in self.isa_indicators:
                return word.upper()
        
        return "INSTRUCTION"
    
    def _extract_learning_name(self, data: str) -> str:
        """Extract name from learning pattern"""
        # Extract key concept
        words = re.findall(r'\b\w+\b', data.lower())
        for word in words:
            if word not in self.learning_indicators and len(word) > 3:
                return word.upper()
        
        return "PATTERN"
    
    def _extract_constraint_name(self, data: str) -> str:
        """Extract name from constraint pattern"""
        # Extract constraint name
        words = re.findall(r'\b\w+\b', data.lower())
        for word in words:
            if word not in self.constraint_indicators and len(word) > 3:
                return word.upper()
        
        return "RULE"
    
    def _extract_domain_name(self, data: str, domain: str) -> str:
        """Extract name from domain-specific data"""
        # Extract key concept
        words = re.findall(r'\b\w+\b', data.lower())
        domain_words = self.domain_keywords.get(domain, [])
        
        for word in words:
            if word not in domain_words and len(word) > 3:
                return word.upper()
        
        return "CONCEPT"
    
    def _sanitize_name(self, text: str) -> str:
        """Sanitize text for use in node name"""
        # Remove special characters, keep alphanumeric and underscores
        sanitized = re.sub(r'[^a-zA-Z0-9_]', '_', text)
        # Remove multiple underscores
        sanitized = re.sub(r'_+', '_', sanitized)
        # Remove leading/trailing underscores
        sanitized = sanitized.strip('_')
        # Limit length
        if len(sanitized) > 50:
            sanitized = sanitized[:50]
        return sanitized or "DATA"


# Global instance
_auto_organizer = AutoOrganizer()


def classify_data(data: str, context: Optional[Dict] = None) -> ClassificationResult:
    """
    Convenience function to classify data automatically
    
    Args:
        data: The data to classify
        context: Optional context (agent_id, user_id, session_id, domain, type)
    
    Returns:
        ClassificationResult with prefix, confidence, and reason
    
    Example:
        >>> result = classify_data("addition operation")
        >>> print(result.prefix)  # "ISA_"
        >>> print(result.confidence)  # 0.85
    """
    return _auto_organizer.classify(data, context)
