#include "Generator.h"
static int globalLabelCount = 0;
Generator::Generator(std::ostream &out) : output(out), regManager(){}
std::string Generator::uniqueLabel(const std::string &prefix)
{
    return prefix + std::to_string(globalLabelCount++);
}
auto Generator::allocWithSpill(RegType type, Stmt *stmt, FunctionContext &ctx) {
    static std::string lastSpilledReg;
    try{
        std::string reg = regManager.alloc(type);
        return reg;
    } catch (const std::runtime_error &e) {
        std::vector<std::string> usedRegs = regManager.getUsedRegisters();
        std::set<std::string> liveVars;
        if(stmt && !stmt->liveVars.empty()){
            liveVars.insert(stmt->liveVars.begin(), stmt->liveVars.end());
        }
        std::string spillReg;
        for (const auto &reg : usedRegs) {
            if(regManager.getRegType(reg) == type && liveVars.find(reg) == liveVars.end() && reg != lastSpilledReg) {
                spillReg = reg;
                break;
            }
        }
        if(spillReg.empty()) {
            for(const auto &reg : usedRegs) {
                if(regManager.getRegType(reg) == type && reg != lastSpilledReg) {
                    spillReg = reg;
                    break;
                }
            }
        }
        if(spillReg.empty()) {
            for(const auto &reg : usedRegs) {
                if(regManager.getRegType(reg) == type) {
                    spillReg = reg;
                    break;
                }
            }
        }
        if(spillReg.empty()) {
            throw std::runtime_error("No available register for spilling");
        }
        int offset;
        offset = ctx.stackSize; 
        ctx.stackSize += 4; 
        output<< "sw " << spillReg << ", " << offset << "(sp)\n"; // Store register value to stack
        regManager.spill(spillReg, offset); // Mark register as spilled
        regManager.release(spillReg); // Release the register
        lastSpilledReg = spillReg;
        std::string newReg = regManager.alloc(type);
        return newReg; // Allocate a new register
    }

}
// Allocate a variable in the current function context
int Generator::allocateVar(FunctionContext &ctx, const std::string &name)
{
    (void)name;
    int offset = ctx.stackSize;
    ctx.stackSize += 4;
    return offset;
}
void Generator::generateExpr(const Expr &expr, FunctionContext &ctx, const std::string &destReg)
{
    generateExprWithOffset(expr, ctx, destReg, 0);
}

