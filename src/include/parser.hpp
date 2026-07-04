#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <stdexcept> // Tambahkan untuk std::runtime_error
#include <map>       // Tambahkan untuk menyimpan variabel
#include "lexer.hpp"
#include "token.hpp"

inline void printASTNodeLine(std::ostream& os, const std::string& prefix, bool isLast, const std::string& nodeType, const std::string& value = "") {
    os << prefix;
    os << (isLast ? "└── " : "├── ");
    os << nodeType;
    if (!value.empty()) {
        os << ": " << value;
    }
    os << "\n";
}

enum class NodeType {
    Program,
    Command,
    Parameter,
    LiteralText,
    VariableDeclaration,
    VariableAccess
};

enum class CommandType {
    Unknown,
    SetModel,
    SetPrompt,
    LoadHistory,
    SaveHistory,
    Generate,
    SetParam,
    Help,
    Exit
};

struct ASTNode {
    NodeType type;
    virtual ~ASTNode() = default;

    std::string getNodeTypeName() const;
    virtual void printAST_tree(std::ostream& os, const std::string& prefix = "", bool isLast = true) const = 0;
};

struct ProgramNode : public ASTNode {
    std::vector<std::unique_ptr<ASTNode>> children;

    ProgramNode();
    void printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const override;
};

struct VariableAccessNode : public ASTNode {
    std::string variableName;

    VariableAccessNode();
    VariableAccessNode(const std::string& name);
    void printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const override;
};

struct CommandNode : public ASTNode {
    std::string commandName;
    CommandType cmdType; // Perbaikan: Diubah dari 'type' menjadi 'cmdType' agar tidak bentrok
    std::vector<std::unique_ptr<struct ParameterNode>> parameters;
    std::vector<std::unique_ptr<ASTNode>> arguments;

    CommandNode();
    ~CommandNode() override = default;
    void printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const override;
};

struct ParameterNode : public ASTNode {
    std::string key;
    std::string value;

    ParameterNode();
    ParameterNode(const std::string& k, const std::string& v);
    ~ParameterNode() override = default;
    void printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const override;
};

struct LiteralTextNode : public ASTNode {
    std::string text;

    LiteralTextNode();
    LiteralTextNode(const std::string& t);
    ~LiteralTextNode() override = default;
    void printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const override;
};

struct VariableDeclarationNode : public ASTNode {
    std::string variableName;
    std::unique_ptr<ASTNode> value_node;

    VariableDeclarationNode();
    VariableDeclarationNode(const std::string& name, std::unique_ptr<ASTNode> val_node);
    ~VariableDeclarationNode() override = default;
    void printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const override;
};

class Parser {
public:
    explicit Parser(Lexer& lexer);
    std::unique_ptr<ProgramNode> parse();
    const std::vector<std::string>& get_errors() const;
    bool has_errors() const;

private:
    Lexer& m_lexer;
    Token m_current_token;
    std::vector<std::string> m_errors;
    std::map<std::string, std::string> m_variables; // Untuk menyimpan variabel yang dideklarasikan

    void advance();
    void consume(TokenType expected_type);
    void add_error(const std::string& message);
    bool match(TokenType type);

    std::unique_ptr<ASTNode> parse_statement();
    std::unique_ptr<CommandNode> parse_command();
    std::unique_ptr<ParameterNode> parse_parameter();
    std::unique_ptr<ASTNode> parse_argument();
    std::unique_ptr<VariableDeclarationNode> parse_variable_declaration();
    std::unique_ptr<VariableAccessNode> parse_variable_access();
    std::unique_ptr<LiteralTextNode> parse_literal_text();

    // Helper untuk mengonversi string command menjadi CommandType
    CommandType get_command_type(const std::string& command_name) const;
};
