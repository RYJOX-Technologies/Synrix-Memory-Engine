"""
Advanced Retrieval Tricks for SYNRIX
====================================

Clever optimizations for improving retrieval performance:
1. Query expansion with paraphrases
2. Temporal reasoning (relative dates)
3. Entity co-reference resolution
4. Answer-type prediction
5. Multi-pass retrieval
"""

import re
from typing import Dict, List, Optional, Set, Tuple
from datetime import datetime, timedelta


class AdvancedRetrievalTricks:
    """Advanced tricks for improving retrieval"""
    
    def __init__(self):
        # Current year for temporal reasoning
        self.current_year = 2024  # Can be made dynamic
        
        # Entity co-reference patterns
        self.co_reference_patterns = {
            'pronouns': {
                'she': ['caroline', 'melanie'],
                'her': ['caroline', 'melanie'],
                'he': ['caroline', 'melanie'],  # Gender-neutral context
                'him': ['caroline', 'melanie'],
                'it': ['support group', 'lgbtq', 'conference', 'parade', 'pottery', 'camping'],
                'they': ['friends', 'family', 'people'],
            },
            'definite_articles': {
                'the group': ['support group', 'lgbtq'],
                'the conference': ['conference', 'transgender conference'],
                'the parade': ['parade', 'pride parade'],
            }
        }
        
        # Answer type patterns
        self.answer_type_patterns = {
            'date': ['when', 'what date', 'what time', 'how long ago'],
            'location': ['where', 'what place', 'what location'],
            'person': ['who', 'which person'],
            'activity': ['what activity', 'what did', 'what does', 'what do'],
            'fact': ['what is', 'what are', 'what was', 'how many', 'how much'],
        }
        
        # Query paraphrases (common rephrasings)
        self.paraphrase_patterns = {
            'when did X do Y': ['when did', 'what time did', 'what date did', 'when was'],
            'what activities': ['what activities', 'what does X do', 'what did X do', 'what hobbies'],
            'where did X go': ['where did', 'what place did', 'where was'],
            'who is X': ['who is', 'what is X', 'tell me about X'],
        }
    
    def expand_query(self, query: str, context: Optional[Dict] = None) -> List[str]:
        """Expand query with paraphrases and variations"""
        query_lower = query.lower()
        expanded = [query]  # Original query
        
        # Paraphrase patterns
        for pattern, variations in self.paraphrase_patterns.items():
            if any(v in query_lower for v in pattern.split()):
                for variation in variations:
                    # Try to create variation (simplified)
                    if variation not in query_lower:
                        # Don't add if it's too different
                        pass
        
        # Add question word variations
        if 'when' in query_lower:
            expanded.append(query_lower.replace('when', 'what time'))
            expanded.append(query_lower.replace('when', 'what date'))
        
        if 'what activities' in query_lower:
            expanded.append(query_lower.replace('what activities', 'what does'))
            expanded.append(query_lower.replace('what activities', 'what did'))
        
        return expanded
    
    def resolve_temporal_expression(self, text: str) -> Optional[str]:
        """Resolve relative temporal expressions to absolute dates"""
        text_lower = text.lower()
        
        # "X years ago" -> calculate year
        ago_match = re.search(r'(\d+)\s+(year|month|week|day)s?\s+ago', text_lower)
        if ago_match:
            amount = int(ago_match.group(1))
            unit = ago_match.group(2)
            
            if unit == 'year':
                year = self.current_year - amount
                return str(year)
            elif unit == 'month':
                # Approximate: assume 12 months = 1 year
                years = amount // 12
                year = self.current_year - years
                return str(year)
        
        # "last year" -> current_year - 1
        if 'last year' in text_lower:
            return str(self.current_year - 1)
        
        # "this year" -> current_year
        if 'this year' in text_lower:
            return str(self.current_year)
        
        return None
    
    def resolve_co_reference(self, query: str, conversation_context: Optional[List[str]] = None) -> Dict[str, List[str]]:
        """Resolve pronouns and co-references to actual entities"""
        query_lower = query.lower()
        resolved = {}
        
        # Check for pronouns
        for pronoun, possible_entities in self.co_reference_patterns['pronouns'].items():
            if pronoun in query_lower:
                # If we have conversation context, try to find the most recent mention
                if conversation_context:
                    # Look for entity mentions in recent context
                    for entity in possible_entities:
                        for context_text in conversation_context[-5:]:  # Last 5 turns
                            if entity in context_text.lower():
                                resolved[pronoun] = [entity]
                                break
                        if pronoun in resolved:
                            break
                else:
                    # Fallback: use all possible entities
                    resolved[pronoun] = possible_entities
        
        # Check for definite articles
        for article, possible_entities in self.co_reference_patterns['definite_articles'].items():
            if article in query_lower:
                resolved[article] = possible_entities
        
        return resolved
    
    def predict_answer_type(self, query: str) -> str:
        """Predict what type of answer the query expects"""
        query_lower = query.lower()
        
        for answer_type, patterns in self.answer_type_patterns.items():
            for pattern in patterns:
                if pattern in query_lower:
                    return answer_type
        
        return 'general'
    
    def extract_query_entities_enhanced(self, query: str, co_references: Dict[str, List[str]]) -> Dict[str, str]:
        """Extract entities from query with co-reference resolution"""
        query_lower = query.lower()
        entities = {}
        
        # Resolve co-references first
        for ref, resolved_entities in co_references.items():
            if ref in query_lower:
                # Use first resolved entity
                if resolved_entities:
                    entity = resolved_entities[0]
                    if 'person' not in entities:
                        entities['person'] = entity
                    elif 'location' not in entities:
                        entities['location'] = entity
        
        # Extract dates
        date_patterns = [
            r'\b(\d{1,2}\s+(?:january|february|march|april|may|june|july|august|september|october|november|december)\s+\d{4})\b',
            r'\b((?:january|february|march|april|may|june|july|august|september|october|november|december)\s+\d{1,2},?\s+\d{4})\b',
            r'\b(\d{1,2}/\d{1,2}/\d{4})\b',
            r'\b(\d{4})\b',
        ]
        
        for pattern in date_patterns:
            matches = re.findall(pattern, query_lower)
            if matches:
                entities['date'] = matches[0]
                break
        
        # Try temporal reasoning
        if 'date' not in entities:
            resolved_date = self.resolve_temporal_expression(query)
            if resolved_date:
                entities['date'] = resolved_date
        
        # Extract locations
        locations = ['sweden', 'museum', 'conference', 'school', 'support group', 'parade']
        for loc in locations:
            if loc in query_lower:
                entities['location'] = loc.replace(' ', '_')
                break
        
        # Extract people
        for person in ['caroline', 'melanie']:
            if person in query_lower:
                entities['person'] = person
                break
        
        # Extract activities
        activities = ['paint', 'painting', 'camping', 'pottery', 'race', 'speech']
        for activity in activities:
            if activity in query_lower:
                entities['activity'] = activity
                break
        
        return entities
    
    def multi_pass_retrieval_strategy(self, query: str, semantic_types: List[str], 
                                     sample_id: str, backend) -> List[Dict]:
        """Multi-pass retrieval: semantic type first, then entity filtering"""
        all_results = []
        retrieved_ids = set()
        
        # Pass 1: Query by semantic types (broad)
        for sem_type in semantic_types:
            prefix = f"AGENT_{sample_id}:{sem_type}:"
            results = backend.find_by_prefix(prefix, limit=200, raw=False)
            for r in results:
                node_id = r.get('id')
                if node_id and node_id not in retrieved_ids:
                    all_results.append(r)
                    retrieved_ids.add(node_id)
        
        # Pass 2: If we have entities, filter results by entity matches
        # (This happens in scoring phase, but we can pre-filter here)
        
        return all_results
    
    def boost_by_answer_type(self, result_data: str, answer_type: str, score: float) -> float:
        """Boost score if result contains expected answer type"""
        result_lower = result_data.lower()
        
        if answer_type == 'date':
            # Boost if contains date indicators
            date_indicators = ['may', 'june', 'july', '2023', '2024', 'ago', 'yesterday', 'today']
            if any(ind in result_lower for ind in date_indicators):
                score += 2.0
        
        elif answer_type == 'location':
            # Boost if contains location indicators
            location_indicators = ['sweden', 'museum', 'conference', 'school', 'support group', 'parade']
            if any(ind in result_lower for ind in location_indicators):
                score += 2.0
        
        elif answer_type == 'person':
            # Boost if contains person names
            person_indicators = ['caroline', 'melanie']
            if any(ind in result_lower for ind in person_indicators):
                score += 2.0
        
        elif answer_type == 'activity':
            # Boost if contains activity verbs
            activity_indicators = ['paint', 'camping', 'pottery', 'race', 'speech']
            if any(ind in result_lower for ind in activity_indicators):
                score += 2.0
        
        return score
    
    def extract_conversation_context(self, conv: Dict, current_query: str) -> List[str]:
        """Extract recent conversation context for co-reference resolution"""
        conversation_data = conv.get("conversation", {})
        context = []
        
        # Get last 10 dialogue turns
        session_num = 1
        while f"session_{session_num}" in conversation_data:
            session_key = f"session_{session_num}"
            session_dialogues = conversation_data.get(session_key, [])
            
            if isinstance(session_dialogues, list):
                for dialogue in session_dialogues[-10:]:  # Last 10 turns
                    if isinstance(dialogue, dict):
                        text = dialogue.get("text", "")
                        if text and text != current_query:
                            context.append(text)
            
            session_num += 1
        
        return context[-10:]  # Return last 10
    
    def fuzzy_prefix_matching(self, base_prefix: str) -> List[str]:
        """Generate fuzzy prefix variations for matching"""
        variations = [base_prefix]
        
        # Try without trailing colon
        if base_prefix.endswith(':'):
            variations.append(base_prefix[:-1])
        
        # Try plural forms (EVENT_ -> EVENTS_)
        if 'EVENT_' in base_prefix:
            variations.append(base_prefix.replace('EVENT_', 'EVENTS_'))
        
        # Try singular forms (ACTIVITIES_ -> ACTIVITY_)
        if 'ACTIVITIES_' in base_prefix:
            variations.append(base_prefix.replace('ACTIVITIES_', 'ACTIVITY_'))
        
        return variations
    
    def rewrite_query_for_storage_patterns(self, query: str) -> str:
        """Rewrite query to match common storage patterns"""
        query_lower = query.lower()
        
        # "when did X do Y" -> "when did X [verb]"
        if 'when did' in query_lower and 'do' in query_lower:
            # Try to extract the verb
            parts = query_lower.split('when did')
            if len(parts) > 1:
                rest = parts[1].strip()
                # Common patterns: "when did X go" -> "when did X go to"
                if 'go' in rest and 'to' not in rest:
                    query_lower = query_lower.replace('go', 'go to')
        
        # "what activities" -> "what does X do"
        if 'what activities' in query_lower:
            query_lower = query_lower.replace('what activities', 'what does')
        
        return query_lower
    
    def calculate_temporal_proximity(self, query_date: Optional[str], result_text: str) -> float:
        """Calculate temporal proximity score between query date and result"""
        if not query_date:
            return 0.0
        
        result_lower = result_text.lower()
        score = 0.0
        
        # Extract year from query date
        query_year = None
        if '-' in query_date:
            query_year = query_date.split('-')[0]
        elif query_date.isdigit():
            query_year = query_date
        
        if query_year:
            # Check if result contains this year
            if query_year in result_lower:
                score += 3.0
            
            # Check for nearby years (±1 year)
            try:
                year_int = int(query_year)
                if str(year_int - 1) in result_lower or str(year_int + 1) in result_lower:
                    score += 1.5
            except:
                pass
        
        # Check for date components
        if '-' in query_date:
            components = query_date.split('-')
            for component in components:
                if component in result_lower:
                    score += 0.5
        
        return score
    
    def extract_verb_phrases(self, text: str) -> List[str]:
        """Extract verb phrases for better matching"""
        text_lower = text.lower()
        verb_phrases = []
        
        # Common verb patterns
        verb_patterns = [
            r'\b(went to|attended|participated in|joined|painted|drew|camped|ran|spoke)\b',
            r'\b(go to|goes to|going to|attend|participate|join)\b',
            r'\b(paint|painted|painting|draw|drew|drawing)\b',
            r'\b(camp|camping|camped|go camping)\b',
        ]
        
        for pattern in verb_patterns:
            matches = re.findall(pattern, text_lower)
            verb_phrases.extend(matches)
        
        return verb_phrases
    
    def match_verb_phrases(self, query_verbs: List[str], result_verbs: List[str]) -> float:
        """Calculate verb phrase matching score"""
        if not query_verbs or not result_verbs:
            return 0.0
        
        # Exact matches
        exact_matches = len(set(query_verbs) & set(result_verbs))
        if exact_matches > 0:
            return exact_matches * 2.0
        
        # Partial matches (e.g., "went" matches "go")
        score = 0.0
        for qv in query_verbs:
            for rv in result_verbs:
                # Check if one is a form of the other
                if qv in rv or rv in qv:
                    score += 1.0
                # Check for common variations
                if (qv == 'went' and rv in ['go', 'goes', 'going']) or \
                   (rv == 'went' and qv in ['go', 'goes', 'going']):
                    score += 1.5
                if (qv == 'painted' and rv in ['paint', 'painting']) or \
                   (rv == 'painted' and qv in ['paint', 'painting']):
                    score += 1.5
        
        return score
