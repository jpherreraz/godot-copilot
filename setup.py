from setuptools import setup, find_packages

setup(
    name="godot_docs_retriever",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "langchain-community>=0.0.20",
        "chromadb==0.4.22",
        "sentence-transformers==2.5.1"
    ],
) 