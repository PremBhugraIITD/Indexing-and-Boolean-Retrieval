# Indexing and Boolean Retrieval System

A complete C++11 implementation of an information retrieval system with inverted index construction, compression using variable-byte encoding, and Boolean query processing capabilities.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Requirements](#system-requirements)
- [Project Structure](#project-structure)
- [Installation](#installation)
- [Usage](#usage)
  - [Task 1: Vocabulary Extraction](#task-1-vocabulary-extraction)
  - [Task 2 & 3: Index Construction and Compression](#task-2--3-index-construction-and-compression)
  - [Task 4: Boolean Retrieval](#task-4-boolean-retrieval)
- [Architecture](#architecture)
- [Compression Techniques](#compression-techniques)
- [Query Syntax](#query-syntax)
- [Output Formats](#output-formats)
- [Implementation Details](#implementation-details)
- [Advanced Features](#advanced-features)
- [Performance](#performance)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## 🎯 Overview

This project implements a complete indexing and Boolean retrieval system for text document collections. It processes JSON-formatted corpora, builds compressed inverted indexes, and supports complex Boolean queries with AND, OR, and NOT operators.

**Key Capabilities:**
- Multi-file JSON corpus processing
- Vocabulary extraction with stopword filtering
- Positional inverted index construction
- Variable-byte encoding with delta compression
- Boolean query evaluation with operator precedence
- UTF-16 encoding support for real-world datasets

## ✨ Features

### Core Functionality
- ✅ **Custom Tokenization**: Lowercase conversion, digit removal, stopword filtering
- ✅ **Inverted Index**: Position-aware indexing for phrase query support
- ✅ **Compression**: Variable-byte encoding with delta encoding for positions
- ✅ **Boolean Queries**: Full support for AND, OR, NOT with parentheses
- ✅ **Cross-Platform**: Works on Windows and Linux

### Advanced Features
- 🌟 **Multi-file Corpus Support**: Processes directories with multiple corpus files
- 🌟 **UTF-16 Compatibility**: Automatic detection and conversion
- 🌟 **Implicit AND Insertion**: Natural query syntax (e.g., "information retrieval")
- 🌟 **Compression Statistics**: Detailed reporting of compression ratios
- 🌟 **Error Recovery**: Graceful handling of malformed JSON

## 💻 System Requirements

- **C++ Compiler**: g++ 4.8+ or clang++ with C++11 support
- **Operating System**: Linux, macOS, or Windows (with MinGW/MSVC)
- **Memory**: Sufficient RAM for corpus (typically 2-4GB for large datasets)
- **Disk Space**: ~3x corpus size for intermediate files

## 📁 Project Structure

```
.
├── README.md                  # This file
├── README.txt                 # Quick reference guide
├── tokenizer.h               # Core tokenization functions
├── utilities.h               # Cross-platform utilities and JSON parsing
├── tokenize_corpus.cpp       # Task 1: Vocabulary extraction
├── build_index.cpp           # Tasks 2 & 3: Indexing and compression
├── retrieval.cpp             # Task 4: Boolean query processing
├── build.sh                  # Master compilation script
├── tokenize_corpus.sh        # Task 1 execution script
├── build_index.sh            # Tasks 2 & 3 execution script
└── retrieval.sh              # Task 4 execution script
```

## 🚀 Installation

### Quick Start

```bash
# Extract the archive
unzip 20XXCSXX999.zip
cd 20XXCSXX999

# Compile all programs
bash build.sh
```

### Manual Compilation

**Linux/macOS:**
```bash
g++ -std=c++11 -O2 -o tokenize_corpus tokenize_corpus.cpp
g++ -std=c++11 -O2 -o build_index build_index.cpp
g++ -std=c++11 -O2 -o retrieval retrieval.cpp
```

**Windows (MinGW):**
```bash
g++ -std=c++11 -O2 -o tokenize_corpus.exe tokenize_corpus.cpp
g++ -std=c++11 -O2 -o build_index.exe build_index.cpp
g++ -std=c++11 -O2 -o retrieval.exe retrieval.cpp
```

## 📖 Usage

### Task 1: Vocabulary Extraction

Extracts unique vocabulary from JSON corpus and sets up stopwords.

```bash
bash tokenize_corpus.sh <corpus_dir> <stopwords_file> <vocab_dir>
```

**Example:**
```bash
bash tokenize_corpus.sh ./corpus ./stopwords.txt ./vocab_dir
```

**Input Format (JSON Lines):**
```json
{"doc_id": "doc1", "title": "Information Retrieval", "abstract": "This paper discusses..."}
{"doc_id": "doc2", "title": "Machine Learning", "abstract": "Deep learning methods..."}
```

**Outputs:**
- `vocab_dir/vocab.txt` - Sorted vocabulary (one term per line)
- `vocab_dir/stopwords.txt` - Copy of stopwords for later tasks

### Task 2 & 3: Index Construction and Compression

Builds inverted index and compresses it using variable-byte encoding.

```bash
bash build_index.sh <corpus_dir> <vocab_path> <index_dir> <compressed_dir>
```

**Example:**
```bash
bash build_index.sh ./corpus ./vocab_dir/vocab.txt ./index_dir ./compressed_dir
```

**Outputs:**

*Uncompressed (index_dir/):*
- `index.json` - Human-readable inverted index

*Compressed (compressed_dir/):*
- `postings.bin` - Variable-byte encoded postings lists
- `doc_map.json` - Document ID mapping (string → integer)
- `metadata.json` - Term metadata (offsets and lengths)

### Task 4: Boolean Retrieval

Processes Boolean queries and returns matching documents.

```bash
bash retrieval.sh <compressed_dir> <query_file> <output_dir>
```

**Example:**
```bash
bash retrieval.sh ./compressed_dir ./queries.json ./results
```

**Query Format (JSON Lines):**
```json
{"query_id": "Q1", "title": "information retrieval"}
{"query_id": "Q2", "title": "machine learning AND algorithms"}
{"query_id": "Q3", "title": "(data OR information) AND NOT medical"}
```

**Output:**
- `output_dir/docids.txt` - Results in 4-column format (qid docid rank score)

## 🏗️ Architecture

### Component Overview

```
┌─────────────────┐
│  tokenizer.h    │ ─┐
│  utilities.h    │  │ Shared Components
└─────────────────┘  │ (Header-only)
                     │
┌────────────────────┼────────────────────┐
│                    │                    │
▼                    ▼                    ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│Task 1       │  │Tasks 2 & 3  │  │Task 4       │
│Vocabulary   │→ │Index Build  │→ │Boolean      │
│Extraction   │  │& Compress   │  │Retrieval    │
└─────────────┘  └─────────────┘  └─────────────┘
```

### Data Flow

```
JSON Corpus Files
       ↓
   Tokenization (Task 1)
       ↓
   vocab.txt + stopwords.txt
       ↓
   Index Construction (Task 2)
       ↓
   index.json (uncompressed)
       ↓
   Compression (Task 3)
       ↓
   postings.bin + metadata.json + doc_map.json
       ↓
   Decompression & Query Processing (Task 4)
       ↓
   docids.txt (results)
```

## 🗜️ Compression Techniques

### 1. Document ID Mapping
Converts string document IDs to compact integer representations.

**Example:**
```
"PMC123456" → 0
"PMC789012" → 1
```

### 2. Variable-Byte Encoding
Encodes integers using 7 bits per byte, with MSB as continuation bit.

**Example:**
```
127 → [127]                    (1 byte)
128 → [128, 1]                 (2 bytes)
16384 → [128, 128, 1]          (3 bytes)
```

### 3. Delta Encoding
Stores position differences instead of absolute positions.

**Example:**
```
Positions: [5, 7, 12, 15, 20]
Encoded:   [5, 2,  5,  3,  5]
```

### Compression Results
- **Typical Ratio**: 2x - 5x compression
- **Space Savings**: 50-80% reduction in index size
- **Fast Decompression**: O(n) time complexity

## 🔍 Query Syntax

### Supported Operators

| Operator | Precedence | Associativity | Description |
|----------|------------|---------------|-------------|
| NOT      | 3 (High)   | Right         | Negation    |
| AND      | 2 (Medium) | Left          | Intersection|
| OR       | 1 (Low)    | Left          | Union       |

### Query Examples

```sql
-- Simple term query (implicit AND)
information retrieval

-- Explicit Boolean operators
information AND retrieval
machine OR learning
data AND NOT medical

-- Complex queries with parentheses
(information OR data) AND retrieval
machine AND learning AND NOT neural
(covid OR coronavirus) AND (vaccine OR treatment)

-- Operator precedence
A OR B AND C           -- Equivalent to: A OR (B AND C)
A AND NOT B OR C       -- Equivalent to: (A AND (NOT B)) OR C
```

### Query Preprocessing

1. **Tokenization**: Lowercase, digit removal, stopword filtering
2. **Operator Canonicalization**: Convert to uppercase (AND, OR, NOT)
3. **Implicit AND Insertion**: "term1 term2" → "term1 AND term2"
4. **Parentheses Spacing**: "term(query)" → "term ( query )"

## 📊 Output Formats

### Vocabulary File (vocab.txt)
```
algorithm
data
information
learning
machine
retrieval
```

### Index File (index.json)
```json
{
  "information": {
    "doc1": [0, 5, 12],
    "doc3": [2, 8]
  },
  "retrieval": {
    "doc1": [1, 13],
    "doc2": [4]
  }
}
```

### Results File (docids.txt)
```
Q1 doc1 1 1
Q1 doc3 2 1
Q2 doc2 1 1
```

**Format**: `<query_id> <doc_id> <rank> <score>`
- Rank: 1-based ranking
- Score: Always 1 for Boolean retrieval
- Documents sorted lexicographically

## 🔧 Implementation Details

### Tokenization Algorithm
```cpp
1. Convert text to lowercase
2. Replace digits with spaces
3. Split on whitespace
4. Filter out stopwords
5. Return token list
```

### Index Construction
```cpp
for each document in corpus:
    tokens = tokenize(document.content)
    for position, token in enumerate(tokens):
        if token in vocabulary:
            index[token][doc_id].append(position)
```

### Boolean Evaluation (AST-based)
```cpp
1. Preprocess query (tokenize, insert implicit ANDs)
2. Convert infix to postfix (Shunting Yard Algorithm)
3. Build Abstract Syntax Tree (AST)
4. Recursive evaluation:
   - Leaf nodes: Lookup postings
   - AND nodes: Set intersection
   - OR nodes: Set union
   - NOT nodes: Set difference (universe - operand)
```

## 🌟 Advanced Features

### UTF-16 Support
Automatically detects and converts UTF-16 Little Endian encoded files.

```cpp
// BOM Detection
if (file[0] == 0xFF && file[1] == 0xFE) {
    // UTF-16 LE detected, convert to ASCII
}
```

### Multi-file Corpus Processing
Processes all files in a directory automatically.

```bash
corpus/
├── file1.json    # Processed
├── file2.json    # Processed
├── file3.json    # Processed
└── ...
```

### Error Recovery
- Gracefully handles malformed JSON lines
- Continues processing on file-level errors
- Validates query syntax before evaluation

### Cross-Platform Compatibility
- Conditional compilation for Windows/Linux
- Platform-specific file operations
- Consistent path handling

## 📈 Performance

### Time Complexity
- **Vocabulary Extraction**: O(N × M) where N = documents, M = avg doc length
- **Index Construction**: O(N × M × log V) where V = vocabulary size
- **Compression**: O(T × D) where T = terms, D = avg postings length
- **Query Evaluation**: O(Q × R) where Q = query complexity, R = result size

### Space Complexity
- **Vocabulary**: O(V) where V = unique terms
- **Uncompressed Index**: O(T × D × P) where P = avg positions per posting
- **Compressed Index**: ~20-50% of uncompressed size
- **Runtime**: O(I) where I = index size for decompression

### Benchmarks (Approximate)
- **Vocabulary**: ~1000 docs/second
- **Indexing**: ~500 docs/second
- **Compression**: ~1000 terms/second
- **Retrieval**: ~100-1000 queries/second

## 💡 Examples

### Complete Workflow

```bash
# Step 1: Extract vocabulary
bash tokenize_corpus.sh ./my_corpus ./stopwords.txt ./vocab

# Step 2: Build and compress index
bash build_index.sh ./my_corpus ./vocab/vocab.txt ./index ./compressed

# Step 3: Process queries
bash retrieval.sh ./compressed ./queries.json ./results

# View results
cat ./results/docids.txt
```

### Example Corpus (corpus/doc1.json)
```json
{"doc_id": "paper001", "title": "Information Retrieval Systems", "abstract": "Modern IR systems use inverted indexes for efficient document retrieval."}
{"doc_id": "paper002", "title": "Machine Learning Algorithms", "abstract": "Deep learning has revolutionized natural language processing."}
```

### Example Queries (queries.json)
```json
{"query_id": "1", "title": "information retrieval"}
{"query_id": "2", "title": "machine learning"}
{"query_id": "3", "title": "(information OR data) AND NOT medical"}
```

### Expected Output (results/docids.txt)
```
1 paper001 1 1
2 paper002 1 1
```

## 🐛 Troubleshooting

### Common Issues

**Problem**: Compilation errors
```bash
# Solution: Ensure C++11 support
g++ -std=c++11 -o program program.cpp
```

**Problem**: Cannot open corpus file
```bash
# Solution: Check file paths are correct
ls -la corpus/
```

**Problem**: Empty query results
```bash
# Solution: Verify terms exist in vocabulary
grep "searchterm" vocab/vocab.txt
```

**Problem**: UTF-16 encoding issues
```bash
# Solution: System auto-detects UTF-16, but verify BOM
hexdump -C queries.json | head -1
# Should show: FF FE for UTF-16 LE
```

**Problem**: Stopwords not filtering
```bash
# Solution: Ensure stopwords.txt is in correct location
ls -la vocab/stopwords.txt
```

### Debug Mode

Add debug output by modifying source code:
```cpp
// In retrieval.cpp, uncomment:
// print_tree(root);  // Visualize query AST
```

## 📚 References

### Algorithms Used
- **Shunting Yard Algorithm**: Infix to postfix conversion
- **Variable-Byte Encoding**: Integer compression
- **Delta Encoding**: Position list compression
- **Set Operations**: STL algorithms (intersection, union, difference)

### Standards
- **C++11**: ISO/IEC 14882:2011
- **JSON Lines**: http://jsonlines.org/
- **UTF-16**: Unicode Standard

## 📄 License

This project is part of COL 764 coursework at IIT Delhi.

## 👥 Author

**Repository**: [PremBhugraIITD/Indexing-and-Boolean-Retrieval](https://github.com/PremBhugraIITD/Indexing-and-Boolean-Retrieval)

---

## 🙏 Acknowledgments

- COL 764 Course Staff
- CORD-19 Dataset (for testing UTF-16 support)
- C++ Standard Library Documentation

---

**Last Updated**: October 2025

For quick reference, see [README.txt](README.txt) for concise usage instructions.
