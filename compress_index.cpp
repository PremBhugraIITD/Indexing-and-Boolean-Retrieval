#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <cstdint>
#include <sstream>

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
map<string, map<string, vector<uint32_t>>> parse_json_index(const string &json_content)
{
    map<string, map<string, vector<uint32_t>>> index;

    // Simple JSON parser for our specific index format
    istringstream iss(json_content);
    string line;
    string current_term;

    while (getline(iss, line))
    {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line == "{" || line == "}")
            continue;

        // Check if this is a term line (starts with quote)
        if (line[0] == '"' && line.find("\": {") != string::npos)
        {
            size_t end_quote = line.find('"', 1);
            current_term = line.substr(1, end_quote - 1);
        }
        // Check if this is a document line
        else if (line[0] == '"' && line.find("\": [") != string::npos)
        {
            size_t end_quote = line.find('"', 1);
            string doc = line.substr(1, end_quote - 1);

            // Parse positions
            size_t start_bracket = line.find('[');
            size_t end_bracket = line.find(']');
            string positions_str = line.substr(start_bracket + 1, end_bracket - start_bracket - 1);

            vector<uint32_t> positions;
            if (!positions_str.empty())
            {
                istringstream pos_stream(positions_str);
                string pos;
                while (getline(pos_stream, pos, ','))
                {
                    pos.erase(0, pos.find_first_not_of(" \t"));
                    pos.erase(pos.find_last_not_of(" \t") + 1);
                    if (!pos.empty())
                    {
                        positions.push_back(stoul(pos));
                    }
                }
            }

            index[current_term][doc] = positions;
        }
    }

    return index;
}

// Manual JSON writing functions
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

void write_doc_map_json(const vector<string> &doc_list, const string &filename)
{
    ofstream out(filename);
    out << "[\n";
    for (size_t i = 0; i < doc_list.size(); i++)
    {
        out << "  \"" << escape_json_string(doc_list[i]) << "\"";
        if (i + 1 < doc_list.size())
            out << ",";
        out << "\n";
    }
    out << "]\n";
    out.close();
}

void write_metadata_json(const map<string, pair<size_t, size_t>> &metadata, const string &filename)
{
    ofstream out(filename);
    out << "{\n";

    auto it = metadata.begin();
    while (it != metadata.end())
    {
        out << "  \"" << escape_json_string(it->first) << "\": {\n";
        out << "    \"offset\": " << it->second.first << ",\n";
        out << "    \"length\": " << it->second.second << "\n";
        out << "  }";

        ++it;
        if (it != metadata.end())
            out << ",";
        out << "\n";
    }

    out << "}\n";
    out.close();
}

// Variable-byte encoding functions
vector<uint8_t> encode_vbyte(uint32_t value)
{
    vector<uint8_t> result;
    while (value >= 128)
    {
        result.push_back((value & 127) | 128);
        value >>= 7;
    }
    result.push_back(value & 127);
    return result;
}

void encode_vbyte_list(const vector<uint32_t> &values, vector<uint8_t> &output)
{
    for (uint32_t value : values)
    {
        auto encoded = encode_vbyte(value);
        output.insert(output.end(), encoded.begin(), encoded.end());
    }
}

