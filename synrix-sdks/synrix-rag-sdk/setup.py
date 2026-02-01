"""
Setup script for SYNRIX Local RAG SDK.
"""

from setuptools import setup, find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

with open("requirements.txt", "r", encoding="utf-8") as fh:
    requirements = [line.strip() for line in fh if line.strip() and not line.startswith("#")]

setup(
    name="synrix-rag",
    version="1.0.0",
    author="RYJOX Technologies",
    author_email="support@ryjoxtechnologies.com",
    description="Local RAG SDK built on top of SYNRIX - Production-ready semantic search and document storage",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://ryjoxtechnologies.com",
    project_urls={
        "Documentation": "https://docs.ryjoxtechnologies.com/synrix-rag",
        "Source": "https://github.com/ryjox/synrix-rag",
        "Pricing": "https://ryjoxtechnologies.com/pricing",
    },
    packages=find_packages(),
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "License :: Other/Proprietary License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.8",
    install_requires=requirements,
    extras_require={
        "dev": [
            "pytest>=7.0.0",
            "black>=23.0.0",
            "flake8>=6.0.0",
        ],
        "langchain": [
            "langchain>=0.1.0",
        ],
    },
    keywords="rag, vector-database, semantic-search, embeddings, ai, machine-learning, synrix",
    include_package_data=True,
    zip_safe=False,
)
