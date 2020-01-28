#ifndef PARSER
#define PARSER

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

class Parser {
public:
    struct Token {
        enum TokenType { FILEPATH, CONSTANT, ADD, SUB, MULT, DIV, SUBTERM };
        Token(TokenType type, std::string s) : type(type), s(s) {}
        Token(char operation);
        bool isAddOrSub() const { return type == ADD ||  type == SUB; }
        bool isMultOrDiv() const {return type == MULT ||  type == DIV; }
        bool isOperator() const { return isAddOrSub() || isMultOrDiv(); }
        TokenType type;
        std::string s;
    };

    struct Node {
        enum NodeType { INVALID, INPUTFILEPATH, OUTPUTFILEPATH, CONSTANT, ADD, SUB, MULT, DIV, ASSIGN };
        Node() : type(INVALID), path(""), constant(0.0f), left(nullptr), right(nullptr) {}
        ~Node() { if (left) delete left; if (right) delete right; }
        std::string toString() const;
        void evaluate(std::function<void(const Parser::Node* node)>& lambda) const;
        NodeType type;
        std::string path;
        float constant;
        Node* left;
        Node* right;
    };

    // Creates a new parser object and parses expression.
    Parser(std::string expression);

    // Returns whether the parsed expression was valid.
    bool isValid() const { return _isValid; }

    // If !isValid(), this returns a reason for why expression
    // could not be parsed.
    std::string getErrorMessage() const { return _errorMessage; }

    // If isValid(), this returns the root node of type ASSIGN.
    const Node* getRoot() const { return &_root; }

private:
    Node* parse(std::string s);
    Node* parse(std::vector<Parser::Token> serialized);
    std::vector<Parser::Token> serializeOperandsAndParentheses(std::string s);
    Node _root;

    bool _isValid;
    std::string _errorMessage;
};


#endif  // PARSER