// 新增：带sp偏移的表达式生成
void Generator::generateExprWithOffset(const Expr &expr, FunctionContext &ctx, const std::string &destReg, int extraSpOffset)
{
    if (const auto intLit = dynamic_cast<const IntLit *>(&expr))
    {
        output << "li " << destReg << "," << intLit->value << "\n";
    }
    else if (const auto *var = dynamic_cast<const Var *>(&expr))
    {
        int offset = ctx.findVar(var->name);
        if (offset == -1)
        {
            throw std::runtime_error("Variable " + var->name + " not found in context");
        }
        output << "lw " << destReg << ", " << (offset + extraSpOffset) << "(sp)\n";
    }
    else if (const auto *binop = dynamic_cast<const BinOpExpr *>(&expr))
    {
        RegType regType = (binop->op == BinOp::And || binop->op == BinOp::Or) ? RegType::SAVE : RegType::TEMP;
        std::string leftReg = allocWithSpill(regType,nullptr, ctx);
        generateExprWithOffset(*binop->left, ctx, leftReg, extraSpOffset);
        // short circuit evaluation
        if (binop->op == BinOp::And)
        {
            std::string falseLabel = uniqueLabel("and_false_");
            std::string endLabel = uniqueLabel("and_end_");
            output << "beqz " << leftReg << ", " << falseLabel << "\n";
            std::string rightReg = allocWithSpill(regType,nullptr, ctx);
            generateExprWithOffset(*binop->right, ctx, rightReg, extraSpOffset);
            output << "mv " << leftReg << ", " << rightReg << "\n";
            regManager.release(rightReg);
            output << "j " << endLabel << "\n";
            output << falseLabel << ":\n";
            output << "li " << leftReg << ", 0\n";
            output << endLabel << ":\n";
            output << "mv " << destReg << ", " << leftReg << "\n";
            regManager.release(leftReg);
            return;
        }
        std::string rightReg = allocWithSpill(regType,nullptr, ctx);
        generateExprWithOffset(*binop->right, ctx, rightReg, extraSpOffset);
        switch (binop->op)
        {
        case BinOp::Add:
            output << "add " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Sub:
            output << "sub " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Mul:
            output << "mul " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Div:
            output << "div " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Mod:
            output << "rem " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Lt:
            output << "slt " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Gt:
            output << "slt " << destReg << ", " << rightReg << ", " << leftReg << "\n";
            break;
        case BinOp::Le:
            output << "slt " << destReg << ", " << rightReg << ", " << leftReg << "\n";
            output << "xori " << destReg << ", " << destReg << ", 1\n";
            break;
        case BinOp::Ge:
            output << "slt " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            output << "xori " << destReg << ", " << destReg << ", 1\n";
            break;
        case BinOp::Eq:
            output << "sub " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            output << "seqz " << destReg << ", " << destReg << "\n";
            break;
        case BinOp::Ne:
            output << "sub " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            output << "snez " << destReg << ", " << destReg << "\n";
            break;
        case BinOp::And:
            output << "and " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        case BinOp::Or:
            output << "or " << destReg << ", " << leftReg << ", " << rightReg << "\n";
            break;
        default:
            throw std::runtime_error("Unknown binary operator");
        }
        if (!regManager.isSpilled(leftReg)) regManager.release(leftReg);
        if (!regManager.isSpilled(rightReg)) regManager.release(rightReg);
    }
    else if (const auto *unop = dynamic_cast<const UnOpExpr *>(&expr))
    {
        std::string tempReg = allocWithSpill(RegType::TEMP, nullptr, ctx);
        generateExprWithOffset(*unop->right, ctx, tempReg, extraSpOffset);
        switch (unop->op)
        {
        case UnOp::Neg:
            output << "neg " << destReg << ", " << tempReg << "\n";
            break;
        case UnOp::Not:
            output << "seqz " << destReg << ", " << tempReg << "\n";
            break;
        default:
            throw std::runtime_error("Unknown unary operator");
        }
        if (!regManager.isSpilled(tempReg)) regManager.release(tempReg);
    }
    else if (const auto *call = dynamic_cast<const Call *>(&expr))
    {
        // 1. 保存所有 caller-saved 寄存器
        std::vector<std::string> callerSavedRegs = {
            "t0", "t1", "t2", "t3", "t4", "t5", "t6",
            "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7"
        };
        int saveCount = 0;
        std::vector<std::string> actuallySaved;
        for (const auto &reg : callerSavedRegs) {
            if (regManager.isRegInUse(reg)) {
                saveCount++;
                actuallySaved.push_back(reg);
            }
        }
        if (saveCount > 0) {
            output << "addi sp, sp, -" << (saveCount * 4) << "\n";
            int offset = 0;
            for (const auto &reg : actuallySaved) {
                output << "sw " << reg << ", " << offset << "(sp)\n";
                offset += 4;
            }
        }

        // 2. 先为参数区分配空间
        size_t argCount = call->args.size();
        int argAreaSize = argCount * 4;
        if (argAreaSize > 0) {
            output << "addi sp, sp, -" << argAreaSize << "\n";
        }
        // 3. 依次求值每个参数表达式，计算后立即入栈并释放寄存器
        for (size_t i = 0; i < argCount; ++i) {
            std::string tempReg = allocWithSpill(RegType::TEMP, nullptr, ctx);
            // 参数表达式求值时，参数区尚未写入任何参数，所以偏移为整个参数区大小+caller-saved保存区
            generateExprWithOffset(*call->args[i], ctx, tempReg, argAreaSize + saveCount * 4);
            int offset = i * 4;
            output << "sw " << tempReg << ", " << offset << "(sp)\n";
            if (!regManager.isSpilled(tempReg)) regManager.release(tempReg);
        }
        // 4. 调用 call 指令
        output << "call " << call->name << "\n";
        // 5. 回收参数区空间
        if (argAreaSize > 0) {
            output << "addi sp, sp, " << argAreaSize << "\n";
        }
        // 6. 恢复 caller-saved 寄存器
        if (saveCount > 0) {
            int offset = 0;
            for (const auto &reg : actuallySaved) {
                output << "lw " << reg << ", " << offset << "(sp)\n";
                offset += 4;
            }
            output << "addi sp, sp, " << (saveCount * 4) << "\n";
        }
        // 7. 返回值处理
        if (destReg != "a0") {
            output << "mv " << destReg << ", a0\n";
        }
    }
    else
    {
        throw std::runtime_error("Unknown expression type");
    }
}
void Generator::generateStmt(const Stmt &stmt, FunctionContext &ctx, int extraSpOffset)
{
    // same if-else chain
    if (auto block = dynamic_cast<const Block *>(&stmt))
    {
        ctx.pushScope(); // Enter new scope
        for (const auto &s : block->stmts)
        {
            generateStmt(*s, ctx, extraSpOffset); // Generate code for each statement in the block
        }
        ctx.popScope(); // Exit scope
    }
    else if (auto empty = dynamic_cast<const EmptyStmt *>(&stmt))
    {
        (void)empty; // avoid unused parameter warning
    }
    else if (auto exprStmt = dynamic_cast<const ExprStmt *>(&stmt))
    {
        std::string tempReg = allocWithSpill(RegType::TEMP, const_cast<Stmt *>(&stmt), ctx);
        generateExprWithOffset(*exprStmt->expr, ctx, tempReg, extraSpOffset); // Generate code for expression statement
        regManager.release(tempReg);                 // Release temporary register
    }
    else if (auto assign = dynamic_cast<const Assign *>(&stmt))
    {
        std::string tempReg = allocWithSpill(RegType::TEMP, const_cast<Stmt *>(&stmt), ctx);
        generateExprWithOffset(*assign->value, ctx, tempReg, extraSpOffset); // Generate code for value expression
        int offset = ctx.findVar(assign->name);
        if (offset == -1)
        {
            throw std::runtime_error("Variable " + assign->name + " not found in context");
        }
        // 使用栈指针偏移：sp + (frameSize - 4 - offset)
        output << "sw " << tempReg << ", " << offset << "(sp)\n";
        regManager.release(tempReg); // Release temporary register
    }
    else if (auto decl = dynamic_cast<const Decl *>(&stmt))
    {
        int offset = allocateVar(ctx, decl->name); // Allocate variable in the current context
        ctx.addVar(decl->name, offset);            // Add to current scope
        if (decl->value)
        {
            std::string tempReg = allocWithSpill(RegType::TEMP, const_cast<Stmt *>(&stmt), ctx);
            generateExprWithOffset(*decl->value, ctx, tempReg, extraSpOffset); // Generate code for initialization value
            output << "sw " << tempReg << ", " << offset << "(sp)\n";
            regManager.release(tempReg); // Release temporary register
        }
    }
    else if (auto ifStmt = dynamic_cast<const If *>(&stmt))
    {
        std::string elseLabel = uniqueLabel("if_else_");
        std::string condReg = allocWithSpill(RegType::TEMP,const_cast<Stmt *>(&stmt), ctx);
        generateExprWithOffset(*ifStmt->condition, ctx, condReg, extraSpOffset);            // Generate code for condition expression
        output << "beqz " << condReg << ", " << elseLabel << "\n"; // If condition is false, jump to else label
        regManager.release(condReg);                               // Release condition register
        generateStmt(*ifStmt->thenBody, ctx, extraSpOffset);                      // Generate code for then body

        if (ifStmt->elseBody)
        {
            std::string endLabel = uniqueLabel("if_end_");
            output << "j " << endLabel << "\n"; // Jump to end label only if there's an else body
            output << elseLabel << ":\n";
            generateStmt(*ifStmt->elseBody, ctx, extraSpOffset); // Generate code for else body
            output << endLabel << ":\n";          // End of if statement
        }
        else
        {
            output << elseLabel << ":\n"; // Just place the else label, no end label needed
        }
    }
    else if (auto whileStmt = dynamic_cast<const While *>(&stmt))
    {
        std::string startLabel = uniqueLabel("while_start_");
        std::string endLabel = uniqueLabel("while_end_");

            // Push labels to stack for break/continue
            ctx.loopDepth++;
            ctx.loopStartLabels.push_back(startLabel);
            ctx.loopEndLabels.push_back(endLabel);

        output << startLabel << ":\n";
        std::string condReg = allocWithSpill(RegType::TEMP,const_cast<Stmt *>(&stmt), ctx);
        generateExprWithOffset(*whileStmt->condition, ctx, condReg, extraSpOffset);
        output << "beqz " << condReg << ", " << endLabel << "\n";
        regManager.release(condReg);          // Release condition register
        generateStmt(*whileStmt->body, ctx, extraSpOffset);  // Generate code for while body
        output << "j " << startLabel << "\n"; // Jump back to start of while loop
        output << endLabel << ":\n";          // End of while loop

        // Pop labels from stack
        ctx.loopDepth--;
        ctx.loopStartLabels.pop_back();
        ctx.loopEndLabels.pop_back();
    }
    else if (auto breakStmt = dynamic_cast<const Break *>(&stmt))
    {
        (void)breakStmt; // avoid unused parameter warning
        if (ctx.loopDepth == 0 || ctx.loopEndLabels.empty())
        {
            throw std::runtime_error("Break statement outside of loop");
        }
        else
        {
            output << "j " << ctx.loopEndLabels.back() << "\n"; // Jump to end of current loop
        }
    }
    else if (auto continueStmt = dynamic_cast<const Continue *>(&stmt))
    {
        (void)continueStmt; // avoid unused parameter warning
        if (ctx.loopDepth == 0 || ctx.loopStartLabels.empty())
        {
            throw std::runtime_error("Continue statement outside of loop");
        }
        else
        {
            output << "j " << ctx.loopStartLabels.back() << "\n"; // Jump to start of current loop
        }
    }
    else if (auto returnStmt = dynamic_cast<const Return *>(&stmt))
    {
        if (returnStmt->returnValue)
        {
            generateExprWithOffset(*returnStmt->returnValue, ctx, "a0", extraSpOffset);
        }
        // 所有 return 语句跳转到统一出口
        output << "j " << ctx.name << "_return\n";
    }
    else
    {
        throw std::runtime_error("Unknown statement type");
    }
}
void Generator::generateFunc(const FuncDef &func)
{
    regManager.reset();
    contextStack.push(FunctionContext());
    auto &context = contextStack.top();
    context.name = func.name;
    // Initialize the function scope
    context.pushScope();
    output << func.name << ":\n";

    // 1. 为每个参数分配栈空间并记录偏移
    std::vector<int> argOffsets(func.args.size());
    for (size_t i = 0; i < func.args.size(); i++) {
        int offset = allocateVar(context, func.args[i]);
        context.addVar(func.args[i], offset);
        argOffsets[i] = offset;
    }

    // 2. 先生成函数体，获得最大栈空间
    std::ostringstream bodyCode;
    Generator tempGenerator(bodyCode);
    tempGenerator.contextStack = contextStack; // Copy context
    tempGenerator.generateStmt(*func.body, tempGenerator.contextStack.top(), 0);
    context.stackSize = tempGenerator.contextStack.top().stackSize;
    int frameSize = 4 + context.stackSize; // ra(4) + local variables

    // 3. 生成序言，分配栈帧
    output << "addi sp, sp, -" << frameSize << "\n";
    output << "sw ra, " << (frameSize - 4) << "(sp)\n";

    // 4. 保存参数到栈，全部从 caller 的参数区(sp+frameSize+i*4)读取
    for (size_t i = 0; i < func.args.size(); i++) {
        int offset = context.findVar(func.args[i]);
        // sp 已减 frameSize，caller 的参数区在 sp+frameSize
        output << "lw t0, " << (frameSize + i * 4) << "(sp)\n";
        output << "sw t0, " << offset << "(sp)\n";
    }

    output << bodyCode.str();
    // 统一出口标签
    output << func.name << "_return:\n";
    output << "lw ra, " << (frameSize - 4) << "(sp)\n";
    output << "addi sp, sp, " << frameSize << "\n";
    output << "ret\n";
    contextStack.pop();
}

void Generator::generateProg(Program &program)
{
    output << ".text\n";
    // output << ".globl _start\n";
    // output << "_start:\n";
    // output << "    call main\n";
    // output << "    mv a0, a0\n";
    // output << "    li a7, 93\n";
    // output << "    ecall\n";
    output << ".globl main\n";
    for (const auto &func : program.functions)
    {
        generateFunc(*func);
    }
}
