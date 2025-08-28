COL 764 Assignment 1: Indexing and Boolean Retrieval
======================================================

FILE DESCRIPTIONS
=================
tokenizer.h - Header-only tokenization functions for text preprocessing
utilities.h - Cross-platform file operations and JSON parsing utilities
tokenize_corpus.cpp - Task 1: Vocabulary extraction from JSON corpus
build_index.cpp - Tasks 2 & 3: Inverted index construction and compression
retrieval.cpp - Task 4: Boolean query processing and retrieval

SETUP AND COMPILATION
=====================
$ unzip 20XXCSXX999.zip
$ cd 20XXCSXX999
$ bash build.sh

EXECUTION
=========
Task 1 - Vocabulary Extraction:
$ bash tokenize_corpus.sh <corpus_dir> <stopwords_file> <vocab_dir>
Runs: tokenize_corpus executable

Task 2 & 3 - Index Construction & Compression:
$ bash build_index.sh <corpus_dir> <vocab_path> <index_dir> <compressed_dir>
Runs: build_index executable

Task 4 - Boolean Retrieval:
$ bash retrieval.sh <compressed_dir> <query_file> <output_dir>
Runs: retrieval executable

OUTPUT FILES
============
Task 1: vocab_dir/vocab.txt (vocabulary), vocab_dir/stopwords.txt
Task 2 & 3: index_dir/index.json (uncompressed), compressed_dir/postings.bin, doc_map.json, metadata.json
Task 4: output_dir/docids.txt (4-column format: qid docid rank score)
