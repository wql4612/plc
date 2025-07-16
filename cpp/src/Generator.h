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

class Generator {
private:
    std::ostream& output; // Output stream for generated code
    int labelCount; // Counter for unique labels
    RegManager regManager; // Register manager for temporary registers
    
    struct FunctionContext {
        std::string name;
        int stackSize = 0;
        int partVarCount = 0; // Number of variables in the current function
        std::vector<std::map<std::string, int>> scopeStack; // Stack of scopes, each scope maps variable names to offsets
        int loopDepth = 0; // Depth of nested loops used by break and continue
        std::vector<std::string> loopEndLabels; // Stack of loop end labels for break
        std::vector<std::string> loopStartLabels; // Stack of loop start labels for continue
        
        // Helper methods for scope management
        void pushScope() {
            scopeStack.push_back(std::map<std::string, int>());
        }
        
        void popScope() {
            if (!scopeStack.empty()) {
                scopeStack.pop_back();
            }
        }
        
        // Find variable in current scope chain (from innermost to outermost)
        int findVar(const std::string& name) {
            for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) {
                    return found->second;
                }
            }
            return -1; // Not found
        }
        
        // Add variable to current scope
        void addVar(const std::string& name, int offset) {
            if (!scopeStack.empty()) {
                scopeStack.back()[name] = offset;
            }
        }
    };
    std::stack<FunctionContext> contextStack; // Stack of contexts
    
public:
    // Constructor
    Generator(std::ostream& out);
    std::string uniqueLabel(const std::string& prefix);
    int allocateVar(FunctionContext& ctx, const std::string& name = "");
    void generateExpr(const Expr& expr, FunctionContext& ctx, const std::string& destReg = "a0");
    void generateStmt(const Stmt& stmt, FunctionContext& ctx);
    void generateFunc(const FuncDef& func);
    void generateProg(Program& program);
};