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

using namespace std;

// Function to create directory if it doesn't exist (C++11 compatible)
bool create_directory_if_not_exists(const string &path)
{
#ifdef _WIN32
    if (_access(path.c_str(), 0) != 0)
    {
        return _mkdir(path.c_str()) == 0 || system(("mkdir \"" + path + "\" 2>nul").c_str()) == 0;
    }
#else
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        return system(("mkdir -p \"" + path + "\"").c_str()) == 0;
    }
#endif
    return true; // Directory already exists
}

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

// Function to get all files in a directory (C++11 compatible)
vector<string> get_files_in_directory(const string &directory)
{
    vector<string> files;

#ifdef _WIN32
    WIN32_FIND_DATAA findFileData; // Use ASCII version explicitly
    string searchPath = directory + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findFileData); // Use ASCII version

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                files.push_back(directory + "/" + findFileData.cFileName);
            }
        } while (FindNextFileA(hFind, &findFileData) != 0); // Use ASCII version
        FindClose(hFind);
    }
#else
    string command = "find \"" + directory + "\" -maxdepth 1 -type f 2>/dev/null";
    FILE *pipe = popen(command.c_str(), "r");

    if (pipe)
    {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            string filepath = buffer;
            // Remove newline
            if (!filepath.empty() && filepath.back() == '\n')
            {
                filepath.pop_back();
            }
            if (!filepath.empty())
            {
                files.push_back(filepath);
            }
        }
        pclose(pipe);
    }
#endif

    return files;
}

unordered_set<string> load_stopwords(const string &stopwords_file)
{
    unordered_set<string> stopwords;
    ifstream file(stopwords_file);
    string word;
    while (file >> word)
        stopwords.insert(word);
    return stopwords;
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

    // Build vocabulary from corpus
    set<string> vocab;
    vector<string> corpus_files = get_files_in_directory(corpus_dir);

    if (corpus_files.empty())
    {
        cerr << "Warning: No files found in corpus directory: " << corpus_dir << endl;
    }

    for (const string &file_path : corpus_files)
    {
        ifstream file(file_path);
        if (!file.is_open())
        {
            cerr << "Warning: Cannot open file: " << file_path << endl;
            continue;
        }

        string line, content;
        while (getline(file, line))
        {
            content += line + " ";
        }

        vector<string> tokens = tokenize(content, stopwords);
        vocab.insert(tokens.begin(), tokens.end());
        file.close();
    }

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

    cout << "Vocabulary created with " << vocab.size() << " unique tokens." << endl;
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
