#pragma once
#include "compiler/chunk.h"
#include <string>
#include <variant>
#include <vector>

// -------------------------------------------------------
// CvmValue - the only two types CVM++ supports at runtime:
//   int   covers all integer arithmetic
//   bool  covers true / false literals and comparison results
//
// std::variant gives us a type-safe tagged union with no
// manual memory management and no undefined behaviour on
// wrong-type access.
// -------------------------------------------------------
using CvmValue = std::variant<int, bool>;

// -------------------------------------------------------
// VM - stack-based execution engine.
//
// Internals:
//   ip    instruction pointer (index into chunk.code)
//   stack value stack (grows upward, no fixed size limit)
//   vars  variable storage (slot index -> CvmValue)
//         sized to the total number of slots the compiler
//         allocated; indexed by the operand of LOAD/STORE/
//         DEFINE_VAR instructions.
//
// The run() method is the only public interface.
// It executes until HALT or a runtime error.
// -------------------------------------------------------
class VM {
    // --- runtime state ---
    const Chunk*        chunk = nullptr;
    int                 ip    = 0;
    std::vector<CvmValue> stack;
    std::vector<CvmValue> vars;

    // --- stack helpers ---
    void        push(CvmValue v);
    CvmValue    pop();
    CvmValue&   top();

    // --- slot helpers ---
    void        ensureSlot(int slot);

    // --- value helpers ---
    // Extract int from a CvmValue; throws on type mismatch
    int         asInt (const CvmValue& v, const std::string& ctx) const;
    // Extract bool from a CvmValue; both int and bool are accepted
    // (non-zero int is truthy - lets comparison results drive jumps)
    bool        asBool(const CvmValue& v)                          const;
    // Print a CvmValue to stdout
    void        printValue(const CvmValue& v)                      const;

    // --- single fetch-decode-execute step ---
    // Returns false when HALT is reached
    bool        step();

public:
    // Run the compiled program in 'c' to completion.
    // Throws std::runtime_error on any runtime fault.
    void run(const Chunk& c);
};
