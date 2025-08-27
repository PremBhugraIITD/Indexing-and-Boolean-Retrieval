#include <iostream>
#include <unordered_set>
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <cstdlib>
#include <cstdio>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "tokenizer.h"
#include "utilities.h"

using namespace std;

// Function to copy file from source to destination
bool copy_file(const string &source, const string &destination)
{
    ifstream src(source, ios::binary);
    if (!src.is_open())
    {
        cerr << "Error: Cannot open source file: " << source << endl;
        return false;
    }

    ofstream dst(destination, ios::binary);
    if (!dst.is_open())
    {
        cerr << "Error: Cannot create destination file: " << destination << endl;
        return false;
    }

    dst << src.rdbuf();
    return true;
}

// Main function as required by the assignment
void build_vocab(string corpus_dir, string stopwords_file, string vocab_dir)
{
    // Create vocab directory if it doesn't exist
    if (!create_directory_if_not_exists(vocab_dir))
    {
        cerr << "Error: Failed to create vocab directory: " << vocab_dir << endl;
        return;
    }

    // Load stopwords from the provided file
    auto stopwords = load_stopwords(stopwords_file);
    if (stopwords.empty())
    {
        cerr << "Warning: No stopwords loaded from: " << stopwords_file << endl;
    }

    // Copy stopwords.txt to vocab_dir for use by other tasks
    string stopwords_destination = vocab_dir + "/stopwords.txt";
    if (!copy_file(stopwords_file, stopwords_destination))
    {
        cerr << "Warning: Failed to copy stopwords file to: " << stopwords_destination << endl;
    }

    // Build vocabulary from JSON corpus
    set<string> vocab;

    // Find JSON corpus file in the corpus directory
    vector<string> corpus_files = get_files_in_directory(corpus_dir);
    string corpus_file_path;

    // Look for JSON file in corpus directory
    for (const string &file_path : corpus_files)
    {
        if (file_path.find(".json") != string::npos)
        {
            corpus_file_path = file_path;
            break;
        }
    }

    if (corpus_file_path.empty())
    {
        cerr << "Error: No JSON corpus file found in directory: " << corpus_dir << endl;
        return;
    }

    // Open the JSON corpus file
    ifstream corpus_file(corpus_file_path);
    if (!corpus_file.is_open())
    {
        cerr << "Error: Cannot open corpus file: " << corpus_file_path << endl;
        return;
    }

    string json_line;
    int doc_count = 0;
    while (getline(corpus_file, json_line))
    {
        if (json_line.empty())
            continue;

        // Parse JSON document
        Document doc = parse_json_document(json_line);
        if (doc.doc_id.empty() || doc.content.empty())
        {
            cerr << "Warning: Skipping invalid JSON line" << endl;
            continue;
        }

        // Tokenize the content
        vector<string> tokens = tokenize(doc.content, stopwords);
        vocab.insert(tokens.begin(), tokens.end());
        doc_count++;
    }
    corpus_file.close();

    // Save vocabulary to vocab.txt
    string vocab_file_path = vocab_dir + "/vocab.txt";
    ofstream out(vocab_file_path);
    if (!out.is_open())
    {
        cerr << "Error: Cannot create vocab file: " << vocab_file_path << endl;
        return;
    }

    for (const string &word : vocab)
    {
        out << word << "\n";
    }
    out.close();

    cout << "Vocabulary created from " << doc_count << " documents with " << vocab.size() << " unique tokens." << endl;
    cout << "Files created:" << endl;
    cout << "  - " << vocab_file_path << endl;
    cout << "  - " << stopwords_destination << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <corpus_dir> <stopwords_file> <vocab_dir>" << endl;
        return 1;
    }

    string corpus_dir = argv[1];
    string stopwords_file = argv[2];
    string vocab_dir = argv[3];

    // Call the build_vocab function with the required signature
    build_vocab(corpus_dir, stopwords_file, vocab_dir);

    return 0;
}
