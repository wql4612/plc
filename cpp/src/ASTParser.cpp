#include <set>
#include "ASTNode.h"
#include "ASTParser.h"
// 辅助函数：递归收集表达式用到的变量名
static std::set<std::string> getUsedVars(const Expr* expr) {
    std::set<std::string> result;
    if (!expr) return result;
    if (auto v = dynamic_cast<const Var*>(expr)) {
        result.insert(v->name);
    } else if (auto bin = dynamic_cast<const BinOpExpr*>(expr)) {
        auto left = getUsedVars(bin->left.get());
        auto right = getUsedVars(bin->right.get());
        result.insert(left.begin(), left.end());
        result.insert(right.begin(), right.end());
    } else if (auto un = dynamic_cast<const UnOpExpr*>(expr)) {
        auto right = getUsedVars(un->right.get());
        result.insert(right.begin(), right.end());
    } else if (auto call = dynamic_cast<const Call*>(expr)) {
        for (const auto& arg : call->args) {
            auto argVars = getUsedVars(arg.get());
            result.insert(argVars.begin(), argVars.end());
        }
    }
    // IntLit等其它类型不处理
    return result;
}

// 活跃变量分析主入口（递归分析语句）
static void analyzeLiveVariables(Stmt* stmt, std::set<std::string> liveOut) {
    if (!stmt) return;
    // Block
    if (auto block = dynamic_cast<Block*>(stmt)) {
        block->liveOut = std::vector<std::string>(liveOut.begin(), liveOut.end());
        std::set<std::string> currLive = liveOut;
        for (int i = (int)block->stmts.size() - 1; i >= 0; --i) {
            analyzeLiveVariables(block->stmts[i].get(), currLive);
            currLive = std::set<std::string>(block->stmts[i]->liveVars.begin(), block->stmts[i]->liveVars.end());
        }
        block->liveIn = std::vector<std::string>(currLive.begin(), currLive.end());
        block->liveVars = block->liveIn;
        return;
    }
    // Assign
    if (auto assign = dynamic_cast<Assign*>(stmt)) {
        std::set<std::string> used = getUsedVars(assign->value.get());
        std::set<std::string> live = liveOut;
        live.insert(used.begin(), used.end());
        live.erase(assign->name);
        assign->liveVars = std::vector<std::string>(live.begin(), live.end());
        return;
    }
    // Decl
    if (auto decl = dynamic_cast<Decl*>(stmt)) {
        std::set<std::string> used = getUsedVars(decl->value.get());
        std::set<std::string> live = liveOut;
        live.insert(used.begin(), used.end());
        live.erase(decl->name);
        decl->liveVars = std::vector<std::string>(live.begin(), live.end());
        return;
    }
    // If
    if (auto ifstmt = dynamic_cast<If*>(stmt)) {
        analyzeLiveVariables(ifstmt->thenBody.get(), liveOut);
        if (ifstmt->elseBody)
            analyzeLiveVariables(ifstmt->elseBody.get(), liveOut);
        std::set<std::string> condUsed = getUsedVars(ifstmt->condition.get());
        std::set<std::string> liveIn = condUsed;
        liveIn.insert(ifstmt->thenBody->liveVars.begin(), ifstmt->thenBody->liveVars.end());
        if (ifstmt->elseBody)
            liveIn.insert(ifstmt->elseBody->liveVars.begin(), ifstmt->elseBody->liveVars.end());
        ifstmt->liveIn = std::vector<std::string>(liveIn.begin(), liveIn.end());
        ifstmt->liveOut = std::vector<std::string>(liveOut.begin(), liveOut.end());
        ifstmt->liveVars = ifstmt->liveIn;
        return;
    }
    // While
    if (auto whilestmt = dynamic_cast<While*>(stmt)) {
        analyzeLiveVariables(whilestmt->body.get(), liveOut);
        std::set<std::string> condUsed = getUsedVars(whilestmt->condition.get());
        std::set<std::string> liveIn = condUsed;
        liveIn.insert(whilestmt->body->liveVars.begin(), whilestmt->body->liveVars.end());
        whilestmt->liveIn = std::vector<std::string>(liveIn.begin(), liveIn.end());
        whilestmt->liveOut = std::vector<std::string>(liveOut.begin(), liveOut.end());
        whilestmt->liveVars = whilestmt->liveIn;
        return;
    }
    // Return、ExprStmt、Break、Continue等
    std::set<std::string> used;
    if (auto ret = dynamic_cast<Return*>(stmt)) {
        used = getUsedVars(ret->returnValue.get());
    } else if (auto exprstmt = dynamic_cast<ExprStmt*>(stmt)) {
        used = getUsedVars(exprstmt->expr.get());
    }
    std::set<std::string> live = liveOut;
    live.insert(used.begin(), used.end());
    stmt->liveVars = std::vector<std::string>(live.begin(), live.end());
}

