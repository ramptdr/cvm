#include "compiler/compiler.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "vm/vm.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    bool showAST      = false;
    bool showBytecode = false;
    bool runProgram    = true;
    std::string filename;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--ast")      showAST      = true;
        else if (arg == "--bytecode") showBytecode = true;
        else if (arg == "--no-run")   runProgram   = false;
        else                          filename     = arg;
    }

    if (filename.empty()) {
        std::cerr << "Usage: ./cvm [--ast] [--bytecode] [--no-run] <script.cvm>\n";
        return 1;
    }

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: could not open '" << filename << "'\n";
        return 1;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        Parser parser(std::move(tokens));
        Block program = parser.parseProgram();

        if (showAST) {
            std::cout << "=== AST ===\n";
            for (const auto& stmt : program) stmt->print(0);
            std::cout << "===========\n";
        }

        Compiler compiler;
        Chunk chunk = compiler.compile(program);

        if (showBytecode) chunk.disassemble();

        if (runProgram) {
            VM vm;
            vm.run(chunk);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
