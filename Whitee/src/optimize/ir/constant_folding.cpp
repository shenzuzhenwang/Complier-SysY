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
    if (oldVal->value_type == ValueType::INSTRUCTION && s_p_c<Instruction>(oldVal)->resultType == L_VAL_RESULT) // oldVal为左值，new也为同名变量
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
    shared_ptr<Value> newv;
    shared_ptr<Value> val = ins;
    bool replace = false;
    switch (ins->type)
    {
    case InstructionType::BINARY:
    case InstructionType::CMP:  // 两个操作数
    {
        shared_ptr<BinaryInstruction> b_ins = s_p_c<BinaryInstruction>(ins);
        if (b_ins->lhs->value_type == ValueType::NUMBER && b_ins->rhs->value_type == ValueType::NUMBER)  // 两操作数均为常量，则直接得出结果
        {
            shared_ptr<NumberValue> l_val = s_p_c<NumberValue>(b_ins->lhs);
            shared_ptr<NumberValue> r_val = s_p_c<NumberValue>(b_ins->rhs);
            if ((b_ins->op == "/" || b_ins->op == "%") && r_val->number == 0)  // 除零操作
            {
                cerr << "Error occurs in process constant folding: divide 0." << endl;
                return;
            }
            if (b_ins->op == "+")
                newv = Number(l_val->number + r_val->number);
            else if (b_ins->op == "-")
                newv = Number(l_val->number - r_val->number);
            else if (b_ins->op == "*")
                newv = Number(l_val->number * r_val->number);
            else if (b_ins->op == "/")
                newv = Number(l_val->number / r_val->number);
            else if (b_ins->op == "%")
                newv = Number(l_val->number % r_val->number);
            else if (b_ins->op == ">")
                newv = Number(l_val->number > r_val->number);
            else if (b_ins->op == "<")
                newv = Number(l_val->number < r_val->number);
            else if (b_ins->op == "<=")
                newv = Number(l_val->number <= r_val->number);
            else if (b_ins->op == ">=")
                newv = Number(l_val->number >= r_val->number);
            else if (b_ins->op == "==")
                newv = Number(l_val->number == r_val->number);
            else if (b_ins->op == "!=")
                newv = Number(l_val->number != r_val->number);
            else if (b_ins->op == "&&")
                newv = Number((int)((unsigned)l_val->number & (unsigned)r_val->number));
            else if (b_ins->op == "||")
                newv = Number((int)((unsigned)l_val->number | (unsigned)r_val->number));
            else
            {
                cerr << "Error occurs in process constant folding: undefined operator '" + b_ins->op + "'." << endl;
                return;
            }
        }
        else if (b_ins->lhs->value_type == ValueType::NUMBER)  // 二元操作，左操作数为常量
        {
            shared_ptr<NumberValue> l_val = s_p_c<NumberValue>(b_ins->lhs);
            if (l_val->number == 0)  // 左操作数为0
            {
                if (b_ins->op == "*" || b_ins->op == "/" || b_ins->op == "%" || b_ins->op == "&&")  // 这些操作符都会使结果为0
                    newv = Number(0);
                else if (b_ins->op == "+" || b_ins->op == "||")  // 这些操作符都会使结果等于右操作数
                    newv = b_ins->rhs;
                else if (b_ins->op == "-")  // 为-操作，则创建一个新的一元负操作值
                {
                    newv = make_shared<UnaryInstruction>(negOp, b_ins->rhs, b_ins->block);
                    b_ins->rhs->users.insert(newv);
                    maintainLeftValue(newv, b_ins->rhs);  // ？？
                    replace = true;
                }
                else if (ins->type == InstructionType::CMP && (b_ins->op == ">" || b_ins->op == "<" || b_ins->op == "<=" || b_ins->op == ">="))  // 比较操作
                {
                    b_ins->op = b_ins->swapOpConst (b_ins->op);    // 操作符取反
                    shared_ptr<Value> temp = b_ins->lhs;    // 左右操作值交换
                    b_ins->lhs = b_ins->rhs;
                    b_ins->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else if (l_val->number == 1)  // 左操作数为1
            {
                if (b_ins->op == "*")  // 这些操作符都会使结果等于右操作数
                    newv = b_ins->rhs;
                else if (ins->type == InstructionType::CMP && (b_ins->op == ">" || b_ins->op == "<" || b_ins->op == "<=" || b_ins->op == ">="))  // 比较操作
                {
                    b_ins->op = b_ins->swapOpConst(b_ins->op);    // 操作符取反
                    shared_ptr<Value> temp = b_ins->lhs;   // 左右操作值交换
                    b_ins->lhs = b_ins->rhs;
                    b_ins->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else if (l_val->number == -1)  // 左操作数为1
            {
                if (b_ins->op == "*")  // 为*操作，则创建一个新的一元负操作值
                {
                    newv = make_shared<UnaryInstruction>(negOp, b_ins->rhs, b_ins->block);
                    b_ins->rhs->users.insert(newv);
                    maintainLeftValue(newv, b_ins->rhs);
                    replace = true;
                }
                else if (ins->type == InstructionType::CMP && (b_ins->op == ">" || b_ins->op == "<" || b_ins->op == "<=" || b_ins->op == ">="))
                {
                    b_ins->op = b_ins->swapOpConst(b_ins->op);
                    shared_ptr<Value> temp = b_ins->lhs;
                    b_ins->lhs = b_ins->rhs;
                    b_ins->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else  // 不是特殊值，仅比较操作，将常数交换到右操作数
            {
                if (ins->type == InstructionType::CMP && (b_ins->op == ">" || b_ins->op == "<" || b_ins->op == "<=" || b_ins->op == ">="))
                {
                    b_ins->op = b_ins->swapOpConst(b_ins->op);
                    shared_ptr<Value> temp = b_ins->lhs;
                    b_ins->lhs = b_ins->rhs;
                    b_ins->rhs = temp;
                    return;
                }
                else
                    return;
            }
        }
        else if (b_ins->rhs->value_type == ValueType::NUMBER)  // 二元操作，左操作数为常量
        {
            shared_ptr<NumberValue> r_val = s_p_c<NumberValue>(b_ins->rhs);
            if (r_val->number == 0)   // 右操作数为0
            {
                if (b_ins->op == "*" || b_ins->op == "&&")  // 这些操作符都会使结果为0
                    newv = Number(0);
                else if (b_ins->op == "+" || b_ins->op == "||" || b_ins->op == "-")  // 这些操作符都会使结果等于左操作数
                    newv = b_ins->lhs;
                else
                    return;
            }
            else if (r_val->number == 1)   // 右操作数为1
            {
                if (b_ins->op == "*" || b_ins->op == "/")   // 这些操作符都会使结果等于左操作数
                    newv = b_ins->lhs;
                else if (b_ins->op == "%")  // 这些操作符都会使结果为0
                    newv = Number(0);
                else
                    return;
            }
            else if (r_val->number == -1)   // 右操作数为-1
            {
                if (b_ins->op == "*") // 为*操作，则创建一个新的一元负操作值
                {
                    newv = make_shared<UnaryInstruction>(negOp, b_ins->lhs, b_ins->block);
                    b_ins->lhs->users.insert(newv);
                    maintainLeftValue(newv, b_ins->lhs);
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
            if ((b_ins->op == "-" || b_ins->op == "%") && b_ins->lhs == b_ins->rhs)   // 当左右操作数一样，则结果为0
            {
                newv = Number(0);
            }
            else if (b_ins->op == "/" && b_ins->lhs == b_ins->rhs)   // 当左右操作数一样，则结果为1
            {
                newv = Number(1);
            }
            else if (b_ins->op == "-" && dynamic_cast<UnaryInstruction *>(b_ins->rhs.get()))
            {
                if (s_p_c<UnaryInstruction>(b_ins->rhs)->op == "-")   // 右操作数有取负，且操作符为-，则右操作数提出值
                {
                    b_ins->op = "+";
                    shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction>(b_ins->rhs);
                    b_ins->rhs = unary->value;
                    unary->users.erase(b_ins);
                    b_ins->rhs->users.insert(b_ins);
                    return;
                }
                else
                    return;
            }
            else if (b_ins->op == "+" && dynamic_cast<UnaryInstruction *>(b_ins->rhs.get()))
            {
                if (s_p_c<UnaryInstruction>(b_ins->rhs)->op == "-") // 操作符为+，右操作数有取负，左右操作相等，则结果0
                {
                    shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction>(b_ins->rhs);
                    if (unary->value == b_ins->lhs)
                    {
                        newv = Number(0);
                    }
                    else
                        return;
                }
                else
                    return;
            }
            else if (b_ins->op == "+" && dynamic_cast<UnaryInstruction *>(b_ins->lhs.get()))
            {
                if (s_p_c<UnaryInstruction>(b_ins->lhs)->op == "-") // 操作符为+，左操作数有取负，左右操作相等，则结果0
                {
                    shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction>(b_ins->lhs);
                    if (unary->value == b_ins->rhs)
                    {
                        newv = Number(0);
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
        shared_ptr<UnaryInstruction> u_ins = s_p_c<UnaryInstruction>(ins);
        if (u_ins->op == "+")   // 正号，直接提取值
            newv = u_ins->value;
        else if (u_ins->value->value_type == ValueType::NUMBER)  // 为常数
        {
            shared_ptr<NumberValue> value = s_p_c<NumberValue>(u_ins->value);
            if (u_ins->op == "-")
                newv = Number(-value->number);  // 负号变为负数
            else if (u_ins->op == notOp)
                newv = Number(!value->number);  // 非号取非
            else
            {
                cerr << "Error occurs in process constant folding: undefined operator '" + u_ins->op + "'." << endl;
                return;
            }
        }
        else if (dynamic_cast<UnaryInstruction *>(u_ins->value.get()))  // 一元操作里还是一元操作
        {
            shared_ptr<UnaryInstruction> value = s_p_c<UnaryInstruction>(u_ins->value);
            if (u_ins->op == "-" && value->op == "-")  // 两个均为负号
                newv = value->value;
            else if (u_ins->op == "!" && value->op == "!")  // 两个均为非
                newv = value->value;
            else if (u_ins->op == "!")  // 外层非，里层+-无所谓
            {
                newv = make_shared<UnaryInstruction>(u_ins->op, value->value, u_ins->block);
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
        shared_ptr<LoadInstruction> l_ins = s_p_c<LoadInstruction>(ins);
        if (l_ins->address->value_type == ValueType::CONSTANT && l_ins->offset->value_type == ValueType::NUMBER)  // 地址为const array，偏移量为常数
        {
            shared_ptr<ConstantValue> const_array = s_p_c<ConstantValue>(l_ins->address);
            shared_ptr<NumberValue> offset_number = s_p_c<NumberValue>(l_ins->offset);
            if (const_array->values.count(offset_number->number) != 0)
                newv = Number(const_array->values.at(offset_number->number));  // 直接提取const array中的值
            else
                newv = Number(0);
        }
        else
            return;
        break;
    }
    case InstructionType::PHI:   // phi
    {
        shared_ptr<PhiInstruction> pIns = s_p_c<PhiInstruction>(ins);
        newv = remove_trivial_phi(pIns);   // 去除不重要的phi
        if (newv == pIns)
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
                it = s_p_c<Instruction>(newv);
        }
    }
    unordered_set<shared_ptr<Value>> users = val->users;
    for (auto &it : users)
    {
        it->replaceUse(val, newv);
    }
    val->abandonUse();
    for (auto &it : users)
    {
        if (it->value_type == ValueType::INSTRUCTION)   // 最终运行的结果也是一个值，递归折叠
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
void constant_folding(shared_ptr<Module> &module)
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
            unused_instruction_delete(bb);
        }
    }
}