// Helper method
void ASTParser::skipWhitespace(){
    while (currentPos < currentLine.size() && isspace(currentLine[currentPos])) {
        currentPos++;
    }
}
void ASTParser::skipToNextLine() {
    while (currentPos < currentLine.size() && currentLine[currentPos] != '\n') {
        currentPos++;
    }
    if (currentPos < currentLine.size()) {
        currentPos++; // Skip the newline character
    }
    currentLineNumber++;
    if (std::getline(*inputStream, currentLine)) {
        currentPos = 0; // Reset position for the new line
    } else {
        currentLine.clear(); // End of file
    }
}

std::string ASTParser::readKeyword() {
    skipWhitespace();
    size_t start = currentPos;
    while (currentPos < currentLine.size() && (isalnum(currentLine[currentPos])) ){
        currentPos++;
    }
    if (start == currentPos) {
        error("Expected keyword");
    }
    std::string keyword = currentLine.substr(start, currentPos - start);
    return keyword;
}
std::string ASTParser::readIdentifier() {
    skipWhitespace();
    size_t start = currentPos;
    while (currentPos < currentLine.size() && (isalnum(currentLine[currentPos]) || currentLine[currentPos] == '_')) {
        currentPos++;
    }
    if (start == currentPos) {
        error("Expected identifier");
    }
    return currentLine.substr(start, currentPos - start);
}
int ASTParser::readInteger() {
    skipWhitespace();
    size_t start = currentPos;
    
    // Handle optional sign (+ or -)
    if (currentPos < currentLine.size() && 
        (currentLine[currentPos] == '+' || currentLine[currentPos] == '-')) {
        currentPos++;
    }
    
    // Read digits
    size_t digitStart = currentPos;
    while (currentPos < currentLine.size() && isdigit(currentLine[currentPos])) {
        currentPos++;
    }
    
    // Check if we found any digits
    if (digitStart == currentPos) {
        error("Expected integer literal");
    }
    
    // Extract the full number string (including sign if present)
    std::string intStr = currentLine.substr(start, currentPos - start);
    
    try {
        return std::stoi(intStr);
    } catch (const std::invalid_argument&) {    
        error("Invalid integer literal: " + intStr);
    } catch (const std::out_of_range&) {
        error("Integer literal out of range: " + intStr);
    }
    return 0; // This line will never be reached due to error handling
}
std::string ASTParser::readSymbol() {
    skipWhitespace();
    // ( ) [ ] , ; + - * / % < > ! & |
    // += -= *= /= %= <= >= == != && ||
    if (currentPos >= currentLine.size()) {
        error("Expected symbol but reached end of line");
    }
    char c1 = currentLine[currentPos];
    char c2 = (currentPos + 1 < currentLine.size()) ? currentLine[currentPos + 1] : '\0';
    if(c1 == '(' || c1 == ')' || c1 == '[' || c1 == ']' || c1 == ',' || c1 == ';' ) 
    {
        currentPos++; // Skip the single character
        return std::string(1, c1);
    }else if(c1 == '+' || c1 == '-' || c1 == '*' || c1 == '/' || c1 == '%' || c1 == '<' || 
       c1 == '>' || c1 == '=' || c1 == '!')
    {
        if(c2 == '=' ) {
            // Handle two-character symbols
            std::string symbol = std::string(1, c1) + c2;
            currentPos += 2; // Skip both characters
            return symbol;
        } else {
            // Single character symbol
            currentPos++; // Skip the single character
            return std::string(1, c1);
        }
    }else if(c1 == '&'){
        if(c2 == '&'){
            currentPos += 2; // Skip both characters
            return "&&";
        }else{
            currentPos ++;
            return "&";
        }
    }else if(c1 == '|'){
        if(c2 == '|'){
            currentPos += 2; // Skip both characters
            return "||";
        }else{
            currentPos ++;
            return "|";
        }
    }else{
        error("Unknown symbol: " + std::string(1, c1));
    }
    return ""; // This line will never be reached due to error handling
}
bool ASTParser::hasMoreTokens() {
    skipWhitespace();
    return currentPos < currentLine.size();
}
bool ASTParser::isAtEndOfLine() {
    skipWhitespace();
    return currentPos >= currentLine.size() || currentLine[currentPos] == '\n';
}
std::unique_ptr<Program> ASTParser::parseProgram() {
    std::vector<std::unique_ptr<FuncDef>> functions;
    while (!inputStream->eof()) {
        if (isAtEndOfLine() || currentLine.empty() || 
            (currentLine.length() > 0 && currentLine[0] == '/')) {
            skipToNextLine();
            continue;
        }
        skipWhitespace();
        if (matchKeyword("Function")) {
            std::unique_ptr<FuncDef> funcDef = parseFunction();
            if (funcDef) {
                // 对每个函数体做活跃变量分析
                if (funcDef->body) {
                    std::set<std::string> liveOut; // 函数出口活跃变量为空
                    analyzeLiveVariables(funcDef->body.get(), liveOut);
                }
                functions.push_back(std::move(funcDef));
            }
            continue;
        } else {
            if (inputStream->eof()) {
                break;
            }
            skipToNextLine();
        }
    }
    auto program = std::make_unique<Program>();
    program->functions = std::move(functions);
    return program;
}
std::unique_ptr<FuncDef> ASTParser::parseFunction() {
    try {
        skipWhitespace();
        // Function name
        std::string funcName = readIdentifier();
        skipWhitespace();
        expectSymbol("(");
        skipWhitespace();
        expectKeyword("returns");
        skipWhitespace();
        RetType retType = parseReturnType(readKeyword());
        skipWhitespace();
        expectSymbol(")");
        skipToNextLine();
        // Parse parameters
        expectKeyword("Parameters");
        std::vector<std::string> params = parseParameters();
        // Parse body
        expectKeyword("Body");
        skipToNextLine();
        std::unique_ptr<Stmt> body = parseStatement();
        if (!body) {
            error("Function body cannot be empty");
        }
        return std::make_unique<FuncDef>(funcName, retType, std::move(params), std::move(body));
    } catch (const std::exception& e) {
        throw;
    }
}
RetType ASTParser::parseReturnType(const std::string& typeStr) {
    if (typeStr == "int") {
        return RetType::Int;
    } else if (typeStr == "void") {
        return RetType::Void;
    } else {
        error("Unknown return type: " + typeStr);
    }
    return RetType::Void; // This line will never be reached due to error handling
}
std::vector<std::string> ASTParser::parseParameters() {
    try {
        skipWhitespace();
        std::vector<std::string> params;
        
        if (matchSymbol("[")) {
            skipWhitespace();
            // Handle empty parameter list
            if (matchSymbol("]")) {
                skipToNextLine();
                return params;
            }
            
            // Parse parameter list
            while (hasMoreTokens()) {
                skipWhitespace();
                std::string param = readIdentifier();
                params.push_back(param);
                skipWhitespace();
                
                if (matchSymbol(";")) {
                    skipWhitespace();
                    continue;
                } else if (matchSymbol("]")) {
                    break;
                } else {
                    error("Expected ';' or ']' in parameter list");
                }
            }
            // Always skip to next line after parsing parameters
            skipToNextLine();
        } else {
            error("Expected '[' to start parameter list");
        }
        return params;
    } catch (const std::exception& e) {
        throw;
    }
}
std::unique_ptr<Stmt> ASTParser::parseStatement() {
    skipWhitespace();
    std::string keyword = readKeyword();
    
    try {
        if (keyword == "Block") {
            return parseBlock();
        } else if (keyword == "Decl") {
            return parseDecl();
        } else if (keyword == "Assign") {
            return parseAssign();
        } else if (keyword == "If") {
            return parseIf();
        } else if (keyword == "While") {
            return parseWhile();
        } else if (keyword == "Return") {
            return parseReturn();
        } else if (keyword == "Break") {
            return parseBreak();
        } else if (keyword == "Continue") {
            return parseContinue();
        } else if (keyword == "ExprStmt") {
            return parseExprStmt();
        } else if (keyword == "EmptyStmt") {
            return parseEmptyStmt();
        } else {
            error("Unknown statement type: " + keyword);
        }
    } catch (const std::exception& e) {
        throw;
    }
    return nullptr; // This line will never be reached due to error handling
}
std::unique_ptr<Block> ASTParser::parseBlock() {
    try {
        // Get the indent level of the Block: line itself
        int blockIndent = getCurrentIndentLevel();
        
        skipToNextLine();
        
        std::vector<std::unique_ptr<Stmt>> stmts;
        
        while (!inputStream->eof() && !currentLine.empty()) {
            if (isAtEndOfLine() || currentLine.empty()) {
                skipToNextLine();
                continue;
            }
            
            int currentIndent = getCurrentIndentLevel();
            if (currentIndent <= blockIndent) {
                break; // End of block
            }
            
            int lineBeforeParsing = currentLineNumber;
            std::unique_ptr<Stmt> stmt = parseStatement();
            if (stmt) {
                stmts.push_back(std::move(stmt));
            }
            
            if (currentLineNumber == lineBeforeParsing) {
                skipToNextLine();
            }
        }
        
        return std::make_unique<Block>(std::move(stmts));
    } catch (const std::exception& e) {
        throw;
    }
}

