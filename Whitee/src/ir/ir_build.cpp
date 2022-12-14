/*********************************************************************
 * @file   ir_build.cpp
 * @brief  IR生成
 * 
 * @author 神祖
 * @date   June 2022
 *********************************************************************/
#include <iostream>
#include <initializer_list>

#include "ir_build.h"

shared_ptr<Module> module;

unordered_map<string, shared_ptr<Function>> globalFunctionMap;      // name <--> Function
unordered_map<string, shared_ptr<ConstantValue>> globalConstantMap; // name <--> Constant Array
unordered_map<string, shared_ptr<GlobalValue>> globalVariableMap;   // name <--> Global Variable
unordered_map<string, shared_ptr<Value>> localArrayMap;             // name <--> Local Array
unordered_map<string, shared_ptr<StringValue>> globalStringMap;     // string <--> string pointer

string mulOp = "*"; 
string addOp = "+";

unsigned int loopDepth = 0;  // 循环层数

void blockToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<BlockNode> &block,
               shared_ptr<BasicBlock> &loopJudge, shared_ptr<BasicBlock> &loopEnd, bool &afterJump);

void blockToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<BlockNode> &block);

void varDefToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb,
                const shared_ptr<VarDefNode> &varDef, bool &afterJump);

void stmtToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<StmtNode> &stmt,
              shared_ptr<BasicBlock> &loopJudge, shared_ptr<BasicBlock> &loopEnd, bool &afterJump);

shared_ptr<Value> expToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<ExpNode> &exp);

void conditionToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<ExpNode> &cond,
                   shared_ptr<BasicBlock> &trueBlock, shared_ptr<BasicBlock> &falseBlock);

void pointerToIr(const shared_ptr<LValNode> &lVal, shared_ptr<Value> &address, shared_ptr<Value> &offset,
                 shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb);

/**
 * @brief 开始构建IR，逐步分析语法树中的变量，函数  有Decl | FuncDef
 * @param compUnit 语法树根节点
 * @return IR生成
 */
shared_ptr<Module> buildIrModule(shared_ptr<CompUnitNode> &compUnit)
{
    module = make_shared<Module>(); 
    for (const auto &decl : compUnit->declList)
    {
        if (dynamic_cast<ConstDeclNode *>(decl.get()))
        {
            for (auto &def : s_p_c<ConstDeclNode>(decl)->constDefList)
            {
                if (def->ident->ident->symbolType == SymbolType::CONST_ARRAY)  // 只有常量数组才存储，常量不用存
                {
                    shared_ptr<Value> value = make_shared<ConstantValue>(def);
                    module->globalConstants.push_back(value);
                    globalConstantMap.insert({def->ident->ident->usageName, s_p_c<ConstantValue>(value)});
                }
            }
        }
        else
        {
            for (auto &def : s_p_c<VarDeclNode>(decl)->varDefList)   // 全局变量
            {
                shared_ptr<Value> value = make_shared<GlobalValue>(def);
                module->globalVariables.push_back(value);
                globalVariableMap.insert({def->ident->ident->usageName, s_p_c<GlobalValue>(value)});
            }
        }
    }
    for (auto &funcNode : compUnit->funcDefList)
    {
        shared_ptr<Function> function = make_shared<Function>();
        shared_ptr<BasicBlock> entryBlock = make_shared<BasicBlock>(function, true, loopDepth);
        module->functions.push_back(function);
        function->name = funcNode->ident->ident->usageName;
        function->funcType = funcNode->funcType;
        function->entryBlock = entryBlock;
        function->blocks.push_back(entryBlock);
        globalFunctionMap.insert({function->name, function});
        if (funcNode->funcFParams)
        {
            for (auto &param : funcNode->funcFParams->funcParamList)
            {
                shared_ptr<Value> paramValue = make_shared<ParameterValue>(function, param);
                function->params.push_back(paramValue);
                if (s_p_c<ParameterValue>(paramValue)->variableType == VariableType::INT)  // 局部变量
                {
                    write_variable(entryBlock, s_p_c<ParameterValue>(paramValue)->name, paramValue);
                }
                else
                {
                    localArrayMap[param->ident->ident->usageName] = paramValue;  // 局部数组
                }
            }
        }
        blockToIr(function, entryBlock, funcNode->block);
    }
    return module;
}

/**
 * @brief Transform a block(this 'block' is the concept of AST, instead of SSA) to IR.  block内有Decl | Stmt
 * @param func 所在的IR函数
 * @param bb 生成的IR基本块
 * @param block 语法树函数中的块
 * @param loopJudge 循环判断
 * @param loopEnd 循环结束
 * @param afterJump 是否已跳出循环
 */
void blockToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<BlockNode> &block,
               shared_ptr<BasicBlock> &loopJudge, shared_ptr<BasicBlock> &loopEnd, bool &afterJump)
{
    if (block == nullptr)
        return;
    for (const auto &item : block->blockItems)
    {
        if (dynamic_cast<VarDeclNode *>(item.get()))  // 局部变量定义
        {
            shared_ptr<VarDeclNode> varDecl = s_p_c<VarDeclNode>(item);
            for (const auto &varDef : varDecl->varDefList)
            {
                varDefToIr(func, bb, varDef, afterJump);
            }
        }
        else if (dynamic_cast<ConstDeclNode *>(item.get()))  // const变量定义
        {
            shared_ptr<ConstDeclNode> constDecl = s_p_c<ConstDeclNode>(item);
            for (auto &constDef : constDecl->constDefList)
            {
                if (constDef->ident->ident->symbolType == SymbolType::CONST_ARRAY)
                {
                    shared_ptr<Value> value = make_shared<ConstantValue>(constDef);
                    module->globalConstants.push_back(value);
                    globalConstantMap.insert({constDef->ident->ident->usageName, s_p_c<ConstantValue>(value)});
                }
            }
        }
        else if (dynamic_cast<StmtNode *>(item.get()))
        {
            stmtToIr(func, bb, s_p_c<StmtNode>(item), loopJudge, loopEnd, afterJump);
        }
        else
        {
            cerr << "Error occurs in process blockToIr." << endl;
        }
    }
}

void blockToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<BlockNode> &block)
{
    shared_ptr<BasicBlock> judge, end;
    bool afterJump = false;    // 还未跳出
    blockToIr(func, bb, block, judge, end, afterJump);
}

/**
 * @brief 将变量定义转为IR
 * @param func 所在的IR函数
 * @param bb 生成的IR基本块
 * @param varDef 语法树变量定义
 * @param afterJump 是否跳出
 */
void varDefToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<VarDefNode> &varDef, bool &afterJump)
{
    if (afterJump)  // 已经跳出
        return;
    func->variables.insert({varDef->ident->ident->usageName, varDef->dimension == 0 ? VariableType::INT : VariableType::POINTER});

    if (varDef->dimension == 0 && varDef->type == InitType::INIT)  // 初始化的局部变量
    {
        shared_ptr<InitValValNode> initVal = s_p_c<InitValValNode>(varDef->initVal);
        shared_ptr<Value> exp = expToIr(func, bb, initVal->exp);
        if (exp->value_type == ValueType::INSTRUCTION)    // 一个非常数值，IR将其记录为一个变量
        {
            shared_ptr<Instruction> insExp = s_p_c<Instruction>(exp);
            insExp->resultType = L_VAL_RESULT;
            insExp->caughtVarName = varDef->ident->ident->usageName;
        }
        write_variable(bb, varDef->ident->ident->usageName, exp); 
    }
    else if (varDef->dimension != 0)   // 数组
    {
        int units = 1;
        for (const auto &d : varDef->dimensions)  // 计算元素个数
            units *= d;
        shared_ptr<Value> alloc = make_shared<AllocInstruction>(varDef->ident->ident->usageName, units * _W_LEN, units, bb);
        bb->instructions.push_back(s_p_c<Instruction>(alloc));
        localArrayMap.insert({s_p_c<AllocInstruction>(alloc)->name, s_p_c<AllocInstruction>(alloc)});  // 局部数组加入localArrayMap

        if (varDef->type == InitType::INIT)  // 初始化的数组
        {
            vector<pair<int, shared_ptr<ExpNode>>> initValues = varDef->initVal->toOneDimensionArray(0, units);
            int curIndex = 0;
            for (auto &it : initValues)
            {
                for (; curIndex < it.first; ++curIndex)  // 给未指定的位置赋值0
                {
                    shared_ptr<Value> zero = Number(0);
                    shared_ptr<Value> offset = Number(curIndex);
                    
                    shared_ptr<Instruction> store = make_shared<StoreInstruction>(zero, alloc, offset, bb);
                    user_use(store, {zero, alloc, offset});
                    bb->instructions.push_back(store);
                }
                // 给指定初始化的位置赋值
                ++curIndex;
                shared_ptr<Value> exp = expToIr(func, bb, it.second);
                shared_ptr<Value> offset = Number(it.first);
                
                shared_ptr<Instruction> store = make_shared<StoreInstruction>(exp, alloc, offset, bb);
                user_use(store, {exp, alloc, offset});
                bb->instructions.push_back(store);
            }
            for (; curIndex < units; ++curIndex)  // 给未指定的位置赋值0
            {
                shared_ptr<Value> zero = Number(0);
                shared_ptr<Value> offset = Number(curIndex);
                shared_ptr<Instruction> store = make_shared<StoreInstruction>(zero, alloc, offset, bb);
                user_use(store, {zero, alloc, offset});
                bb->instructions.push_back(store);
            }
        }
    }
}

