#include "Generator.h"

Generator::Generator(std::ostream& out) : output(out), labelCount(0), regManager() {
    // RegManager has a default constructor, so it will be properly initialized
}

std::string Generator::uniqueLabel(const std::string &prefix)
{
    return prefix + std::to_string(labelCount++);
}
// Allocate a variable in the current function context
int Generator::allocateVar(FunctionContext &ctx, const std::string &name)
{
    (void)name; // avoid unused parameter warning
    ctx.stackSize += 4;
    return ctx.stackSize;
}
void Generator::generateExpr(const Expr &expr, FunctionContext &ctx, const std::string &destReg)
{
    // saidly in switch we couldn't define new variables
    // so the if-else-if-else chain is used
    if (const auto intLit = dynamic_cast<const IntLit *>(&expr))
    {
        output << "li " << destReg << "," << intLit->value << "\n"; // Load immediate value into a0
    }
    else if (const auto *var = dynamic_cast<const Var *>(&expr))
    {
        int offset = ctx.findVar(var->name);
        if (offset == -1)
        {
            throw std::runtime_error("Variable " + var->name + " not found in context");
        }
        // 使用栈指针偏移：sp + (frameSize - 4 - offset)
        int frameSize = 4 + ctx.stackSize;
        output << "lw " << destReg << ", " << (frameSize - 4 - offset) << "(sp)\n";
    }
    else if (const auto *binop = dynamic_cast<const BinOpExpr *>(&expr))
    {
        std::string leftReg = regManager.alloc();
        generateExpr(*binop->left, ctx, leftReg);
        // short circuit evaluation
        if (binop->op == BinOp::And)
        {
            std::string falseLabel = uniqueLabel("and_false_");
            std::string endLabel = uniqueLabel("and_end_");
            output << "beqz " << leftReg << ", " << falseLabel << "\n"; // If left is false, jump to false label
            generateExpr(*binop->right, ctx, leftReg);      // Generate code for right operand
            output << "j " << endLabel << "\n";          // Jump to end label
            output << falseLabel << ":\n";
            output << "li " << leftReg << ", 0\n"; // Set result to false (0)
            output << endLabel << ":\n";
            output << "mv " << destReg << ", " << leftReg << "\n"; // Move result to destination register
            regManager.release(); 
            return;
        }
        std::string rightReg = regManager.alloc();
        generateExpr(*binop->right, ctx, rightReg); // Generate code for right operand
        switch (binop->op)
        {
        case BinOp::Add:
            output << "add " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Add operation
            break;
        case BinOp::Sub:
            output << "sub " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Subtract operation
            break;
        case BinOp::Mul:
            output << "mul " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Multiply operation
            break;
        case BinOp::Div:
            output << "div " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Divide operation
            break;
        case BinOp::Mod:
            output << "rem " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Modulus operation
            break;
        case BinOp::Lt:
            output << "slt " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Less than operation
            break;
        case BinOp::Gt:
            output << "slt " << destReg << ", " << rightReg << ", " << leftReg << "\n"; // Greater than: left > right => right < left
            break;
        case BinOp::Le:
            output << "slt " << destReg << ", " << rightReg << ", " << leftReg << "\n"; // Less than or equal: left <= right => !(right < left)
            output << "xori " << destReg << ", " << destReg << ", 1\n";
            break;
        case BinOp::Ge:
            output << "slt " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Greater than or equal: left >= right => !(left < right)
            output << "xori " << destReg << ", " << destReg << ", 1\n";
            break;
        case BinOp::Eq:
            output << "sub " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Equal: left == right
            output << "seqz " << destReg << ", " << destReg << "\n";
            break;
        case BinOp::Ne:
            output << "sub " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Not equal: left != right
            output << "snez " << destReg << ", " << destReg << "\n";
            break;
        case BinOp::And:
            output << "and " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Logical AND operation
            break;
        case BinOp::Or:
            output << "or " << destReg << ", " << leftReg << ", " << rightReg << "\n"; // Logical OR operation
            break;
        default:
            throw std::runtime_error("Unknown binary operator");
        }
        regManager.release();
        regManager.release();
    }
    else if (const auto *unop = dynamic_cast<const UnOpExpr *>(&expr))
    {
        std::string tempReg = regManager.alloc();
        generateExpr(*unop->right, ctx, tempReg); // Generate code for right operand
        switch (unop->op)
        {
        case UnOp::Neg:
            output << "neg " << destReg << ", " << tempReg << "\n"; // Negation operation
            break;
        case UnOp::Not:
            output << "seqz " << destReg << ", " << tempReg << "\n"; // Logical NOT operation
            break;
        default:
            throw std::runtime_error("Unknown unary operator");
        }
        regManager.release(); // Release temporary register
    }
    else if (const auto *call = dynamic_cast<const Call *>(&expr))
    {
        // 保存所有正在使用的临时寄存器
        std::vector<std::string> savedRegs = regManager.getUsedRegisters();
        
        // 如果有保存的寄存器，需要调整栈指针
        if (!savedRegs.empty()) {
            int additionalSpace = savedRegs.size() * 4;
            output << "addi sp, sp, -" << additionalSpace << "\n";
            
            // 将所有正在使用的寄存器保存到栈上
            for (size_t i = 0; i < savedRegs.size(); i++) {
                // 直接使用从栈顶开始的偏移
                output << "sw " << savedRegs[i] << ", " << (i * 4) << "(sp)\n";
            }
        }
        
        // 准备函数参数
        for (size_t i = 0; i < call->args.size() && i <= MAX_REG_COUNT; i++)
        {
            generateExpr(*call->args[i], ctx, "a" + std::to_string(i)); // Generate code for each argument
        }
        
        // 调用函数
        output << "call " << call->name << "\n"; // Call the function
        
        // 恢复所有保存的寄存器
        if (!savedRegs.empty()) {
            for (size_t i = 0; i < savedRegs.size(); i++) {
                // 按照保存时的顺序恢复
                output << "lw " << savedRegs[i] << ", " << (i * 4) << "(sp)\n";
            }
            
            // 恢复栈指针
            int additionalSpace = savedRegs.size() * 4;
            output << "addi sp, sp, " << additionalSpace << "\n";
        }
        
        // 移动返回值到目标寄存器
        if (destReg != "a0")
        {
            output << "mv " << destReg << ", a0\n"; // Move return value to destination register
        }
    }
    else
    {
        throw std::runtime_error("Unknown expression type");
    }
}
void Generator::generateStmt(const Stmt &stmt, FunctionContext &ctx)
{
    // same if-else chain
    if (auto block = dynamic_cast<const Block *>(&stmt))
    {
        ctx.pushScope(); // Enter new scope
        for (const auto &s : block->stmts)
        {
            generateStmt(*s, ctx); // Generate code for each statement in the block
        }
        ctx.popScope(); // Exit scope
    }
    else if (auto empty = dynamic_cast<const EmptyStmt *>(&stmt))
    {
        (void)empty; // avoid unused parameter warning
    }
    else if (auto exprStmt = dynamic_cast<const ExprStmt *>(&stmt))
    {
        std::string tempReg = regManager.alloc();
        generateExpr(*exprStmt->expr, ctx, tempReg); // Generate code for expression statement
        regManager.release(); // Release temporary register
    }
    else if (auto assign = dynamic_cast<const Assign *>(&stmt))
    {
        std::string tempReg = regManager.alloc();
        generateExpr(*assign->value, ctx, tempReg); // Generate code for value expression
        int offset = ctx.findVar(assign->name);
        if (offset == -1)
        {
            throw std::runtime_error("Variable " + assign->name + " not found in context");
        }
        // 使用栈指针偏移：sp + (frameSize - 4 - offset)
        int frameSize = 4 + ctx.stackSize;
        output << "sw " << tempReg << ", " << (frameSize - 4 - offset) << "(sp)\n";
        regManager.release(); // Release temporary register
    }
    else if (auto decl = dynamic_cast<const Decl *>(&stmt))
    {
        int offset = allocateVar(ctx, decl->name); // Allocate variable in the current context
        ctx.addVar(decl->name, offset);            // Add to current scope
        if (decl->value)
        {
            std::string tempReg = regManager.alloc();
            generateExpr(*decl->value, ctx, tempReg); // Generate code for initialization value
            // 使用栈指针偏移：sp + (frameSize - 4 - offset)
            int frameSize = 4 + ctx.stackSize;
            output << "sw " << tempReg << ", " << (frameSize - 4 - offset) << "(sp)\n";
            regManager.release(); // Release temporary register
        }
    }
    else if (auto ifStmt = dynamic_cast<const If *>(&stmt))
    {
        std::string elseLabel = uniqueLabel("if_else_");
        std::string condReg = regManager.alloc();
        generateExpr(*ifStmt->condition, ctx, condReg); // Generate code for condition expression
        output << "beqz " << condReg << ", " << elseLabel << "\n";  // If condition is false, jump to else label
        regManager.release(); // Release condition register
        generateStmt(*ifStmt->thenBody, ctx);        // Generate code for then body
        
        if (ifStmt->elseBody)
        {
            std::string endLabel = uniqueLabel("if_end_");
            output << "j " << endLabel << "\n";          // Jump to end label only if there's an else body
            output << elseLabel << ":\n";                
            generateStmt(*ifStmt->elseBody, ctx); // Generate code for else body
            output << endLabel << ":\n"; // End of if statement
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
        std::string condReg = regManager.alloc();
        generateExpr(*whileStmt->condition, ctx, condReg);
        output << "beqz " << condReg << ", " << endLabel << "\n";
        regManager.release(); // Release condition register
        generateStmt(*whileStmt->body, ctx);  // Generate code for while body
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
            generateExpr(*returnStmt->returnValue, ctx, "a0");
        }
        // Generate function epilogue and return (只恢复ra)
        int frameSize = 4 + ctx.stackSize;
        output << "lw ra, " << (frameSize - 4) << "(sp)\n";
        output << "addi sp, sp, " << frameSize << "\n";
        output << "ret\n";
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
    
    // Store function arguments on stack first to calculate total stack size
    for (size_t i = 0; i < func.args.size() && i < 8; i++)
    {
        int offset = allocateVar(context, func.args[i]);
        context.addVar(func.args[i], offset);
    }
    
    // Generate body first to determine total stack size needed
    std::ostringstream bodyCode;
    Generator tempGenerator(bodyCode);
    tempGenerator.contextStack = contextStack;  // Copy context
    
    tempGenerator.generateStmt(*func.body, tempGenerator.contextStack.top());
    
    // Update our context with the stack size from temp generator
    context.stackSize = tempGenerator.contextStack.top().stackSize;
    
    // Calculate total stack frame size (只保存ra，不用s0)
    int frameSize = 4 + context.stackSize; // ra(4) + local variables
    
    // Generate prologue - 只保存ra
    output << "addi sp, sp, -" << frameSize << "\n"; // Reserve space for frame
    output << "sw ra, " << (frameSize - 4) << "(sp)\n";    // Save return address at top of frame
    
    // Store function arguments on stack (直接使用sp偏移)
    for (size_t i = 0; i < func.args.size() && i < 8; i++)
    {
        int offset = context.findVar(func.args[i]);
        if (offset != -1) {
            // 参数存储位置：sp + (frameSize - 4 - offset)
            output << "sw a" << i << ", " << (frameSize - 4 - offset) << "(sp)\n";
        }
    }
    
    // Output the body code
    output << bodyCode.str();
    
    // Function epilogue (only if no explicit return was generated)
    if (func.rtype == RetType::Void)
    {
        output << "lw ra, " << (frameSize - 4) << "(sp)\n";
        output << "addi sp, sp, " << frameSize << "\n";
        output << "ret\n";
    }
    contextStack.pop();
}

void Generator::generateProg(Program &program)
{
    // RISC-V Assembly Header
    output << ".text\n";
    output << ".globl main\n";
    
    // Generate code for each function
    for (const auto &func : program.functions)
    {
        generateFunc(*func);
    }
}