// Parse individual statement types
std::unique_ptr<Decl> ASTParser::parseDecl() {
    try {
        expectSymbol("(");
        skipWhitespace();
        std::string varName = readIdentifier();
        skipWhitespace();
        expectSymbol(")");
        skipToNextLine();
        
        // Parse the initialization expression
        std::unique_ptr<Expr> initExpr = parseExpression();
        if (!initExpr) {
            error("Expected initialization expression for variable declaration");
        }
        return std::make_unique<Decl>(varName, std::move(initExpr));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<Assign> ASTParser::parseAssign() {
    try {
        expectSymbol("(");
        skipWhitespace();
        std::string varName = readIdentifier();
        skipWhitespace();
        expectSymbol(")");
        skipToNextLine();
        
        std::unique_ptr<Expr> value = parseExpression();
        return std::make_unique<Assign>(varName, std::move(value));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<If> ASTParser::parseIf() {
    try {
        skipToNextLine();
        if(matchKeyword("Condition")){
            skipToNextLine();
        }else{
            error("Expected 'Condition' keyword in If statement");
        }
        // Parse condition
        std::unique_ptr<Expr> condition = parseExpression();
        
        if(matchKeyword("Then")){
            skipToNextLine();
        }else{
            skipToNextLine();
            expectKeyword("Then");
            skipToNextLine();
        }
        // Parse then branch
        std::unique_ptr<Stmt> thenBranch = parseStatement();
        
        // Check for else branch (optional)
        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (matchKeyword("Else")) {
            skipToNextLine();
            elseBranch = parseStatement();
        }
        
        return std::make_unique<If>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<While> ASTParser::parseWhile() {
    try {
        skipToNextLine();
        
        // Parse "Condition" keyword
        if(matchKeyword("Condition")){
            skipToNextLine();
        }else{
            error("Expected 'Condition' keyword in While statement");
        }
        
        // Parse condition expression
        std::unique_ptr<Expr> condition = parseExpression();
        skipToNextLine();
        
        // Parse "Body" keyword
        if(matchKeyword("Body")){
            skipToNextLine();
        }else{
            error("Expected 'Body' keyword in While statement");
        }
        
        // Parse body
        std::unique_ptr<Stmt> body = parseStatement();
        
        return std::make_unique<While>(std::move(condition), std::move(body));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<Return> ASTParser::parseReturn() {
    try {
        skipToNextLine();
        
        // Parse return value (optional)
        std::unique_ptr<Expr> returnValue = nullptr;
        if (!isAtEndOfLine()) {
            returnValue = parseExpression();
        }
        
        return std::make_unique<Return>(std::move(returnValue));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<Break> ASTParser::parseBreak() {
    return std::make_unique<Break>();
}

std::unique_ptr<Continue> ASTParser::parseContinue() {
    return std::make_unique<Continue>();
}

std::unique_ptr<ExprStmt> ASTParser::parseExprStmt() {
    try {
        skipToNextLine();
        
        std::unique_ptr<Expr> expr = parseExpression();
        return std::make_unique<ExprStmt>(std::move(expr));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<EmptyStmt> ASTParser::parseEmptyStmt() {
    return std::make_unique<EmptyStmt>();
}

// Expression parsing methods
std::unique_ptr<Expr> ASTParser::parseExpression() {
    skipWhitespace();
    std::string Keyword = readKeyword();
    
    try {
        if (Keyword == "IntLit") {
            return parseIntLit();
        } else if (Keyword == "Var") {
            return parseVar();
        } else if (Keyword == "Call") {
            return parseCall();
        } else if (Keyword == "Binop") {
            return parseBinOpExpr();
        } else if (Keyword == "Unop") {
            return parseUnOpExpr();
        } else {
            error("Unknown expression type: " + Keyword);
        }
    } catch (const std::exception& e) {
        throw;
    }
    return nullptr;
}

std::unique_ptr<IntLit> ASTParser::parseIntLit() {
    try {
        expectSymbol("(");
        skipWhitespace();
        int value = readInteger();
        skipWhitespace();
        expectSymbol(")");
        return std::make_unique<IntLit>(value);
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<Var> ASTParser::parseVar() {
    try {
        expectSymbol("(");
        skipWhitespace();
        std::string name = readIdentifier();
        skipWhitespace();
        expectSymbol(")");
        return std::make_unique<Var>(name);
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<Call> ASTParser::parseCall() {
    try {
        expectSymbol("(");
        skipWhitespace();
        std::string funcName = readIdentifier();
        skipWhitespace();
        expectSymbol(")");
        skipToNextLine();
        
        std::vector<std::unique_ptr<Expr>> args;
        int argIndex = 0;
        // int baseIndent = getCurrentIndentLevel();
        while (!inputStream->eof()) {
            if (isAtEndOfLine() || currentLine.empty()) {
                skipToNextLine();
                continue;
            }
            // Look for Arg[n]: pattern
            std::string expectedArg = "Arg[" + std::to_string(argIndex) + "]";
            if (matchKeyword(expectedArg)) {
                skipToNextLine();
                std::unique_ptr<Expr> arg = parseExpression();
                if (arg) {
                    args.push_back(std::move(arg));
                }
                argIndex++;
            } else {
                break;
            }
        }
        return std::make_unique<Call>(funcName, std::move(args));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<BinOpExpr> ASTParser::parseBinOpExpr() {
    try {
        skipToNextLine();
        // Parse "Operator: <op>" line
        expectKeyword("Operator");
        skipWhitespace();
        std::string opStr = readSymbol();
        skipWhitespace();
        BinOp op = parseBinOperatorSymbol(opStr);
        skipToNextLine();
        
        // Parse "Left" section
        expectKeyword("Left");
        skipToNextLine();
        std::unique_ptr<Expr> left = parseExpression();
        // For parseCall will skip to next line automatically
        // we need to check if there is keyword right
        if(matchKeyword("Right")){
            skipToNextLine();
        }else{
            skipToNextLine();
            expectKeyword("Right");
            skipToNextLine();
        }
        std::unique_ptr<Expr> right = parseExpression();
        
        return std::make_unique<BinOpExpr>(std::move(left), op, std::move(right));
    } catch (const std::exception& e) {
        throw;
    }
}

std::unique_ptr<UnOpExpr> ASTParser::parseUnOpExpr() {
    try {
        // Parse "Unop(<op>)" format
        expectSymbol("(");
        skipWhitespace();
        std::string opStr = readSymbol();
        skipWhitespace();
        expectSymbol(")");
        UnOp op = parseUnOperatorSymbol(opStr);
        skipToNextLine();
        
        // Parse operand
        std::unique_ptr<Expr> operand = parseExpression();
        
        return std::make_unique<UnOpExpr>(op, std::move(operand));
    } catch (const std::exception& e) {
        throw;
    }
}

BinOp ASTParser::parseBinOperator(const std::string& opStr) {
    if (opStr == "Add") return BinOp::Add;
    else if (opStr == "Sub") return BinOp::Sub;
    else if (opStr == "Mul") return BinOp::Mul;
    else if (opStr == "Div") return BinOp::Div;
    else if (opStr == "Mod") return BinOp::Mod;
    else if (opStr == "Lt") return BinOp::Lt;
    else if (opStr == "Gt") return BinOp::Gt;
    else if (opStr == "Le") return BinOp::Le;
    else if (opStr == "Ge") return BinOp::Ge;
    else if (opStr == "Eq") return BinOp::Eq;
    else if (opStr == "Ne") return BinOp::Ne;
    else if (opStr == "And") return BinOp::And;
    else if (opStr == "Or") return BinOp::Or;
    else {
        error("Unknown binary operator: " + opStr);
        return BinOp::Add; // Never reached
    }
}

BinOp ASTParser::parseBinOperatorSymbol(const std::string& opStr) {
    if (opStr == "+") return BinOp::Add;
    else if (opStr == "-") return BinOp::Sub;
    else if (opStr == "*") return BinOp::Mul;
    else if (opStr == "/") return BinOp::Div;
    else if (opStr == "%") return BinOp::Mod;
    else if (opStr == "<") return BinOp::Lt;
    else if (opStr == ">") return BinOp::Gt;
    else if (opStr == "<=") return BinOp::Le;
    else if (opStr == ">=") return BinOp::Ge;
    else if (opStr == "==") return BinOp::Eq;
    else if (opStr == "!=") return BinOp::Ne;
    else if (opStr == "&&") return BinOp::And;
    else if (opStr == "||") return BinOp::Or;
    else {
        error("Unknown binary operator symbol: " + opStr);
        return BinOp::Add; // Never reached
    }
}

UnOp ASTParser::parseUnOperator(const std::string& opStr) {
    if (opStr == "Neg") return UnOp::Neg;
    else if (opStr == "Not") return UnOp::Not;
    else {
        error("Unknown unary operator: " + opStr);
        return UnOp::Neg; // Never reached
    }
}

UnOp ASTParser::parseUnOperatorSymbol(const std::string& opStr) {
    if (opStr == "-") return UnOp::Neg;
    else if (opStr == "!") return UnOp::Not;
    else {
        error("Unknown unary operator symbol: " + opStr);
        return UnOp::Neg; // Never reached
    }
}

// Utility methods implementation
void ASTParser::expectSymbol(const std::string& symbol) {
    if (!matchSymbol(symbol)) {
        error("Expected symbol '" + symbol + "'");
    }
}
void ASTParser::expectKeyword(const std::string& keyword) {
    if (!matchKeyword(keyword)) {
        error("Expected keyword '" + keyword + "'");
    }
}
bool ASTParser::matchSymbol(const std::string& symbol) {
    skipWhitespace();
    if (currentPos + symbol.length() > currentLine.size()) {
        return false;
    }
    std::string found = currentLine.substr(currentPos, symbol.length());
    if (found == symbol) {
        currentPos += symbol.length();
        return true;
    }
    return false;
}
bool ASTParser::matchKeyword(const std::string& keyword) {
    skipWhitespace();
    
    // Check if we have enough characters
    if (currentPos + keyword.length() > currentLine.size()) {
        return false;
    }
    
    // Check if the keyword matches
    std::string found = currentLine.substr(currentPos, keyword.length());
    if (found == keyword) {
        currentPos += keyword.length();
        return true;
    } else {
        return false;
    }
}

int ASTParser::getCurrentIndentLevel() {
    int indent = 0;
    for (size_t i = 0; i < currentLine.size() && isspace(currentLine[i]); i++) {
        if (currentLine[i] == ' ') {
            indent++;
        } else if (currentLine[i] == '\t') {
            indent += 4; // Assume tab = 4 spaces
        }
    }
    return indent;
}

void ASTParser::error(const std::string& message) {
    throw std::runtime_error("Parse error at line " + std::to_string(currentLineNumber) + ": " + message);
}

// Constructor and main parsing methods
ASTParser::ASTParser(const std::string& filename){
    // Initialize counters
    currentPos = 0;
    currentLineNumber = 1;
    ownsStream = true;
    
    // Open the input file
    std::ifstream* file = new std::ifstream(filename);
    if (!file->is_open()) {
        delete file;
        throw std::runtime_error("Failed to open file: " + filename);
    }
    inputStream = file;
    
    // Read first line
    if (std::getline(*inputStream, currentLine)) {
        currentPos = 0;
    }
}

ASTParser::ASTParser(std::istream& input) {
    // Initialize counters
    currentPos = 0;
    currentLineNumber = 1;
    ownsStream = false;
    inputStream = &input;
    
    // Read first line
    if (std::getline(*inputStream, currentLine)) {
        currentPos = 0;
    }
}

ASTParser::~ASTParser() {
    if (ownsStream && inputStream) {
        std::ifstream* file = static_cast<std::ifstream*>(inputStream);
        if (file->is_open()) {
            file->close();
        }
        delete file;
    }
}

std::unique_ptr<Program> ASTParser::parse() {
    return parseProgram();
}

bool ASTParser::isOpen() const {
    return inputStream != nullptr && inputStream->good();
}