/**
 * @brief 将statements转为IR
 * @param func 所在的IR函数
 * @param bb 生成的IR基本块
 * @param stmt 语法树stmt
 * @param loopJudge 循环判断
 * @param loopEnd 循环结束
 * @param afterJump 是否一定跳出
 */
void stmtToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<StmtNode> &stmt,
              shared_ptr<BasicBlock> &loopJudge, shared_ptr<BasicBlock> &loopEnd, bool &afterJump)
{
    if (afterJump)  // 已经跳出
        return;
    switch (stmt->type)   // STMT_ASSIGN | STMT_EXP | STMT_BLOCK | STMT_IF | STMT_IF_ELSE | STMT_WHILE | STMT_BREAK | STMT_CONTINUE | STMT_RETURN | STMT_RETURN_VOID | STMT_EMPTY
    {
    case StmtType::STMT_EMPTY:
        return;
    case StmtType::STMT_EXP:   // 空表达式
    {
        expToIr(func, bb, stmt->exp);
        return;
    }
    case StmtType::STMT_BLOCK:  // 代码块
    {
        blockToIr(func, bb, stmt->block, loopJudge, loopEnd, afterJump);
        return;
    }
    case StmtType::STMT_ASSIGN:   // 赋值语句
    {
        shared_ptr<Value> value = expToIr(func, bb, stmt->exp);  // 被赋值
        shared_ptr<SymbolTableItem> identItem = stmt->lVal->ident->ident;  // 赋值对象
        // 查找左值
        shared_ptr<Value> address, offset;
        switch (identItem->symbolType)  //  CONST_VAR | CONST_ARRAY | VAR | ARRAY | VOID_FUNC | RET_FUNC
        {
        case SymbolType::VAR:
        {
            if (identItem->blockId.first == 0)  // 全局变量
            {
                pointerToIr(stmt->lVal, address, offset, func, bb);
                shared_ptr<Instruction> ins = make_shared<StoreInstruction>(value, address, offset, bb);
                user_use(ins, {value, address, offset});
                bb->instructions.push_back(ins);
            }
            else
            {
                if (value->value_type == ValueType::INSTRUCTION)  // 一个值非常数值，IR将其记录为一个变量
                {
                    shared_ptr<Instruction> insValue = s_p_c<Instruction>(value);
                    insValue->resultType = L_VAL_RESULT;
                    insValue->caughtVarName = stmt->lVal->ident->ident->usageName;
                }
                write_variable(bb, stmt->lVal->ident->ident->usageName, value);
            }
            return;
        }
        case SymbolType::ARRAY:
        {
            if (value->value_type == ValueType::INSTRUCTION && s_p_c<Instruction>(value)->resultType != L_VAL_RESULT)  // 被赋值为一个非左值 
            {
                shared_ptr<Instruction> insValue = s_p_c<Instruction>(value);  // 将此值转为左值
                insValue->resultType = L_VAL_RESULT;
                insValue->caughtVarName = generateTempLeftValueName();
            }
            pointerToIr(stmt->lVal, address, offset, func, bb);
            shared_ptr<Instruction> ins = make_shared<StoreInstruction>(value, address, offset, bb);
            user_use(ins, {value, address, offset});
            bb->instructions.push_back(ins);
            return;
        }
        default:
            cerr << "Error occurs in stmt assign to IR: invalid l-value type." << endl;
            return;
        }
    }
    case StmtType::STMT_RETURN:
    {
        shared_ptr<Value> value = expToIr(func, bb, stmt->exp);  // 返回值
        shared_ptr<Instruction> ins = make_shared<ReturnInstruction>(FuncType::FUNC_INT, value, bb);
        user_use(ins, {value});
        bb->instructions.push_back(ins);
        afterJump = true;    // 已经跳出
        return;
    }
    case StmtType::STMT_RETURN_VOID:
    {
        shared_ptr<Value> value = nullptr;
        shared_ptr<Instruction> ins = make_shared<ReturnInstruction>(FuncType::FUNC_VOID, value, bb);
        bb->instructions.push_back(ins);
        afterJump = true;    // 已经跳出
        return;
    }
    case StmtType::STMT_IF:
    {
        shared_ptr<BasicBlock> endIf = make_shared<BasicBlock>(func, true, loopDepth);
        shared_ptr<BasicBlock> ifStmt = make_shared<BasicBlock>(func, true, loopDepth);
        // 转换cond
        conditionToIr(func, bb, stmt->cond, ifStmt, endIf);
        // 每个状态的前驱和后继
        bb->successors.insert({endIf, ifStmt});
        endIf->predecessors.insert(bb);
        ifStmt->predecessors.insert(bb);
        // block变为if后的block
        bb = endIf;
        // ifstmt状态加入block
        func->blocks.push_back(ifStmt);
        // 标记if是否一定跳出
        bool ifAfterJump = false;
        // 分析ifstmt
        stmtToIr(func, ifStmt, stmt->stmt, loopJudge, loopEnd, ifAfterJump);
        
        if (!ifAfterJump)  // 如果没有发生跳出，则添加一个跳转回endif块
        {
            shared_ptr<Instruction> jmp = make_shared<JumpInstruction>(endIf, ifStmt);
            ifStmt->instructions.push_back(jmp);
            // ifstmt的后继为endif，endif的前驱为ifstmt
            ifStmt->successors.insert(endIf);
            endIf->predecessors.insert(ifStmt);
        }

        func->blocks.push_back (endIf);   // 不一定跳出

        return;
    }
    case StmtType::STMT_IF_ELSE:
    {
        shared_ptr<BasicBlock> endIf = make_shared<BasicBlock>(func, true, loopDepth);
        shared_ptr<BasicBlock> ifStmt = make_shared<BasicBlock>(func, true, loopDepth);
        shared_ptr<BasicBlock> elseStmt = make_shared<BasicBlock>(func, true, loopDepth);
        conditionToIr(func, bb, stmt->cond, ifStmt, elseStmt);
        // 每个状态的前驱和后继
        bb->successors.insert({ifStmt, elseStmt});
        ifStmt->predecessors.insert(bb);
        elseStmt->predecessors.insert(bb);
        bb = endIf;
        // 标记if和else是否有跳出
        bool ifAfterJump = false, elseAfterJump = false;
        // 分析ifstmt
        func->blocks.push_back(ifStmt);
        stmtToIr(func, ifStmt, stmt->stmt, loopJudge, loopEnd, ifAfterJump);
        if (!ifAfterJump)    // 如果没有发生跳出，则添加一个跳转回endif块
        {
            shared_ptr<Instruction> jmpIf = make_shared<JumpInstruction>(endIf, ifStmt);
            ifStmt->instructions.push_back(jmpIf);
            // maintain successors and predecessors.
            ifStmt->successors.insert(endIf);
            endIf->predecessors.insert(ifStmt);
        }
        // 分析elsestmt
        func->blocks.push_back(elseStmt);
        stmtToIr(func, elseStmt, stmt->elseStmt, loopJudge, loopEnd, elseAfterJump);
        if (!elseAfterJump)    // 如果没有发生跳出，则添加一个跳转回endif块
        {
            shared_ptr<Instruction> jmpElse = make_shared<JumpInstruction>(endIf, elseStmt);
            elseStmt->instructions.push_back(jmpElse);
            // maintain successors and predecessors.
            elseStmt->successors.insert(endIf);
            endIf->predecessors.insert(elseStmt);
        }
        if (ifAfterJump && elseAfterJump)  // if和else都发生跳转，即必然跳转
            afterJump = true;
        else
            func->blocks.push_back(endIf);
        return;
    }
    case StmtType::STMT_WHILE:
    {
        // 声明循环头、循环结束和循环体
        shared_ptr<BasicBlock> whileBody = make_shared<BasicBlock>(func, false, loopDepth + 1);  // 循环中的块不封闭
        shared_ptr<BasicBlock> whileJudge = make_shared<BasicBlock> (func, true, loopDepth + 1);
        shared_ptr<BasicBlock> whileEnd = make_shared<BasicBlock> (func, true, loopDepth);
        shared_ptr<BasicBlock> preWhileBody = whileBody;
        // cond真则进入whileBody，假则进入whileEnd
        conditionToIr(func, bb, stmt->cond, whileBody, whileEnd);
        // 维护前驱后继，注意whileJudge不能在whileBody之前分析。
        bb->successors.insert({whileEnd, whileBody});
        whileEnd->predecessors.insert(bb);
        whileBody->predecessors.insert(bb);
        // assign bb as while end.
        bb = whileEnd;
        // 标记while是否有跳出
        bool whileBodyAfterJump = false;
        func->blocks.push_back(whileBody);

        ++loopDepth; // goto while body.
        stmtToIr(func, whileBody, stmt->stmt, whileJudge, whileEnd, whileBodyAfterJump);
        --loopDepth;
        if (!whileBodyAfterJump)// while无跳出，则添加跳转到whileJudge
        {
            shared_ptr<Instruction> jmpJudge = make_shared<JumpInstruction>(whileJudge, whileBody);
            whileBody->instructions.push_back(jmpJudge);
            whileBody->successors.insert(whileJudge);
            whileJudge->predecessors.insert(whileBody);
        }
        if (!whileJudge->predecessors.empty())  // whileJudge前驱不为空
        {
            func->blocks.push_back(whileJudge);
            // cond真则进入whileBody，假则进入whileEnd
            ++loopDepth;
            conditionToIr(func, whileJudge, stmt->cond, preWhileBody, whileEnd);
            --loopDepth;

            whileEnd->predecessors.insert(whileJudge);
            whileJudge->successors.insert(whileEnd);
            preWhileBody->predecessors.insert(whileJudge);
            whileJudge->successors.insert(preWhileBody);
        }
        seal_basic_block(preWhileBody);

        func->blocks.push_back(whileEnd);
        return;
    }
    case StmtType::STMT_BREAK:
    {
        if (!loopEnd)  // 没有循环
            cerr << "Error occurs in stmt to IR: break without a loop." << endl;
        shared_ptr<Instruction> jmp = make_shared<JumpInstruction>(loopEnd, bb);
        bb->instructions.push_back(jmp);
        bb->successors.insert(loopEnd);
        loopEnd->predecessors.insert(bb);
        afterJump = true;  // 发生跳出
        return;
    }
    default:
        if (!loopJudge)  // 没有循环
            cerr << "Error occurs in stmt to IR: continue without a loop." << endl;
        shared_ptr<Instruction> jmp = make_shared<JumpInstruction>(loopJudge, bb);
        bb->instructions.push_back(jmp);
        bb->successors.insert(loopJudge);
        loopJudge->predecessors.insert(bb);
        afterJump = true;  // 发生跳出
    }
}

