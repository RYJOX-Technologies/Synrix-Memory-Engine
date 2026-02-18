"""
Setup script for SYNRIX Free Tier Package (25k node limit)
"""

from setuptools import setup, find_packages
from pathlib import Path

# Read README
readme_file = Path(__file__).parent / "README.md"
long_description = readme_file.read_text() if readme_file.exists() else ""

setup(
    name="synrix-free-tier-25k",
    version="0.1.0",
    description="SYNRIX Free Tier - AI Agent Memory System with Robotics SDK (25k node limit)",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="SYNRIX Team",
    packages=find_packages(),
    package_data={
        "synrix": [
            "*.dll",  # Windows DLLs
            "*.so",   # Linux shared libraries (if included)
        ],
    },
    python_requires=">=3.8",
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
    ],
)
