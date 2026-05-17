#include "parser.h"
#include <iostream>
#include <stdexcept>
#include <string>

static std::string indent_str(int n) {
    return std::string(n * 2, ' ');
}

void NumberLiteral::print(int indent) const {
    std::cout << indent_str(indent) << "NumberLiteral(" << value << ")\n";
}

void BoolLiteral::print(int indent) const {
    std::cout << indent_str(indent) << "BoolLiteral(" << (value ? "true" : "false") << ")\n";
}

void IdentExpr::print(int indent) const {
    std::cout << indent_str(indent) << "IdentExpr(" << name << ")\n";
}

void BinaryExpr::print(int indent) const {
    std::cout << indent_str(indent) << "BinaryExpr(" << op << ")\n";
    left->print(indent + 1);
    right->print(indent + 1);
}

void LetStmt::print(int indent) const {
    std::cout << indent_str(indent) << "LetStmt(" << name << ")\n";
    init->print(indent + 1);
}

void AssignStmt::print(int indent) const {
    std::cout << indent_str(indent) << "AssignStmt(" << name << ")\n";
    value->print(indent + 1);
}

void PrintStmt::print(int indent) const {
    std::cout << indent_str(indent) << "PrintStmt\n";
    value->print(indent + 1);
}

void InputStmt::print(int indent) const {
    std::cout << indent_str(indent) << "InputStmt(" << varName << ")\n";
}

void IfStmt::print(int indent) const {
    std::cout << indent_str(indent) << "IfStmt\n";
    std::cout << indent_str(indent + 1) << "Condition:\n";
    cond->print(indent + 2);
    std::cout << indent_str(indent + 1) << "Then:\n";
    for (const auto& s : thenBlock) {
        s->print(indent + 2);
    }
    if (!elseBlock.empty()) {
        std::cout << indent_str(indent + 1) << "Else:\n";
        for (const auto& s : elseBlock) {
            s->print(indent + 2);
        }
    }
}

void WhileStmt::print(int indent) const {
    std::cout << indent_str(indent) << "WhileStmt\n";
    std::cout << indent_str(indent + 1) << "Condition:\n";
    cond->print(indent + 2);
    std::cout << indent_str(indent + 1) << "Body:\n";
    for (const auto& s : body) {
        s->print(indent + 2);
    }
}

Parser::Parser(std::vector<Token> toks) : tokens(std::move(toks)), pos(0) {}

Token Parser::peek() const {
    return tokens[pos];
}

Token Parser::peekNext() const {
    if (pos + 1 < static_cast<int>(tokens.size())) {
        return tokens[pos + 1];
    }
    return tokens.back();
}

bool Parser::check(TokenType t) const {
    return peek().type == t;
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

void Parser::pushScope() {
    scopes.push_back({});
}

void Parser::popScope() {
    scopes.pop_back();
}

void Parser::declare(const Token& name) {
    if (scopes.back().find(name.value) != scopes.back().end()) {
        throw std::runtime_error(
            "[Line " + std::to_string(name.line) + "] Parse error: "
            "variable '" + name.value + "' is already declared in this scope");
    }

    scopes.back().insert(name.value);
}

bool Parser::isDeclared(const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        if (it->find(name) != it->end()) {
            return true;
        }
    }
    return false;
}

Token Parser::advance() {
    Token t = tokens[pos];
    if (!isAtEnd()) {
        pos++;
    }
    return t;
}

Token Parser::consume(TokenType expected, const std::string& errMsg) {
    if (check(expected)) {
        return advance();
    }

    Token cur = peek();
    throw std::runtime_error(
        "[Line " + std::to_string(cur.line) + "] Parse error: " +
        errMsg + " (got '" + cur.value + "')");
}

Block Parser::parseProgram() {
    pushScope();

    Block stmts;
    while (!isAtEnd()) {
        stmts.push_back(parseStatement());
    }

    popScope();
    return stmts;
}

Block Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expected '{' to open block");
    pushScope();

    Block stmts;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        stmts.push_back(parseStatement());
    }

    consume(TokenType::RBRACE, "Expected '}' to close block");
    popScope();
    return stmts;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    TokenType t = peek().type;

    if (t == TokenType::LET) {
        return parseLet();
    }
    if (t == TokenType::IF) {
        return parseIf();
    }
    if (t == TokenType::WHILE) {
        return parseWhile();
    }
    if (t == TokenType::PRINT) {
        return parsePrint();
    }
    if (t == TokenType::INPUT) {
        return parseInput();
    }

    return parseAssignOrExprStmt();
}

