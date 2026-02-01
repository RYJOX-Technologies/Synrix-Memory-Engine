"""
Licensing and Usage Tracking for SYNRIX Local RAG SDK.

Tracks node usage and enforces tier limits.
"""

import os
import json
from pathlib import Path
from typing import Optional, Dict, Any
from enum import Enum

# Pricing tiers
class PricingTier(Enum):
    FREE = {"limit": 100_000, "price": 0, "name": "Free"}
    STARTER = {"limit": 1_000_000, "price": 99, "name": "Starter"}
    PRO = {"limit": 10_000_000, "price": 299, "name": "Pro"}
    ENTERPRISE = {"limit": 50_000_000, "price": 499, "name": "Enterprise"}
    UNLIMITED = {"limit": float('inf'), "price": 999, "name": "Unlimited"}


class LicenseManager:
    """
    Manages licenses and tracks usage.
    """
    
    def __init__(self, license_key: Optional[str] = None):
        """
        Initialize license manager.
        
        Args:
            license_key: License key (if None, uses free tier)
        """
        self.license_key = license_key or os.getenv("SYNRIX_RAG_LICENSE_KEY")
        self.license_file = Path.home() / ".synrix_rag" / "license.json"
        self.usage_file = Path.home() / ".synrix_rag" / "usage.json"
        
        # Load license
        self.tier = self._load_license()
        self.usage = self._load_usage()
    
    def _load_license(self) -> PricingTier:
        """Load license from file or environment."""
        if self.license_file.exists():
            try:
                with open(self.license_file, 'r') as f:
                    data = json.load(f)
                    tier_name = data.get("tier", "FREE")
                    return PricingTier[tier_name]
            except:
                pass
        
        # Default to free tier
        return PricingTier.FREE
    
    def _load_usage(self) -> Dict[str, Any]:
        """Load usage statistics."""
        if self.usage_file.exists():
            try:
                with open(self.usage_file, 'r') as f:
                    return json.load(f)
            except:
                pass
        return {"node_count": 0, "last_updated": None}
    
    def _save_usage(self):
        """Save usage statistics."""
        self.usage_file.parent.mkdir(parents=True, exist_ok=True)
        with open(self.usage_file, 'w') as f:
            json.dump(self.usage, f, indent=2)
    
    def get_current_node_count(self, synrix_memory) -> int:
        """Get current node count from SYNRIX."""
        if synrix_memory is None:
            return self.usage.get("node_count", 0)
        
        try:
            # Get actual count from SYNRIX
            if hasattr(synrix_memory, 'count'):
                actual_count = synrix_memory.count()
            elif hasattr(synrix_memory, 'backend') and hasattr(synrix_memory.backend, 'count'):
                actual_count = synrix_memory.backend.count()
            else:
                # Fallback: query all and count
                try:
                    all_nodes = synrix_memory.query("", limit=1000000)
                    actual_count = len(all_nodes) if isinstance(all_nodes, list) else 0
                except:
                    actual_count = self.usage.get("node_count", 0)
            
            # Update usage if it's different
            if actual_count != self.usage.get("node_count", 0):
                self.usage["node_count"] = actual_count
                self._save_usage()
            return actual_count
        except Exception as e:
            # Fallback to stored usage
            return self.usage.get("node_count", 0)
    
    def check_limit(self, current_count: int, additional_nodes: int = 0) -> tuple[bool, str]:
        """
        Check if operation would exceed tier limit.
        
        Returns:
            (allowed, message)
        """
        limit = self.tier.value["limit"]
        new_count = current_count + additional_nodes
        
        if new_count > limit:
            return False, f"Tier limit exceeded. Current: {current_count:,}, Limit: {limit:,}, Would be: {new_count:,}. Upgrade to higher tier."
        
        if new_count > limit * 0.9:  # 90% warning
            return True, f"Warning: Approaching tier limit ({new_count:,}/{limit:,}). Consider upgrading."
        
        return True, "OK"
    
    def get_tier_info(self) -> Dict[str, Any]:
        """Get current tier information."""
        return {
            "tier": self.tier.value["name"],
            "limit": self.tier.value["limit"],
            "price": self.tier.value["price"],
            "current_usage": self.usage.get("node_count", 0),
            "remaining": max(0, self.tier.value["limit"] - self.usage.get("node_count", 0))
        }
    
    def update_usage(self, node_count: int):
        """Update usage statistics."""
        self.usage["node_count"] = node_count
        import datetime
        self.usage["last_updated"] = datetime.datetime.now().isoformat()
        self._save_usage()


def get_license_manager(license_key: Optional[str] = None) -> LicenseManager:
    """Get license manager instance."""
    return LicenseManager(license_key=license_key)
