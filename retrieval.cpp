#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <set>
#include <stack>
#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif
#include "tokenizer.h"

using namespace std;

// QueryNode structure for AST
struct QueryNode {
    string value;  // operator or term
    QueryNode* left;
    QueryNode* right;
    
    QueryNode(string v) : value(v), left(nullptr), right(nullptr) {}
    ~QueryNode() {
        delete left;
        delete right;
    }
};

vector<string> global_all_docs; // Global vector to store all document names

// Variable-byte decoding function
uint32_t decode_vbyte(const vector<uint8_t> &data, size_t &pos)
{
    uint32_t result = 0;
    uint32_t shift = 0;

    while (pos < data.size())
    {
        uint8_t byte = data[pos++];
        result |= (byte & 127) << shift;
        if ((byte & 128) == 0)
            break; // Final byte has MSB=0
        shift += 7;
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
        if (line.length() >= 2 && line[0] == '"')
        {
            size_t end = line.find_last_of('"');
            if (end > 0 && end != line.find_first_of('"'))
            {
                docs.push_back(line.substr(1, end - 1));
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
    bool has_offset = false, has_length = false;

    while (getline(iss, line))
    {
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        // Look for term names
        if (line.length() >= 2 && line[0] == '"' && line.find("\": {") != string::npos)
        {
            size_t end_quote = line.find('"', 1);
            current_term = line.substr(1, end_quote - 1);
            has_offset = has_length = false;
        }
        // Look for offset
        else if (line.find("\"offset\": ") != string::npos)
        {
            size_t start = line.find(": ") + 2;
            size_t end = line.find_last_of(',');
            if (end == string::npos)
                end = line.length();
            offset = stoul(line.substr(start, end - start));
            has_offset = true;
        }
        // Look for length
        else if (line.find("\"length\": ") != string::npos)
        {
            size_t start = line.find(": ") + 2;
            size_t end = line.length();
            length = stoul(line.substr(start, end - start));
            has_length = true;

            if (!current_term.empty() && has_offset && has_length)
            {
                metadata[current_term] = make_pair(offset, length);
            }
        }
    }

    return metadata;
}

// JSON escaping function
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

// Helper function to check if a token is a Boolean operator
bool is_operator(const string &token)
{
    string upper_token = token;
    transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);
    return upper_token == "AND" || upper_token == "OR" || upper_token == "NOT";
}

// Helper function to check if a token is a parenthesis
bool is_parenthesis(const string &token)
{
    return token == "(" || token == ")";
}

// Helper function to check if a token is a term (not operator or parenthesis)
bool is_term(const string &token)
{
    return !is_operator(token) && !is_parenthesis(token);
}

// Query preprocessing function for Task 4.2
vector<string> preprocess_query(const string &title, const unordered_set<string> &stopwords)
{
    if (title.empty())
    {
        return vector<string>();
    }

    // Step 1: Add spaces around parentheses
    string modified_title = title;

    // Replace ( with " ( "
    size_t pos = 0;
    while ((pos = modified_title.find('(', pos)) != string::npos)
    {
        modified_title.replace(pos, 1, " ( ");
        pos += 3;
    }

    // Replace ) with " ) "
    pos = 0;
    while ((pos = modified_title.find(')', pos)) != string::npos)
    {
        modified_title.replace(pos, 1, " ) ");
        pos += 3;
    }

    // Step 2: Tokenize using existing tokenizer
    vector<string> tokens = tokenize(modified_title, stopwords);

    if (tokens.empty())
    {
        return vector<string>();
    }

    // Step 3: Canonicalize operators (convert to uppercase)
    for (string &token : tokens)
    {
        string upper_token = token;
        transform(upper_token.begin(), upper_token.end(), upper_token.begin(), ::toupper);

        if (upper_token == "AND" || upper_token == "OR" || upper_token == "NOT")
        {
            token = upper_token;
        }
        // Parentheses remain as-is: "(" and ")"
    }

    // Step 4: Insert implicit AND operators
    vector<string> result;

    for (size_t i = 0; i < tokens.size(); i++)
    {
        result.push_back(tokens[i]);

        // Check if we need to insert AND after current token
        if (i + 1 < tokens.size())
        {
            const string &current = tokens[i];
            const string &next = tokens[i + 1];

            bool insert_and = false;

            // Case 1: term term
            if (is_term(current) && is_term(next))
            {
                insert_and = true;
            }
            // Case 2: term (
            else if (is_term(current) && next == "(")
            {
                insert_and = true;
            }
            // Case 3: ) term
            else if (current == ")" && is_term(next))
            {
                insert_and = true;
            }
            // Case 4: ) (
            else if (current == ")" && next == "(")
            {
                insert_and = true;
            }
            // Case 5: term NOT
            else if (is_term(current) && next == "NOT")
            {
                insert_and = true;
            }

            if (insert_and)
            {
                result.push_back("AND");
            }
        }
    }

    return result;
}

// Required function declarations for assignment compliance
map<string, map<string, vector<uint32_t>>> decompress_index(const string &compressed_dir);
QueryNode* query_parser(vector<string> query_tokens);
void boolean_retrieval(const map<string, map<string, vector<uint32_t>>> &inverted_index, const string &path_to_query_file, const string &output_dir);

// Helper functions for query processing
int get_precedence(const string &op);
bool is_right_associative(const string &op);
vector<string> infix_to_postfix(const vector<string> &tokens);
QueryNode* build_tree(const vector<string> &postfix);
vector<string> evaluate_tree(QueryNode *root, const map<string, map<string, vector<uint32_t>>> &index);

// Main decompression function
map<string, map<string, vector<uint32_t>>> decompress_index(const string &compressed_dir)
{
    map<string, map<string, vector<uint32_t>>> index;

    // Load doc_map.json
    ifstream doc_map_file(compressed_dir + "/doc_map.json");
    if (!doc_map_file.is_open())
    {
        cerr << "Error: Cannot open " << compressed_dir << "/doc_map.json" << endl;
        return index;
    }
    string doc_map_content((istreambuf_iterator<char>(doc_map_file)), istreambuf_iterator<char>());
    doc_map_file.close();
    vector<string> doc_map = parse_doc_map(doc_map_content);

    // Initialize global_all_docs
    global_all_docs = doc_map;
    sort(global_all_docs.begin(), global_all_docs.end());

    // Load metadata.json
    ifstream metadata_file(compressed_dir + "/metadata.json");
    if (!metadata_file.is_open())
    {
        cerr << "Error: Cannot open " << compressed_dir << "/metadata.json" << endl;
        return index;
    }
    string metadata_content((istreambuf_iterator<char>(metadata_file)), istreambuf_iterator<char>());
    metadata_file.close();
    map<string, pair<size_t, size_t>> metadata = parse_metadata(metadata_content);

    // Load postings.bin
    ifstream postings_file(compressed_dir + "/postings.bin", ios::binary);
    if (!postings_file.is_open())
    {
        cerr << "Error: Cannot open " << compressed_dir << "/postings.bin" << endl;
        return index;
    }
    vector<uint8_t> postings_data((istreambuf_iterator<char>(postings_file)), istreambuf_iterator<char>());
    postings_file.close();

    // Decompress each term
    for (const auto &term_meta : metadata)
    {
        const string &term = term_meta.first;
        size_t offset = term_meta.second.first;
        size_t length = term_meta.second.second;

        if (offset >= postings_data.size())
        {
            cerr << "Warning: Invalid offset for term " << term << endl;
            continue;
        }

        size_t pos = offset;
        uint32_t doc_count = decode_vbyte(postings_data, pos);

        for (uint32_t d = 0; d < doc_count; d++)
        {
            uint32_t doc_id = decode_vbyte(postings_data, pos);
            uint32_t pos_count = decode_vbyte(postings_data, pos);

            // Decode positions (first is absolute, rest are deltas)
            vector<uint32_t> positions;
            if (pos_count > 0)
            {
                uint32_t first_pos = decode_vbyte(postings_data, pos);
                positions.push_back(first_pos);

                for (uint32_t i = 1; i < pos_count; i++)
                {
                    uint32_t delta = decode_vbyte(postings_data, pos);
                    positions.push_back(positions.back() + delta);
                }
            }

            // Map doc_id to document string
            string doc_name;
            if (doc_id < doc_map.size())
            {
                doc_name = doc_map[doc_id];
            }
            else
            {
                doc_name = "DOC_" + to_string(doc_id);
            }

            index[term][doc_name] = positions;
        }
    }

    // Write decompressed_index.json to compressed_dir
    ofstream out(compressed_dir + "/decompressed_index.json");
    if (out.is_open())
    {
        // Sort terms for deterministic output
        vector<string> terms;
        for (const auto &term_entry : index)
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
            for (const auto &doc_entry : index[term])
            {
                docs.push_back(doc_entry.first);
            }
            sort(docs.begin(), docs.end());

            for (size_t d = 0; d < docs.size(); d++)
            {
                const string &doc = docs[d];
                out << "    \"" << escape_json_string(doc) << "\": [";

                const auto &positions = index[term][doc];
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
    }

    return index;
}

// get_precedence function
int get_precedence(const string &op)
{
    if (op == "NOT")
        return 3;
    if (op == "AND")
        return 2;
    if (op == "OR")
        return 1;
    return 0; // For terms and parentheses
}

// check if operator is right-associative
bool is_right_associative(const string &op)
{
    return op == "NOT"; // NOT is right-associative
}

// infix_to_postfix function
vector<string> infix_to_postfix(const vector<string> &tokens)
{
    vector<string> postfix;
    stack<string> operators;

    for (const auto &token : tokens)
    {
        if (!is_operator(token) && !is_parenthesis(token))
        {
            // Term - add to output
            postfix.push_back(token);
        }
        else if (token == "(")
        {
            operators.push(token);
        }
        else if (token == ")")
        {
            while (!operators.empty() && operators.top() != "(")
            {
                postfix.push_back(operators.top());
                operators.pop();
            }
            if (!operators.empty())
                operators.pop(); // Remove "("
        }
        else
        {
            // Operator - handle associativity
            while (!operators.empty() && operators.top() != "(" &&
                   (is_right_associative(token) ? get_precedence(operators.top()) > get_precedence(token) : get_precedence(operators.top()) >= get_precedence(token)))
            {
                postfix.push_back(operators.top());
                operators.pop();
            }
            operators.push(token);
        }
    }

    while (!operators.empty())
    {
        postfix.push_back(operators.top());
        operators.pop();
    }

    return postfix;
}

// build_tree function
QueryNode *build_tree(const vector<string> &postfix)
{
    stack<QueryNode *> node_stack;

    for (const auto &token : postfix)
    {
        if (!is_operator(token))
        {
            // Term - create leaf node
            node_stack.push(new QueryNode(token));
        }
        else
        {
            // Operator - pop nodes and create internal node
            QueryNode *node = new QueryNode(token);
            if (token == "NOT")
            {
                if (!node_stack.empty())
                {
                    node->right = node_stack.top();
                    node_stack.pop();
                }
                else
                {
                    // Error: NOT operator without operand
                    delete node;
                    // Clean up remaining nodes
                    while (!node_stack.empty())
                    {
                        delete node_stack.top();
                        node_stack.pop();
                    }
                    return nullptr;
                }
            }
            else
            {
                // Binary operator (AND, OR)
                if (node_stack.size() >= 2)
                {
                    node->right = node_stack.top();
                    node_stack.pop();
                    node->left = node_stack.top();
                    node_stack.pop();
                }
                else
                {
                    // Error: Binary operator without enough operands
                    delete node;
                    // Clean up remaining nodes
                    while (!node_stack.empty())
                    {
                        delete node_stack.top();
                        node_stack.pop();
                    }
                    return nullptr;
                }
            }
            node_stack.push(node);
        }
    }

    // Should have exactly one node left (the root)
    if (node_stack.size() == 1)
    {
        return node_stack.top();
    }
    else
    {
        // Error: malformed expression
        while (!node_stack.empty())
        {
            delete node_stack.top();
            node_stack.pop();
        }
        return nullptr;
    }
}

// evaluate_tree function
vector<string> evaluate_tree(QueryNode *root,
                             const map<string, map<string, vector<uint32_t>>> &index)
{

    if (!root)
        return {};

    if (!is_operator(root->value))
    {
        // Leaf node (term)
        auto it = index.find(root->value);
        if (it != index.end())
        {
            vector<string> docs;
            for (const auto &doc_entry : it->second)
            {
                docs.push_back(doc_entry.first);
            }
            sort(docs.begin(), docs.end());
            return docs;
        }
        else
        {
            return {}; // Term not found
        }
    }

    // Internal node (operator)
    vector<string> left_result = evaluate_tree(root->left, index);
    vector<string> right_result = evaluate_tree(root->right, index);

    if (root->value == "AND")
    {
        vector<string> result;
        set_intersection(left_result.begin(), left_result.end(),
                         right_result.begin(), right_result.end(),
                         back_inserter(result));
        return result;
    }
    else if (root->value == "OR")
    {
        vector<string> result;
        set_union(left_result.begin(), left_result.end(),
                  right_result.begin(), right_result.end(),
                  back_inserter(result));
        return result;
    }
    else if (root->value == "NOT")
    {
        // NOT operator - only right child is relevant

        vector<string> result;
        set_difference(global_all_docs.begin(), global_all_docs.end(),
                       right_result.begin(), right_result.end(),
                       back_inserter(result));
        return result;
    }

    return {};
}

// Query parser function as required by assignment (Task 4.3)
QueryNode* query_parser(vector<string> query_tokens)
{
    // Convert infix to postfix
    vector<string> postfix = infix_to_postfix(query_tokens);
    
    // Build and return AST
    return build_tree(postfix);
}

void print_tree(QueryNode *node, int depth = 0)
{
    if (!node)
        return;
    print_tree(node->right, depth + 1);
    for (int i = 0; i < depth; i++)
        cout << "    ";
    cout << node->value << "\n";
    print_tree(node->left, depth + 1);
}

// Helper function to load stopwords with fallback paths
unordered_set<string> load_stopwords_with_fallback(const string &output_dir)
{
    unordered_set<string> stopwords;

    // Try output_dir/../stopwords.txt first
    string primary_path = output_dir + "/../stopwords.txt";
    ifstream file(primary_path);

    if (!file.is_open())
    {
        // Fallback to ./stopwords.txt
        file.open("./stopwords.txt");
        if (!file.is_open())
        {
            file.open("stopwords.txt");
        }
    }

    if (file.is_open())
    {
        string word;
        while (getline(file, word))
        {
            // Lowercase the stopword
            transform(word.begin(), word.end(), word.begin(), ::tolower);
            // Remove any trailing whitespace
            word.erase(word.find_last_not_of(" \t\r\n") + 1);
            if (!word.empty())
            {
                stopwords.insert(word);
            }
        }
        file.close();
    }

    return stopwords;
}

// Helper function to parse queries from JSON file
vector<pair<string, string>> parse_queries_file(const string &path_to_query_file)
{
    vector<pair<string, string>> queries; // (qid, title) pairs
    ifstream file(path_to_query_file);

    if (!file.is_open())
    {
        cerr << "Warning: Cannot open query file: " << path_to_query_file << endl;
        return queries;
    }

    string line;
    while (getline(file, line))
    {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] != '{')
            continue;

        string qid, title;
        bool has_qid = false, has_title = false;

        // Simple JSON parsing for qid and title
        size_t pos = 0;
        while (pos < line.length())
        {
            // Look for "query_id" field
            size_t qid_pos = line.find("\"query_id\"", pos);
            if (qid_pos != string::npos)
            {
                size_t colon_pos = line.find(":", qid_pos);
                if (colon_pos != string::npos)
                {
                    size_t value_start = line.find("\"", colon_pos);
                    if (value_start != string::npos)
                    {
                        value_start++;
                        size_t value_end = line.find("\"", value_start);
                        if (value_end != string::npos)
                        {
                            qid = line.substr(value_start, value_end - value_start);
                            has_qid = true;
                        }
                    }
                }
            }

            // Look for "title" field
            size_t title_pos = line.find("\"title\"", pos);
            if (title_pos != string::npos)
            {
                size_t colon_pos = line.find(":", title_pos);
                if (colon_pos != string::npos)
                {
                    size_t value_start = line.find("\"", colon_pos);
                    if (value_start != string::npos)
                    {
                        value_start++;
                        size_t value_end = line.find("\"", value_start);
                        if (value_end != string::npos)
                        {
                            title = line.substr(value_start, value_end - value_start);
                            has_title = true;
                        }
                    }
                }
            }

            if (has_qid && has_title)
                break;

            pos = max(qid_pos, title_pos);
            if (pos == string::npos)
                break;
            pos++;
        }

        if (has_qid && has_title)
        {
            queries.push_back(make_pair(qid, title));
        }
        else if (has_qid && !has_title)
        {
            cerr << "Warning: Query " << qid << " missing title field, skipping." << endl;
        }
    }

    file.close();
    return queries;
}

// Helper function to evaluate query tree using postings map
vector<string> evaluate_tree_with_postings(QueryNode *root,
                                           const map<string, vector<string>> &postings_map,
                                           const vector<string> &universe)
{
    if (!root)
        return {};

    if (!is_operator(root->value))
    {
        // Leaf node (term)
        auto it = postings_map.find(root->value);
        if (it != postings_map.end())
        {
            return it->second; // Already sorted
        }
        else
        {
            return {}; // Term not found, return empty set
        }
    }

    // Internal node (operator)
    vector<string> left_result = evaluate_tree_with_postings(root->left, postings_map, universe);
    vector<string> right_result = evaluate_tree_with_postings(root->right, postings_map, universe);

    if (root->value == "AND")
    {
        vector<string> result;
        set_intersection(left_result.begin(), left_result.end(),
                         right_result.begin(), right_result.end(),
                         back_inserter(result));
        return result;
    }
    else if (root->value == "OR")
    {
        vector<string> result;
        set_union(left_result.begin(), left_result.end(),
                  right_result.begin(), right_result.end(),
                  back_inserter(result));
        return result;
    }
    else if (root->value == "NOT")
    {
        // NOT operator - universe minus right operand
        vector<string> result;
        set_difference(universe.begin(), universe.end(),
                       right_result.begin(), right_result.end(),
                       back_inserter(result));
        return result;
    }

    return {};
}

// Main boolean retrieval function as required by assignment (Task 4.4)
void boolean_retrieval(
    const map<string, map<string, vector<uint32_t>>> &inverted_index,
    const string &path_to_query_file,
    const string &output_dir)
{
    // Load stopwords
    unordered_set<string> stopwords = load_stopwords_with_fallback(output_dir);

    // Load queries
    vector<pair<string, string>> queries = parse_queries_file(path_to_query_file);
    if (queries.empty())
    {
        cerr << "No valid queries found in file: " << path_to_query_file << endl;
        return;
    }

    // Build postings_map: term → sorted vector<string> of docIDs
    map<string, vector<string>> postings_map;
    set<string> all_docs_set;

    for (const auto &term_entry : inverted_index)
    {
        const string &term = term_entry.first;
        vector<string> docs;

        for (const auto &doc_entry : term_entry.second)
        {
            docs.push_back(doc_entry.first);
            all_docs_set.insert(doc_entry.first);
        }

        sort(docs.begin(), docs.end());
        postings_map[term] = docs;
    }

    // Build universe: sorted vector<string> of all docIDs
    vector<string> universe(all_docs_set.begin(), all_docs_set.end());
    sort(universe.begin(), universe.end());

    // Create output directory if it doesn't exist
#ifdef _WIN32
    _mkdir(output_dir.c_str());
#else
    system(("mkdir -p \"" + output_dir + "\"").c_str());
#endif

    // Open output file
    string output_file_path = output_dir + "/docids.txt";
    ofstream output_file(output_file_path);
    if (!output_file.is_open())
    {
        cerr << "Error: Cannot create output file: " << output_file_path << endl;
        return;
    }

    // Process each query
    for (const auto &query_pair : queries)
    {
        const string &qid = query_pair.first;
        const string &title = query_pair.second;

        // Preprocess query
        vector<string> processed = preprocess_query(title, stopwords);
        if (processed.empty())
        {
            continue; // Skip empty queries
        }

        // Convert to postfix
        vector<string> postfix = infix_to_postfix(processed);

        // Build tree
        QueryNode *root = build_tree(postfix);
        if (root == nullptr)
        {
            cerr << "Warning: Failed to parse query " << qid << ": \"" << title << "\", skipping." << endl;
            continue;
        }

        // Evaluate query
        vector<string> results = evaluate_tree_with_postings(root, postings_map, universe);

        // Sort results lexicographically (should already be sorted from set operations)
        sort(results.begin(), results.end());

        // Write 4-column format output: qid docid rank score
        for (size_t i = 0; i < results.size(); i++)
        {
            int rank = i + 1; // 1-based ranking
            output_file << qid << " " << results[i] << " " << rank << " 1" << endl;
        }

        // Clean up tree
        delete root;
    }

    output_file.close();
    cout << "Boolean retrieval completed. Results written to: " << output_file_path << endl;
}

// Main function as required by assignment
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " <COMPRESSED_DIR> <QUERY_FILE_PATH> <OUTPUT_DIR>" << endl;
        return 1;
    }

    string compressed_dir = argv[1];
    string query_file_path = argv[2];
    string output_dir = argv[3];

    // Create output directory if it doesn't exist
#ifdef _WIN32
    _mkdir(output_dir.c_str());
#else
    system(("mkdir -p \"" + output_dir + "\"").c_str());
#endif

    // Task 4.1: Decompress index
    auto index = decompress_index(compressed_dir);
    
    if (index.empty())
    {
        cerr << "Error: Failed to decompress index from " << compressed_dir << endl;
        return 1;
    }

    // Task 4.2, 4.3, 4.4: Process queries and perform boolean retrieval
    boolean_retrieval(index, query_file_path, output_dir);
    
    return 0;
}