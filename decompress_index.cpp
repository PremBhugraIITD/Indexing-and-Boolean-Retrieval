#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <sstream>
#include <algorithm>
using namespace std;

// Helper function to get file size using standard C++
size_t get_file_size(const string &filename)
{
    ifstream file(filename, ios::binary | ios::ate);
    if (file.is_open())
    {
        return file.tellg();
    }
    return 0;
}

// Decode variable-byte
uint32_t decode_vbyte(const vector<uint8_t> &data, size_t &pos)
{
    uint32_t result = 0;
    uint32_t shift = 0;

    while (pos < data.size())
    {
        uint8_t byte = data[pos++];
        result |= (byte & 127) << shift;
        if ((byte & 128) == 0)
            break;
        shift += 7;
    }
    return result;
}

vector<uint32_t> decode_vbyte_list(const vector<uint8_t> &data, size_t &pos, uint32_t count)
{
    vector<uint32_t> result;
    for (uint32_t i = 0; i < count; i++)
    {
        result.push_back(decode_vbyte(data, pos));
    }
    return result;
}

// Parse doc_map.json manually
vector<string> parse_doc_map(const string &json_content)
{
    vector<string> docs;
    istringstream iss(json_content);
    string line;

    while (getline(iss, line))
    {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Look for quoted strings (document names)
        if (line[0] == '"' && line.back() == '"' || line.back() == ',')
        {
            size_t start = 1;
            size_t end = line.find_last_of('"');
            if (end > start)
            {
                docs.push_back(line.substr(start, end - start));
            }
        }
    }
    return docs;
}

// Parse metadata.json manually
map<string, pair<size_t, size_t>> parse_metadata(const string &json_content)
{
    map<string, pair<size_t, size_t>> metadata;
    istringstream iss(json_content);
    string line;
    string current_term;
    size_t offset = 0, length = 0;

    while (getline(iss, line))
    {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Look for term names
        if (line[0] == '"' && line.find("\": {") != string::npos)
        {
            size_t end_quote = line.find('"', 1);
            current_term = line.substr(1, end_quote - 1);
        }
        // Look for offset
        else if (line.find("\"offset\": ") != string::npos)
        {
            size_t start = line.find(": ") + 2;
            size_t end = line.find_last_of(',');
            if (end == string::npos)
                end = line.length();
            offset = stoul(line.substr(start, end - start));
        }
        // Look for length
        else if (line.find("\"length\": ") != string::npos)
        {
            size_t start = line.find(": ") + 2;
            size_t end = line.length();
            length = stoul(line.substr(start, end - start));

            if (!current_term.empty())
            {
                metadata[current_term] = make_pair(offset, length);
            }
        }
    }

    return metadata;
}

string escape_json_string(const string &s)
{
    string result;
    for (char c : s)
    {
        switch (c)
        {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
            break;
        }
    }
    return result;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <compressed_dir> <output_file>" << endl;
        cerr << "Example: " << argv[0] << " mock_corpus/compressed_dir_test reconstructed.json" << endl;
        return 1;
    }

    string compressed_dir = argv[1];
    string output_file = argv[2];

    cout << "Loading compressed index from: " << compressed_dir << endl;

    // Load DocID mapping
    ifstream doc_map_file(compressed_dir + "/doc_map.json");
    if (!doc_map_file.is_open())
    {
        cerr << "Error: Cannot open doc_map.json" << endl;
        return 1;
    }
    string doc_map_content((istreambuf_iterator<char>(doc_map_file)), istreambuf_iterator<char>());
    doc_map_file.close();
    vector<string> id_to_doc = parse_doc_map(doc_map_content);
    cout << "Loaded " << id_to_doc.size() << " document mappings." << endl;

    // Load metadata
    ifstream metadata_file(compressed_dir + "/metadata.json");
    if (!metadata_file.is_open())
    {
        cerr << "Error: Cannot open metadata.json" << endl;
        return 1;
    }
    string metadata_content((istreambuf_iterator<char>(metadata_file)), istreambuf_iterator<char>());
    metadata_file.close();
    auto metadata = parse_metadata(metadata_content);
    cout << "Loaded metadata for " << metadata.size() << " terms." << endl;

    // Load binary postings
    ifstream postings_file(compressed_dir + "/postings.bin", ios::binary);
    if (!postings_file.is_open())
    {
        cerr << "Error: Cannot open postings.bin" << endl;
        return 1;
    }
    vector<uint8_t> postings_data((istreambuf_iterator<char>(postings_file)), istreambuf_iterator<char>());
    postings_file.close();
    cout << "Loaded " << postings_data.size() << " bytes of compressed postings." << endl;

    // Reconstruct the index
    map<string, map<string, vector<uint32_t>>> reconstructed_index;

    for (const auto &term_meta : metadata)
    {
        string term = term_meta.first;
        size_t offset = term_meta.second.first;
        size_t length = term_meta.second.second;

        // Extract data for this term
        size_t pos = offset;
        uint32_t doc_count = decode_vbyte(postings_data, pos);

        for (uint32_t d = 0; d < doc_count; d++)
        {
            uint32_t doc_id = decode_vbyte(postings_data, pos);
            uint32_t pos_count = decode_vbyte(postings_data, pos);

            vector<uint32_t> delta_positions = decode_vbyte_list(postings_data, pos, pos_count);

            // Convert delta back to absolute positions
            vector<uint32_t> positions;
            if (!delta_positions.empty())
            {
                positions.push_back(delta_positions[0]);
                for (size_t i = 1; i < delta_positions.size(); i++)
                {
                    positions.push_back(positions[i - 1] + delta_positions[i]);
                }
            }

            if (doc_id < id_to_doc.size())
            {
                string doc_name = id_to_doc[doc_id];
                reconstructed_index[term][doc_name] = positions;
            }
        }
    }

    cout << "Reconstructed index with " << reconstructed_index.size() << " terms." << endl;

    // Write reconstructed index to JSON file
    ofstream out(output_file);
    if (!out.is_open())
    {
        cerr << "Error: Cannot create output file " << output_file << endl;
        return 1;
    }

    // Sort terms for consistent output
    vector<string> terms;
    for (const auto &term_entry : reconstructed_index)
    {
        terms.push_back(term_entry.first);
    }
    sort(terms.begin(), terms.end());

    out << "{\n";
    for (size_t t = 0; t < terms.size(); t++)
    {
        const string &term = terms[t];
        out << "  \"" << escape_json_string(term) << "\": {\n";

        // Sort documents for this term
        vector<string> docs;
        for (const auto &doc_entry : reconstructed_index[term])
        {
            docs.push_back(doc_entry.first);
        }
        sort(docs.begin(), docs.end());

        for (size_t d = 0; d < docs.size(); d++)
        {
            const string &doc = docs[d];
            out << "    \"" << escape_json_string(doc) << "\": [";

            const auto &positions = reconstructed_index[term][doc];
            for (size_t i = 0; i < positions.size(); i++)
            {
                if (i > 0)
                    out << ", ";
                out << positions[i];
            }
            out << "]";

            if (d + 1 < docs.size())
                out << ",";
            out << "\n";
        }

        out << "  }";
        if (t + 1 < terms.size())
            out << ",";
        out << "\n";
    }
    out << "}\n";

    out.close();
    cout << "Reconstructed index saved to: " << output_file << endl;

    return 0;
}
