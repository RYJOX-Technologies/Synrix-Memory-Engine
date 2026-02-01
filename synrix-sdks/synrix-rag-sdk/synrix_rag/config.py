"""
Configuration management for SYNRIX Local RAG SDK.

Supports loading API keys from config file, environment variables, or direct parameters.
"""

import os
import json
from pathlib import Path
from typing import Optional, Dict, Any

# Default config file locations (in order of preference)
CONFIG_PATHS = [
    Path.home() / ".synrix_rag" / "config.json",
    Path.cwd() / ".synrix_rag" / "config.json",
    Path.cwd() / "synrix_rag_config.json",
]


def load_config(config_path: Optional[str] = None) -> Dict[str, Any]:
    """
    Load configuration from file or return empty dict.
    
    Args:
        config_path: Optional path to config file. If None, searches default locations.
        
    Returns:
        Dictionary with configuration values
    """
    config = {}
    
    # Try user-specified path first
    if config_path:
        config_file = Path(config_path)
        if config_file.exists():
            try:
                with open(config_file, 'r') as f:
                    config = json.load(f)
                return config
            except (json.JSONDecodeError, IOError) as e:
                print(f"[WARN] Failed to load config from {config_path}: {e}")
    
    # Try default locations
    for config_file in CONFIG_PATHS:
        if config_file.exists():
            try:
                with open(config_file, 'r') as f:
                    config = json.load(f)
                return config
            except (json.JSONDecodeError, IOError):
                continue
    
    return config


def get_api_key(
    service: str,
    api_key: Optional[str] = None,
    config_path: Optional[str] = None
) -> Optional[str]:
    """
    Get API key from parameter, config file, or environment variable.
    
    Priority order:
    1. Direct parameter (api_key)
    2. Config file
    3. Environment variable
    
    Args:
        service: Service name ("openai" or "cohere")
        api_key: Direct API key parameter (highest priority)
        config_path: Optional path to config file
        
    Returns:
        API key string or None if not found
    """
    # Priority 1: Direct parameter
    if api_key:
        return api_key
    
    # Priority 2: Config file
    config = load_config(config_path)
    config_key = f"{service}_api_key"
    if config_key in config:
        return config[config_key]
    
    # Priority 3: Environment variable
    env_key = f"{service.upper()}_API_KEY"
    return os.getenv(env_key)


def create_config_file(
    config_path: Optional[str] = None,
    openai_key: Optional[str] = None,
    cohere_key: Optional[str] = None
) -> Path:
    """
    Create a config file with API keys.
    
    Args:
        config_path: Path where to create config file (default: ~/.synrix_rag/config.json)
        openai_key: OpenAI API key to store
        cohere_key: Cohere API key to store
        
    Returns:
        Path to created config file
    """
    if config_path:
        config_file = Path(config_path)
    else:
        config_file = CONFIG_PATHS[0]  # ~/.synrix_rag/config.json
    
    # Create directory if needed
    config_file.parent.mkdir(parents=True, exist_ok=True)
    
    config = {}
    if config_file.exists():
        try:
            with open(config_file, 'r') as f:
                config = json.load(f)
        except (json.JSONDecodeError, IOError):
            pass
    
    # Update with provided keys
    if openai_key:
        config["openai_api_key"] = openai_key
    if cohere_key:
        config["cohere_api_key"] = cohere_key
    
    # Write config file
    with open(config_file, 'w') as f:
        json.dump(config, f, indent=2)
    
    # Set restrictive permissions (Unix only)
    if os.name != 'nt':
        os.chmod(config_file, 0o600)
    
    return config_file
