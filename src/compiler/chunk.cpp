#include "chunk.h"
#include <iomanip>
#include <iostream>

// Human-readable opcode names (must match OpCode enum order)
static const char* opName(OpCode op) {
    switch (op) {
        case OpCode::PUSH_INT:      return "PUSH_INT";
        case OpCode::PUSH_BOOL:     return "PUSH_BOOL";
        case OpCode::LOAD_VAR:      return "LOAD_VAR";
        case OpCode::STORE_VAR:     return "STORE_VAR";
        case OpCode::DEFINE_VAR:    return "DEFINE_VAR";
        case OpCode::ADD:           return "ADD";
        case OpCode::SUB:           return "SUB";
        case OpCode::MUL:           return "MUL";
        case OpCode::DIV:           return "DIV";
        case OpCode::EQ:            return "EQ";
        case OpCode::LT:            return "LT";
        case OpCode::GT:            return "GT";
        case OpCode::NEGATE:        return "NEGATE";
        case OpCode::JUMP:          return "JUMP";
        case OpCode::JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OpCode::PRINT:         return "PRINT";
        case OpCode::INPUT:         return "INPUT";
        case OpCode::HALT:          return "HALT";
        default:                    return "???";
    }
}

void Chunk::disassemble() const {
    std::cout << "\n=== BYTECODE DISASSEMBLY ===\n";
    std::cout << std::left;

    // Print constant pool
    if (!constants.empty()) {
        std::cout << "Constants:\n";
        for (int i = 0; i < (int)constants.size(); ++i)
            std::cout << "  [" << i << "] " << constants[i] << "\n";
    }

    // Print variable table
    if (!varNames.empty()) {
        std::cout << "Variables:\n";
        for (int i = 0; i < (int)varNames.size(); ++i)
            std::cout << "  [" << i << "] " << varNames[i] << "\n";
    }

    // Print instructions
    std::cout << "Instructions:\n";
    for (int i = 0; i < (int)code.size(); ++i) {
        const Instruction& instr = code[i];
        std::cout << "  " << std::setw(4) << i << "  "
                  << std::setw(16) << opName(instr.op);

        // Print operand with context where useful
        switch (instr.op) {
            case OpCode::PUSH_INT:
                std::cout << constants[instr.operand]
                          << "  (const[" << instr.operand << "])";
                break;
            case OpCode::PUSH_BOOL:
                std::cout << (instr.operand ? "true" : "false");
                break;
            case OpCode::LOAD_VAR:
            case OpCode::STORE_VAR:
            case OpCode::DEFINE_VAR:
            case OpCode::INPUT:
                if (instr.operand < (int)varNames.size())
                    std::cout << varNames[instr.operand]
                              << "  (var[" << instr.operand << "])";
                break;
            case OpCode::JUMP:
            case OpCode::JUMP_IF_FALSE:
                std::cout << "-> " << instr.operand;
                break;
            default:
                break;
        }
        std::cout << "\n";
    }
    std::cout << "============================\n\n";
}
