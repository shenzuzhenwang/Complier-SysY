#include "ir_optimize.h"

void blockCommonSubexpressionElimination(shared_ptr<BasicBlock> &bb);

/**
 * @brief 局部公共子表达式删除   非自己写
 * @param module 
 */
void localCommonSubexpressionElimination(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            blockCommonSubexpressionElimination(bb);
        }
    }
}

/**
 * @brief 主要在于 对比两个指令的表达式是否相同（hashCode），以及对比相同表达式的两个值相同（equals）
 * @param bb 
 */
void blockCommonSubexpressionElimination(shared_ptr<BasicBlock> &bb)
{
    unordered_map<unsigned long long, unordered_set<shared_ptr<Value>>> hashMap;
    for (auto it = bb->instructions.begin(); it != bb->instructions.end();)
    {
        shared_ptr<Instruction> ins = *it;
        if (ins->type == BINARY || ins->type == UNARY)  // 块中的一元二元运算指令
        {
            unsigned long long hashCode = ins->hashCode();  // 相同的表达式，应有相同的hashCode
            if (hashMap.count(hashCode) != 0)
            {
                unordered_set<shared_ptr<Value>> tempSet = hashMap.at(hashCode);
                bool replace = false;
                for (auto i : tempSet)
                {
                    if (ins->equals(i))  // 当两个的值相同
                    {
                        replace = true;
                        if (i->valueType != INSTRUCTION)
                        {
                            cerr << "Error occurs in process LCSE: non-instruction value in map." << endl;
                        }
                        shared_ptr<Instruction> insInMap = s_p_c<Instruction>(i);
                        if (insInMap->resultType == R_VAL_RESULT)
                        {
                            insInMap->resultType = L_VAL_RESULT;
                            insInMap->caughtVarName = generateTempLeftValueName();
                        }
                        unordered_set<shared_ptr<Value>> users = ins->users;  // 将ins转换i指令
                        shared_ptr<Value> toBeReplaced = ins;
                        for (auto &user : users)
                        {
                            user->replaceUse(toBeReplaced, i);
                        }
                        ins->abandonUse();
                        it = bb->instructions.erase(it);
                        break;
                    }
                }
                if (!replace)
                    ++it;
            }
            else
            {
                unordered_set<shared_ptr<Value>> newSet;
                newSet.insert(ins);
                hashMap[hashCode] = newSet;
                ++it;
            }
        }
        else
            ++it;
    }
}
