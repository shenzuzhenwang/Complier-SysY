#include "ir_optimize.h"

string negOp = "-"; 
string notOp = "!"; 

/**
 * @brief ԭֵΪ��ֵ�����´�����ҲΪ��ֵ
 * @param newVal �´�����ֵ
 * @param oldVal ԭֵ
 */
void maintainLeftValue(shared_ptr<Value> &newVal, shared_ptr<Value> &oldVal)
{
    if (oldVal->valueType == ValueType::INSTRUCTION && s_p_c<Instruction>(oldVal)->resultType == L_VAL_RESULT) // oldValΪ��ֵ��newҲΪͬ������
    {
        s_p_c<Instruction>(newVal)->resultType = L_VAL_RESULT;
        s_p_c<Instruction>(newVal)->caughtVarName = s_p_c<Instruction>(oldVal)->caughtVarName;
    }
}

/**
 * @brief ����ָ���п����۵����������۵�
 * @param ins ��ָ��
 */
void fold(shared_ptr<Instruction> &ins)
{
    shared_ptr<Value> newVal;
    shared_ptr<Value> insVal = ins;
    bool replace = false;
    switch (ins->type)
    {
    case InstructionType::BINARY:
    case InstructionType::CMP:  // ����������
    {
        shared_ptr<BinaryInstruction> bIns = s_p_c<BinaryInstruction>(ins);
        if (bIns->lhs->valueType == ValueType::NUMBER && bIns->rhs->valueType == ValueType::NUMBER)  // ����������Ϊ��������ֱ�ӵó����
        {
            shared_ptr<NumberValue> lOpVal = s_p_c<NumberValue>(bIns->lhs);
            shared_ptr<NumberValue> rOpVal = s_p_c<NumberValue>(bIns->rhs);
            if ((bIns->op == "/" || bIns->op == "%") && rOpVal->number == 0)  // �������
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
        else if (bIns->lhs->valueType == ValueType::NUMBER)  // ��Ԫ�������������Ϊ����
        {
            shared_ptr<NumberValue> lOpVal = s_p_c<NumberValue>(bIns->lhs);
            if (lOpVal->number == 0)  // �������Ϊ0
            {
                if (bIns->op == "*" || bIns->op == "/" || bIns->op == "%" || bIns->op == "&&")  // ��Щ����������ʹ���Ϊ0
                    newVal = getNumberValue(0);
                else if (bIns->op == "+" || bIns->op == "||")  // ��Щ����������ʹ��������Ҳ�����
                    newVal = bIns->rhs;
                else if (bIns->op == "-")  // Ϊ-�������򴴽�һ���µ�һԪ������ֵ
                {
                    newVal = make_shared<UnaryInstruction>(negOp, bIns->rhs, bIns->block);
                    bIns->rhs->users.insert(newVal);
                    maintainLeftValue(newVal, bIns->rhs);
                    replace = true;
                }
                else if (ins->type == InstructionType::CMP && (bIns->op == ">" || bIns->op == "<" || bIns->op == "<=" || bIns->op == ">="))  // �Ƚϲ���
                {
                    bIns->op = bIns->swapOpConst (bIns->op);    // ������ȡ��
                    shared_ptr<Value> temp = bIns->lhs;    // ���Ҳ���ֵ����
                    bIns->lhs = bIns->rhs;
                    bIns->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else if (lOpVal->number == 1)  // �������Ϊ1
            {
                if (bIns->op == "*")  // ��Щ����������ʹ��������Ҳ�����
                    newVal = bIns->rhs;
                else if (ins->type == InstructionType::CMP && (bIns->op == ">" || bIns->op == "<" || bIns->op == "<=" || bIns->op == ">="))  // �Ƚϲ���
                {
                    bIns->op = bIns->swapOpConst(bIns->op);    // ������ȡ��
                    shared_ptr<Value> temp = bIns->lhs;   // ���Ҳ���ֵ����
                    bIns->lhs = bIns->rhs;
                    bIns->rhs = temp;
                    return;
                }
                else
                    return;
            }
            else if (lOpVal->number == -1)  // �������Ϊ1
            {
                if (bIns->op == "*")  // Ϊ*�������򴴽�һ���µ�һԪ������ֵ
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
            else  // ��������ֵ�����Ƚϲ������������������Ҳ�����
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
        else if (bIns->rhs->valueType == ValueType::NUMBER)  // ��Ԫ�������������Ϊ����
        {
            shared_ptr<NumberValue> rOpVal = s_p_c<NumberValue>(bIns->rhs);
            if (rOpVal->number == 0)   // �Ҳ�����Ϊ0
            {
                if (bIns->op == "*" || bIns->op == "&&")  // ��Щ����������ʹ���Ϊ0
                    newVal = getNumberValue(0);
                else if (bIns->op == "+" || bIns->op == "||" || bIns->op == "-")  // ��Щ����������ʹ��������������
                    newVal = bIns->lhs;
                else
                    return;
            }
            else if (rOpVal->number == 1)   // �Ҳ�����Ϊ1
            {
                if (bIns->op == "*" || bIns->op == "/")   // ��Щ����������ʹ��������������
                    newVal = bIns->lhs;
                else if (bIns->op == "%")  // ��Щ����������ʹ���Ϊ0
                    newVal = getNumberValue(0);
                else
                    return;
            }
            else if (rOpVal->number == -1)   // �Ҳ�����Ϊ-1
            {
                if (bIns->op == "*") // Ϊ*�������򴴽�һ���µ�һԪ������ֵ
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
        else   // ���Ҳ���������Ϊ����
        {
            if ((bIns->op == "-" || bIns->op == "%") && bIns->lhs == bIns->rhs)   // �����Ҳ�����һ��������Ϊ0
            {
                newVal = getNumberValue(0);
            }
            else if (bIns->op == "/" && bIns->lhs == bIns->rhs)   // �����Ҳ�����һ��������Ϊ1
            {
                newVal = getNumberValue(1);
            }
            else if (bIns->op == "-" && dynamic_cast<UnaryInstruction *>(bIns->rhs.get()))
            {
                if (s_p_c<UnaryInstruction>(bIns->rhs)->op == "-")   // �Ҳ�������ȡ�����Ҳ�����Ϊ-�����Ҳ��������ֵ
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
                if (s_p_c<UnaryInstruction>(bIns->rhs)->op == "-") // ������Ϊ+���Ҳ�������ȡ�������Ҳ�����ȣ�����0
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
                if (s_p_c<UnaryInstruction>(bIns->lhs)->op == "-") // ������Ϊ+�����������ȡ�������Ҳ�����ȣ�����0
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
    case InstructionType::UNARY:   // һԪ����
    {
        shared_ptr<UnaryInstruction> uIns = s_p_c<UnaryInstruction>(ins);
        if (uIns->op == "+")   // ���ţ�ֱ����ȡֵ
            newVal = uIns->value;
        else if (uIns->value->valueType == ValueType::NUMBER)  // Ϊ����
        {
            shared_ptr<NumberValue> value = s_p_c<NumberValue>(uIns->value);
            if (uIns->op == "-")
                newVal = getNumberValue(-value->number);  // ���ű�Ϊ����
            else if (uIns->op == notOp)
                newVal = getNumberValue(!value->number);  // �Ǻ�ȡ��
            else
            {
                cerr << "Error occurs in process constant folding: undefined operator '" + uIns->op + "'." << endl;
                return;
            }
        }
        else if (dynamic_cast<UnaryInstruction *>(uIns->value.get()))  // һԪ�����ﻹ��һԪ����
        {
            shared_ptr<UnaryInstruction> value = s_p_c<UnaryInstruction>(uIns->value);
            if (uIns->op == "-" && value->op == "-")  // ������Ϊ����
                newVal = value->value;
            else if (uIns->op == "!" && value->op == "!")  // ������Ϊ��
                newVal = value->value;
            else if (uIns->op == "!")  // ���ǣ����+-����ν
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
    case InstructionType::LOAD:   // ���ز���
    {
        shared_ptr<LoadInstruction> lIns = s_p_c<LoadInstruction>(ins);
        if (lIns->address->valueType == ValueType::CONSTANT && lIns->offset->valueType == ValueType::NUMBER)  // ��ַΪconst array��ƫ����Ϊ����
        {
            shared_ptr<ConstantValue> constArray = s_p_c<ConstantValue>(lIns->address);
            shared_ptr<NumberValue> offsetNumber = s_p_c<NumberValue>(lIns->offset);
            if (constArray->values.count(offsetNumber->number) != 0)
                newVal = getNumberValue(constArray->values.at(offsetNumber->number));  // ֱ����ȡconst array�е�ֵ
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
        newVal = removeTrivialPhi(pIns);   // ȥ������Ҫ��phi
        if (newVal == pIns)
            return;
        break;
    }
    default:
        return;
    }
    if (replace)   // ������һ���µ�ֵ��������Ҫ�滻ָ��
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
        if (it->valueType == ValueType::INSTRUCTION)   // �������еĽ��Ҳ��һ��ֵ���ݹ��۵�
        {
            shared_ptr<Instruction> itIns = s_p_c<Instruction>(it);
            fold(itIns);
        }
    }
}

/**
 * @brief �����۵�  ֱ�Ӽ�������Ա������ֵ����Ϊ����
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
