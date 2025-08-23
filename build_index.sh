#!/bin/bash
# Usage: ./build_index.sh <corpus_dir> <vocab_file> <index_dir>

# Ensure the script is run with the correct number of arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <corpus_dir> <vocab_file> <index_dir>"
    exit 1
fi

CORPUS_DIR="$1"
VOCAB_FILE="$2"
INDEX_DIR="$3"

# Ensure output directory exists
mkdir -p "$INDEX_DIR"

# Check input corpus directory
if [ ! -d "$CORPUS_DIR" ]; then
    echo "Error: Corpus directory not found: $CORPUS_DIR"
    exit 1
fi

# Check vocab file
if [ ! -f "$VOCAB_FILE" ]; then
    echo "Error: Vocab file not found: $VOCAB_FILE"
    exit 1
fi

# Compile the C++ program if not already compiled
# if [ ! -f build_index ]; then
echo "Compiling build_index.cpp..."
g++ -std=c++17 -O2 build_index.cpp -o build_index
if [ $? -ne 0 ]; then
    echo "Error: Compilation of build_index.cpp failed."
    exit 1
fi
# fi

# Call the compiled C++ program
./build_index "$CORPUS_DIR" "$VOCAB_FILE" "$INDEX_DIR"

# Check if the program ran successfully
if [ $? -ne 0 ]; then
    echo "Error: Inverted index construction failed."
    exit 1
fi

# Notify the user of successful completion
echo "Inverted index construction completed successfully."
echo "Index saved to $INDEX_DIR/index.json"
# End of script
