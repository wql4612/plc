#pragma once
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>
#include <cctype>
#include <iostream>
#include "ASTNode.h"

class ASTParser {
private:
    std::istream* inputStream;
    bool ownsStream;
    std::ofstream logFile;
    std::string currentLine;
    size_t currentPos;
    int currentLineNumber;
    
    // Helper methods for parsing
    void skipWhitespace();
    void skipToNextLine();
    std::string readKeyword();
    std::string readIdentifier();
    std::string readSymbol();
    int readInteger();
    bool hasMoreTokens();
    bool isAtEndOfLine();
    
    // Parse different node types
    std::unique_ptr<Program> parseProgram();
    std::unique_ptr<FuncDef> parseFunction();
    RetType parseReturnType(const std::string& typeStr);
    std::vector<std::string> parseParameters();
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<Block> parseBlock();
    std::unique_ptr<Decl> parseDecl();
    std::unique_ptr<Assign> parseAssign();
    std::unique_ptr<If> parseIf();
    std::unique_ptr<While> parseWhile();
    std::unique_ptr<Return> parseReturn();
    std::unique_ptr<Break> parseBreak();
    std::unique_ptr<Continue> parseContinue();
    std::unique_ptr<ExprStmt> parseExprStmt();
    std::unique_ptr<EmptyStmt> parseEmptyStmt();
    
    // Parse expressions
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<IntLit> parseIntLit();
    std::unique_ptr<Var> parseVar();
    std::unique_ptr<Call> parseCall();
    std::unique_ptr<BinOpExpr> parseBinOpExpr();
    std::unique_ptr<UnOpExpr> parseUnOpExpr();
    
    // Parse operators
    BinOp parseBinOperator(const std::string& opStr);
    BinOp parseBinOperatorSymbol(const std::string& opStr);
    UnOp parseUnOperator(const std::string& opStr);
    UnOp parseUnOperatorSymbol(const std::string& opStr);
    
    // Utility methods
    void expectSymbol(const std::string& symbol);
    void expectKeyword(const std::string& keyword);
    bool matchSymbol(const std::string& symbol);
    bool matchKeyword(const std::string& keyword);
    int getCurrentIndentLevel();
    void error(const std::string& message);
    
public:
    // Constructors
    ASTParser(const std::string& filename);
    ASTParser(std::istream& inputStream);
    
    // Destructor
    ~ASTParser();
    
    // Main parsing method
    std::unique_ptr<Program> parse();
    
    // Check if stream is ready
    bool isOpen() const;
};