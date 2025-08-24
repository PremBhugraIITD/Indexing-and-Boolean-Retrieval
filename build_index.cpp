#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "tokenizer.h"

using namespace std;
namespace fs = filesystem;

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

    if (!fs::exists(corpus_dir) || !fs::is_directory(corpus_dir))
    {
        cerr << "Error: corpus_dir not found: " << corpus_dir << endl;
        return 1;
    }
    if (!fs::exists(vocab_file))
    {
        cerr << "Error: vocab_file not found: " << vocab_file << endl;
        return 1;
    }
    fs::create_directories(index_dir);

    auto V = load_vocab(vocab_file);

    // Load stopwords (must match tokenizer’s)
    unordered_set<string> stopwords;
    if (fs::exists("stopwords.txt"))
        stopwords = load_stopwords("stopwords.txt");

    unordered_map<string, unordered_map<string, vector<int>>> index;

    for (const auto &entry : fs::directory_iterator(corpus_dir))
    {
        if (!fs::is_regular_file(entry))
            continue;

        string docID = entry.path().filename().string();
        ifstream fin(entry.path());
        if (!fin.is_open())
            continue;

        string line, content;
        while (getline(fin, line))
            content += line + " ";
        auto tokens = tokenize(content, stopwords);
        cerr << "Doc: " << docID << "\n";
        int pos = 0;
        for (auto &tok : tokens)
        {
            cerr << "  token: [" << tok << "]";
            if (V.find(tok) != V.end())
            {
                index[tok][docID].push_back(pos);
                cerr << " in vocab\n";
            }
            else
            {
                cerr << " not in vocab\n";
            }
            ++pos;
        }
    }

    string out_path = (fs::path(index_dir) / "index.json").string();
    ofstream out(out_path);

    vector<string> terms;
    for (auto &kv : index)
        terms.push_back(kv.first);
    sort(terms.begin(), terms.end());

    out << "{\n";
    for (size_t t = 0; t < terms.size(); t++)
    {
        const string &term = terms[t];
        out << "  \"" << json_escape(term) << "\": {\n";

        vector<string> docs;
        for (auto &kv : index[term])
            docs.push_back(kv.first);
        sort(docs.begin(), docs.end());

        for (size_t d = 0; d < docs.size(); d++)
        {
            const string &doc = docs[d];
            out << "    \"" << json_escape(doc) << "\": [";
            for (size_t i = 0; i < index[term][doc].size(); i++)
            {
                if (i)
                    out << ", ";
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
    return 0;
}