/**
 * @brief 将普通表达式转化为IR。PrimaryExpNode | UnaryExpNode | MulExpNode | AddExpNode | RelExpNode | EqExpNode
 * @param func 所在的IR函数
 * @param bb 生成的IR基本块
 * @param exp 语法树exp表达式
 * @return 表达式的结果值，在void函数调用中，它没有任何意义。
 */
shared_ptr<Value> expToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<ExpNode> &exp)
{
    if (dynamic_cast<PrimaryExpNode *>(exp.get()))
    {
        auto p = s_p_c<PrimaryExpNode>(exp);
        switch (p->type)      //  PrimaryExp -> '(' Exp ')'  |  IntConst  |  LVal
        {
        case PrimaryExpType::PRIMARY_L_VAL:
        {
            shared_ptr<SymbolTableItem> identItem = p->lVal->ident->ident;
            shared_ptr<Value> address, offset;
            switch (identItem->symbolType)   //  CONST_ARRAY | VAR | ARRAY
            {
            case SymbolType::VAR:
            {
                if (identItem->blockId.first == 0)  // 全局变量
                {
                    pointerToIr(p->lVal, address, offset, func, bb);
                    shared_ptr<Instruction> ins = make_shared<LoadInstruction>(address, offset, bb);
                    user_use(ins, {address, offset});
                    bb->instructions.push_back(ins);
                    return ins;
                }
                // 常量传播、复制传播    直接取出对应变量的值
                return read_variable(bb, identItem->usageName);
            }
            case SymbolType::CONST_ARRAY:
            case SymbolType::ARRAY:
            {
                if (p->lVal->exps.empty())  // 数组无[]，提取指针
                {
                    if (localArrayMap.count(p->lVal->ident->ident->usageName) != 0)
                        return localArrayMap.at(p->lVal->ident->ident->usageName);
                    else if (globalVariableMap.count(p->lVal->ident->ident->usageName) != 0)
                        return globalVariableMap.at(p->lVal->ident->ident->usageName);
                    else if (globalConstantMap.count(p->lVal->ident->ident->usageName) != 0)
                        return globalConstantMap.at(p->lVal->ident->ident->usageName);
                    cerr << "Error occurs in process primary exp to IR: array is not defined." << endl;
                    return nullptr;
                }
                else  // 提取数组的某一维
                {
                    pointerToIr(p->lVal, address, offset, func, bb);
                    if (p->lVal->exps.size() == p->lVal->dimension)  // 维数与[]数一样，提取某个元素
                    {
                        shared_ptr<Instruction> load = make_shared<LoadInstruction>(address, offset, bb);
                        user_use(load, {address, offset});
                        bb->instructions.push_back(load);
                        return load;
                    }
                    else  // 维数与[]数一样，提取某个指针
                    {
                        shared_ptr<Instruction> pt = make_shared<BinaryInstruction>(addOp, address, offset, bb);
                        user_use(pt, {address, offset});
                        bb->instructions.push_back(pt);
                        return pt;
                    }
                }
            }
            default:
                cerr << "Error occurs in process PrimaryExpNode to IR." << endl;
                return nullptr;
            }
        }
        case PrimaryExpType::PRIMARY_STRING:
        {
            if (globalStringMap.count(p->str) != 0)
            {
                return globalStringMap.at(p->str);
            }
            shared_ptr<StringValue> str = make_shared<StringValue>(p->str);
            globalStringMap[p->str] = str;
            module->globalStrings.push_back(str);
            return str;
        }
        case PrimaryExpType::PRIMARY_NUMBER:  // 全局的常量数字
            return Number(p->number);
        default:
            return expToIr(func, bb, p->exp);
        }
    }
    else if (dynamic_cast<UnaryExpNode *>(exp.get()))
    {
        auto p = s_p_c<UnaryExpNode>(exp);
        switch (p->type)  // UnaryExp -> PrimaryExp  |  IdentUsage '(' [FuncRParams] ')'  |  ('+' | '−' | '!') UnaryExp
        {
        case UnaryExpType::UNARY_PRIMARY:
        {
            shared_ptr<Value> value = expToIr(func, bb, p->primaryExp);
            if (p->op == "+")
                return value;
            shared_ptr<Instruction> ins = make_shared<UnaryInstruction>(p->op, value, bb);
            user_use(ins, {value});
            bb->instructions.push_back(ins);
            return ins;
        }
        case UnaryExpType::UNARY_FUNC_NON_PARAM:
        case UnaryExpType::UNARY_FUNC:
        {
            vector<shared_ptr<Value>> params;
            if (p->funcRParams)  // 如果有参数
            {
                for (const auto &i : p->funcRParams->exps)
                {
                    shared_ptr<Value> tmpExp = expToIr(func, bb, i);
                    if (p->funcRParams->exps.size() > 1 && tmpExp->value_type == ValueType::INSTRUCTION && s_p_c<Instruction>(tmpExp)->resultType != L_VAL_RESULT) //当参数为非左值
                    {
                        s_p_c<Instruction>(tmpExp)->resultType = L_VAL_RESULT;
                        s_p_c<Instruction>(tmpExp)->caughtVarName = generateArgumentLeftValueName(p->ident->ident->usageName);
                    }
                    params.push_back(tmpExp);
                }
            }

            shared_ptr<Value> invoke;
            if (InvokeInstruction::sysFuncMap.count(p->ident->ident->name) != 0)  // 调用运行时函数
            {
                invoke = make_shared<InvokeInstruction>(p->ident->ident->name, params, bb);
            }
            else  // 调用自定义函数
            {
                shared_ptr<Function> targetFunction = globalFunctionMap.at(p->ident->ident->usageName);
                func->callees.insert(targetFunction);
                targetFunction->callers.insert(func);
                invoke = make_shared<InvokeInstruction>(targetFunction, params, bb);
            }
            user_use(invoke, params);
            bb->instructions.push_back(s_p_c<Instruction>(invoke));
            if (p->op == "+")
                return invoke;
            shared_ptr<Instruction> ins = make_shared<UnaryInstruction>(p->op, invoke, bb);
            user_use(ins, {invoke});
            bb->instructions.push_back(ins);
            return ins;
        }
        default:
            shared_ptr<Value> value = expToIr(func, bb, p->unaryExp);
            if (p->op == "+")
                return value;
            shared_ptr<Instruction> ins = make_shared<UnaryInstruction>(p->op, value, bb);
            user_use(ins, {value});
            bb->instructions.push_back(ins);
            return ins;
        }
    }
    else if (dynamic_cast<MulExpNode *>(exp.get()))
    {
        auto p = s_p_c<MulExpNode>(exp);
        if (p->mulExp == nullptr)
        {
            return expToIr(func, bb, p->unaryExp);
        }
        else
        {
            shared_ptr<Value> lhs = expToIr(func, bb, p->mulExp);
            shared_ptr<Value> rhs = expToIr(func, bb, p->unaryExp);
            shared_ptr<Instruction> ins = make_shared<BinaryInstruction>(p->op, lhs, rhs, bb);
            user_use(ins, {lhs, rhs});
            bb->instructions.push_back(ins);
            return ins;
        }
    }
    else if (dynamic_cast<AddExpNode *>(exp.get()))
    {
        auto p = s_p_c<AddExpNode>(exp);
        if (p->addExp == nullptr)
        {
            return expToIr(func, bb, p->mulExp);
        }
        else
        {
            shared_ptr<Value> lhs = expToIr(func, bb, p->addExp);
            shared_ptr<Value> rhs = expToIr(func, bb, p->mulExp);
            shared_ptr<Instruction> ins = make_shared<BinaryInstruction>(p->op, lhs, rhs, bb);
            user_use(ins, {lhs, rhs});
            bb->instructions.push_back(ins);
            return ins;
        }
    }
    else if (dynamic_cast<RelExpNode *>(exp.get()))
    {
        auto p = s_p_c<RelExpNode>(exp);
        if (p->relExp == nullptr)
        {
            return expToIr(func, bb, p->addExp);
        }
        else
        {
            shared_ptr<Value> lhs = expToIr(func, bb, p->relExp);
            shared_ptr<Value> rhs = expToIr(func, bb, p->addExp);
            if (lhs->value_type == INSTRUCTION && s_p_c<Instruction>(lhs)->resultType == R_VAL_RESULT && rhs->value_type == INSTRUCTION && s_p_c<Instruction>(rhs)->resultType == R_VAL_RESULT) // 比较的两侧均为右值
            {
                s_p_c<Instruction>(lhs)->resultType = L_VAL_RESULT;
                s_p_c<Instruction>(lhs)->caughtVarName = generateTempLeftValueName();
            }
            shared_ptr<Instruction> ins = make_shared<BinaryInstruction>(p->op, lhs, rhs, bb);
            user_use(ins, {lhs, rhs});
            bb->instructions.push_back(ins);
            return ins;
        }
    }
    else if (dynamic_cast<EqExpNode *>(exp.get()))
    {
        auto p = s_p_c<EqExpNode>(exp);
        if (p->eqExp == nullptr)
        {
            return expToIr(func, bb, p->relExp);
        }
        else
        {
            shared_ptr<Value> lhs = expToIr(func, bb, p->eqExp);
            shared_ptr<Value> rhs = expToIr(func, bb, p->relExp);
            if (lhs->value_type == INSTRUCTION && s_p_c<Instruction>(lhs)->resultType == R_VAL_RESULT && rhs->value_type == INSTRUCTION && s_p_c<Instruction>(rhs)->resultType == R_VAL_RESULT) // 比较的两侧均为右值
            {
                s_p_c<Instruction>(lhs)->resultType = L_VAL_RESULT;
                s_p_c<Instruction>(lhs)->caughtVarName = generateTempLeftValueName();
            }
            shared_ptr<Instruction> ins = make_shared<BinaryInstruction>(p->op, lhs, rhs, bb);
            user_use(ins, {lhs, rhs});
            bb->instructions.push_back(ins);
            return ins;
        }
    }
    else
    {
        cerr << "Error occurs in process expToIr: invalid expression type." << endl;
    }
    return nullptr;
}

