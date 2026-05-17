#pragma once

#include <string>
#include <vector>

enum class TokenType {
    NUMBER,
    IDENTIFIER,

    LET,
    PRINT,
    INPUT,
    IF,
    ELSE,
    WHILE,
    TRUE_LIT,
    FALSE_LIT,

    PLUS,
    MINUS,
    STAR,
    SLASH,

    EQ_EQ,
    LESS,
    GREATER,

    ASSIGN,

    LBRACE,
    RBRACE,

    END_OF_FILE
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
    std::string src;
    int pos;
    int line;

public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    char peek();
    char peekNext();
    char advance();
    void skipWhitespace();
    Token readNumber();
    Token readIdentifierOrKeyword();
};
