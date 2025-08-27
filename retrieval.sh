#!/bin/bash

# retrieval.sh - Shell script for Task 4: Boolean Retrieval
# Usage: ./retrieval.sh <COMPRESSED_DIR> <QUERY_FILE_PATH> <OUTPUT_DIR>

# Check if correct number of arguments provided
if [ $# -ne 3 ]; then
    echo "Usage: $0 <COMPRESSED_DIR> <QUERY_FILE_PATH> <OUTPUT_DIR>"
    echo "Example: $0 /path/to/compressed_dir /path/to/queries.json /path/to/output_dir"
    exit 1
fi

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Compile the C++ program
echo "Compiling retrieval.cpp..."
g++ -std=c++11 -o "${SCRIPT_DIR}/retrieval" "${SCRIPT_DIR}/retrieval.cpp"

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Error: Compilation failed!"
    exit 1
fi

echo "Compilation successful."

# Execute the compiled program with provided arguments
echo "Running retrieval with arguments:"
echo "  Compressed Directory: $1"
echo "  Query File Path: $2"
echo "  Output Directory: $3"
echo ""

"${SCRIPT_DIR}/retrieval" "$1" "$2" "$3"

# Check if execution was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "Task 4 (Boolean Retrieval) completed successfully!"
else
    echo "Error: Boolean retrieval failed!"
    exit 1
fi
