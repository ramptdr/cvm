#include "compiler.h"
#include <stdexcept>

// ===============================================================
//  Entry point
// ===============================================================

Chunk Compiler::compile(Block& program) {
    // Global scope wraps the whole program
    scopes.pushScope();
    compileStmts(program);
    scopes.popScope();
    chunk.emit(OpCode::HALT);
    return std::move(chunk);
}

// Compile a list of statements without touching the scope stack.
// Used at top-level and inside if/while after the caller has
// already pushed the appropriate scope.
void Compiler::compileStmts(Block& block) {
    for (auto& stmt : block)
        stmt->accept(*this);
}

// Compile a { block } - pushes its own scope so that 'let'
// inside the block gets a fresh slot even if the same name
// exists in an outer scope (shadowing).
void Compiler::compileBlock(Block& block) {
    scopes.pushScope();
    compileStmts(block);
    scopes.popScope();
}

// ===============================================================
//  Expressions
// ===============================================================

// 42  ->  PUSH_INT <constant-pool-index>
void Compiler::visit(NumberLiteral& node) {
    int idx = chunk.addConstant(node.value);
    chunk.emit(OpCode::PUSH_INT, idx);
}

// true/false  ->  PUSH_BOOL 1/0
void Compiler::visit(BoolLiteral& node) {
    chunk.emit(OpCode::PUSH_BOOL, node.value ? 1 : 0);
}

// x  ->  LOAD_VAR <slot>
// Resolves from innermost scope outward, so shadowed inner
// variables correctly map to their own slot.
void Compiler::visit(IdentExpr& node) {
    int slot = scopes.resolve(node.name);
    if (slot == -1)
        throw std::runtime_error(
            "Compiler: undefined variable '" + node.name + "'");
    chunk.emit(OpCode::LOAD_VAR, slot);
}

// BinaryExpr covers all operators plus the parser's unary-minus
// encoding (BinaryExpr("-", NumberLiteral(0), right)).
void Compiler::visit(BinaryExpr& node) {
    // Detect unary-minus encoding and emit a cleaner NEGATE
    if (node.op == "-") {
        auto* zero = dynamic_cast<NumberLiteral*>(node.left.get());
        if (zero && zero->value == 0) {
            node.right->accept(*this);
            chunk.emit(OpCode::NEGATE);
            return;
        }
    }

    // Normal binary: left, right, operator
    node.left->accept(*this);
    node.right->accept(*this);

    if      (node.op == "+")  chunk.emit(OpCode::ADD);
    else if (node.op == "-")  chunk.emit(OpCode::SUB);
    else if (node.op == "*")  chunk.emit(OpCode::MUL);
    else if (node.op == "/")  chunk.emit(OpCode::DIV);
    else if (node.op == "==") chunk.emit(OpCode::EQ);
    else if (node.op == "<")  chunk.emit(OpCode::LT);
    else if (node.op == ">")  chunk.emit(OpCode::GT);
    else
        throw std::runtime_error(
            "Compiler: unknown operator '" + node.op + "'");
}

// ===============================================================
//  Statements
// ===============================================================

// let x = <expr>
//
// Key fix: always allocate a BRAND NEW slot via scopes.define().
// This means  `let x = 1; if true { let x = 2 }`  gives:
//   outer x -> slot 0
//   inner x -> slot 1   (separate, never overwrites slot 0)
//
// Bytecode: <expr>  DEFINE_VAR <new-slot>
void Compiler::visit(LetStmt& node) {
    node.init->accept(*this);              // push initial value
    int slot = scopes.define(node.name);   // NEW slot in current scope
    chunk.setVarName(slot, node.name);     // debug label only
    chunk.emit(OpCode::DEFINE_VAR, slot);
}

// x = <expr>
//
// Resolves from innermost scope outward, so assignment inside
// a block correctly targets the innermost binding of that name.
//
// Bytecode: <expr>  STORE_VAR <slot>
void Compiler::visit(AssignStmt& node) {
    node.value->accept(*this);
    int slot = scopes.resolve(node.name);
    if (slot == -1)
        throw std::runtime_error(
            "Compiler: assignment to undeclared variable '" + node.name + "'");
    chunk.emit(OpCode::STORE_VAR, slot);
}

// print <expr>
// Bytecode: <expr>  PRINT
void Compiler::visit(PrintStmt& node) {
    node.value->accept(*this);
    chunk.emit(OpCode::PRINT);
}

// input <varName>
// Bytecode: INPUT <slot>
// (VM reads from stdin and stores directly into vars[slot])
void Compiler::visit(InputStmt& node) {
    int slot = scopes.resolve(node.varName);
    if (slot == -1)
        throw std::runtime_error(
            "Compiler: 'input' on undeclared variable '" + node.varName + "'");
    chunk.emit(OpCode::INPUT, slot);
}

// if <cond> { thenBlock } [ else { elseBlock } ]
//
// Layout:
//   <cond>
//   JUMP_IF_FALSE -> elseStart   (or afterIf when no else)
//   <thenBlock>                  (own scope)
//   JUMP          -> afterIf     (only when else exists)
//   elseStart:
//   <elseBlock>                  (own scope)
//   afterIf:
//
// Each branch gets compileBlock() so variables declared inside
// an if/else branch live only in that branch's scope.
void Compiler::visit(IfStmt& node) {
    node.cond->accept(*this);

    int jumpIfFalse = chunk.emit(OpCode::JUMP_IF_FALSE, 0);

    compileBlock(node.thenBlock);

    if (!node.elseBlock.empty()) {
        int jumpOverElse = chunk.emit(OpCode::JUMP, 0);
        chunk.patchJump(jumpIfFalse);      // false -> start of else
        compileBlock(node.elseBlock);
        chunk.patchJump(jumpOverElse);     // end of then -> after else
    } else {
        chunk.patchJump(jumpIfFalse);      // false -> after then
    }
}

// while <cond> { body }
//
// Layout:
//   loopStart:
//   <cond>
//   JUMP_IF_FALSE -> afterLoop
//   <body>                       (own scope - fresh each iteration)
//   JUMP          -> loopStart
//   afterLoop:
//
// The body gets compileBlock() so variables declared inside
// the loop (e.g. let temp = ...) are scoped per-iteration
// at the compiler level (slots are still static, but names
// resolve correctly and don't leak outside).
void Compiler::visit(WhileStmt& node) {
    int loopStart = (int)chunk.code.size();

    node.cond->accept(*this);

    int exitJump = chunk.emit(OpCode::JUMP_IF_FALSE, 0);

    compileBlock(node.body);

    chunk.emit(OpCode::JUMP, loopStart);
    chunk.patchJump(exitJump);
}
