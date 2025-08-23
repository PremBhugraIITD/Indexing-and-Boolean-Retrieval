#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <sstream>
#include <cctype>
using namespace std;

inline vector<string> tokenize(const string& text, const unordered_set<string>& stopwords) {
    string s = text;
    // Lowercase and remove digits
    for (char& c : s) {
        if (isdigit(c)) {
            c = ' ';
        } else {
            c = tolower(c);
        }
    }

    // Tokenize by whitespace
    vector<string> tokens;
    istringstream iss(s);
    string token;
    while (iss >> token) {
        if (!stopwords.count(token) && !token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}
