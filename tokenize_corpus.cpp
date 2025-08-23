#include <iostream>
#include <unordered_set>
#include <set>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include "tokenizer.h"

using namespace std;
namespace fs = filesystem;

unordered_set<string> load_stopwords(const string& stopwords_file) {
    unordered_set<string> stopwords;
    ifstream file(stopwords_file);
    string word;
    while (file >> word) stopwords.insert(word);
    return stopwords;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <corpus_dir> <stopwords_file> <vocab_dir>" << endl;
        return 1;
    }

    string corpus_dir = argv[1];
    string stopwords_file = argv[2];
    string vocab_dir = argv[3];

    auto stopwords = load_stopwords(stopwords_file);
    set<string> vocab;

    for (const auto& entry : fs::directory_iterator(corpus_dir)) {
        ifstream file(entry.path());
        if (!file.is_open()) continue;

        string line, content;
        while (getline(file, line)) content += line + " ";
        vector<string> tokens = tokenize(content, stopwords);
        vocab.insert(tokens.begin(), tokens.end());
    }

    fs::create_directories(vocab_dir);
    ofstream out(vocab_dir + "/vocab.txt");
    for (const string& word : vocab) out << word << "\n";
    out.close();

    cout << "Vocabulary created with " << vocab.size() << " unique tokens." << endl;
    return 0;
}
