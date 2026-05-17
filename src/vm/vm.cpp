#include "vm.h"
#include <iostream>
#include <stdexcept>
#include <string>

// ===============================================================
//  Stack helpers
// ===============================================================

void VM::push(CvmValue v) {
    stack.push_back(std::move(v));
}

CvmValue VM::pop() {
    if (stack.empty())
        throw std::runtime_error("VM: stack underflow");
    CvmValue v = std::move(stack.back());
    stack.pop_back();
    return v;
}

CvmValue& VM::top() {
    if (stack.empty())
        throw std::runtime_error("VM: stack underflow on peek");
    return stack.back();
}

// ===============================================================
//  Variable slot helpers
// ===============================================================

// Grow vars[] if the compiler allocated more slots than we have.
// Called before any DEFINE_VAR so the slot always exists.
void VM::ensureSlot(int slot) {
    if (slot >= (int)vars.size())
        vars.resize(slot + 1, CvmValue{0});
}

// ===============================================================
//  Value helpers
// ===============================================================

int VM::asInt(const CvmValue& v, const std::string& ctx) const {
    if (auto* i = std::get_if<int>(&v)) return *i;
    throw std::runtime_error("VM: expected integer in " + ctx);
}

// Truthy rule: bool true, or any non-zero int.
// This lets comparison results (int 1/0) drive JUMP_IF_FALSE
// without an extra bool-cast instruction.
bool VM::asBool(const CvmValue& v) const {
    if (auto* b = std::get_if<bool>(&v)) return *b;
    if (auto* i = std::get_if<int> (&v)) return *i != 0;
    return false;
}

void VM::printValue(const CvmValue& v) const {
    if (auto* i = std::get_if<int> (&v)) { std::cout << *i << "\n"; return; }
    if (auto* b = std::get_if<bool>(&v)) { std::cout << (*b ? "true" : "false") << "\n"; return; }
}

// ===============================================================
//  Single fetch-decode-execute step
//  Returns false when HALT is reached, true otherwise.
// ===============================================================

