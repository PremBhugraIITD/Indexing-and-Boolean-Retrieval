#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <set>
#include <cstdint>
#include <cerrno>
#ifdef _WIN32
#include <direct.h>
#define mkdir _mkdir
#endif
#include "include/json.hpp"

using namespace std;
using json = nlohmann::json;

// Helper function to create directory (Windows compatible)
bool create_directory(const string &path)
{
#ifdef _WIN32
    return mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

// Helper function to get file size
size_t get_file_size(const string &filename)
{
    struct stat st;
    if (stat(filename.c_str(), &st) == 0)
    {
        return st.st_size;
    }
    return 0;
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

    // Create output directory
    create_directory(compressed_dir);

    // Load the original index.json
    ifstream index_file(index_path);
    if (!index_file.is_open())
    {
        cerr << "Error: Cannot open index file: " << index_path << endl;
        return 1;
    }

    json index;
    index_file >> index;
    index_file.close();

    cout << "Loaded index with " << index.size() << " terms." << endl;

    // Step 1: Create DocID mapping (string -> integer)
    map<string, uint32_t> doc_to_id;
    vector<string> id_to_doc;
    uint32_t doc_counter = 0;

    // Collect all unique document IDs
    set<string> all_docs;
    for (const auto &term_entry : index.items())
    {
        for (const auto &doc_entry : term_entry.value().items())
        {
            all_docs.insert(doc_entry.key());
        }
    }

    // Create sorted mapping
    for (const string &doc : all_docs)
    {
        doc_to_id[doc] = doc_counter++;
        id_to_doc.push_back(doc);
    }

    cout << "Created DocID mapping for " << doc_counter << " documents." << endl;

    // Save DocID mapping
    json doc_map_json = json::array();
    for (const string &doc : id_to_doc)
    {
        doc_map_json.push_back(doc);
    }

    ofstream doc_map_file(compressed_dir + "/doc_map.json");
    doc_map_file << doc_map_json.dump(2);
    doc_map_file.close();

    // Step 2: Compress postings and create metadata
    ofstream postings_file(compressed_dir + "/postings.bin", ios::binary);
    json metadata;

    size_t current_offset = 0;

    // Process terms in sorted order (they should already be sorted from build_index)
    vector<string> terms;
    for (const auto &term_entry : index.items())
    {
        terms.push_back(term_entry.key());
    }
    sort(terms.begin(), terms.end()); // Ensure sorted order

    for (const string &term : terms)
    {
        vector<uint8_t> compressed_data;

        // Get postings for this term
        const auto &postings = index[term];

        // Collect and sort doc IDs
        vector<string> docs;
        for (const auto &doc_entry : postings.items())
        {
            docs.push_back(doc_entry.key());
        }
        sort(docs.begin(), docs.end());

        // Encode number of documents for this term
        auto doc_count_encoded = encode_vbyte(docs.size());
        compressed_data.insert(compressed_data.end(), doc_count_encoded.begin(), doc_count_encoded.end());

        // For each document
        for (const string &doc : docs)
        {
            uint32_t doc_id = doc_to_id[doc];
            vector<uint32_t> positions = postings[doc].get<vector<uint32_t>>();

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
        metadata[term] = {
            {"offset", current_offset},
            {"length", compressed_data.size()}};

        current_offset += compressed_data.size();

        if (terms.size() <= 10 || find(terms.begin(), terms.begin() + min(5, (int)terms.size()), term) != terms.begin() + min(5, (int)terms.size()))
        {
            cout << "Compressed term '" << term << "': " << compressed_data.size() << " bytes" << endl;
        }
    }

    postings_file.close();

    // Save metadata
    ofstream metadata_file(compressed_dir + "/metadata.json");
    metadata_file << metadata.dump(2);
    metadata_file.close();

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