std::unique_ptr<Stmt> Parser::parseLet() {
    consume(TokenType::LET, "Expected 'let'");
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'let'");
    consume(TokenType::ASSIGN, "Expected '=' after variable name in 'let'");
    auto init = parseExpression();
    declare(name);
    return std::make_unique<LetStmt>(name.value, std::move(init));
}

std::unique_ptr<Stmt> Parser::parseIf() {
    consume(TokenType::IF, "Expected 'if'");
    auto cond = parseExpression();
    Block thenBlock = parseBlock();
    Block elseBlock;

    if (check(TokenType::ELSE)) {
        advance();
        elseBlock = parseBlock();
    }

    return std::make_unique<IfStmt>(
        std::move(cond),
        std::move(thenBlock),
        std::move(elseBlock));
}

std::unique_ptr<Stmt> Parser::parseWhile() {
    consume(TokenType::WHILE, "Expected 'while'");
    auto cond = parseExpression();
    Block body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body));
}

std::unique_ptr<Stmt> Parser::parsePrint() {
    consume(TokenType::PRINT, "Expected 'print'");
    auto val = parseExpression();
    return std::make_unique<PrintStmt>(std::move(val));
}

std::unique_ptr<Stmt> Parser::parseInput() {
    consume(TokenType::INPUT, "Expected 'input'");
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'input'");
    if (!isDeclared(name.value)) {
        throw std::runtime_error(
            "[Line " + std::to_string(name.line) + "] Parse error: "
            "input variable '" + name.value + "' must be declared with 'let' first");
    }
    return std::make_unique<InputStmt>(name.value);
}

std::unique_ptr<Stmt> Parser::parseAssignOrExprStmt() {
    if (check(TokenType::IDENTIFIER) && peekNext().type == TokenType::ASSIGN) {
        Token name = advance();
        if (!isDeclared(name.value)) {
            throw std::runtime_error(
                "[Line " + std::to_string(name.line) + "] Parse error: "
                "cannot assign to undeclared variable '" + name.value + "'");
        }
        advance();
        auto val = parseExpression();
        return std::make_unique<AssignStmt>(name.value, std::move(val));
    }

    Token cur = peek();
    throw std::runtime_error(
        "[Line " + std::to_string(cur.line) + "] Parse error: "
        "unexpected token '" + cur.value + "'. "
        "Did you mean 'let', 'print', 'if', 'while', or an assignment?");
}

std::unique_ptr<Expr> Parser::parseExpression() {
    auto left = parseAddSub();

    if (check(TokenType::EQ_EQ) ||
        check(TokenType::LESS) ||
        check(TokenType::GREATER)) {
        Token op = advance();
        auto right = parseAddSub();
        left = std::make_unique<BinaryExpr>(
            op.value,
            std::move(left),
            std::move(right));

        if (check(TokenType::EQ_EQ) ||
            check(TokenType::LESS) ||
            check(TokenType::GREATER)) {
            Token cur = peek();
            throw std::runtime_error(
                "[Line " + std::to_string(cur.line) + "] Parse error: "
                "only one comparison is allowed at a time");
        }
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseAddSub() {
    auto left = parseMulDiv();

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        Token op = advance();
        auto right = parseMulDiv();
        left = std::make_unique<BinaryExpr>(
            op.value,
            std::move(left),
            std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseMulDiv() {
    auto left = parseUnary();

    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        Token op = advance();
        auto right = parseUnary();
        left = std::make_unique<BinaryExpr>(
            op.value,
            std::move(left),
            std::move(right));
    }

    return left;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (check(TokenType::MINUS)) {
        Token op = advance();
        auto right = parseUnary();
        return std::make_unique<BinaryExpr>(
            op.value,
            std::make_unique<NumberLiteral>(0),
            std::move(right));
    }

    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    Token t = peek();

    if (t.type == TokenType::NUMBER) {
        advance();
        return std::make_unique<NumberLiteral>(std::stoi(t.value));
    }

    if (t.type == TokenType::TRUE_LIT) {
        advance();
        return std::make_unique<BoolLiteral>(true);
    }

    if (t.type == TokenType::FALSE_LIT) {
        advance();
        return std::make_unique<BoolLiteral>(false);
    }

    if (t.type == TokenType::IDENTIFIER) {
        advance();
        if (!isDeclared(t.value)) {
            throw std::runtime_error(
                "[Line " + std::to_string(t.line) + "] Parse error: "
                "use of undeclared variable '" + t.value + "'");
        }
        return std::make_unique<IdentExpr>(t.value);
    }

    throw std::runtime_error(
        "[Line " + std::to_string(t.line) + "] Parse error: "
        "unexpected token '" + t.value + "' in expression");
}