/**
 * @brief 转换 IF 或 WHILE 语句的条件表达式，可构建新的基本块。  LOrExpNode | LAndExpNode | EqExpNode | CondNode
 * @param func 所在的IR函数
 * @param bb 生成的IR基本块
 * @param cond 语法树条件表达式exp
 * @param trueBlock 为真跳入的IR块
 * @param falseBlock 为假跳入的IR块
 */
void conditionToIr(shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb, const shared_ptr<ExpNode> &cond,
                   shared_ptr<BasicBlock> &trueBlock, shared_ptr<BasicBlock> &falseBlock)
{
    if (dynamic_cast<LOrExpNode *>(cond.get()))
    {
        const shared_ptr<LOrExpNode> lOrExp = s_p_c<LOrExpNode>(cond);
        if (lOrExp->lOrExp == nullptr)
        {
            conditionToIr(func, bb, lOrExp->lAndExp, trueBlock, falseBlock);
        }
        else  // 有 || 短路求值
        {
            // 声明一个新的基本块作为第二个条件判断块
            shared_ptr<BasicBlock> logicOrBlock = make_shared<BasicBlock>(func, true, loopDepth);
            
            conditionToIr(func, bb, lOrExp->lOrExp, trueBlock, logicOrBlock);
            // 如果条件为真则进入trueBlock，条件为假进入logicOrBlock
            bb->successors.insert({trueBlock, logicOrBlock});
            trueBlock->predecessors.insert(bb);
            logicOrBlock->predecessors.insert(bb);
            // 将 block 更改为第二个条件块
            bb = logicOrBlock;
            func->blocks.push_back(logicOrBlock);
            // deal with the logic and condition.
            conditionToIr(func, bb, lOrExp->lAndExp, trueBlock, falseBlock);
        }
    }
    else if (dynamic_cast<LAndExpNode *>(cond.get()))
    {
        const shared_ptr<LAndExpNode> lAndExp = s_p_c<LAndExpNode>(cond);
        if (lAndExp->lAndExp != nullptr) // 有 && 短路求值
        {
            // 声明一个新的基本块作为第二个条件判断块
            shared_ptr<BasicBlock> logicAndBlock = make_shared<BasicBlock>(func, true, loopDepth);
            conditionToIr(func, bb, lAndExp->lAndExp, logicAndBlock, falseBlock);
            // 如果条件为真则进入logicAndBlock，条件为假进入falseBlock
            bb->successors.insert({falseBlock, logicAndBlock});
            falseBlock->predecessors.insert(bb);
            logicAndBlock->predecessors.insert(bb);
            // 将 block 更改为第二个条件块
            bb = logicAndBlock;
            func->blocks.push_back(logicAndBlock);
        }
        // deal with the equal condition.
        conditionToIr(func, bb, lAndExp->eqExp, trueBlock, falseBlock);
    }
    else if (dynamic_cast<EqExpNode *>(cond.get()))
    {
        shared_ptr<Value> exp = expToIr(func, bb, cond);
        shared_ptr<Value> ins = make_shared<BranchInstruction>(exp, trueBlock, falseBlock, bb);
        user_use(ins, {exp});
        bb->instructions.push_back(s_p_c<Instruction>(ins));
    }
    else if (dynamic_cast<CondNode *>(cond.get()))
    {
        const shared_ptr<LOrExpNode> lOrExp = s_p_c<CondNode>(cond)->lOrExp;
        conditionToIr(func, bb, lOrExp, trueBlock, falseBlock);
    }
    else
    {
        cerr << "Error occurs in condition to IR: invalid expression type." << endl;
    }
}

