#include "ir_optimize.h"

#include <stack>
#include <queue>
#include <ctime>
#include <set>

time_t startAllocTime;  // ��ʼ������ͻͼʱ��
bool conflictGraphBuildSuccess;   // ��ͻͼ�ɹ�����
unsigned long CONFLICT_GRAPH_TIMEOUT = 10;  // ��ͻͼ������ʱ�޶ȣ�10s

unordered_map<shared_ptr<Value>, shared_ptr<unordered_set<shared_ptr<Value>>>> conflictGraph;  // ��ͻͼ  V <--> ��ͻ����
// ���·��  ��ʼ�� <--> ��ֹ�� <--> ·����
unordered_map<shared_ptr<BasicBlock>, shared_ptr<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>>> blockPath;

/**
 * @brief ��ͻͼ�Ƿ񹹽���ʱ
 * @param startTime ��ʼʱ��
 * @param timeout ��ʱ�޶�
 * @return true ��ʱ��false δ��ʱ
 */
inline bool checkRegAllocTimeout(time_t startTime, unsigned long timeout)
{
    if (time(nullptr) - startTime > timeout)
    {
        conflictGraphBuildSuccess = false;
        return true;
    }
    return false;
}

void initConflictGraph(shared_ptr<Function> &func);

void buildConflictGraph(shared_ptr<Function> &func);

void allocRegister(shared_ptr<Function> &func);

void addAliveValue(shared_ptr<Instruction> &value, shared_ptr<Instruction> &owner, shared_ptr<BasicBlock> &ownerBlock);

void addAliveValue(shared_ptr<Value> &value, shared_ptr<Instruction> &owner, shared_ptr<BasicBlock> &ownerBlock);

unordered_set<shared_ptr<BasicBlock>> getBlockPathPoints(shared_ptr<BasicBlock> &from, shared_ptr<BasicBlock> &to);

void getBlockReachableBlocks(shared_ptr<BasicBlock> &bb, shared_ptr<BasicBlock> &cannotArrive, unordered_set<shared_ptr<BasicBlock>> &ans);

void outputConflictGraph(const string &funcName);

/**
 * @brief �Ĵ�������
 * @param func 
 */
void registerAlloc(shared_ptr<Function> &func)
{
    conflictGraph.clear();
    blockPath.clear();
    initConflictGraph(func);
    conflictGraphBuildSuccess = true;
    startAllocTime = time(nullptr);
    buildConflictGraph(func);
    if (_debugIrOptimize)
        outputConflictGraph(func->name);

    if (conflictGraphBuildSuccess)  // ��ͻͼ�����ɹ�
        allocRegister(func);
    else
    {
        cerr << "Time out when building conflict graph of function " << func->name << "." << endl;
    }
}

/**
 * @brief ��ʼ����ͻͼ����ÿ����ֵ����һ����set����ÿ��ֵĿǰ�޳�ͻ
 * @param func 
 */
void initConflictGraph(shared_ptr<Function> &func)
{
    for (auto &arg : func->params)  // ����������ͻ��ʼ��Ϊ��
    {
        shared_ptr<unordered_set<shared_ptr<Value>>> tempSet = make_shared<unordered_set<shared_ptr<Value>>>();
        conflictGraph[arg] = tempSet;
    }
    for (auto &bb : func->blocks)
    {
        for (auto &ins : bb->instructions)
        {
            if (ins->resultType == L_VAL_RESULT && conflictGraph.count(ins) == 0)  // ������ֵ�ĳ�ͻ��ʼ��Ϊ��
            {
                shared_ptr<unordered_set<shared_ptr<Value>>> tempSet = make_shared<unordered_set<shared_ptr<Value>>>();
                conflictGraph[ins] = tempSet;
            }
        }
    }
}

/**
 * @brief ������ͻͼ
 * @param func 
 */
