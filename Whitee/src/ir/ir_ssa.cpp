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
// https://www.gqrelic.com/2022/04/07/ssa-book-1/

shared_ptr<Value> read_variable_recursively(shared_ptr<BasicBlock> &bb, string &varName);

/**
 * @brief 从一个块的SSA MAP中读取变量
 * @param bb 变量所在的块
 * @param varName 变量的名字
 * @return 变量的值
 */
shared_ptr<Value> read_variable(shared_ptr<BasicBlock> &block, string &name)
{
    if (block->ssa_map.count(name) != 0)
        return block->ssa_map.at(name);
    return read_variable_recursively(block, name);
}

/**
 * @brief 将变量写入块的SSA MAP
 * @param bb 变量所在的块
 * @param varName 变量的名字
 * @param value 给变量写入的值
 */
void write_variable(shared_ptr<BasicBlock> &block, const string &varName, const shared_ptr<Value> &value)
{
    block->ssa_map[varName] = value;
    if (dynamic_cast<PhiInstruction *>(value.get()))   // 写入phi函数的值，phi被此块使用
    {
        value->users.insert(block);
    }
}

/**
 * @brief 递归向前驱的块寻找变量的值
 * @param bb 开始寻找的块
 * @param varName 变量的名字
 * @return 变量的值
 */
shared_ptr<Value> read_variable_recursively(shared_ptr<BasicBlock> &block, string &name)
{
    if (!block->sealed)  // 块不封闭，仅存在于循环体，此时可能前驱未加入完
    {
        shared_ptr<PhiInstruction> empty_phi = make_shared<PhiInstruction>(name, block);
        block->incomplete_phis[name] = empty_phi;
        write_variable(block, name, empty_phi);
        return empty_phi;
    }
    else if (block->predecessors.size() == 1)  // 只有一个前驱的情况：不需要 phi
    {
        shared_ptr<BasicBlock> predecessor = *(block->predecessors.begin());
        shared_ptr<Value> val = read_variable(predecessor, name);  // 继续递归向前寻找变量的值
        write_variable(block, name, val);  // 找到后写入现在的块
        return val;
    }
    else  // 如果有两个前驱块
    {
        shared_ptr<Value> val = make_shared<PhiInstruction>(name, block);
        write_variable(block, name, val);                   // 先在块中写一个无操作数的phi，为了破坏可能的循环
        shared_ptr<PhiInstruction> phi = s_p_c<PhiInstruction>(val);
        block->phis.insert(phi);

        val = add_phi_operands(block, name, phi);  // 加入操作数
        write_variable(block, name, val);
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
shared_ptr<Value> add_phi_operands(shared_ptr<BasicBlock> &bb, string &varName, shared_ptr<PhiInstruction> &phi)
{
    for (auto &it : bb->predecessors)  // 从前驱中确定操作数
    {
        shared_ptr<BasicBlock> pred = it;
        shared_ptr<Value> v = read_variable(pred, varName);  // 递归向前驱块寻找同名变量的值，可能会由于循环，找到一样phi
        shared_ptr<BasicBlock> block;
        phi->operands.insert({it, v});
        v->users.insert(phi);
    }
    return remove_trivial_phi(phi);  // 由于可能由于循环，phi的操作数的值为phi自己；或是两个操作数相同，此时需要去除phi
}

/**
 * @brief 去除不重要的phi
 * @param phi 
 * @return 
 */
shared_ptr<Value> remove_trivial_phi(shared_ptr<PhiInstruction> &phi)
{
    shared_ptr<Value> same;
    shared_ptr<Value> self = phi->shared_from_this();
    for (auto &it : phi->operands)     // 操作数为自己和另一个值的时候，此phi可以用另一个值代替
    {
        if (it.second == same || it.second == self)  // 两个操作数的值：其中一个为此phi，或两个值相同，此时，需要去除此phi
            continue;
        if (same != nullptr)  // phi有两个非自己的操作数，重要
            return phi;
        same = it.second;
    }
    if (same == nullptr)     // 不可达或在开始块中，无操作数
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
            shared_ptr<Value> new_val = remove_trivial_phi(use);
            if (use == same)
            {
                same = new_val;
            }
        }
    }
    return same;
}

/**
 * @brief 在前驱查询变量定义 加入变量的phi可能值
 * @param bb 此块
 */
void seal_basic_block(shared_ptr<BasicBlock> &bb)
{
    if (!bb->sealed)
    {
        for (auto &it : bb->incomplete_phis)
        {
            string varName = it.first;   // 变量名
            shared_ptr<PhiInstruction> phi = it.second;  // 空phi
            bb->phis.insert(phi);
            add_phi_operands(bb, varName, phi);  // 加入操作数
        }
        bb->incomplete_phis.clear();
        bb->sealed = true;
    }
    else
    {
        cerr << "Error occurs in process seal basic block: the block is sealed." << endl;
    }
}
