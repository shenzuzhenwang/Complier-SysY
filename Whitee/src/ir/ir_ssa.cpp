/*********************************************************************
 * @file   ir_ssa.cpp
 * @brief  SSA��̬����ֵ����
 * 
 * @author ����
 * @date   June 2022
 *********************************************************************/
#include "ir_ssa.h"

#include <iostream>

// https://zhuanlan.zhihu.com/p/360692294
// https://www.gqrelic.com/2022/04/07/ssa-book-1/

shared_ptr<Value> readLocalVariableRecursively(shared_ptr<BasicBlock> &bb, string &varName);

/**
 * @brief ��һ�����SSA MAP�ж�ȡ����
 * @param bb �������ڵĿ�
 * @param varName ����������
 * @return ������ֵ
 */
shared_ptr<Value> readLocalVariable(shared_ptr<BasicBlock> &bb, string &varName)
{
    if (bb->localVarSsaMap.count(varName) != 0)
        return bb->localVarSsaMap.at(varName);
    return readLocalVariableRecursively(bb, varName);
}

/**
 * @brief ������д����SSA MAP
 * @param bb �������ڵĿ�
 * @param varName ����������
 * @param value ������д���ֵ
 */
void writeLocalVariable(shared_ptr<BasicBlock> &bb, const string &varName, const shared_ptr<Value> &value)
{
    bb->localVarSsaMap[varName] = value;
    if (dynamic_cast<PhiInstruction *>(value.get()))   // д��phi������ֵ��phi���˿�ʹ��
    {
        value->users.insert(bb);
    }
}

/**
 * @brief �ݹ���ǰ���Ŀ�Ѱ�ұ�����ֵ
 * @param bb ��ʼѰ�ҵĿ�
 * @param varName ����������
 * @return ������ֵ
 */
shared_ptr<Value> readLocalVariableRecursively(shared_ptr<BasicBlock> &bb, string &varName)
{
    if (!bb->sealed)  // �鲻��գ���������ѭ���壬��ʱ����ǰ��δ������
    {
        shared_ptr<PhiInstruction> emptyPhi = make_shared<PhiInstruction>(varName, bb);
        bb->incompletePhis[varName] = emptyPhi;
        writeLocalVariable(bb, varName, emptyPhi);
        return emptyPhi;
    }
    else if (bb->predecessors.size() == 1)  // ֻ��һ��ǰ�������������Ҫ phi
    {
        shared_ptr<BasicBlock> predecessor = *(bb->predecessors.begin());
        shared_ptr<Value> val = readLocalVariable(predecessor, varName);  // �����ݹ���ǰѰ�ұ�����ֵ
        writeLocalVariable(bb, varName, val);  // �ҵ���д�����ڵĿ�
        return val;
    }
    else  // �������������ǰ����
    {
        shared_ptr<Value> val = make_shared<PhiInstruction>(varName, bb);
        writeLocalVariable(bb, varName, val);                   // ���ڿ���дһ���޲�������phi��Ϊ���ƻ����ܵ�ѭ��
        shared_ptr<PhiInstruction> phi = s_p_c<PhiInstruction>(val);
        bb->phis.insert(phi);

        val = addPhiOperands(bb, varName, phi);  // ���������
        writeLocalVariable(bb, varName, val);
        return val;
    }
}

/**
 * @brief ���������phi�Ĳ�����
 * @param bb phi���ڵĿ�
 * @param varName ����������
 * @param phi phi����
 * @return 
 */
shared_ptr<Value> addPhiOperands(shared_ptr<BasicBlock> &bb, string &varName, shared_ptr<PhiInstruction> &phi)
{
    for (auto &it : bb->predecessors)  // ��ǰ����ȷ��������
    {
        shared_ptr<BasicBlock> pred = it;
        shared_ptr<Value> v = readLocalVariable(pred, varName);  // �ݹ���ǰ����Ѱ��ͬ��������ֵ����������ѭ�����ҵ��˴�phi
        shared_ptr<BasicBlock> block;
        phi->operands.insert({it, v});
        v->users.insert(phi);
    }
    return removeTrivialPhi(phi);  // ���ڿ�������ѭ����phi�Ĳ�������ֵΪphi�Լ�������������������ͬ����ʱ��Ҫȥ��phi
}

/**
 * @brief ȥ������Ҫ��phi
 * @param phi 
 * @return 
 */
shared_ptr<Value> removeTrivialPhi(shared_ptr<PhiInstruction> &phi)
{
    shared_ptr<Value> same;
    shared_ptr<Value> self = phi->shared_from_this();
    for (auto &it : phi->operands)     // ������Ϊ�Լ�����һ��ֵ��ʱ�򣬴�phi��������һ��ֵ����
    {
        if (it.second == same || it.second == self)  // ������������ֵ������һ��Ϊ��phi��������ֵ��ͬ����ʱ����Ҫȥ����phi
            continue;
        if (same != nullptr)  // phi���������Լ��Ĳ���������Ҫ
            return phi;
        same = it.second;
    }
    if (same == nullptr)     // ���ɴ���ڿ�ʼ���У��޲�����
        same = make_shared<UndefinedValue>(phi->localVarName);
    phi->users.erase(phi);    // �ҳ�����ʹ����� phi ��ֵ������������
    unordered_set<shared_ptr<Value>> users = phi->users;
    shared_ptr<Value> toBeReplaced = phi;
    phi->block->phis.erase(phi);
    for (auto &it : users)
    {
        it->replaceUse(toBeReplaced, same);  // �������õ� phi �ĵط����Ϊ same ���Ƴ� phi
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
    for (auto &it : users)   // ��ȥ�ݹ��Ƴ�ʹ�ô� phi �� phi ָ���Ϊ�����ܱ�ò���Ҫ��trivial��
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
 * @brief ��ǰ����ѯ�������� ���������phi����ֵ
 * @param bb �˿�
 */
void sealBasicBlock(shared_ptr<BasicBlock> &bb)
{
    if (!bb->sealed)
    {
        for (auto &it : bb->incompletePhis)
        {
            string varName = it.first;   // ������
            shared_ptr<PhiInstruction> phi = it.second;  // ��phi
            bb->phis.insert(phi);
            addPhiOperands(bb, varName, phi);  // ���������
        }
        bb->incompletePhis.clear();
        bb->sealed = true;
    }
    else
    {
        cerr << "Error occurs in process seal basic block: the block is sealed." << endl;
    }
}