/**
 * @brief 将数组、全局值、全局数组或常量值的地址转换为IR。
 * @param lVal 语法树左值
 * @param address 基地址
 * @param offset 偏移量
 * @param func 所在的IR函数
 * @param bb 生成的IR基本块
 */
void pointerToIr(const shared_ptr<LValNode> &lVal, shared_ptr<Value> &address, shared_ptr<Value> &offset, shared_ptr<Function> &func, shared_ptr<BasicBlock> &bb)
{
    shared_ptr<SymbolTableItem> identItem = lVal->ident->ident;
    switch (identItem->symbolType)  //  CONST_VAR | CONST_ARRAY | VAR | ARRAY | VOID_FUNC | RET_FUNC
    {
    case SymbolType::VAR:
    {
        if (identItem->blockId.first == 0)
        {
            address = globalVariableMap.at(identItem->usageName);  // 全局变量，直接获取
            offset = Number(0);
            return;
        }
        else
        {
            cerr << "Error occurs in pointer to IR: cast a local variable." << endl;
            return;
        }
    }
    case SymbolType::CONST_ARRAY:
    case SymbolType::ARRAY:
    {
        int size = 1;
        for (int i = 1; i < identItem->numOfEachDimension.size(); ++i)
            size *= identItem->numOfEachDimension.at(i);
        offset = nullptr;
        for (int i = 0; i < lVal->exps.size(); ++i)  // 根据[]中的值，计算地址偏移量
        {
            shared_ptr<Value> number = Number(size);
            shared_ptr<Value> off = expToIr(func, bb, lVal->exps.at(i));
            shared_ptr<Value> mul = make_shared<BinaryInstruction>(mulOp, off, number, bb);
            user_use(mul, {number, off});
            bb->instructions.push_back(s_p_c<Instruction>(mul));
            if (offset)
            {
                shared_ptr<Value> oldOffset = offset;
                offset = make_shared<BinaryInstruction>(addOp, offset, mul, bb);
                user_use(offset, {oldOffset, mul});
                bb->instructions.push_back(s_p_c<Instruction>(offset));
            }
            else
            {
                offset = mul;
            }
            if (i + 1 < identItem->numOfEachDimension.size())   // size更新为此指针一次指向的元素个数
                size /= identItem->numOfEachDimension.at(i + 1);
        }
        if (lVal->exps.size() < identItem->numOfEachDimension.size())  // []数小于维数，为指针
        {
            shared_ptr<Value> oldOffset = offset;
            shared_ptr<Value> four = Number(_W_LEN);
            offset = make_shared<BinaryInstruction>(mulOp, offset, four, bb);
            user_use(offset, {oldOffset, four});
            bb->instructions.push_back(s_p_c<Instruction>(offset));
        }
        if (identItem->symbolType == SymbolType::CONST_ARRAY) // 全局变量数组
            address = globalConstantMap.at(identItem->usageName);
        else if (identItem->blockId.first == 0)  // 全局变量数组
            address = globalVariableMap.at(identItem->usageName);
        else  // 局部变量数组
            address = localArrayMap.at(identItem->usageName);
        return;
    }
    default:
        cerr << "Error occurs in pointer to IR: invalid symbol type." << endl;
    }
}
