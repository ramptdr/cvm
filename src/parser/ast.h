#pragma once

#include "lexer/lexer.h"
#include <memory>
#include <string>
#include <vector>

struct Expr;
struct Stmt;
struct NumberLiteral;
struct BoolLiteral;
struct IdentExpr;
struct BinaryExpr;
struct LetStmt;
struct AssignStmt;
struct PrintStmt;
struct InputStmt;
struct IfStmt;
struct WhileStmt;

using Block = std::vector<std::unique_ptr<Stmt>>;

struct Visitor {
    virtual void visit(NumberLiteral& node) = 0;
    virtual void visit(BoolLiteral& node) = 0;
    virtual void visit(IdentExpr& node) = 0;
    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(LetStmt& node) = 0;
    virtual void visit(AssignStmt& node) = 0;
    virtual void visit(PrintStmt& node) = 0;
    virtual void visit(InputStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual ~Visitor() = default;
};

struct Expr {
    virtual ~Expr() = default;
    virtual void print(int indent = 0) const = 0;
    virtual void accept(Visitor& visitor) = 0;
};

struct NumberLiteral : Expr {
    int value;

    explicit NumberLiteral(int v) : value(v) {}
    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct BoolLiteral : Expr {
    bool value;

    explicit BoolLiteral(bool v) : value(v) {}
    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct IdentExpr : Expr {
    std::string name;

    explicit IdentExpr(const std::string& n) : name(n) {}
    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct BinaryExpr : Expr {
    std::string op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(const std::string& op,
               std::unique_ptr<Expr> l,
               std::unique_ptr<Expr> r)
        : op(op), left(std::move(l)), right(std::move(r)) {}

    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct Stmt {
    virtual ~Stmt() = default;
    virtual void print(int indent = 0) const = 0;
    virtual void accept(Visitor& visitor) = 0;
};

struct LetStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> init;

    LetStmt(const std::string& n, std::unique_ptr<Expr> e)
        : name(n), init(std::move(e)) {}

    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct AssignStmt : Stmt {
    std::string name;
    std::unique_ptr<Expr> value;

    AssignStmt(const std::string& n, std::unique_ptr<Expr> v)
        : name(n), value(std::move(v)) {}

    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct PrintStmt : Stmt {
    std::unique_ptr<Expr> value;

    explicit PrintStmt(std::unique_ptr<Expr> v) : value(std::move(v)) {}
    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct InputStmt : Stmt {
    std::string varName;

    explicit InputStmt(const std::string& n) : varName(n) {}
    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct IfStmt : Stmt {
    std::unique_ptr<Expr> cond;
    Block thenBlock;
    Block elseBlock;

    IfStmt(std::unique_ptr<Expr> c, Block tb, Block eb)
        : cond(std::move(c)),
          thenBlock(std::move(tb)),
          elseBlock(std::move(eb)) {}

    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Expr> cond;
    Block body;

    WhileStmt(std::unique_ptr<Expr> c, Block b)
        : cond(std::move(c)), body(std::move(b)) {}

    void print(int indent = 0) const override;
    void accept(Visitor& visitor) override { visitor.visit(*this); }
};
