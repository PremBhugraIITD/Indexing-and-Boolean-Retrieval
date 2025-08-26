#!/bin/bash

# tokenize_corpus.sh - Shell script for Task 1: Custom Tokenizer
# Usage: ./tokenize_corpus.sh <CORPUS_DIR> <STOPWORDS_FILE> <VOCAB_DIR>

# Check if correct number of arguments provided
if [ $# -ne 3 ]; then
    echo "Usage: $0 <CORPUS_DIR> <STOPWORDS_FILE> <VOCAB_DIR>"
    echo "Example: $0 /path/to/corpus /path/to/stopwords.txt /path/to/vocab_dir"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Compile the C++ program
echo "Compiling tokenize_corpus.cpp..."
g++ -std=c++11 -o "${SCRIPT_DIR}/tokenize_corpus" "${SCRIPT_DIR}/tokenize_corpus.cpp"

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed!"
    exit 1
fi

echo "Compilation successful."

# Execute the compiled program with provided arguments
echo "Running tokenizer with arguments:"
echo "  Corpus Directory: $1"
echo "  Stopwords File: $2"
echo "  Vocabulary Directory: $3"
echo ""

"${SCRIPT_DIR}/tokenize_corpus" "$1" "$2" "$3"

# Check if execution was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "Task 1 (Custom Tokenizer) completed successfully!"
else
    echo "Error: Tokenization failed!"
    exit 1
fi
