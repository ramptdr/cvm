#pragma once

#include "ast.h"
#include "lexer/lexer.h"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

class Parser {
    std::vector<Token> tokens;
    std::vector<std::unordered_set<std::string>> scopes;
    int pos = 0;

    Token peek() const;
    Token peekNext() const;
    bool check(TokenType t) const;
    bool isAtEnd() const;
    void pushScope();
    void popScope();
    void declare(const Token& name);
    bool isDeclared(const std::string& name) const;
    Token advance();
    Token consume(TokenType expected, const std::string& errMsg);

    Block parseBlock();

    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Stmt> parseLet();
    std::unique_ptr<Stmt> parseIf();
    std::unique_ptr<Stmt> parseWhile();
    std::unique_ptr<Stmt> parsePrint();
    std::unique_ptr<Stmt> parseInput();
    std::unique_ptr<Stmt> parseAssignOrExprStmt();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAddSub();
    std::unique_ptr<Expr> parseMulDiv();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();

public:
    explicit Parser(std::vector<Token> tokens);

    Block parseProgram();
};
