#pragma once
#include "parser/ast.h"
#include "chunk.h"
#include <string>
#include <unordered_map>
#include <vector>

// -------------------------------------------------------
// ScopeStack - compiler-side name -> slot resolution.
//
// Each scope level is a map from variable name to its
// absolute slot index in the VM's variable array.
// On every 'let', we allocate a NEW slot regardless of
// whether the same name exists in an outer scope.
// On lookup we walk from innermost scope outward so the
// innermost binding shadows any outer one.
// On scope exit the inner slots are simply forgotten -
// the VM's variable array keeps growing monotonically,
// which is fine for a simple language without closures.
// -------------------------------------------------------
class ScopeStack {
    // Each entry: name -> slot
    std::vector<std::unordered_map<std::string, int>> scopes;
    int nextSlot = 0;   // ever-increasing slot counter

public:
    void pushScope() { scopes.push_back({}); }
    void popScope()  { scopes.pop_back(); }

    // Allocate a brand-new slot for 'name' in the current scope.
    // Returns the new slot index.
    int define(const std::string& name) {
        int slot = nextSlot++;
        scopes.back()[name] = slot;
        return slot;
    }

    // Resolve 'name' from innermost scope outward.
    // Returns slot index, or -1 if not found.
    int resolve(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second;
        }
        return -1;
    }

    int totalSlots() const { return nextSlot; }
};

// -------------------------------------------------------
// Compiler - walks the AST via the Visitor interface
// and emits bytecode into a Chunk.
// -------------------------------------------------------
class Compiler : public Visitor {
    Chunk      chunk;
    ScopeStack scopes;

    // --- Visitor: expressions ---
    void visit(NumberLiteral& node) override;
    void visit(BoolLiteral&   node) override;
    void visit(IdentExpr&     node) override;
    void visit(BinaryExpr&    node) override;

    // --- Visitor: statements ---
    void visit(LetStmt&    node) override;
    void visit(AssignStmt& node) override;
    void visit(PrintStmt&  node) override;
    void visit(InputStmt&  node) override;
    void visit(IfStmt&     node) override;
    void visit(WhileStmt&  node) override;

    // Compile a sequence of statements (opens/closes NO scope -
    // the caller is responsible for push/popScope around blocks)
    void compileStmts(Block& block);

    // Compile a { block } - pushes and pops its own scope
    void compileBlock(Block& block);

public:
    Chunk compile(Block& program);
};
