#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif
using namespace std;

// Function to create directory if it doesn't exist (C++11 compatible)
inline bool create_directory_if_not_exists(const string &path)
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

// Document structure for JSON corpus processing
struct Document
{
    string doc_id;
    string content; // Combined indexable content
};

// Function to get all files in a directory (C++11 compatible)
inline vector<string> get_files_in_directory(const string &directory)
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

// Load stopwords from file
inline unordered_set<string> load_stopwords(const string &stopwords_file)
{
    unordered_set<string> stopwords;
    ifstream file(stopwords_file);
    string word;
    while (file >> word)
        stopwords.insert(word);
    return stopwords;
}

// Simple JSON parsing function for extracting field values from JSON lines
inline string extract_json_field(const string &json_line, const string &field)
{
    string field_key = "\"" + field + "\"";
    size_t key_pos = json_line.find(field_key);
    if (key_pos == string::npos)
        return "";

    size_t colon_pos = json_line.find(":", key_pos);
    if (colon_pos == string::npos)
        return "";

    size_t start_quote = json_line.find("\"", colon_pos);
    if (start_quote == string::npos)
        return "";

    size_t end_quote = start_quote + 1;
    while (end_quote < json_line.length() &&
           (json_line[end_quote] != '"' || json_line[end_quote - 1] == '\\'))
    {
        end_quote++;
    }

    if (end_quote >= json_line.length())
        return "";

    return json_line.substr(start_quote + 1, end_quote - start_quote - 1);
}

// Parse a JSON line into a Document structure
inline Document parse_json_document(const string &json_line)
{
    Document doc;
    doc.doc_id = extract_json_field(json_line, "doc_id");

    // Combine title and abstract as indexable content
    string title = extract_json_field(json_line, "title");
    string abstract = extract_json_field(json_line, "abstract");

    doc.content = title;
    if (!title.empty() && !abstract.empty())
    {
        doc.content += " ";
    }
    doc.content += abstract;

    return doc;
}