bool VM::step() {
    // Bounds check - should never fire on well-formed bytecode
    if (ip < 0 || ip >= (int)chunk->code.size())
        throw std::runtime_error("VM: instruction pointer out of bounds");

    const Instruction& instr = chunk->code[ip];
    ip++;   // advance past current instruction before executing
            // (JUMP / JUMP_IF_FALSE override ip after this)

    switch (instr.op) {

        // -------------------------------------------------------
        //  PUSH_INT  operand = index into constants[]
        //  Load an integer literal onto the stack.
        // -------------------------------------------------------
        case OpCode::PUSH_INT: {
            int idx = instr.operand;
            if (idx < 0 || idx >= (int)chunk->constants.size())
                throw std::runtime_error("VM: PUSH_INT constant index out of range");
            push(CvmValue{chunk->constants[idx]});
            break;
        }

        // -------------------------------------------------------
        //  PUSH_BOOL  operand = 0 (false) or 1 (true)
        // -------------------------------------------------------
        case OpCode::PUSH_BOOL: {
            push(CvmValue{instr.operand != 0});
            break;
        }

        // -------------------------------------------------------
        //  DEFINE_VAR  operand = slot
        //  Pop value and store it in vars[slot].
        //  Used only by 'let' declarations.
        //  STORE_VAR does the same thing at runtime; the distinction
        //  matters only conceptually (first write vs re-assignment).
        // -------------------------------------------------------
        case OpCode::DEFINE_VAR: {
            ensureSlot(instr.operand);
            vars[instr.operand] = pop();
            break;
        }

        // -------------------------------------------------------
        //  STORE_VAR  operand = slot
        //  Pop value and overwrite vars[slot].
        //  Used by assignment statements  x = <expr>.
        // -------------------------------------------------------
        case OpCode::STORE_VAR: {
            int slot = instr.operand;
            if (slot >= (int)vars.size())
                throw std::runtime_error(
                    "VM: STORE_VAR to uninitialised slot " +
                    std::to_string(slot));
            vars[slot] = pop();
            break;
        }

        // -------------------------------------------------------
        //  LOAD_VAR  operand = slot
        //  Push vars[slot] onto the stack.
        // -------------------------------------------------------
        case OpCode::LOAD_VAR: {
            int slot = instr.operand;
            if (slot < 0 || slot >= (int)vars.size())
                throw std::runtime_error(
                    "VM: LOAD_VAR from uninitialised slot " +
                    std::to_string(slot));
            push(vars[slot]);
            break;
        }

        // -------------------------------------------------------
        //  Arithmetic - pop two ints, push result int
        // -------------------------------------------------------
        case OpCode::ADD: {
            CvmValue b = pop(), a = pop();
            push(CvmValue{asInt(a,"ADD") + asInt(b,"ADD")});
            break;
        }
        case OpCode::SUB: {
            CvmValue b = pop(), a = pop();
            push(CvmValue{asInt(a,"SUB") - asInt(b,"SUB")});
            break;
        }
        case OpCode::MUL: {
            CvmValue b = pop(), a = pop();
            push(CvmValue{asInt(a,"MUL") * asInt(b,"MUL")});
            break;
        }
        case OpCode::DIV: {
            CvmValue b = pop(), a = pop();
            int divisor = asInt(b, "DIV");
            if (divisor == 0)
                throw std::runtime_error("VM: division by zero");
            push(CvmValue{asInt(a,"DIV") / divisor});
            break;
        }

        // -------------------------------------------------------
        //  NEGATE - pop int, push its negation
        //  Generated by the compiler for unary minus.
        // -------------------------------------------------------
        case OpCode::NEGATE: {
            CvmValue a = pop();
            push(CvmValue{-asInt(a, "NEGATE")});
            break;
        }

        // -------------------------------------------------------
        //  Comparison - pop two values, push int 1 or 0.
        //  Pushing an int (not bool) means the result can also
        //  be used in arithmetic, matching simple language semantics.
        // -------------------------------------------------------
        case OpCode::EQ: {
            CvmValue b = pop(), a = pop();
            // int == int  or  bool == bool
            push(CvmValue{int(a == b)});
            break;
        }
        case OpCode::LT: {
            CvmValue b = pop(), a = pop();
            push(CvmValue{int(asInt(a,"LT") < asInt(b,"LT"))});
            break;
        }
        case OpCode::GT: {
            CvmValue b = pop(), a = pop();
            push(CvmValue{int(asInt(a,"GT") > asInt(b,"GT"))});
            break;
        }

        // -------------------------------------------------------
        //  JUMP  operand = absolute instruction index
        //  Unconditional jump - used at end of then-block to
        //  skip over the else-block, and back-edge of while loop.
        // -------------------------------------------------------
        case OpCode::JUMP: {
            ip = instr.operand;
            break;
        }

        // -------------------------------------------------------
        //  JUMP_IF_FALSE  operand = absolute instruction index
        //  Pop the top value; if falsy, jump.
        //  Used for if-conditions and while-conditions.
        //  We consume the value (don't peek) so the stack stays
        //  clean - conditions are not reused after the branch.
        // -------------------------------------------------------
        case OpCode::JUMP_IF_FALSE: {
            CvmValue cond = pop();
            if (!asBool(cond))
                ip = instr.operand;
            break;
        }

        // -------------------------------------------------------
        //  PRINT - pop and print to stdout
        // -------------------------------------------------------
        case OpCode::PRINT: {
            CvmValue v = pop();
            printValue(v);
            break;
        }

        // -------------------------------------------------------
        //  INPUT  operand = slot
        //  Read one integer from stdin, store in vars[slot].
        //  The parser guarantees the variable was declared first.
        // -------------------------------------------------------
        case OpCode::INPUT: {
            int slot = instr.operand;
            ensureSlot(slot);
            int value = 0;
            if (!(std::cin >> value))
                throw std::runtime_error(
                    "VM: INPUT failed - expected an integer");
            vars[slot] = CvmValue{value};
            break;
        }

        // -------------------------------------------------------
        //  HALT - stop execution cleanly
        // -------------------------------------------------------
        case OpCode::HALT: {
            return false;   // signals run() to exit the loop
        }

        default:
            throw std::runtime_error(
                "VM: unknown opcode " +
                std::to_string(static_cast<int>(instr.op)));
    }

    return true;    // keep running
}

// ===============================================================
//  Public entry point
// ===============================================================

void VM::run(const Chunk& c) {
    chunk = &c;
    ip    = 0;
    stack.clear();

    // Pre-size vars to the number of debug labels the compiler
    // registered.  This is a lower bound; ensureSlot() will grow
    // it further if needed (e.g. for shadowed variables whose
    // slots exceed the label count).
    vars.assign(c.varNames.size(), CvmValue{0});

    while (step()) { /* fetch-decode-execute */ }
}
