"""
Setup script for SYNRIX Python SDK

This file is kept for backward compatibility.
Modern builds use pyproject.toml (PEP 517/518).
"""

from setuptools import setup, find_packages

# Read README
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="synrix",
    version="0.1.0",
    description="Python SDK for SYNRIX - A local-first knowledge graph system for AI applications",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="SYNRIX Team",
    author_email="contact@synrix.dev",
    url="https://github.com/synrix/synrix-python-sdk",
    packages=find_packages(exclude=["examples", "tests", "*.tests", "*.tests.*"]),
    python_requires=">=3.8",
    install_requires=[
        "requests>=2.28.0",
    ],
    extras_require={
        "langchain": [
            "langchain>=0.1.0",
            "langchain-community>=0.1.0",
            "langchain-core>=0.1.0",
        ],
        "openai": [
            "fastapi>=0.100.0",
            "uvicorn[standard]>=0.23.0",
            "pydantic>=2.0.0",
        ],
        "telemetry": [
            "psutil>=5.9.0",
        ],
        "dev": [
            "pytest>=7.0.0",
            "pytest-cov>=4.0.0",
            "black>=22.0.0",
            "flake8>=5.0.0",
            "mypy>=1.0.0",
        ],
        "all": [
            "langchain>=0.1.0",
            "langchain-community>=0.1.0",
            "langchain-core>=0.1.0",
            "fastapi>=0.100.0",
            "uvicorn[standard]>=0.23.0",
            "pydantic>=2.0.0",
            "psutil>=5.9.0",
        ],
    },
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Database",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Operating System :: OS Independent",
    ],
    keywords=[
        "knowledge-graph",
        "semantic-search",
        "ai",
        "rag",
        "vector-database",
        "langchain",
        "openai",
        "memory",
        "embeddings",
    ],
    entry_points={
        "console_scripts": [
            "synrix-tour=synrix.examples.tour:run_tour",
        ],
    },
)
