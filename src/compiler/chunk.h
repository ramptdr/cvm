
#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// -------------------------------------------------------
// Every instruction the VM knows how to execute.
// -------------------------------------------------------
enum class OpCode : uint8_t {
    // Stack manipulation
    PUSH_INT,       // operand: index into constants[]
    PUSH_BOOL,      // operand: 0 = false, 1 = true

    // Variables  (operand = absolute slot index in VM's variable array)
    LOAD_VAR,       // push vars[operand] onto stack
    STORE_VAR,      // pop stack -> vars[operand]
    DEFINE_VAR,     // pop stack -> vars[operand]  (first definition via let)

    // Arithmetic
    ADD,
    SUB,
    MUL,
    DIV,

    // Comparison (result pushed as int 1 or 0)
    EQ,             // ==
    LT,             // <
    GT,             // >

    // Unary
    NEGATE,         // negate top of stack

    // Control flow (operand = absolute instruction index)
    JUMP,           // unconditional
    JUMP_IF_FALSE,  // pop; jump if 0/false

    // I/O
    PRINT,          // pop and print top of stack
    INPUT,          // operand = slot; read int from stdin -> vars[operand]

    // End
    HALT,
};

struct Instruction {
    OpCode op;
    int    operand = 0;
};

// -------------------------------------------------------
// Chunk - the compiled program.
// This is a DUMB flat container. It knows nothing about
// scope or variable names. The Compiler resolves all names
// to integer slot indices before emitting instructions.
// varNames is kept only so the disassembler can print
// human-readable labels - the VM never uses it.
// -------------------------------------------------------
struct Chunk {
    std::vector<Instruction> code;
    std::vector<int>         constants;
    std::vector<std::string> varNames;   // slot -> name (debug only)

    // Emit one instruction; return its index in code[]
    int emit(OpCode op, int operand = 0) {
        code.push_back({op, operand});
        return (int)code.size() - 1;
    }

    // Add integer literal to constant pool; return pool index
    int addConstant(int value) {
        constants.push_back(value);
        return (int)constants.size() - 1;
    }

    // Store a debug label for a slot (called by Compiler only)
    void setVarName(int slot, const std::string& name) {
        if (slot >= (int)varNames.size())
            varNames.resize(slot + 1, "?");
        varNames[slot] = name;
    }

    // Back-patch a jump instruction's target to the current end of code
    void patchJump(int instrIndex) {
        code[instrIndex].operand = (int)code.size();
    }

    void disassemble() const;
};
