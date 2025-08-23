#!/bin/bash
#Usage: ./tokenize_corpus.sh <corpus_dir> <stopwords_file> <vocab_dir>

# Ensure the script is run with the correct number of arguments
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <corpus_dir> <stopwords_file> <vocab_dir>"
    exit 1
fi

CORPUS_DIR="$1"
STOPWORDS_FILE="$2"
VOCAB_DIR="$3"

# Ensure output directory exists
mkdir -p "$VOCAB_DIR"

# Compile the C++ program if not already compiled
# if [ ! -f tokenize_corpus ]; then
g++ -std=c++17 -O2 tokenize_corpus.cpp -o tokenize_corpus
# fi
# Call the compiled C++ program
./tokenize_corpus "$CORPUS_DIR" "$STOPWORDS_FILE" "$VOCAB_DIR"

# Check if the program ran successfully
if [ $? -ne 0 ]; then
    echo "Error: Tokenization failed."
    exit 1
fi
# Notify the user of successful completion
echo "Tokenization completed successfully. Vocabulary saved to $VOCAB_DIR/vocab.txt"
# End of script
# Note: Ensure that the C++ program is compiled before running this script.
