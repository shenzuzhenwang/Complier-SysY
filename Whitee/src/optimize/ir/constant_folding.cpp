#include "ir_optimize.h"

string negOp = "-"; 
string notOp = "!"; 

/**
 * @brief 原值为左值，则新创建的也为左值
 * @param newVal 新创建的值
 * @param oldVal 原值
 */
void maintainLeftValue(shared_ptr<Value> &newVal, shared_ptr<Value> &oldVal)
{
    if (oldVal->valueType == ValueType::INSTRUCTION && s_p_c<Instruction>(oldVal)->resultType == L_VAL_RESULT) // oldVal为左值，new也为同名变量
    {
        s_p_c<Instruction>(newVal)->resultType = L_VAL_RESULT;
        s_p_c<Instruction>(newVal)->caughtVarName = s_p_c<Instruction>(oldVal)->caughtVarName;
    }
}

/**
 * @brief 将此指令中可能折叠的量进行折叠
 * @param ins 此指令
 */
void fold(shared_ptr<Instruction> &ins)
{
    shared_ptr<Value> newVal;
    shared_ptr<Value> insVal = ins;
    bool replace = false;
    switch (ins->type)
    {
    case InstructionType::BINARY:
    case InstructionType::CMP:  // 两个操作数
    {
        shared_ptr<BinaryInstruction> bIns = s_p_c<BinaryInstruction>(ins);
        if (bIns->lhs->valueType == ValueType::NUMBER && bIns->rhs->valueType == ValueType::NUMBER)  // 两操作数均为常量，则直接得出结果
        {
            shared_ptr<NumberValue> lOpVal = s_p_c<NumberValue>(bIns->lhs);
            shared_ptr<NumberValue> rOpVal = s_p_c<NumberValue>(bIns->rhs);
            if ((bIns->op == "/" || bIns->op == "%") && rOpVal->number == 0)  // 除零操作
            {
                cerr << "Error occurs in process constant folding: divide 0." << endl;
                return;
            }
            if (bIns->op == "+")
                newVal = getNumberValue(lOpVal->number + rOpVal->number);
            else if (bIns->op == "-")
                newVal = getNumberValue(lOpVal->number - rOpVal->number);
            else if (bIns->op == "*")
                newVal = getNumberValue(lOpVal->number * rOpVal->number);
            else if (bIns->op == "/")
                newVal = getNumberValue(lOpVal->number / rOpVal->number);
            else if (bIns->op == "%")
                newVal = getNumberValue(lOpVal->number % rOpVal->number);
            else if (bIns->op == ">")
                newVal = getNumberValue(lOpVal->number > rOpVal->number);
            else if (bIns->op == "<")
                newVal = getNumberValue(lOpVal->number < rOpVal->number);
            else if (bIns->op == "<=")
                newVal = getNumberValue(lOpVal->number <= rOpVal->number);
            else if (bIns->op == ">=")
                newVal = getNumberValue(lOpVal->number >= rOpVal->number);
            else if (bIns->op == "==")
                newVal = getNumberValue(lOpVal->number == rOpVal->number);
            else if (bIns->op == "!=")
                newVal = getNumberValue(lOpVal->number != rOpVal->number);
            else if (bIns->op == "&&")
                newVal = getNumberValue((int)((unsigned)lOpVal->number & (unsigned)rOpVal->number));
            else if (bIns->op == "||")
                newVal = getNumberValue((int)((unsigned)lOpVal->number | (unsigned)rOpVal->number));
            else
            {
                cerr << "Error occurs in process constant folding: undefined operator '" + bIns->op + "'." << endl;
                return;
            }
        }
        else if (bIns->lhs->valueType == ValueType::NUMBER)  // 二元操作，左操作数为常量
        {
            shared_ptr<NumberValue> lOpVal = s_p_c<NumberValue>(bIns->lhs);
            if (lOpVal->number == 0)  // 左操作数为0
            {
                if (bIns->op == "*" || bIns->op == "/" || bIns->op == "%" || bIns->op == "&&")  // 这些操作符都会使结果为0
                    newVal = getNumberValue(0);
                else if (bIns->op == "+" || bIns->op == "||")  // 这些操作符都会使结果等于右操作数
                    newVal = bIns->rhs;
                else if (bIns->op == "-")  // 为-操作，则创建一个新的一元负操作值
                {
                    newVal = make_shared<UnaryInstruction>(negOp, bIns->rhs, bIns->block);
                    bIns->rhs->users.insert(newVal);
                    maintainLeftValue(newVal, bIns->rhs);
                    replace = true;
                }
                else if (ins->type == InstructionType::CMP && (bIns->op == ">" || bIns->op == "<" || bIns->op == "<=" || bIns->op == ">="))  // 比较操作
                {
                    bIns->op = bIns->swapOpConst (bIns->op);    // 操作符取反
                    shared_ptr<Value> temp = bIns->lhs;    // 左右操作值交换
                    bIns->lhs = bIns->rhs;
                    bIns->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else if (lOpVal->number == 1)  // 左操作数为1
            {
                if (bIns->op == "*")  // 这些操作符都会使结果等于右操作数
                    newVal = bIns->rhs;
                else if (ins->type == InstructionType::CMP && (bIns->op == ">" || bIns->op == "<" || bIns->op == "<=" || bIns->op == ">="))  // 比较操作
                {
                    bIns->op = bIns->swapOpConst(bIns->op);    // 操作符取反
                    shared_ptr<Value> temp = bIns->lhs;   // 左右操作值交换
                    bIns->lhs = bIns->rhs;
                    bIns->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else if (lOpVal->number == -1)  // 左操作数为1
            {
                if (bIns->op == "*")  // 为*操作，则创建一个新的一元负操作值
                {
                    newVal = make_shared<UnaryInstruction>(negOp, bIns->rhs, bIns->block);
                    bIns->rhs->users.insert(newVal);
                    maintainLeftValue(newVal, bIns->rhs);
                    replace = true;
                }
                else if (ins->type == InstructionType::CMP && (bIns->op == ">" || bIns->op == "<" || bIns->op == "<=" || bIns->op == ">="))
                {
                    bIns->op = bIns->swapOpConst(bIns->op);
                    shared_ptr<Value> temp = bIns->lhs;
                    bIns->lhs = bIns->rhs;
                    bIns->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else  // 不是特殊值，仅比较操作，将常数交换到右操作数
            {
                if (ins->type == InstructionType::CMP && (bIns->op == ">" || bIns->op == "<" || bIns->op == "<=" || bIns->op == ">="))
                {
                    bIns->op = bIns->swapOpConst(bIns->op);
                    shared_ptr<Value> temp = bIns->lhs;
                    bIns->lhs = bIns->rhs;
                    bIns->rhs = temp;
                    return;
                }
                else
                    return;
            }
        }
        else if (bIns->rhs->valueType == ValueType::NUMBER)  // 二元操作，左操作数为常量
        {
            shared_ptr<NumberValue> rOpVal = s_p_c<NumberValue>(bIns->rhs);
            if (rOpVal->number == 0)   // 右操作数为0
            {
                if (bIns->op == "*" || bIns->op == "&&")  // 这些操作符都会使结果为0
                    newVal = getNumberValue(0);
                else if (bIns->op == "+" || bIns->op == "||" || bIns->op == "-")  // 这些操作符都会使结果等于左操作数
                    newVal = bIns->lhs;
                else
                    return;
            }
            else if (rOpVal->number == 1)   // 右操作数为1
            {
                if (bIns->op == "*" || bIns->op == "/")   // 这些操作符都会使结果等于左操作数
                    newVal = bIns->lhs;
                else if (bIns->op == "%")  // 这些操作符都会使结果为0
                    newVal = getNumberValue(0);
                else
                    return;
            }
            else if (rOpVal->number == -1)   // 右操作数为-1
            {
                if (bIns->op == "*") // 为*操作，则创建一个新的一元负操作值
                {
                    newVal = make_shared<UnaryInstruction>(negOp, bIns->lhs, bIns->block);
                    bIns->lhs->users.insert(newVal);
                    maintainLeftValue(newVal, bIns->lhs);
                    replace = true;
                }
                else
                    return;
            }
            else
                return;
        }
        else   // 左右操作数均不为常量
        {
            if ((bIns->op == "-" || bIns->op == "%") && bIns->lhs == bIns->rhs)   // 当左右操作数一样，则结果为0
            {
                newVal = getNumberValue(0);
            }
            else if (bIns->op == "/" && bIns->lhs == bIns->rhs)   // 当左右操作数一样，则结果为1
            {
                newVal = getNumberValue(1);
            }
            else if (bIns->op == "-" && dynamic_cast<UnaryInstruction *>(bIns->rhs.get()))
            {
                if (s_p_c<UnaryInstruction>(bIns->rhs)->op == "-")   // 右操作数有取负，且操作符为-，则右操作数提出值
                {
                    bIns->op = "+";
                    shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction>(bIns->rhs);
                    bIns->rhs = unary->value;
                    unary->users.erase(bIns);
                    bIns->rhs->users.insert(bIns);
                    return;
                }
                else
                    return;
            }
            else if (bIns->op == "+" && dynamic_cast<UnaryInstruction *>(bIns->rhs.get()))
            {
                if (s_p_c<UnaryInstruction>(bIns->rhs)->op == "-") // 操作符为+，右操作数有取负，左右操作相等，则结果0
                {
                    shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction>(bIns->rhs);
                    if (unary->value == bIns->lhs)
                    {
                        newVal = getNumberValue(0);
                    }
                    else
                        return;
                }
                else
                    return;
            }
            else if (bIns->op == "+" && dynamic_cast<UnaryInstruction *>(bIns->lhs.get()))
            {
                if (s_p_c<UnaryInstruction>(bIns->lhs)->op == "-") // 操作符为+，左操作数有取负，左右操作相等，则结果0
                {
                    shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction>(bIns->lhs);
                    if (unary->value == bIns->rhs)
                    {
                        newVal = getNumberValue(0);
                    }
                    else
                        return;
                }
                else
                    return;
            }
            else
                return;
        }
        break;
    }
    case InstructionType::UNARY:   // 一元操作
    {
        shared_ptr<UnaryInstruction> uIns = s_p_c<UnaryInstruction>(ins);
        if (uIns->op == "+")   // 正号，直接提取值
            newVal = uIns->value;
        else if (uIns->value->valueType == ValueType::NUMBER)  // 为常数
        {
            shared_ptr<NumberValue> value = s_p_c<NumberValue>(uIns->value);
            if (uIns->op == "-")
                newVal = getNumberValue(-value->number);  // 负号变为负数
            else if (uIns->op == notOp)
                newVal = getNumberValue(!value->number);  // 非号取非
            else
            {
                cerr << "Error occurs in process constant folding: undefined operator '" + uIns->op + "'." << endl;
                return;
            }
        }
        else if (dynamic_cast<UnaryInstruction *>(uIns->value.get()))  // 一元操作里还是一元操作
        {
            shared_ptr<UnaryInstruction> value = s_p_c<UnaryInstruction>(uIns->value);
            if (uIns->op == "-" && value->op == "-")  // 两个均为负号
                newVal = value->value;
            else if (uIns->op == "!" && value->op == "!")  // 两个均为非
                newVal = value->value;
            else if (uIns->op == "!")  // 外层非，里层+-无所谓
            {
                newVal = make_shared<UnaryInstruction>(uIns->op, value->value, uIns->block);
            }
            else
                return;
        }
        else
            return;
        break;
    }
    case InstructionType::LOAD:   // 加载操作
    {
        shared_ptr<LoadInstruction> lIns = s_p_c<LoadInstruction>(ins);
        if (lIns->address->valueType == ValueType::CONSTANT && lIns->offset->valueType == ValueType::NUMBER)  // 地址为const array，偏移量为常数
        {
            shared_ptr<ConstantValue> constArray = s_p_c<ConstantValue>(lIns->address);
            shared_ptr<NumberValue> offsetNumber = s_p_c<NumberValue>(lIns->offset);
            if (constArray->values.count(offsetNumber->number) != 0)
                newVal = getNumberValue(constArray->values.at(offsetNumber->number));  // 直接提取const array中的值
            else
                newVal = getNumberValue(0);
        }
        else
            return;
        break;
    }
    case InstructionType::PHI:   // phi
    {
        shared_ptr<PhiInstruction> pIns = s_p_c<PhiInstruction>(ins);
        newVal = removeTrivialPhi(pIns);   // 去除不重要的phi
        if (newVal == pIns)
            return;
        break;
    }
    default:
        return;
    }
    if (replace)   // 创建了一个新的值，并且需要替换指令
    {
        for (auto &it : ins->block->instructions)
        {
            if (it == ins)
                it = s_p_c<Instruction>(newVal);
        }
    }
    unordered_set<shared_ptr<Value>> users = insVal->users;
    for (auto &it : users)
    {
        it->replaceUse(insVal, newVal);
    }
    insVal->abandonUse();
    for (auto &it : users)
    {
        if (it->valueType == ValueType::INSTRUCTION)   // 最终运行的结果也是一个值，递归折叠
        {
            shared_ptr<Instruction> itIns = s_p_c<Instruction>(it);
            fold(itIns);
        }
    }
}

/**
 * @brief 常量折叠  直接计算出可以被计算的值，作为常量
 * @param module 
 */
void constantFolding(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            for (auto &ins : bb->instructions)
            {
                if (!ins->users.empty())
                    fold(ins);
            }
            removeUnusedInstructions(bb);
        }
    }
}
