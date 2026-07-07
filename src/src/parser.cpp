
#include "parser.hpp"
#include <stdexcept>
#include <algorithm>

std::string ASTNode::getNodeTypeName() const {
    switch (type) {
        case NodeType::Program: return "Program";
        case NodeType::Command: return "Command";
        case NodeType::Parameter: return "Parameter";
        case NodeType::LiteralText: return "LiteralText";
        case NodeType::VariableDeclaration: return "VariableDeclaration";
        case NodeType::VariableAccess: return "VariableAccess";
        case NodeType::LiteralInt: return "LiteralInt";
        case NodeType::LiteralFloat: return "LiteralFloat";
        case NodeType::LiteralBool: return "LiteralBool";
        default: return "Unknown";
    }
}

ProgramNode::ProgramNode() { 
    type = NodeType::Program; 
}

void ProgramNode::printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const {
    printASTNodeLine(os, prefix, isLast, getNodeTypeName());
    std::string childPrefix = prefix + (isLast ? "    " : "│   ");
    for (size_t i = 0; i < children.size(); ++i) {
        bool isLastChild = (i == children.size() - 1);
        children[i]->printAST_tree(os, childPrefix, isLastChild);
    }
}
VariableAccessNode::VariableAccessNode() { 
    type = NodeType::VariableAccess; 
}

VariableAccessNode::VariableAccessNode(const std::string& name) : variableName(name) { 
    type = NodeType::VariableAccess; 
}

void VariableAccessNode::printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const {
    printASTNodeLine(os, prefix, isLast, getNodeTypeName(), variableName);
}

CommandNode::CommandNode() : cmdType(CommandType::Unknown) { 
    type = NodeType::Command; 
}

void CommandNode::printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const {
    printASTNodeLine(os, prefix, isLast, getNodeTypeName(), commandName);
    std::string childPrefix = prefix + (isLast ? "    " : "│   ");

    size_t totalChildren = parameters.size() + arguments.size();
    size_t currentChildIdx = 0;

    for (size_t i = 0; i < parameters.size(); ++i) {
        bool isLastChild = (currentChildIdx == totalChildren - 1);
        parameters[i]->printAST_tree(os, childPrefix, isLastChild);
        currentChildIdx++;
    }

    for (size_t i = 0; i < arguments.size(); ++i) {
        bool isLastChild = (currentChildIdx == totalChildren - 1);
        arguments[i]->printAST_tree(os, childPrefix, isLastChild);
        currentChildIdx++;
    }
}

ParameterNode::ParameterNode() { 
    type = NodeType::Parameter; 
}

ParameterNode::ParameterNode(const std::string& k, const std::string& v) : key(k), value(v) { 
    type = NodeType::Parameter; 

}

void ParameterNode::printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const {
    printASTNodeLine(os, prefix, isLast, getNodeTypeName(), key + "=" + value);
}

LiteralTextNode::LiteralTextNode() { 
    type = NodeType::LiteralText; 
}

LiteralTextNode::LiteralTextNode(const std::string& t) : text(t) { 
    type = NodeType::LiteralText; 
}

void LiteralTextNode::printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const {
    printASTNodeLine(os, prefix, isLast, getNodeTypeName(), "\"" + text + "\"");
}

// Implementation of VariableDeclarationNode
VariableDeclarationNode::VariableDeclarationNode() { 
    type = NodeType::VariableDeclaration; 
}

VariableDeclarationNode::VariableDeclarationNode(const std::string& name, std::unique_ptr<ASTNode> val_node)
    : variableName(name), value_node(std::move(val_node)) { 
    type = NodeType::VariableDeclaration; 
}

void VariableDeclarationNode::printAST_tree(std::ostream& os, const std::string& prefix, bool isLast) const {
    printASTNodeLine(os, prefix, isLast, getNodeTypeName(), variableName);
    if (value_node) {
        std::string childPrefix = prefix + (isLast ? "    " : "│   ");
        value_node->printAST_tree(os, childPrefix, true);
    }
}

// Parser Implementation
Parser::Parser(Lexer& lexer) : m_lexer(lexer) {
    advance(); // Initialize m_current_token
}

std::unique_ptr<ProgramNode> Parser::parse() {
    auto program_node = std::make_unique<ProgramNode>();
    while (m_current_token.type != TokenType::END_OF_FILE) {
        try {
            program_node->children.push_back(parse_statement());
        } catch (const std::runtime_error& e) {
            add_error(e.what());
            while (m_current_token.type != TokenType::COMMAND && m_current_token.type != TokenType::END_OF_FILE) {
                advance();
            }
        }
    }
    return program_node;
}

const std::vector<std::string>& Parser::get_errors() const {
    return m_errors;
}

bool Parser::has_errors() const {
    return !m_errors.empty();
}

