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
    virtual std::unique_ptr<Expr> foldConstants() {
        return nullptr; // Default implementation does nothing
    }
};

class IntLit : public Expr {
public:
    IntLit(int v) 
        : value(v) {}
    int value;
    std::unique_ptr<Expr> foldConstants() override {
        return std::make_unique<IntLit>(value); // Return itself as it is already constant
    }
};

class Var : public Expr {
public:
    Var(const std::string& n) 
        : name(n) {}
    std::string name;
    std::unique_ptr<Expr> foldConstants() override {
        return std::make_unique<Var>(name); // Return itself as it is a variable
    }
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
    std::unique_ptr<Expr> foldConstants() override {
        auto leftFolded = left->foldConstants();
        auto rightFolded = right->foldConstants();
        if (auto leftLit = dynamic_cast<IntLit*>(leftFolded.get())) {
            if (auto rightLit = dynamic_cast<IntLit*>(rightFolded.get())) {
                int result;
                switch (op) {
                    case BinOp::Add: result = leftLit->value + rightLit->value;
                        break;
                    case BinOp::Sub: result = leftLit->value - rightLit->value;
                        break;
                    case BinOp::Mul: result = leftLit->value * rightLit->value;
                        break;
                    case BinOp::Div:
                        if (rightLit->value == 0) {
                            throw std::runtime_error("Division by zero");
                        }
                        result = leftLit->value / rightLit->value;
                        break;
                    case BinOp::Mod:
                        if (rightLit->value == 0) {
                            throw std::runtime_error("Modulo by zero");
                        }
                        result = leftLit->value % rightLit->value;
                        break;
                    case BinOp::Lt: result = leftLit->value < rightLit->value ? 1 : 0;
                        break;
                    case BinOp::Gt: result = leftLit->value > rightLit->value ? 1 : 0;
                        break;
                    case BinOp::Le: result = leftLit->value <= rightLit->value ? 1 : 0;
                        break;
                    case BinOp::Ge: result = leftLit->value >= rightLit->value ? 1 : 0;
                        break;
                    case BinOp::Eq: result = leftLit->value == rightLit->value ? 1 : 0;
                        break;
                    case BinOp::Ne: result = leftLit->value != rightLit->value ? 1 : 0;
                        break;
                    case BinOp::And: result = (leftLit->value != 0 && rightLit->value != 0) ? 1 : 0;
                        break;
                    case BinOp::Or: result = (leftLit->value != 0 || rightLit->value != 0) ? 1 : 0;
                        break;
                }
                return std::make_unique<IntLit>(result);
            }
        }
        // If we can't fold constants, return a new BinOpExpr with folded children
        return std::make_unique<BinOpExpr>(std::move(leftFolded), op, std::move(rightFolded));
    }   
};

class UnOpExpr : public Expr {
public:
    UnOp op;
    std::unique_ptr<Expr> right;

    UnOpExpr(UnOp o, std::unique_ptr<Expr> r)
        : op(o), right(std::move(r)) {}
    std::unique_ptr<Expr> foldConstants() override {
        auto rightFolded = right->foldConstants();
        if (auto rightLit = dynamic_cast<IntLit*>(rightFolded.get())) {
            int result;
            switch (op) {
                case UnOp::Neg: result = -rightLit->value;
                    break;
                case UnOp::Not: result = (rightLit->value == 0) ? 1 : 0;
                    break;
            }
            return std::make_unique<IntLit>(result);
        }
        // If we can't fold constants, return a new UnOpExpr with folded child
        return std::make_unique<UnOpExpr>(op, std::move(rightFolded));
    }
};

class Call : public Expr {
public:
    std::string name;
    std::vector<std::unique_ptr<Expr>> args;

    Call(const std::string& name, std::vector<std::unique_ptr<Expr>> a)
        : name(name), args(std::move(a)) {}

    std::unique_ptr<Expr> foldConstants() override {
        std::vector<std::unique_ptr<Expr>> foldedArgs;
        for (auto& arg : args) {
            auto folded = arg->foldConstants();
            foldedArgs.push_back(folded ? std::move(folded) : std::move(arg));
        }
        return std::make_unique<Call>(name, std::move(foldedArgs));
    }
};

//Base class for all statements
class Stmt {
public:
    // 活跃变量分析结果：该语句结点的活跃变量集合
    std::vector<std::string> liveVars;
    virtual ~Stmt() = default;
    // Default implementation does nothing
    virtual std::unique_ptr<Stmt> foldConstants() {
        return nullptr; // Default implementation does nothing
    }
};

