#!/bin/bash

# build.sh - Master build script for COL 764 Assignment 1
# This script compiles all C++ programs for the Indexing and Boolean Retrieval project
# Usage: bash build.sh

echo "======================================="
echo "COL 764 Assignment 1 - Build Script"
echo "Indexing and Boolean Retrieval"
echo "======================================="
echo ""

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Flag to track overall compilation success
OVERALL_SUCCESS=true

echo "Compiling all C++ programs..."
echo ""

# Task 1: Compile tokenize_corpus.cpp
echo "[1/3] Compiling Task 1: Custom Tokenizer (tokenize_corpus.cpp)..."
if g++ -std=c++11 -o "${SCRIPT_DIR}/tokenize_corpus" "${SCRIPT_DIR}/tokenize_corpus.cpp"; then
    echo "✓ tokenize_corpus.cpp compiled successfully"
else
    echo "✗ Error: tokenize_corpus.cpp compilation failed!"
    OVERALL_SUCCESS=false
fi
echo ""

# Tasks 2 & 3: Compile build_index.cpp (merged indexing and compression)
echo "[2/3] Compiling Tasks 2 & 3: Inverted Index and Compression (build_index.cpp)..."
if g++ -std=c++11 -o "${SCRIPT_DIR}/build_index" "${SCRIPT_DIR}/build_index.cpp"; then
    echo "✓ build_index.cpp compiled successfully"
else
    echo "✗ Error: build_index.cpp compilation failed!"
    OVERALL_SUCCESS=false
fi
echo ""

# Task 4: Compile retrieval.cpp
echo "[3/3] Compiling Task 4: Boolean Retrieval (retrieval.cpp)..."
if g++ -std=c++11 -o "${SCRIPT_DIR}/retrieval" "${SCRIPT_DIR}/retrieval.cpp"; then
    echo "✓ retrieval.cpp compiled successfully"
else
    echo "✗ Error: retrieval.cpp compilation failed!"
    OVERALL_SUCCESS=false
fi
echo ""

# Check overall success
if [ "$OVERALL_SUCCESS" = true ]; then
    echo "======================================="
    echo "✓ ALL PROGRAMS COMPILED SUCCESSFULLY!"
    echo "======================================="
    echo ""
    echo "Available executables:"
    echo "  - ./tokenize_corpus    (Task 1: Custom Tokenizer)"
    echo "  - ./build_index        (Tasks 2 & 3: Indexing and Compression)"
    echo "  - ./retrieval          (Task 4: Boolean Retrieval)"
    echo ""
    echo "Available shell scripts:"
    echo "  - ./tokenize_corpus.sh <CORPUS_DIR> <STOPWORDS_FILE> <VOCAB_DIR>"
    echo "  - ./build_index.sh     <CORPUS_DIR> <VOCAB_PATH> <INDEX_DIR> <COMPRESSED_DIR>"
    echo "  - ./retrieval.sh       <COMPRESSED_DIR> <QUERY_FILE_PATH> <OUTPUT_DIR>"
    echo ""
    echo "Build completed successfully. You can now run the individual task scripts."
    exit 0
else
    echo "======================================="
    echo "✗ BUILD FAILED!"
    echo "======================================="
    echo ""
    echo "Some programs failed to compile. Please check the error messages above."
    echo "Fix the compilation errors and run 'bash build.sh' again."
    exit 1
fi