void Parser::advance() {
    m_current_token = m_lexer.getNextToken();
}

void Parser::consume(TokenType expected_type) {
    if (m_current_token.type == expected_type) {
        advance();
    } else {
        std::string error_msg = "Expected token type " + getTokenName(expected_type) + 
                                " but got " + getTokenName(m_current_token.type) + 
                                " with value '" + m_current_token.value + "'";
        throw std::runtime_error(error_msg);
    }
}

void Parser::add_error(const std::string& message) {
    m_errors.push_back("Parser Error: " + message);
}

bool Parser::match(TokenType type) {
    if (m_current_token.type == type) {
        advance();
        return true;
    }
    return false;
}

std::unique_ptr<ASTNode> Parser::parse_statement() {
    if (m_current_token.type == TokenType::COMMAND) {
        return parse_command();
    } else if (m_current_token.type == TokenType::TEXT) {
        return parse_literal_text();
    } else if (m_current_token.type == TokenType::UNKNOWN) {
        std::string val = m_current_token.value;
        advance();
        throw std::runtime_error("Encountered Unknown token: " + val);
    }
    
    std::string error_msg = "Unexpected token at start of statement: '" + m_current_token.value + "'";
    advance(); // Prevent infinite loop on bad token
    throw std::runtime_error(error_msg);
}

std::unique_ptr<CommandNode> Parser::parse_command() {
    auto command_node = std::make_unique<CommandNode>();
    
    std::string raw_cmd = m_current_token.value;
    if (!raw_cmd.empty() && raw_cmd[0] == '/') {
        raw_cmd = raw_cmd.substr(1);
    }
    
    command_node->commandName = raw_cmd;
    command_node->cmdType = get_command_type(raw_cmd);
    advance();
    while (m_current_token.type != TokenType::END_OF_FILE) {
        if (m_current_token.type == TokenType::PARAMETER) {
            command_node->parameters.push_back(parse_parameter());
        } 
        else {
            command_node->arguments.push_back(parse_argument());
        }
    }
    
    return command_node;
}

std::unique_ptr<ParameterNode> Parser::parse_parameter() {
    std::string raw_param = m_current_token.value;
    advance(); // Consume PARAMETER token

    std::string key = raw_param;
    std::string value = "";

    // Hapus awalan '-' ganda atau tunggal (e.g. --temp -> temp)
    size_t start_idx = 0;
    while (start_idx < key.length() && key[start_idx] == '-') {
        start_idx++;
    }
    key = key.substr(start_idx);

    if (m_current_token.type == TokenType::INT || 
        m_current_token.type == TokenType::FLOAT || 
        m_current_token.type == TokenType::TEXT) {
        
        value = m_current_token.value; 
    } 
    else {
        size_t eq_pos = key.find('=');
        if (eq_pos != std::string::npos) {
            value = key.substr(eq_pos + 1);
            key = key.substr(0, eq_pos);
        }
    }
    return std::make_unique<ParameterNode>(key, value);
}

std::unique_ptr<ASTNode> Parser::parse_argument() {
    // Basic lexer doesn't separate Variables, so everything is TEXT
    return parse_literal_text();
}

std::unique_ptr<VariableDeclarationNode> Parser::parse_variable_declaration() {
    // Fallback: This is not supported by the simplified Lexer Token set.
    throw std::runtime_error("Variable declarations are not supported by the current Lexer token set.");
}

std::unique_ptr<VariableAccessNode> Parser::parse_variable_access() {
    // Fallback: This is not supported by the simplified Lexer Token set.
    throw std::runtime_error("Variable access is not supported by the current Lexer token set.");
}

std::unique_ptr<LiteralTextNode> Parser::parse_literal_text() {
    std::string text_value = m_current_token.value;
    advance(); // Consume the TEXT token
    return std::make_unique<LiteralTextNode>(text_value);
}

CommandType Parser::get_command_type(const std::string& command_name) const {
    if (command_name == "set-model" || command_name == "set_model") return CommandType::SetModel;
    if (command_name == "add-url-model" || command_name == "add_url_model") return CommandType::AddUrlModel;
    if (command_name == "set-prompt" || command_name == "set_prompt") return CommandType::SetPrompt;
    if (command_name == "load-history" || command_name == "load_history") return CommandType::LoadHistory;
    if (command_name == "save-history" || command_name == "save_history") return CommandType::SaveHistory;
    if (command_name == "generate") return CommandType::Generate;
    if (command_name == "help") return CommandType::Help;
    if (command_name == "show-params" || command_name == "show_params") return CommandType::ShowParams;
    if (command_name == "set-param" || command_name == "set_param") return CommandType::SetParam;
    if (command_name == "exit") return CommandType::Exit;
    return CommandType::Unknown;
}