// Decode variable-byte (for testing/verification)
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

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <index_json_path> <compressed_dir>" << endl;
        return 1;
    }

    string index_path = argv[1];
    string compressed_dir = argv[2];

    // Create output directory - use simple approach for compatibility
    // Note: Assumes directory exists or is created manually
    // This is acceptable since we're focusing on core compression logic

    // Load and parse the original index.json manually
    ifstream index_file(index_path);
    if (!index_file.is_open())
    {
        cerr << "Error: Cannot open index file: " << index_path << endl;
        return 1;
    }

    // Read entire file content
    string json_content((istreambuf_iterator<char>(index_file)), istreambuf_iterator<char>());
    index_file.close();

    // Parse JSON manually
    auto index = parse_json_index(json_content);

    cout << "Loaded index with " << index.size() << " terms." << endl;

    // Step 1: Create DocID mapping (string -> integer)
    map<string, uint32_t> doc_to_id;
    vector<string> id_to_doc;
    uint32_t doc_counter = 0;

    // Collect all unique document IDs
    set<string> all_docs;
    for (const auto &term_entry : index)
    {
        for (const auto &doc_entry : term_entry.second)
        {
            all_docs.insert(doc_entry.first);
        }
    }

    // Create sorted mapping
    for (const string &doc : all_docs)
    {
        doc_to_id[doc] = doc_counter++;
        id_to_doc.push_back(doc);
    }

    cout << "Created DocID mapping for " << doc_counter << " documents." << endl;

    // Save DocID mapping using manual JSON writing
    write_doc_map_json(id_to_doc, compressed_dir + "/doc_map.json");

    // Step 2: Compress postings and create metadata
    ofstream postings_file(compressed_dir + "/postings.bin", ios::binary);
    map<string, pair<size_t, size_t>> metadata; // term -> (offset, length)

    size_t current_offset = 0;

    // Process terms in sorted order
    vector<string> terms;
    for (const auto &term_entry : index)
    {
        terms.push_back(term_entry.first);
    }
    sort(terms.begin(), terms.end()); // Ensure sorted order

    for (const string &term : terms)
    {
        vector<uint8_t> compressed_data;

        // Get postings for this term
        const auto &postings = index.at(term);

        // Collect and sort doc IDs
        vector<string> docs;
        for (const auto &doc_entry : postings)
        {
            docs.push_back(doc_entry.first);
        }
        sort(docs.begin(), docs.end());

        // Encode number of documents for this term
        auto doc_count_encoded = encode_vbyte(docs.size());
        compressed_data.insert(compressed_data.end(), doc_count_encoded.begin(), doc_count_encoded.end());

        // For each document
        for (const string &doc : docs)
        {
            uint32_t doc_id = doc_to_id[doc];
            vector<uint32_t> positions = postings.at(doc);

            // Encode document ID
            auto doc_id_encoded = encode_vbyte(doc_id);
            compressed_data.insert(compressed_data.end(), doc_id_encoded.begin(), doc_id_encoded.end());

            // Encode number of positions
            auto pos_count_encoded = encode_vbyte(positions.size());
            compressed_data.insert(compressed_data.end(), pos_count_encoded.begin(), pos_count_encoded.end());

            // Apply delta encoding to positions (optional but good for compression)
            vector<uint32_t> delta_positions;
            if (!positions.empty())
            {
                delta_positions.push_back(positions[0]); // First position as-is
                for (size_t i = 1; i < positions.size(); i++)
                {
                    delta_positions.push_back(positions[i] - positions[i - 1]); // Delta
                }
            }

            // Encode positions
            encode_vbyte_list(delta_positions, compressed_data);
        }

        // Write compressed data to binary file
        postings_file.write(reinterpret_cast<const char *>(compressed_data.data()), compressed_data.size());

        // Store metadata
        metadata[term] = make_pair(current_offset, compressed_data.size());

        current_offset += compressed_data.size();

        if (terms.size() <= 10 || find(terms.begin(), terms.begin() + min(5, (int)terms.size()), term) != terms.begin() + min(5, (int)terms.size()))
        {
            cout << "Compressed term '" << term << "': " << compressed_data.size() << " bytes" << endl;
        }
    }

    postings_file.close();

    // Save metadata using manual JSON writing
    write_metadata_json(metadata, compressed_dir + "/metadata.json");

    cout << "Compression complete!" << endl;
    cout << "Files created:" << endl;
    cout << "  - doc_map.json (DocID mapping)" << endl;
    cout << "  - postings.bin (compressed postings)" << endl;
    cout << "  - metadata.json (term metadata)" << endl;

    // Print compression statistics
    size_t original_size = get_file_size(index_path);
    size_t compressed_size = get_file_size(compressed_dir + "/postings.bin") +
                             get_file_size(compressed_dir + "/doc_map.json") +
                             get_file_size(compressed_dir + "/metadata.json");

    cout << "Original size: " << original_size << " bytes" << endl;
    cout << "Compressed size: " << compressed_size << " bytes" << endl;
    cout << "Compression ratio: " << (double)original_size / compressed_size << "x" << endl;

    return 0;
}