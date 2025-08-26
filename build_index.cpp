#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
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

unordered_set<string> load_vocab(const string &vocab_file)
{
    unordered_set<string> V;
    ifstream fin(vocab_file);
    string w;
    while (fin >> w)
        V.insert(w);
    return V;
}

unordered_set<string> load_stopwords(const string &stopwords_file)
{
    unordered_set<string> S;
    ifstream fin(stopwords_file);
    string w;
    while (fin >> w)
        S.insert(w);
    return S;
}

string json_escape(const string &s)
{
    string out;
    for (char c : s)
    {
        switch (c)
        {
        case '\"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out += c;
        }
    }
    return out;
}

// Type definition for inverted index as required by assignment
typedef unordered_map<string, unordered_map<string, vector<int>>> inverted_index;

// Function to build inverted index as required by assignment
inverted_index build_index(string collection_dir, string vocab_path);

// Function to save inverted index as required by assignment  
void save_index(inverted_index index, string index_dir);

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <corpus_dir> <vocab_file> <index_dir>" << endl;
        return 1;
    }

    string corpus_dir = argv[1];
    string vocab_file = argv[2];
    string index_dir = argv[3];

    // Build the inverted index using required function
    auto index = build_index(corpus_dir, vocab_file);
    
    // Save the index using required function
    save_index(index, index_dir);
    
    return 0;
}

// Function implementations

inverted_index build_index(string collection_dir, string vocab_path)
{
    auto V = load_vocab(vocab_path);

    // Load stopwords from vocab directory (set up by Task 1)
    unordered_set<string> stopwords;
    string vocab_dir = vocab_path.substr(0, vocab_path.find_last_of("/\\"));
    string stopwords_path = vocab_dir + "/stopwords.txt";
    
    ifstream stopwords_file(stopwords_path);
    if (stopwords_file.is_open()) {
        stopwords = load_stopwords(stopwords_path);
    }

    inverted_index index;

    // Use system command to list files and save to temp file
    string temp_file = "temp_filelist.txt";
#ifdef _WIN32
    string command = "dir /b \"" + collection_dir + "\" > " + temp_file;
#else
    string command = "ls \"" + collection_dir + "\" > " + temp_file;
#endif
    
    system(command.c_str());
    
    // Read the file list
    ifstream file_list(temp_file);
    string filename;
    while (getline(file_list, filename)) {
        if (filename.empty()) continue;
        
        string filepath = collection_dir + "/" + filename;
        ifstream fin(filepath);
        if (!fin.is_open()) continue;

        string line, content;
        while (getline(fin, line))
            content += line + " ";
        auto tokens = tokenize(content, stopwords);
        cerr << "Doc: " << filename << "\n";
        int pos = 0;
        for (auto &tok : tokens) {
            cerr << "  token: [" << tok << "]";
            if (V.find(tok) != V.end()) {
                index[tok][filename].push_back(pos);
                cerr << " in vocab\n";
            } else {
                cerr << " not in vocab\n";
            }
            ++pos;
        }
        fin.close();
    }
    file_list.close();
    
    // Clean up temp file
    remove(temp_file.c_str());

    return index;
}

void save_index(inverted_index index, string index_dir)
{
    // Create directory if needed
#ifdef _WIN32
    _mkdir(index_dir.c_str());
#else
    system(("mkdir -p \"" + index_dir + "\"").c_str());
#endif

    string out_path = index_dir + "/index.json";
    ofstream out(out_path);

    vector<string> terms;
    for (auto &kv : index)
        terms.push_back(kv.first);
    sort(terms.begin(), terms.end());

    out << "{\n";
    for (size_t t = 0; t < terms.size(); t++) {
        const string &term = terms[t];
        out << "  \"" << json_escape(term) << "\": {\n";

        vector<string> docs;
        for (auto &kv : index[term])
            docs.push_back(kv.first);
        sort(docs.begin(), docs.end());

        for (size_t d = 0; d < docs.size(); d++) {
            const string &doc = docs[d];
            out << "    \"" << json_escape(doc) << "\": [";
            for (size_t i = 0; i < index[term][doc].size(); i++) {
                if (i) out << ", ";
                out << index[term][doc][i];
            }
            out << "]";
            out << (d + 1 < docs.size() ? ",\n" : "\n");
        }
        out << "  }";
        out << (t + 1 < terms.size() ? ",\n" : "\n");
    }
    out << "}\n";

    out.close();
    cout << "Inverted index saved to " << out_path << endl;
}
