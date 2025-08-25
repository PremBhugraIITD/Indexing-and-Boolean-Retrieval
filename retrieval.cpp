#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <sstream>
#include <algorithm>

using namespace std;

// Variable-byte decoding function
uint32_t decode_vbyte(const vector<uint8_t>& data, size_t& pos) {
    uint32_t result = 0;
    uint32_t shift = 0;
    
    while (pos < data.size()) {
        uint8_t byte = data[pos++];
        result |= (byte & 127) << shift;
        if ((byte & 128) == 0) break; // Final byte has MSB=0
        shift += 7;
    }
    return result;
}

// Parse doc_map.json manually
vector<string> parse_doc_map(const string& json_content) {
    vector<string> docs;
    istringstream iss(json_content);
    string line;
    
    while (getline(iss, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Look for quoted strings (document names)
        if (line.length() >= 2 && line[0] == '"') {
            size_t end = line.find_last_of('"');
            if (end > 0 && end != line.find_first_of('"')) {
                docs.push_back(line.substr(1, end - 1));
            }
        }
    }
    return docs;
}

// Parse metadata.json manually
map<string, pair<size_t, size_t>> parse_metadata(const string& json_content) {
    map<string, pair<size_t, size_t>> metadata;
    istringstream iss(json_content);
    string line;
    string current_term;
    size_t offset = 0, length = 0;
    bool has_offset = false, has_length = false;
    
    while (getline(iss, line)) {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Look for term names
        if (line.length() >= 2 && line[0] == '"' && line.find("\": {") != string::npos) {
            size_t end_quote = line.find('"', 1);
            current_term = line.substr(1, end_quote - 1);
            has_offset = has_length = false;
        }
        // Look for offset
        else if (line.find("\"offset\": ") != string::npos) {
            size_t start = line.find(": ") + 2;
            size_t end = line.find_last_of(',');
            if (end == string::npos) end = line.length();
            offset = stoul(line.substr(start, end - start));
            has_offset = true;
        }
        // Look for length
        else if (line.find("\"length\": ") != string::npos) {
            size_t start = line.find(": ") + 2;
            size_t end = line.length();
            length = stoul(line.substr(start, end - start));
            has_length = true;
            
            if (!current_term.empty() && has_offset && has_length) {
                metadata[current_term] = make_pair(offset, length);
            }
        }
    }
    
    return metadata;
}

// JSON escaping function
string escape_json_string(const string& s) {
    string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

// Main decompression function
map<string, map<string, vector<uint32_t>>> decompress_index(const string& compressed_dir) {
    map<string, map<string, vector<uint32_t>>> index;
    
    // Load doc_map.json
    ifstream doc_map_file(compressed_dir + "/doc_map.json");
    if (!doc_map_file.is_open()) {
        cerr << "Error: Cannot open " << compressed_dir << "/doc_map.json" << endl;
        return index;
    }
    string doc_map_content((istreambuf_iterator<char>(doc_map_file)), istreambuf_iterator<char>());
    doc_map_file.close();
    vector<string> doc_map = parse_doc_map(doc_map_content);
    
    // Load metadata.json
    ifstream metadata_file(compressed_dir + "/metadata.json");
    if (!metadata_file.is_open()) {
        cerr << "Error: Cannot open " << compressed_dir << "/metadata.json" << endl;
        return index;
    }
    string metadata_content((istreambuf_iterator<char>(metadata_file)), istreambuf_iterator<char>());
    metadata_file.close();
    map<string, pair<size_t, size_t>> metadata = parse_metadata(metadata_content);
    
    // Load postings.bin
    ifstream postings_file(compressed_dir + "/postings.bin", ios::binary);
    if (!postings_file.is_open()) {
        cerr << "Error: Cannot open " << compressed_dir << "/postings.bin" << endl;
        return index;
    }
    vector<uint8_t> postings_data((istreambuf_iterator<char>(postings_file)), istreambuf_iterator<char>());
    postings_file.close();
    
    // Decompress each term
    for (const auto& term_meta : metadata) {
        const string& term = term_meta.first;
        size_t offset = term_meta.second.first;
        size_t length = term_meta.second.second;
        
        if (offset >= postings_data.size()) {
            cerr << "Warning: Invalid offset for term " << term << endl;
            continue;
        }
        
        size_t pos = offset;
        uint32_t doc_count = decode_vbyte(postings_data, pos);
        
        for (uint32_t d = 0; d < doc_count; d++) {
            uint32_t doc_id = decode_vbyte(postings_data, pos);
            uint32_t pos_count = decode_vbyte(postings_data, pos);
            
            // Decode positions (first is absolute, rest are deltas)
            vector<uint32_t> positions;
            if (pos_count > 0) {
                uint32_t first_pos = decode_vbyte(postings_data, pos);
                positions.push_back(first_pos);
                
                for (uint32_t i = 1; i < pos_count; i++) {
                    uint32_t delta = decode_vbyte(postings_data, pos);
                    positions.push_back(positions.back() + delta);
                }
            }
            
            // Map doc_id to document string
            string doc_name;
            if (doc_id < doc_map.size()) {
                doc_name = doc_map[doc_id];
            } else {
                doc_name = "DOC_" + to_string(doc_id);
            }
            
            index[term][doc_name] = positions;
        }
    }
    
    // Write decompressed_index.json to compressed_dir
    ofstream out(compressed_dir + "/decompressed_index.json");
    if (out.is_open()) {
        // Sort terms for deterministic output
        vector<string> terms;
        for (const auto& term_entry : index) {
            terms.push_back(term_entry.first);
        }
        sort(terms.begin(), terms.end());
        
        out << "{\n";
        for (size_t t = 0; t < terms.size(); t++) {
            const string& term = terms[t];
            out << "  \"" << escape_json_string(term) << "\": {\n";
            
            // Sort documents for this term
            vector<string> docs;
            for (const auto& doc_entry : index[term]) {
                docs.push_back(doc_entry.first);
            }
            sort(docs.begin(), docs.end());
            
            for (size_t d = 0; d < docs.size(); d++) {
                const string& doc = docs[d];
                out << "    \"" << escape_json_string(doc) << "\": [";
                
                const auto& positions = index[term][doc];
                for (size_t i = 0; i < positions.size(); i++) {
                    if (i > 0) out << ", ";
                    out << positions[i];
                }
                out << "]";
                
                if (d + 1 < docs.size()) out << ",";
                out << "\n";
            }
            
            out << "  }";
            if (t + 1 < terms.size()) out << ",";
            out << "\n";
        }
        out << "}\n";
        out.close();
    }
    
    return index;
}

// Test function for Task 4.1
int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <compressed_dir>" << endl;
        cerr << "Example: " << argv[0] << " mock_corpus/compressed_dir_test" << endl;
        return 1;
    }
    
    string compressed_dir = argv[1];
    cout << "Decompressing index from: " << compressed_dir << endl;
    
    auto index = decompress_index(compressed_dir);
    
    cout << "Decompressed index with " << index.size() << " terms." << endl;
    cout << "Decompressed index saved to: " << compressed_dir << "/decompressed_index.json" << endl;
    
    return 0;
}
