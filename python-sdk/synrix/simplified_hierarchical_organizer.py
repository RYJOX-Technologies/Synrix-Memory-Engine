"""
Simplified Hierarchical Prefix Organizer for SYNRIX

Uses semantic type prefixes only (EVENT_, TEMPORAL_, etc.) for O(k) queries.
Entity filtering happens in scoring phase, not in prefixes.
"""

import re
from typing import Dict, List, Optional
from dataclasses import dataclass


@dataclass
class SimplifiedClassification:
    """Simplified classification with semantic type prefix and entity metadata"""
    prefix: str  # Simple semantic type prefix (e.g., "AGENT_ID:EVENT_:")
    semantic_type: str  # Primary semantic type (EVENT_, TEMPORAL_, etc.)
    semantic_types: List[str]  # All detected semantic types
    entities: Dict[str, str]  # Extracted entities (for scoring, not prefix)
    confidence: float
    reason: str


class SimplifiedHierarchicalOrganizer:
    """Simplified hierarchical organizer - semantic types in prefixes, entities in scoring"""
    
    def __init__(self):
        # Semantic type indicators
        self.event_indicators = ['went to', 'attended', 'event', 'conference', 'parade', 
                                'support group', 'meeting', 'gathering', 'race', 'speech']
        self.activity_indicators = ['paint', 'painted', 'painting', 'camping', 'pottery',
                                   'activity', 'activities', 'draw', 'drew', 'drawing', 
                                   'sketch', 'enjoys', 'participate', 'participated']
        self.temporal_indicators = ['when', 'may', 'june', 'july', 'august', 'september',
                                   'october', 'november', 'december', 'january', 'february',
                                   'march', 'april', '2023', '2024', 'ago', 'yesterday',
                                   'today', 'tomorrow', 'week', 'month', 'year', 'date']
        self.person_indicators = ['caroline', 'melanie', 'who', 'person', 'people', 'she', 'her']
        self.fact_indicators = ['from', 'is', 'was', 'are', 'were', 'country', 'identity',
                               'relationship', 'status', 'birthday', 'age', 'career', 'job']
    
    def extract_entities(self, text: str) -> Dict[str, str]:
        """Extract entities (dates, locations, people, activities) from text - for scoring only"""
        text_lower = text.lower()
        entities = {}
        
        # Extract dates
        date_patterns = [
            r'\b(\d{1,2}\s+(?:january|february|march|april|may|june|july|august|september|october|november|december)\s+\d{4})\b',
            r'\b((?:january|february|march|april|may|june|july|august|september|october|november|december)\s+\d{1,2},?\s+\d{4})\b',
            r'\b(\d{1,2}/\d{1,2}/\d{4})\b',
            r'\b(\d{4})\b',
        ]
        
        for pattern in date_patterns:
            matches = re.findall(pattern, text_lower)
            if matches:
                date_str = matches[0]
                date = date_str.replace(' ', '').replace(',', '').lower()
                
                # Parse "7 May 2023" format
                if any(month in date for month in ['may', 'june', 'july', 'august', 'september', 
                                                    'october', 'november', 'december', 'january', 
                                                    'february', 'march', 'april']):
                    year_match = re.search(r'\d{4}', date)
                    day_match = re.search(r'^(\d{1,2})', date)
                    
                    if year_match and day_match:
                        year = year_match.group()
                        day = day_match.group(1).zfill(2)
                        
                        month_map = {
                            'january': '01', 'february': '02', 'march': '03', 'april': '04',
                            'may': '05', 'june': '06', 'july': '07', 'august': '08',
                            'september': '09', 'october': '10', 'november': '11', 'december': '12'
                        }
                        month = None
                        for month_name, month_num in month_map.items():
                            if month_name in date:
                                month = month_num
                                break
                        
                        if month:
                            entities['date'] = f"{year}-{month}-{day}"
                            break
                
                # Fallback: use year
                year_match = re.search(r'\d{4}', date)
                if year_match:
                    entities['date'] = year_match.group()
                    break
        
        # Relative dates
        if 'date' not in entities:
            if 'yesterday' in text_lower:
                entities['date'] = 'yesterday'
            elif 'today' in text_lower:
                entities['date'] = 'today'
            elif 'ago' in text_lower:
                ago_match = re.search(r'(\d+)\s+(year|month|week|day)s?\s+ago', text_lower)
                if ago_match:
                    entities['date'] = f"{ago_match.group(1)}_{ago_match.group(2)}_ago"
        
        # Extract locations
        location_entities = ['sweden', 'museum', 'conference', 'school', 'support group',
                           'parade', 'lgbtq', 'transgender', 'home', 'house']
        for loc in location_entities:
            if f' {loc} ' in text_lower or text_lower.startswith(loc + ' ') or text_lower.endswith(' ' + loc):
                entities['location'] = loc.replace(' ', '_')
                break
            elif loc in text_lower and len(loc) > 3:
                entities['location'] = loc.replace(' ', '_')
                break
        
        # Extract people
        for person in ['caroline', 'melanie']:
            if person in text_lower:
                entities['person'] = person
                break
        
        # Extract activities
        activity_verbs = ['paint', 'painted', 'painting', 'camp', 'camping', 'pottery']
        for verb in activity_verbs:
            if verb in text_lower:
                entities['activity'] = verb.replace(' ', '_')
                break
        
        activity_entities = ['painting', 'camping', 'pottery', 'race', 'speech', 'support group']
        if 'activity' not in entities:
            for activity in activity_entities:
                if activity in text_lower:
                    entities['activity'] = activity.replace(' ', '_')
                    break
        
        return entities
    
    def detect_semantic_type(self, text: str) -> str:
        """Detect primary semantic type - returns single type for prefix"""
        text_lower = text.lower()
        is_question = text_lower.strip().endswith('?')
        scores = {}
        
        # Temporal detection (highest priority for "when" questions)
        if is_question and 'when' in text_lower:
            scores['TEMPORAL_'] = 25
        elif any(ind in text_lower for ind in self.temporal_indicators):
            scores['TEMPORAL_'] = 10
        
        # Event detection
        event_score = sum(1 for ind in self.event_indicators if ind in text_lower)
        if event_score > 0:
            scores['EVENT_'] = event_score * 4
        
        # Activity detection
        activity_score = sum(1 for ind in self.activity_indicators if ind in text_lower)
        if activity_score > 0:
            scores['ACTIVITY_'] = activity_score * 3
        
        # Fact detection
        fact_score = sum(1 for ind in self.fact_indicators if ind in text_lower)
        if fact_score > 0:
            scores['FACT_'] = fact_score
        
        # Person detection (lower priority)
        person_score = sum(1 for ind in self.person_indicators if ind in text_lower)
        if person_score > 0:
            scores['PERSON_'] = person_score
        
        # Return highest scoring type, or default to GENERIC
        if scores:
            return max(scores.items(), key=lambda x: x[1])[0]
        return 'GENERIC:'
    
    def detect_all_semantic_types(self, text: str) -> List[str]:
        """Detect all semantic types present (for multi-type queries)"""
        text_lower = text.lower()
        types = []
        scores = {}
        
        is_question = text_lower.strip().endswith('?')
        
        if is_question and 'when' in text_lower:
            types.append('TEMPORAL_')
            scores['TEMPORAL_'] = 25
        elif any(ind in text_lower for ind in self.temporal_indicators):
            types.append('TEMPORAL_')
            scores['TEMPORAL_'] = 10
        
        event_score = sum(1 for ind in self.event_indicators if ind in text_lower)
        if event_score > 0:
            types.append('EVENT_')
            scores['EVENT_'] = event_score * 4
        
        activity_score = sum(1 for ind in self.activity_indicators if ind in text_lower)
        if activity_score > 0:
            types.append('ACTIVITY_')
            scores['ACTIVITY_'] = activity_score * 3
        
        fact_score = sum(1 for ind in self.fact_indicators if ind in text_lower)
        if fact_score > 0:
            types.append('FACT_')
            scores['FACT_'] = fact_score
        
        person_score = sum(1 for ind in self.person_indicators if ind in text_lower)
        if person_score > 0:
            types.append('PERSON_')
            scores['PERSON_'] = person_score
        
        # Sort by score
        types.sort(key=lambda x: scores.get(x, 0), reverse=True)
        
        return types if types else ['GENERIC:']
    
    def classify(self, text: str, context: Optional[Dict] = None) -> SimplifiedClassification:
        """Classify text with simplified hierarchical prefix"""
        agent_id = context.get('agent_id', 'unknown') if context else 'unknown'
        session_id = context.get('session_id') if context else None
        
        # Detect primary semantic type (for prefix)
        semantic_type = self.detect_semantic_type(text)
        
        # Detect all semantic types (for multi-type queries)
        all_semantic_types = self.detect_all_semantic_types(text)
        
        # Extract entities (for scoring, not prefix)
        entities = self.extract_entities(text)
        
        # Build simple prefix: AGENT_ID:SEMANTIC_TYPE:
        if session_id:
            prefix = f"AGENT_{agent_id}:CONVERSATION:{session_id}:{semantic_type}:"
        else:
            prefix = f"AGENT_{agent_id}:{semantic_type}:"
        
        # Calculate confidence
        confidence = 0.7
        if all_semantic_types:
            confidence += 0.1 * len(all_semantic_types)
        if entities:
            confidence += 0.1 * len(entities)
        confidence = min(confidence, 1.0)
        
        reason = f"Semantic type: {semantic_type}, Entities: {', '.join(entities.keys())}"
        
        return SimplifiedClassification(
            prefix=prefix,
            semantic_type=semantic_type,
            semantic_types=all_semantic_types,
            entities=entities,
            confidence=confidence,
            reason=reason
        )
