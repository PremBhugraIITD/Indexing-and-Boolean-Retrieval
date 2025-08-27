#!/bin/bash

# build_index.sh - Shell script for Task 2 & 3: Inverted Index and Index Compression
# Usage: ./build_index.sh <CORPUS_DIR> <VOCAB_PATH> <INDEX_DIR> <COMPRESSED_DIR>

# Check if correct number of arguments provided
if [ $# -ne 4 ]; then
    echo "Usage: $0 <CORPUS_DIR> <VOCAB_PATH> <INDEX_DIR> <COMPRESSED_DIR>"
    echo "Example: $0 /path/to/corpus /path/to/vocab.txt /path/to/index_dir /path/to/compressed_dir"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Compile the C++ program
echo "Compiling build_index.cpp..."
g++ -std=c++11 -o "${SCRIPT_DIR}/build_index" "${SCRIPT_DIR}/build_index.cpp"

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed!"
    exit 1
fi

echo "Compilation successful."

# Execute the compiled program with provided arguments
echo "Running build_index with arguments:"
echo "  Corpus Directory: $1"
echo "  Vocabulary Path: $2"
echo "  Index Directory: $3"
echo "  Compressed Directory: $4"
echo ""

"${SCRIPT_DIR}/build_index" "$1" "$2" "$3" "$4"

# Check if execution was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "Task 2 & 3 (Inverted Index and Compression) completed successfully!"
else
    echo "Error: Index building/compression failed!"
    exit 1
fi