class Block : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> stmts;
    // Block 入口/出口活跃变量
    std::vector<std::string> liveIn;
    std::vector<std::string> liveOut;
    Block(std::vector<std::unique_ptr<Stmt>> stmts)
        : stmts(std::move(stmts)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        std::vector<std::unique_ptr<Stmt>> foldedStmts;
        for (auto& stmt : stmts) {
            auto folded = stmt->foldConstants();
            if (folded) {
                foldedStmts.push_back(std::move(folded));
            }
        }
        return std::make_unique<Block>(std::move(foldedStmts));
    }
};

class EmptyStmt : public Stmt {
public:
    EmptyStmt() = default;
    std::unique_ptr<Stmt> foldConstants() override {
        return std::make_unique<EmptyStmt>(); // Return itself as it is an empty statement
    }
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expr;
    
    ExprStmt(std::unique_ptr<Expr> e) 
        : expr(std::move(e)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        auto foldedExpr = expr->foldConstants();
        if (foldedExpr) {
            return std::make_unique<ExprStmt>(std::move(foldedExpr));
        }
        return std::make_unique<ExprStmt>(std::move(expr)); // Return itself if no folding occurred
    }
};

class Assign : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    Assign(const std::string& name, std::unique_ptr<Expr> v)
        : name(name), value(std::move(v)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        auto foldedValue = value->foldConstants();
        if (foldedValue) {
            return std::make_unique<Assign>(name, std::move(foldedValue));
        }
        return std::make_unique<Assign>(name, std::move(value)); // Return itself
    }
};

class Decl : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    Decl(const std::string& name, std::unique_ptr<Expr> init = nullptr)
        : name(name), value(std::move(init)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        auto foldedValue = value ? value->foldConstants() : nullptr;
        if (foldedValue) {
            return std::make_unique<Decl>(name, std::move(foldedValue));
        }
        return std::make_unique<Decl>(name, std::move(value)); // Return itself
    }
};

class If : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBody;
    std::unique_ptr<Stmt> elseBody; 
    // If 入口/出口活跃变量
    std::vector<std::string> liveIn;
    std::vector<std::string> liveOut;
    //else branch is optional
    If(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then, std::unique_ptr<Stmt> elseStmt = nullptr)
        : condition(std::move(cond)), thenBody(std::move(then)), elseBody(std::move(elseStmt)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        auto condFolded = condition ? condition->foldConstants() : nullptr;
        auto thenFolded = thenBody ? thenBody->foldConstants() : nullptr;
        auto elseFolded = elseBody ? elseBody->foldConstants() : nullptr;
        return std::make_unique<If>(
            condFolded ? std::move(condFolded) : std::move(condition),
            thenFolded ? std::move(thenFolded) : std::move(thenBody),
            elseFolded ? std::move(elseFolded) : (elseBody ? std::move(elseBody) : nullptr)
        );
    }
};

class While : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;
    // While 入口/出口活跃变量
    std::vector<std::string> liveIn;
    std::vector<std::string> liveOut;
    While(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        auto condFolded = condition ? condition->foldConstants() : nullptr;
        auto bodyFolded = body ? body->foldConstants() : nullptr;
        return std::make_unique<While>(
            condFolded ? std::move(condFolded) : std::move(condition),
            bodyFolded ? std::move(bodyFolded) : std::move(body)
        );
    }
};

class Break : public Stmt {
public:
    Break() = default;
    std::unique_ptr<Stmt> foldConstants() override {
        return std::make_unique<Break>();
    }
};
class Continue : public Stmt {
public:
    Continue() = default;
    std::unique_ptr<Stmt> foldConstants() override {
        return std::make_unique<Continue>();
    }
};
class Return : public Stmt {
public:
    std::unique_ptr<Expr> returnValue;
    // allow return with or without a value
    Return(std::unique_ptr<Expr> value = nullptr)
        : returnValue(std::move(value)) {}
    std::unique_ptr<Stmt> foldConstants() override {
        auto foldedValue = returnValue ? returnValue->foldConstants() : nullptr;
        return std::make_unique<Return>(foldedValue ? std::move(foldedValue) : (returnValue ? std::move(returnValue) : nullptr));
    }
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

    std::unique_ptr<FuncDef> foldConstants(){
        auto foldedBody = body ? body->foldConstants() : nullptr;
        return std::make_unique<FuncDef>(name, rtype, args, foldedBody ? std::move(foldedBody) : (body ? std::move(body) : nullptr));
    }
};
class Program {
public:
    std::vector<std::unique_ptr<FuncDef>> functions;

    Program() = default;
    Program(std::vector<std::unique_ptr<FuncDef>> f) 
        : functions(std::move(f)) {}

    std::unique_ptr<Program> foldConstants() const {
        std::vector<std::unique_ptr<FuncDef>> foldedFuncs;
        for (const auto& func : functions) {
            foldedFuncs.push_back(func->foldConstants());
        }
        return std::make_unique<Program>(std::move(foldedFuncs));
    }
};