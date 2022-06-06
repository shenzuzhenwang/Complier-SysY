/*********************************************************************
 * @file   ir_ssa.cpp
 * @brief  SSA静态单赋值策略
 * 
 * @author 神祖
 * @date   June 2022
 *********************************************************************/
#include "ir_ssa.h"

#include <iostream>

// https://zhuanlan.zhihu.com/p/360692294

shared_ptr<Value> readLocalVariableRecursively(shared_ptr<BasicBlock> &bb, string &varName);

/**
 * @brief 从一个块的SSA MAP中读取变量
 * @param bb 变量所在的块
 * @param varName 变量的名字
 * @return 变量的值
 */
shared_ptr<Value> readLocalVariable(shared_ptr<BasicBlock> &bb, string &varName)
{
    if (bb->localVarSsaMap.count(varName) != 0)
        return bb->localVarSsaMap.at(varName);
    return readLocalVariableRecursively(bb, varName);
}

/**
 * @brief 将变量写入块的SSA MAP
 * @param bb 变量所在的块
 * @param varName 变量的名字
 * @param value 给变量写入的值
 */
void writeLocalVariable(shared_ptr<BasicBlock> &bb, const string &varName, const shared_ptr<Value> &value)
{
    bb->localVarSsaMap[varName] = value;
    if (dynamic_cast<PhiInstruction *>(value.get()))
    {
        value->users.insert(bb);
    }
}

/**
 * @brief 递归向前驱的块寻找变量
 * @param bb 开始寻找的块
 * @param varName 变量的名字
 * @return 变量的值
 */
shared_ptr<Value> readLocalVariableRecursively(shared_ptr<BasicBlock> &bb, string &varName)
{
    if (!bb->sealed)  // 块不封闭，仅存在于循环体
    {
        shared_ptr<PhiInstruction> emptyPhi = make_shared<PhiInstruction>(varName, bb);
        bb->incompletePhis[varName] = emptyPhi;
        writeLocalVariable(bb, varName, emptyPhi);
        return emptyPhi;
    }
    else if (bb->predecessors.size() == 1)  // 只有一个前驱的情况：不需要 phi
    {
        shared_ptr<BasicBlock> predecessor = *(bb->predecessors.begin());
        shared_ptr<Value> val = readLocalVariable(predecessor, varName);
        writeLocalVariable(bb, varName, val);
        return val;
    }
    else  // 用无操作数的 phi 破坏潜在的循环
    {
        shared_ptr<Value> val = make_shared<PhiInstruction>(varName, bb);
        writeLocalVariable(bb, varName, val);
        shared_ptr<PhiInstruction> phi = s_p_c<PhiInstruction>(val);
        bb->phis.insert(phi);
        val = addPhiOperands(bb, varName, phi);
        writeLocalVariable(bb, varName, val);
        return val;
    }
}

/**
 * @brief 加入变量的phi的操作数
 * @param bb phi所在的块
 * @param varName 变量的名字
 * @param phi phi对象
 * @return 
 */
shared_ptr<Value> addPhiOperands(shared_ptr<BasicBlock> &bb, string &varName, shared_ptr<PhiInstruction> &phi)
{
    for (auto &it : bb->predecessors)  // 从前驱中确定操作数
    {
        shared_ptr<BasicBlock> pred = it;
        shared_ptr<Value> v = readLocalVariable(pred, varName);
        shared_ptr<BasicBlock> block;
        phi->operands.insert({it, v});
        v->users.insert(phi);
    }
    return removeTrivialPhi(phi);
}

/**
 * @brief 去除不重要的phi
 * @param phi 
 * @return 
 */
shared_ptr<Value> removeTrivialPhi(shared_ptr<PhiInstruction> &phi)
{
    shared_ptr<Value> same;
    shared_ptr<Value> self = phi->shared_from_this();
    for (auto &it : phi->operands)     // 操作数为自己和另一个值的时候，此phi可以用另一个值代替
    {
        if (it.second == same || it.second == self)  // 只有一个唯一值或引用自己
            continue;
        if (same != nullptr)  // phi有两个非自己的操作数，重要
            return phi;
        same = it.second;
    }
    if (same == nullptr)     // 不可达或在开始块中
        same = make_shared<UndefinedValue>(phi->localVarName);
    phi->users.erase(phi);    // 找出所有使用这个 phi 的值，除了它本身
    unordered_set<shared_ptr<Value>> users = phi->users;
    shared_ptr<Value> toBeReplaced = phi;
    phi->block->phis.erase(phi);
    for (auto &it : users)
    {
        it->replaceUse(toBeReplaced, same);  // 将所有用到 phi 的地方替代为 same 并移除 phi
    }
    if (_isBuildingIr)
    {
        for (auto& it : phi->operands)
        {
            it.second->users.erase (phi);
        }
        phi->valid = false;
    }
    else
    {
        phi->abandonUse ();
    }
    for (auto &it : users)   // 试去递归移除使用此 phi 的 phi 指令，因为它可能变得不重要（trivial）
    {
        if (dynamic_cast<PhiInstruction *>(it.get()))
        {
            shared_ptr<PhiInstruction> use = s_p_c<PhiInstruction>(it);
            shared_ptr<Value> newVal = removeTrivialPhi(use);
            if (use == same)
            {
                same = newVal;
            }
        }
    }
    return same;
}

/**
 * @brief 在前驱查询变量定义 加入变量的phi可能值
 * @param bb 此块
 */
void sealBasicBlock(shared_ptr<BasicBlock> &bb)
{
    if (!bb->sealed)
    {
        for (auto &it : bb->incompletePhis)
        {
            string varName = it.first;
            shared_ptr<PhiInstruction> phi = it.second;
            bb->phis.insert(phi);
            addPhiOperands(bb, varName, phi);
        }
        bb->incompletePhis.clear();
        bb->sealed = true;
    }
    else
    {
        cerr << "Error occurs in process seal basic block: the block is sealed." << endl;
    }
}
