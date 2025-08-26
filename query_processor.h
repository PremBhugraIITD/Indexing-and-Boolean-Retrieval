#pragma once
#include <string>
#include <vector>
#include <map>

struct QueryNode {
    std::string value;  // operator or term
    QueryNode* left;
    QueryNode* right;
    
    QueryNode(std::string v) : value(v), left(nullptr), right(nullptr) {}
    ~QueryNode() {
        delete left;
        delete right;
    }
};

class QueryProcessor {
public:
    static std::vector<std::string> infix_to_postfix(const std::vector<std::string>& tokens);
    static QueryNode* build_tree(const std::vector<std::string>& postfix);
    static std::vector<std::string> evaluate_tree(QueryNode* root, 
        const std::map<std::string, std::map<std::string, std::vector<uint32_t>>>& index);
private:
    static int get_precedence(const std::string& op);
    static bool is_right_associative(const std::string& op);
};