#pragma once 
// Header guard to prevent multiple inclusions
#include <string>
#include <memory>
#include <vector>

enum class BinOp {
    Add, Sub, Mul, Div, Mod,
    Lt, Gt, Le, Ge, Eq, Ne,
    And, Or
};

enum class UnOp {
    Neg, Not
};
// Base class for all expressions
class Expr{
public:
    virtual ~Expr() = default;//use default destructor
};

class IntLit : public Expr {
public:
    IntLit(int v) 
        : value(v) {}
    int value;
};

class Var : public Expr {
public:
    Var(const std::string& n) 
        : name(n) {}
    std::string name;
};

class BinOpExpr : public Expr {
public:
    //use unique_ptr for memory management
    std::unique_ptr<Expr> left;
    BinOp op;
    std::unique_ptr<Expr> right;
    //use std::move to transfer ownership
    BinOpExpr(std::unique_ptr<Expr> l, BinOp o, std::unique_ptr<Expr> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
};

class UnOpExpr : public Expr {
public:
    UnOp op;
    std::unique_ptr<Expr> right;

    UnOpExpr(UnOp o, std::unique_ptr<Expr> r)
        : op(o), right(std::move(r)) {}
};

class Call : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
    
    Call(const std::string& name, std::vector<std::unique_ptr<Expr>> a)
        : name(name), args(std::move(a)) {}
};

//Base class for all statements
class Stmt {
public:
    virtual ~Stmt() = default;
};

class Block : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> stmts;
    
    Block(std::vector<std::unique_ptr<Stmt>> stmts)
        : stmts(std::move(stmts)) {}
};

class EmptyStmt : public Stmt {};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    
    ExprStmt(std::unique_ptr<Expr> e) 
        : expr(std::move(e)) {}
};

class Assign : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    
    Assign(const std::string& name, std::unique_ptr<Expr> v)
        : name(name), value(std::move(v)) {}
};

class Decl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    // allow initialization with or without an initial value
    Decl(const std::string& name, std::unique_ptr<Expr> init = nullptr)
        : name(name), value(std::move(init)) {}
};

class If : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBody;
    std::unique_ptr<Stmt> elseBody; 
    //else branch is optional
    If(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then, std::unique_ptr<Stmt> elseStmt = nullptr)
        : condition(std::move(cond)), thenBody(std::move(then)), elseBody(std::move(elseStmt)) {}
};

class While : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    While(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {};
};

class Break : public Stmt {
public:
    Break() = default;
};
class Continue : public Stmt {
public:
    Continue() = default;
};
class Return : public Stmt {
public:
    std::unique_ptr<Expr> returnValue;
    // allow return with or without a value
    Return(std::unique_ptr<Expr> value = nullptr)
        : returnValue(std::move(value)) {}
};
//return type
enum class RetType {
    Int, Void
};

class FuncDef {
public:
    std::string name;
    RetType rtype;
    std::vector<std::string> args;
    std::unique_ptr<Stmt> body;
    
    FuncDef(const std::string& name, RetType rt, 
            std::vector<std::string> p, std::unique_ptr<Stmt> b)
        : name(name), rtype(rt), args(std::move(p)), body(std::move(b)) {}
};
class Program {
public:
    std::vector<std::unique_ptr<FuncDef>> functions;
    
    Program() = default;
    Program(std::vector<std::unique_ptr<FuncDef>> f) 
        : functions(std::move(f)) {}
};