void buildConflictGraph(shared_ptr<Function> &func)
{
    for (auto &ins : func->params)  // �����������Ӻ�����ʼ����ʹ����ϣ����Ǳ��ֻ�Ծ��
    {
        if (checkRegAllocTimeout(startAllocTime, CONFLICT_GRAPH_TIMEOUT))
            return;
        for (auto &user : ins->users)
        {
            if (checkRegAllocTimeout(startAllocTime, CONFLICT_GRAPH_TIMEOUT))
                return;
            if (user->valueType != INSTRUCTION)
            {
                cerr << "Error occurs in register alloc: got a non-instruction user." << endl;
            }
            shared_ptr<Instruction> userIns = s_p_c<Instruction>(user);
            if (userIns->type == PHI)
            {
                shared_ptr<PhiMoveInstruction> phiMov = s_p_c<PhiInstruction>(userIns)->phiMove;
                for (auto &op : phiMov->phi->operands)  // ��phi��Ӧ�����в�����
                {
                    if (op.second == ins)  // ������Ϊins
                    {
                        shared_ptr<BasicBlock> phiMovLocation = op.first;
                        if (phiMovLocation->aliveValues.count(ins) != 0)
                            continue;
                        else if (phiMov->blockALiveValues.at(phiMovLocation).count(ins) != 0)
                            continue;
                        auto it = func->entryBlock->instructions.begin();
                        bool searchOtherBlocks = true;
                        // userIns��insVal���ڣ�entryBlock��phiMov��Ϊ��Ծ
                        while (it != func->entryBlock->instructions.end())
                        {
                            addAliveValue(ins, *it, func->entryBlock);
                            if (*it == phiMov && phiMovLocation == func->entryBlock)
                            {
                                searchOtherBlocks = false;
                                break;
                            }
                            ++it;
                        }
                        if (searchOtherBlocks)
                        {
                            // ά��·���飬����������ӵ���Ծ�ڵ��У�·��ΪentryBlock���ڿ���phiMovLocation�Ķ�Ӧָ��
                            unordered_set<shared_ptr<BasicBlock>> pathNodes;
                            if (blockPath.count(func->entryBlock) != 0)
                            {
                                if (blockPath.at(func->entryBlock)->count(phiMovLocation) != 0)
                                {
                                    pathNodes = blockPath.at(func->entryBlock)->at(phiMovLocation);
                                }
                                else
                                {
                                    pathNodes = getBlockPathPoints(func->entryBlock, phiMovLocation);
                                }
                            }
                            else
                                pathNodes = getBlockPathPoints(func->entryBlock, phiMovLocation);
                            for (auto &b : pathNodes)
                            {
                                b->aliveValues.insert(ins);
                            }
                            it = phiMovLocation->instructions.begin();
                            while (it != phiMovLocation->instructions.end())
                            {
                                addAliveValue(ins, *it, phiMovLocation);
                                if (*it == phiMov)
                                    break;
                                ++it;
                            }
                        }
                    }
                }
            }
            else  // ��phiָ��
            {
                if (userIns->block->aliveValues.count(ins) != 0 || userIns->aliveValues.count(ins) != 0)
                {
                    continue;
                }
                else  // ��ָ��δ��ins��Ϊ��Ծ������ͨ��userIns������·���϶���ins���Ϊ��Ծ��
                {
                    auto it = func->entryBlock->instructions.begin();
                    bool searchOtherBlocks = true;  // userIns����entryBlock��
                    // ��ͷ��userIns��ins��Ϊ��Ծ
                    while (it != func->entryBlock->instructions.end())
                    {
                        addAliveValue(ins, *it, func->entryBlock);
                        if (*it == userIns)     // ֱ����entryBlock�ҵ�userIns
                        {
                            searchOtherBlocks = false;
                            break;
                        }
                        ++it;
                    }
                    // userIns��entryBlock��
                    if (searchOtherBlocks)
                    {
                        // ά��·���飬����������ӵ���Ծ�ڵ��У�·��ΪentryBlock��userIns->block�Ķ�Ӧָ��
                        unordered_set<shared_ptr<BasicBlock>> pathNodes;
                        if (blockPath.count(func->entryBlock) != 0)  // entryBlock�Ѽ���blockPath
                        {
                            if (blockPath.at(func->entryBlock)->count(userIns->block) != 0)  // �Ѽ���entryBlock��userIns->block·��
                            {
                                pathNodes = blockPath.at(func->entryBlock)->at(userIns->block);
                            }
                            else
                            {
                                pathNodes = getBlockPathPoints(func->entryBlock, userIns->block);
                            }
                        }
                        else  // entryBlockδ����blockPath����δ��Ϊ���
                        {
                            pathNodes = getBlockPathPoints(func->entryBlock, userIns->block);
                        }
                        for (auto &b : pathNodes)  // ·�����е�ÿ���飬��ins��Ϊ��Ծ
                        {
                            b->aliveValues.insert(ins);
                        }
                        // ����userIns->block�����Ϊ��Ծ
                        it = userIns->block->instructions.begin();
                        while (it != userIns->block->instructions.end())
                        {
                            addAliveValue(ins, *it, userIns->block);
                            if (*it == userIns)  // ��userIns->block��ֱ��userIns��Ϊ��Ծ
                            {
                                break;
                            }
                            ++it;
                        }
                    }
                }
            }
        }
    }
    for (auto &bb : func->blocks)
    {
        if (checkRegAllocTimeout(startAllocTime, CONFLICT_GRAPH_TIMEOUT))
            return;
        for (auto ins = bb->instructions.begin(); ins != bb->instructions.end(); ++ins)  // ����ÿ��ָ��
        {
            if (checkRegAllocTimeout(startAllocTime, CONFLICT_GRAPH_TIMEOUT))
                return;
            shared_ptr<Instruction> insVal = *ins;
            if (insVal->type != PHI_MOV && insVal->resultType == L_VAL_RESULT)  // ��ֵ
            {
                for (auto &user : insVal->users)
                {
                    if (checkRegAllocTimeout(startAllocTime, CONFLICT_GRAPH_TIMEOUT))
                        return;
                    if (user->valueType != INSTRUCTION)
                    {
                        cerr << "Error occurs in register alloc: got a non-instruction user." << endl;
                    }
                    shared_ptr<Instruction> userIns = s_p_c<Instruction>(user);
                    if (userIns->type == PHI)
                    {
                        // �ҵ����е�phi_movλ��
                        shared_ptr<PhiMoveInstruction> phiMov = s_p_c<PhiInstruction>(userIns)->phiMove;
                        for (auto &op : phiMov->phi->operands)
                        {
                            if (op.second == insVal)
                            {
                                shared_ptr<BasicBlock> phiMovLocation = op.first;  // phi������ΪinsVal���ڵĿ�phiMovLocation
                                if (phiMovLocation->aliveValues.count(insVal) != 0)
                                {
                                    continue;
                                }
                                else if (phiMov->blockALiveValues.at(phiMovLocation).count(insVal) != 0)
                                {
                                    continue;
                                }
                                auto it = ins + 1;
                                bool searchOtherBlocks = true;
                                // userIns��insVal���ڣ�ins��userIns��Ϊ��Ծ
                                while (it != bb->instructions.end())
                                {
                                    addAliveValue(insVal, *it, bb);
                                    if (*it == phiMov && phiMovLocation == bb)
                                    {
                                        searchOtherBlocks = false;
                                        break;
                                    }
                                    ++it;
                                }
                                // userIns������phiMovLocation���ڣ�����������·����
                                if (searchOtherBlocks)
                                {
                                    // ά��·���飬����������ӵ���Ծ�ڵ��У�·��ΪinsVal���ڿ���phiMovLocation�Ķ�Ӧָ��
                                    unordered_set<shared_ptr<BasicBlock>> pathNodes;
                                    if (blockPath.count(bb) != 0)
                                    {
                                        if (blockPath.at(bb)->count(phiMovLocation) != 0)
                                        {
                                            pathNodes = blockPath.at(bb)->at(phiMovLocation);
                                        }
                                        else
                                        {
                                            pathNodes = getBlockPathPoints(bb, phiMovLocation);
                                        }
                                    }
                                    else
                                        pathNodes = getBlockPathPoints(bb, phiMovLocation);
                                    for (auto &b : pathNodes)
                                    {
                                        b->aliveValues.insert(insVal);
                                    }
                                    it = phiMovLocation->instructions.begin();
                                    while (it != phiMovLocation->instructions.end())
                                    {
                                        addAliveValue(insVal, *it, phiMovLocation);
                                        if (*it == phiMov)
                                            break;
                                        ++it;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        if (userIns->block->aliveValues.count(insVal) != 0 || userIns->aliveValues.count(insVal) != 0)
                        {
                            continue;
                        }
                        else  // ��ָ��δ��ins��Ϊ��Ծ������ͨ��userIns������·���϶���ins���Ϊ��Ծ��
                        {
                            auto it = ins + 1;
                            bool searchOtherBlocks = true;
                            // userIns��insVal���ڣ�ins��userIns��Ϊ��Ծ
                            while (it != bb->instructions.end())
                            {
                                addAliveValue(insVal, *it, bb);
                                if (*it == userIns)
                                {
                                    searchOtherBlocks = false;
                                    break;
                                }
                                ++it;
                            }
                            // userIns������insVal���ڣ�����������·����
                            if (searchOtherBlocks)
                            {
                                // ά��·���飬����������ӵ���Ծ�ڵ��У�·��ΪinsVal���ڿ���userIns->block�Ķ�Ӧָ��
                                unordered_set<shared_ptr<BasicBlock>> pathNodes;
                                if (blockPath.count(bb) != 0)
                                {
                                    if (blockPath.at(bb)->count(userIns->block) != 0)
                                    {
                                        pathNodes = blockPath.at(bb)->at(userIns->block);
                                    }
                                    else
                                    {
                                        pathNodes = getBlockPathPoints(bb, userIns->block);
                                    }
                                }
                                else
                                {
                                    pathNodes = getBlockPathPoints(bb, userIns->block);
                                }
                                for (auto &b : pathNodes)
                                {
                                    b->aliveValues.insert(insVal);
                                }
                                // search the user block and mark alive.
                                it = userIns->block->instructions.begin();
                                while (it != userIns->block->instructions.end())
                                {
                                    addAliveValue(insVal, *it, userIns->block);
                                    if (*it == userIns)
                                    {
                                        break;
                                    }
                                    ++it;
                                }
                            }
                        }
                    }
                }
            }
            else if (insVal->type == PHI_MOV)
            {
                shared_ptr<PhiMoveInstruction> phiMove = s_p_c<PhiMoveInstruction>(insVal);
                shared_ptr<BasicBlock> targetPhiBlock = phiMove->phi->block;  // phi���ڵ�Ŀ���
                auto it = ins + 1;
                while (it != bb->instructions.end())  // ��insVal����ĩβָ���insVal��Ϊ��Ծ
                {
                    addAliveValue(insVal, *it, bb);
                    ++it;
                }
                if (targetPhiBlock->aliveValues.count(insVal) != 0 || phiMove->phi->aliveValues.count(insVal) != 0)
                    continue;
                it = targetPhiBlock->instructions.begin();
                while (it != targetPhiBlock->instructions.end())   // ��phi���ڵ�Ŀ����ڴ�ͷ��phiָ���insVal��Ϊ��Ծ
                {
                    addAliveValue(insVal, *it, targetPhiBlock);
                    if (*it == phiMove->phi)
                        break;
                    ++it;
                }
            }
        }
    }
    // ������������ֵ�Ļ�Ծ��
    for (auto &bb : func->blocks)  // bb�����ڼ�
    {
        for (auto &aliveVal : bb->aliveValues)
        {
            for (auto &aliveValInside : bb->aliveValues)  // bb�ڼ��Ծ��ֵ�໥��ͻ��aliveValInside��aliveValInside����bb�ڼ��Ծ
            {
                if (aliveVal != aliveValInside)  // �������ֵ��һ�������ͻ
                {
                    if (conflictGraph.count(aliveVal) == 0)
                    {
                        cerr << "Error occurs in process register alloc: conflict graph does not have a l-value." << endl;
                    }
                    else
                    {
                        conflictGraph.at(aliveVal)->insert(aliveValInside);
                    }
                }
            }
            for (auto &ins : bb->instructions)  // aliveValInside��bb�ڼ��Ծ������bb��ָ���ͻ
            {
                if (ins->resultType == L_VAL_RESULT)
                {
                    conflictGraph.at(aliveVal)->insert(ins);
                    conflictGraph.at(ins)->insert(aliveVal);
                }
            }
        }
        for (auto &ins : bb->instructions)  // ins�ڼ�
        {
            unordered_set<shared_ptr<Value>> aliveValues;  // ins�ڼ��Ծ��ֵ
            if (ins->type != PHI_MOV)
            {
                aliveValues = ins->aliveValues;
            }
            else
            {
                aliveValues = s_p_c<PhiMoveInstruction>(ins)->blockALiveValues.at(bb);
            }
            for (auto &aliveVal : aliveValues)
            {
                for (auto &aliveValInside : aliveValues)
                {
                    if (aliveVal != aliveValInside)   // ins�ڼ��Ծ��ֵ�໥��ͻ
                    {
                        if (conflictGraph.count(aliveVal) == 0)
                        {
                            cerr << "Error occurs in process register alloc: conflict graph does not have a l-value." << endl;
                        }
                        else
                        {
                            conflictGraph.at(aliveVal)->insert(aliveValInside);
                        }
                    }
                }
            }
        }
    }
}

/**
 * @brief ��������Ĵ�����ͼ��ɫ
 * @param func 
 */
void allocRegister(shared_ptr<Function> &func)
{
    stack<shared_ptr<Value>> variableWithRegs;
    unordered_map<shared_ptr<Value>, shared_ptr<unordered_set<shared_ptr<Value>>>> tempGraph = conflictGraph;
ALLOC_REGISTER_START:
    for (auto &it : tempGraph)
    {
        shared_ptr<Value> var = it.first;
        if (it.second->size() < _GLB_REG_CNT)  // ��_GLB_REG_CNT����ֵ��ͻ
        {
            for (auto &val : *it.second)
            {
                tempGraph.at(val)->erase(var);  // ��ȥ��var���ӵı�
            }
            tempGraph.erase(var);
            variableWithRegs.push(var);  // ��var��ջ
            goto ALLOC_REGISTER_START;
        }
    }
    if (!tempGraph.empty())  // �������������ֵ��������Ҳ��_GLB_REG_CNT����ֵ��ͻ
    {
        shared_ptr<Value> abandon = tempGraph.begin()->first;  // ѡȡһ������������ֵ���������ڴ��ֵ
        for (auto &it : tempGraph)
        {
            shared_ptr<Value> ins = it.first;
            // ѡȡȨ����С��ֵ
            if (func->variableWeight.at(ins) < func->variableWeight.at(abandon) || (func->variableWeight.at(ins) == func->variableWeight.at(abandon) && ins->id < abandon->id))
            {
                abandon = ins;
            }
        }
        for (auto &it : *tempGraph.at(abandon))
        {
            tempGraph.at(it)->erase(abandon);
        }
        tempGraph.erase(abandon);
        for (auto &it : *conflictGraph.at(abandon))
        {
            conflictGraph.at(it)->erase(abandon);
        }
        conflictGraph.erase(abandon);
        func->variableWithoutReg.insert(abandon);  // ����ֵ�ƻ������ڴ�
        goto ALLOC_REGISTER_START;   // ���·���
    }

    unordered_set<string> validRegs;
    for (int i = 0; i < _GLB_REG_CNT; ++i)
        validRegs.insert(to_string(i + _GLB_REG_START));  // �Ĵ���R4-R12Ϊ��Ч�Ĵ���
    while (!variableWithRegs.empty())
    {
        unordered_set<string> regs = validRegs;
        shared_ptr<Value> value = variableWithRegs.top();  // ���γ�ջ����Ĵ���
        variableWithRegs.pop();
        for (auto &it : *conflictGraph.at(value))  // �����ͻ
        {
            if (func->variableRegs.count(it) != 0)
                regs.erase(func->variableRegs.at(it));
        }
        func->variableRegs[value] = *regs.begin();  // ����ɹ��Ĵ���
    }
}

/**
 * @brief ��ô�from��to�ľ����Ŀ�
 * @param from ��ʼ��
 * @param to �յ��
 * @return ·��
 */
unordered_set<shared_ptr<BasicBlock>> getBlockPathPoints(shared_ptr<BasicBlock> &from, shared_ptr<BasicBlock> &to)
{
    unordered_set<shared_ptr<BasicBlock>> fromReachable;
    unordered_set<shared_ptr<BasicBlock>> ans;
    unordered_set<shared_ptr<BasicBlock>> tempReachable;
    getBlockReachableBlocks(from, from, fromReachable);  // ��from��ʼ��·��������ص�from����ѭ����·���������ѭ���ǵ�����β��·��
    for (auto bb : fromReachable)
    {
        tempReachable.clear();
        getBlockReachableBlocks(bb, from, tempReachable);  // ��from��ʼ·����ÿ���鿪ʼ�����·������to����ans���ϴ˿�
        if (tempReachable.count(to) != 0)
        {
            ans.insert(bb);
        }
    }
    ans.erase(from);
    if (blockPath.count(from) != 0)  // from�������
    {
        shared_ptr<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>> tempMap = blockPath.at(from);
        if (tempMap->count(to) != 0)
        {
            cerr << "Error occurs in process register alloc: search path twice." << endl;
        }
        else
        {
            tempMap->insert({to, ans});  // ��·������blockPath
            blockPath[from] = tempMap;
        }
    }
    else  // fromδ�������
    {
        shared_ptr<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>> tempMap = make_shared<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>>();
        tempMap->insert({to, ans});
        blockPath[from] = tempMap;
    }
    return ans;
}

/**
 * @brief ��ô�bb��cannotArrive�Ŀ��ܾ����Ŀ飬���bb������cannotArrive����ansΪ��bb��ʼ������·��
 * @param bb ��ʼ��
 * @param cannotArrive �յ��
 * @param ans ·����
 */
void getBlockReachableBlocks(shared_ptr<BasicBlock> &bb, shared_ptr<BasicBlock> &cannotArrive, unordered_set<shared_ptr<BasicBlock>> &ans)
{
    queue<shared_ptr<BasicBlock>> blockQueue;
    blockQueue.push(bb);
    while (!blockQueue.empty())
    {
        shared_ptr<BasicBlock> top = blockQueue.front();
        blockQueue.pop();
        for (auto &suc : top->successors)
        {
            if (ans.count(suc) == 0 && suc != cannotArrive)
            {
                ans.insert(suc);
                blockQueue.push(suc);
            }
        }
    }
}

/**
 * @brief ��ownerBlock��ownerʱ��value��Ϊ��Ծ
 * @param value ��Ծ��ֵ
 * @param owner ��Ծ�ڼ��ָ��
 * @param ownerBlock ��Ծ�ڼ�ָ����ڵĿ�
 */
void addAliveValue(shared_ptr<Instruction> &value, shared_ptr<Instruction> &owner, shared_ptr<BasicBlock> &ownerBlock)
{
    if (value->resultType == L_VAL_RESULT)
    {
        if (owner->type == PHI_MOV)  // phi_moveָ���blockALiveValues�����Ծvalue
        {
            s_p_c<PhiMoveInstruction>(owner)->blockALiveValues.at(ownerBlock).insert(value);
        }
        else
            owner->aliveValues.insert(value);
    }
}

/**
 * @brief ��ownerBlock��ownerʱ��value��Ϊ��Ծ
 * @param value ��Ծ��ֵ
 * @param owner ��Ծ�ڼ��ָ��
 * @param ownerBlock ��Ծ�ڼ�ָ����ڵĿ�
 */
void addAliveValue(shared_ptr<Value> &value, shared_ptr<Instruction> &owner, shared_ptr<BasicBlock> &ownerBlock)
{
    if (value->valueType == INSTRUCTION)
    {
        shared_ptr<Instruction> insVal = s_p_c<Instruction>(value);
        addAliveValue(insVal, owner, ownerBlock);
    }
    else if (value->valueType == PARAMETER)
    {
        if (owner->type == PHI_MOV)  // phi_moveָ���blockALiveValues�����Ծvalue
        {
            s_p_c<PhiMoveInstruction>(owner)->blockALiveValues.at(ownerBlock).insert(value);
        }
        else
            owner->aliveValues.insert(value);
    }
    else
    {
        cerr << "Error occurs in process add alive value: add a non-instruction and non-parameter value." << endl;
    }
}

void outputConflictGraph(const string &funcName)
{
    if (_debugIrOptimize)
    {
        const string fileName = debugMessageDirectory + "ir_conflict_graph.txt";
        ofstream irOptimizeStream(fileName, ios::app);
        irOptimizeStream << "Function <" << funcName << ">:" << endl;
        map<unsigned int, shared_ptr<unordered_set<shared_ptr<Value>>>> tempMap;
        for (auto &val : conflictGraph)
        {
            tempMap[val.first->id] = val.second;
        }
        for (auto &value : tempMap)
        {
            irOptimizeStream << "<" << value.first << ">:";
            set<unsigned int> tempValSet;
            for (auto &edge : *value.second)
            {
                tempValSet.insert(edge->id);
            }
            for (auto &edge : tempValSet)
            {
                irOptimizeStream << " <" << edge << ">";
            }
            irOptimizeStream << endl;
        }
        irOptimizeStream << endl;
        irOptimizeStream.close();
    }
}
