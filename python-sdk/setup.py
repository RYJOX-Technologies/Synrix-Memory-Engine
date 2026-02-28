"""
Setup script for SYNRIX Python SDK

This file is kept for backward compatibility.
Modern builds use pyproject.toml (PEP 517/518).
"""

from setuptools import setup, find_packages
from setuptools.command.install import install

# Read README
with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()


class PostInstallCommand(install):
    """Post-installation: Download binary for current platform"""
    def run(self):
        # Run standard install
        install.run(self)
        
        # Download binary (optional, don't fail if it doesn't work)
        try:
            from synrix._download_binary import post_install_download
            post_install_download(verbose=True)
        except Exception as e:
            # Don't fail installation if binary download fails
            print(f"\n⚠️  Note: Could not auto-download binary: {e}")
            print("   You can download it manually from: https://github.com/synrix/synrix/releases")


setup(
    name="synrix",
    version="0.2.0",
    description="Local AI memory engine — O(k) retrieval, no embeddings, no cloud, one binary",
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
        "ai-memory",
        "local-first",
        "edge-ai",
        "knowledge-graph",
        "semantic-search",
        "rag",
        "no-embeddings",
        "vector-database",
        "langchain",
        "qdrant-compatible",
    ],
    entry_points={
        "console_scripts": [
            "synrix-tour=synrix.examples.tour:run_tour",
        ],
    },
    cmdclass={
        'install': PostInstallCommand,
    },
)
