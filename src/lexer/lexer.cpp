#include "lexer.h"
#include <cctype>
#include <iostream>
#include <stdexcept>

// -------------------------------------------------------
// Constructor
// -------------------------------------------------------
Lexer::Lexer(const std::string& source)
    : src(source), pos(0), line(1) {}

// -------------------------------------------------------
// Helper methods
// -------------------------------------------------------

// Look at current character without consuming it
char Lexer::peek() {
    if (pos >= (int)src.size()) return '\0';
    return src[pos];
}

// Look one character ahead without consuming
char Lexer::peekNext() {
    if (pos + 1 >= (int)src.size()) return '\0';
    return src[pos + 1];
}

// Consume current character and move forward
char Lexer::advance() {
    return src[pos++];
}

// Skip spaces and tabs (NOT newlines -- we track those for line numbers)
void Lexer::skipWhitespace() {
    while (pos < (int)src.size()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r')
            advance();
        else
            break;
    }
}

// -------------------------------------------------------
// Read a full number like 42 or 1000
// -------------------------------------------------------
Token Lexer::readNumber() {
    std::string num;
    while (pos < (int)src.size() && isdigit(peek()))
        num += advance();
    return { TokenType::NUMBER, num, line };
}

// -------------------------------------------------------
// Read a word -- then check if it's a keyword or identifier
// -------------------------------------------------------
Token Lexer::readIdentifierOrKeyword() {
    std::string word;
    while (pos < (int)src.size() && (isalnum(peek()) || peek() == '_'))
        word += advance();

    // Check against every keyword
    if (word == "let")   return { TokenType::LET,       word, line };
    if (word == "print") return { TokenType::PRINT,      word, line };
    if (word == "input") return { TokenType::INPUT,      word, line };
    if (word == "if")    return { TokenType::IF,         word, line };
    if (word == "else")  return { TokenType::ELSE,       word, line };
    if (word == "while") return { TokenType::WHILE,      word, line };
    if (word == "true")  return { TokenType::TRUE_LIT,   word, line };
    if (word == "false") return { TokenType::FALSE_LIT,  word, line };

    // Not a keyword -- it's a variable/identifier name
    return { TokenType::IDENTIFIER, word, line };
}

// -------------------------------------------------------
// Main tokenize loop -- heart of the Lexer
// -------------------------------------------------------
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < (int)src.size()) {

        skipWhitespace();
        if (pos >= (int)src.size()) break;

        char c = peek();

        // Track line numbers
        if (c == '\n') {
            line++;
            advance();
            continue;
        }

        // Numbers
        if (isdigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }

        // Identifiers and keywords
        if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }

        // Everything else -- single or double character operators
        advance(); // consume the character

        switch (c) {
            case '+': tokens.push_back({ TokenType::PLUS,    "+", line }); break;
            case '-': tokens.push_back({ TokenType::MINUS,   "-", line }); break;
            case '*': tokens.push_back({ TokenType::STAR,    "*", line }); break;
            case '/': tokens.push_back({ TokenType::SLASH,   "/", line }); break;
            case '{': tokens.push_back({ TokenType::LBRACE,  "{", line }); break;
            case '}': tokens.push_back({ TokenType::RBRACE,  "}", line }); break;
            case '<': tokens.push_back({ TokenType::LESS,    "<", line }); break;
            case '>': tokens.push_back({ TokenType::GREATER, ">", line }); break;

            case '=':
                // Could be = or ==
                if (peek() == '=') {
                    advance(); // consume second =
                    tokens.push_back({ TokenType::EQ_EQ,  "==", line });
                } else {
                    tokens.push_back({ TokenType::ASSIGN, "=",  line });
                }
                break;

            default:
                throw std::runtime_error(
                    "Lex error: unknown character '" +
                    std::string(1, c) +
                    "' on line " + std::to_string(line));
        }
    }

    tokens.push_back({ TokenType::END_OF_FILE, "", line });
    return tokens;
}
