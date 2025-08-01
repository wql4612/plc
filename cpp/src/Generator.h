#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stack>
#include <map>
#include <memory>
#include "ASTNode.h"
#include "RegManager.h"

class Generator
{
private:
    std::ostream &output;  // Output stream for generated code
    RegManager regManager; // Register manager for temporary registers
    struct FunctionContext
    {
        std::string name;
        int stackSize = 0;
        int partVarCount = 0;                               // Number of variables in the current function
        std::vector<std::map<std::string, int>> scopeStack; // Stack of scopes, each scope maps variable names to offsets
        std::map<std::string, int> args;                    // 参数映射：参数名 -> 位置索引
        int loopDepth = 0;                                  // Depth of nested loops used by break and continue
        std::vector<std::string> loopEndLabels;             // Stack of loop end labels for break
        std::vector<std::string> loopStartLabels;           // Stack of loop start labels for continue

        std::set<std::string> savedRegisters;
        // 添加参数
        void addArg(const std::string &name, int index)
        {
            args[name] = index;
        }
        // 查找参数
        int findArg(const std::string &name) const
        {
            auto it = args.find(name);
            if (it != args.end())
            {
                return it->second;
            }
            return -1;
        }
        
        // Helper methods for scope management
        void pushScope()
        {
            scopeStack.push_back(std::map<std::string, int>());
        }

        void popScope()
        {
            if (!scopeStack.empty())
            {
                scopeStack.pop_back();
            }
        }

        // Find variable in current scope chain (from innermost to outermost)
        int findVar(const std::string &name)
        {
            for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it)
            {
                auto found = it->find(name);
                if (found != it->end())
                {
                    return found->second;
                }
            }
            return -1; // Not found
        }

        // Add variable to current scope
        void addVar(const std::string &name, int offset)
        {
            if (!scopeStack.empty())
            {
                scopeStack.back()[name] = offset;
            }
        }

        void addSavedReg(const std::string &reg)
        {
            savedRegisters.insert(reg);
        }
    };
    std::stack<FunctionContext> contextStack; // Stack of contexts

public:
    // Constructor
    Generator(std::ostream &out);
    std::string uniqueLabel(const std::string &prefix);
    int allocateVar(FunctionContext &ctx, const std::string &name = "");
    void generateExpr(const Expr &expr, FunctionContext &ctx, const std::string &destReg = "a0");
    void generateExprWithOffset(const Expr &expr, FunctionContext &ctx, const std::string &destReg, int extraSpOffset);
    void generateStmt(const Stmt &stmt, FunctionContext &ctx, int extraSpOffset = 0);
    void generateFunc(const FuncDef &func);
    void generateProg(Program &program);

    auto allocWithSpill(RegType type, Stmt *stmt, FunctionContext &ctx);
};