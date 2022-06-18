#include "ir_optimize.h"

#include <stack>
#include <queue>
#include <ctime>
#include <set>

time_t startAllocTime;  // 开始构建冲突图时间
bool conflictGraphBuildSuccess;   // 冲突图成功构建
unsigned long CONFLICT_GRAPH_TIMEOUT = 10;  // 冲突图构建超时限度：10s

unordered_map<shared_ptr<Value>, shared_ptr<unordered_set<shared_ptr<Value>>>> conflictGraph;  // 冲突图  V <--> 冲突对象
// 块的路径  起始块 <--> 中止块 <--> 路径块
unordered_map<shared_ptr<BasicBlock>, shared_ptr<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>>> blockPath;

/**
 * @brief 冲突图是否构建超时
 * @param startTime 开始时间
 * @param timeout 超时限度
 * @return true 超时；false 未超时
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
 * @brief 寄存器分配
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

    if (conflictGraphBuildSuccess)  // 冲突图构建成功
        allocRegister(func);
    else
    {
        cerr << "Time out when building conflict graph of function " << func->name << "." << endl;
    }
}

/**
 * @brief 初始化冲突图，给每个左值赋予一个空set，即每个值目前无冲突
 * @param func 
 */
void initConflictGraph(shared_ptr<Function> &func)
{
    for (auto &arg : func->params)  // 函数参数冲突初始化为空
    {
        shared_ptr<unordered_set<shared_ptr<Value>>> tempSet = make_shared<unordered_set<shared_ptr<Value>>>();
        conflictGraph[arg] = tempSet;
    }
    for (auto &bb : func->blocks)
    {
        for (auto &ins : bb->instructions)
        {
            if (ins->resultType == L_VAL_RESULT && conflictGraph.count(ins) == 0)  // 所有左值的冲突初始化为空
            {
                shared_ptr<unordered_set<shared_ptr<Value>>> tempSet = make_shared<unordered_set<shared_ptr<Value>>>();
                conflictGraph[ins] = tempSet;
            }
        }
    }
}

/**
 * @brief 构建冲突图
 * @param func 
 */
void buildConflictGraph(shared_ptr<Function> &func)
{
    for (auto &ins : func->params)  // 函数参数，从函数开始，至使用完毕，都是保持活跃的
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
                for (auto &op : phiMov->phi->operands)  // 此phi对应的所有操作数
                {
                    if (op.second == ins)  // 操作数为ins
                    {
                        shared_ptr<BasicBlock> phiMovLocation = op.first;
                        if (phiMovLocation->aliveValues.count(ins) != 0)
                            continue;
                        else if (phiMov->blockALiveValues.at(phiMovLocation).count(ins) != 0)
                            continue;
                        auto it = func->entryBlock->instructions.begin();
                        bool searchOtherBlocks = true;
                        // userIns在insVal块内，entryBlock至phiMov设为活跃
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
                            // 维护路径块，并将它们添加到活跃节点中：路径为entryBlock所在块至phiMovLocation的对应指令
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
            else  // 非phi指令
            {
                if (userIns->block->aliveValues.count(ins) != 0 || userIns->aliveValues.count(ins) != 0)
                {
                    continue;
                }
                else  // 此指令未将ins设为活跃，则在通往userIns的所有路径上都将ins标记为活跃。
                {
                    auto it = func->entryBlock->instructions.begin();
                    bool searchOtherBlocks = true;  // userIns不再entryBlock中
                    // 从头至userIns将ins设为活跃
                    while (it != func->entryBlock->instructions.end())
                    {
                        addAliveValue(ins, *it, func->entryBlock);
                        if (*it == userIns)     // 直到在entryBlock找到userIns
                        {
                            searchOtherBlocks = false;
                            break;
                        }
                        ++it;
                    }
                    // userIns在entryBlock外
                    if (searchOtherBlocks)
                    {
                        // 维护路径块，并将它们添加到活跃节点中：路径为entryBlock至userIns->block的对应指令
                        unordered_set<shared_ptr<BasicBlock>> pathNodes;
                        if (blockPath.count(func->entryBlock) != 0)  // entryBlock已加入blockPath
                        {
                            if (blockPath.at(func->entryBlock)->count(userIns->block) != 0)  // 已计算entryBlock至userIns->block路径
                            {
                                pathNodes = blockPath.at(func->entryBlock)->at(userIns->block);
                            }
                            else
                            {
                                pathNodes = getBlockPathPoints(func->entryBlock, userIns->block);
                            }
                        }
                        else  // entryBlock未加入blockPath，即未作为起点
                        {
                            pathNodes = getBlockPathPoints(func->entryBlock, userIns->block);
                        }
                        for (auto &b : pathNodes)  // 路径块中的每个块，将ins视为活跃
                        {
                            b->aliveValues.insert(ins);
                        }
                        // 搜索userIns->block并标记为活跃
                        it = userIns->block->instructions.begin();
                        while (it != userIns->block->instructions.end())
                        {
                            addAliveValue(ins, *it, userIns->block);
                            if (*it == userIns)  // 将userIns->block中直到userIns均为活跃
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
        for (auto ins = bb->instructions.begin(); ins != bb->instructions.end(); ++ins)  // 遍历每条指令
        {
            if (checkRegAllocTimeout(startAllocTime, CONFLICT_GRAPH_TIMEOUT))
                return;
            shared_ptr<Instruction> insVal = *ins;
            if (insVal->type != PHI_MOV && insVal->resultType == L_VAL_RESULT)  // 左值
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
                        // 找到所有的phi_mov位置
                        shared_ptr<PhiMoveInstruction> phiMov = s_p_c<PhiInstruction>(userIns)->phiMove;
                        for (auto &op : phiMov->phi->operands)
                        {
                            if (op.second == insVal)
                            {
                                shared_ptr<BasicBlock> phiMovLocation = op.first;  // phi操作数为insVal所在的块phiMovLocation
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
                                // userIns在insVal块内，ins至userIns设为活跃
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
                                // userIns不是在phiMovLocation块内，搜索整个块路径。
                                if (searchOtherBlocks)
                                {
                                    // 维护路径块，并将它们添加到活跃节点中：路径为insVal所在块至phiMovLocation的对应指令
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
                        else  // 此指令未将ins设为活跃，则在通往userIns的所有路径上都将ins标记为活跃。
                        {
                            auto it = ins + 1;
                            bool searchOtherBlocks = true;
                            // userIns在insVal块内，ins至userIns设为活跃
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
                            // userIns不是在insVal块内，搜索整个块路径。
                            if (searchOtherBlocks)
                            {
                                // 维护路径块，并将它们添加到活跃节点中：路径为insVal所在块至userIns->block的对应指令
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
                shared_ptr<BasicBlock> targetPhiBlock = phiMove->phi->block;  // phi所在的目标块
                auto it = ins + 1;
                while (it != bb->instructions.end())  // 将insVal至块末尾指令，将insVal设为活跃
                {
                    addAliveValue(insVal, *it, bb);
                    ++it;
                }
                if (targetPhiBlock->aliveValues.count(insVal) != 0 || phiMove->phi->aliveValues.count(insVal) != 0)
                    continue;
                it = targetPhiBlock->instructions.begin();
                while (it != targetPhiBlock->instructions.end())   // 将phi所在的目标块内从头至phi指令，将insVal设为活跃
                {
                    addAliveValue(insVal, *it, targetPhiBlock);
                    if (*it == phiMove->phi)
                        break;
                    ++it;
                }
            }
        }
    }
    // 计算完了所有值的活跃期
    for (auto &bb : func->blocks)  // bb运行期间
    {
        for (auto &aliveVal : bb->aliveValues)
        {
            for (auto &aliveValInside : bb->aliveValues)  // bb期间活跃的值相互冲突，aliveValInside和aliveValInside都在bb期间活跃
            {
                if (aliveVal != aliveValInside)  // 如果两个值不一样，则冲突
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
            for (auto &ins : bb->instructions)  // aliveValInside在bb期间活跃，则与bb内指令冲突
            {
                if (ins->resultType == L_VAL_RESULT)
                {
                    conflictGraph.at(aliveVal)->insert(ins);
                    conflictGraph.at(ins)->insert(aliveVal);
                }
            }
        }
        for (auto &ins : bb->instructions)  // ins期间
        {
            unordered_set<shared_ptr<Value>> aliveValues;  // ins期间活跃的值
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
                    if (aliveVal != aliveValInside)   // ins期间活跃的值相互冲突
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
 * @brief 分配物理寄存器，图着色
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
        if (it.second->size() < _GLB_REG_CNT)  // 与_GLB_REG_CNT以下值冲突
        {
            for (auto &val : *it.second)
            {
                tempGraph.at(val)->erase(var);  // 减去与var连接的边
            }
            tempGraph.erase(var);
            variableWithRegs.push(var);  // 将var入栈
            goto ALLOC_REGISTER_START;
        }
    }
    if (!tempGraph.empty())  // 其中有着溢出的值，即最终也与_GLB_REG_CNT以上值冲突
    {
        shared_ptr<Value> abandon = tempGraph.begin()->first;  // 选取一个必须舍弃的值，即放入内存的值
        for (auto &it : tempGraph)
        {
            shared_ptr<Value> ins = it.first;
            // 选取权重最小的值
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
        func->variableWithoutReg.insert(abandon);  // 将此值计划放入内存
        goto ALLOC_REGISTER_START;   // 重新分配
    }

    unordered_set<string> validRegs;
    for (int i = 0; i < _GLB_REG_CNT; ++i)
        validRegs.insert(to_string(i + _GLB_REG_START));  // 寄存器R4-R12为有效寄存器
    while (!variableWithRegs.empty())
    {
        unordered_set<string> regs = validRegs;
        shared_ptr<Value> value = variableWithRegs.top();  // 依次出栈分配寄存器
        variableWithRegs.pop();
        for (auto &it : *conflictGraph.at(value))  // 避免冲突
        {
            if (func->variableRegs.count(it) != 0)
                regs.erase(func->variableRegs.at(it));
        }
        func->variableRegs[value] = *regs.begin();  // 分配成功寄存器
    }
}

/**
 * @brief 获得从from至to的经过的块
 * @param from 起始块
 * @param to 终点快
 * @return 路径
 */
unordered_set<shared_ptr<BasicBlock>> getBlockPathPoints(shared_ptr<BasicBlock> &from, shared_ptr<BasicBlock> &to)
{
    unordered_set<shared_ptr<BasicBlock>> fromReachable;
    unordered_set<shared_ptr<BasicBlock>> ans;
    unordered_set<shared_ptr<BasicBlock>> tempReachable;
    getBlockReachableBlocks(from, from, fromReachable);  // 从from开始的路径，如果回到from则是循环的路径，如果不循环是到函数尾的路径
    for (auto bb : fromReachable)
    {
        tempReachable.clear();
        getBlockReachableBlocks(bb, from, tempReachable);  // 从from开始路径的每个块开始，如果路径包含to，则ans加上此块
        if (tempReachable.count(to) != 0)
        {
            ans.insert(bb);
        }
    }
    ans.erase(from);
    if (blockPath.count(from) != 0)  // from做过起点
    {
        shared_ptr<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>> tempMap = blockPath.at(from);
        if (tempMap->count(to) != 0)
        {
            cerr << "Error occurs in process register alloc: search path twice." << endl;
        }
        else
        {
            tempMap->insert({to, ans});  // 将路径加入blockPath
            blockPath[from] = tempMap;
        }
    }
    else  // from未做过起点
    {
        shared_ptr<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>> tempMap = make_shared<unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<BasicBlock>>>>();
        tempMap->insert({to, ans});
        blockPath[from] = tempMap;
    }
    return ans;
}

/**
 * @brief 获得从bb至cannotArrive的可能经过的块，如果bb不经过cannotArrive，则ans为从bb开始的所有路径
 * @param bb 起始块
 * @param cannotArrive 终点块
 * @param ans 路径块
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
 * @brief 将ownerBlock中owner时，value设为活跃
 * @param value 活跃的值
 * @param owner 活跃期间的指令
 * @param ownerBlock 活跃期间指令存在的块
 */
void addAliveValue(shared_ptr<Instruction> &value, shared_ptr<Instruction> &owner, shared_ptr<BasicBlock> &ownerBlock)
{
    if (value->resultType == L_VAL_RESULT)
    {
        if (owner->type == PHI_MOV)  // phi_move指令，在blockALiveValues加入活跃value
        {
            s_p_c<PhiMoveInstruction>(owner)->blockALiveValues.at(ownerBlock).insert(value);
        }
        else
            owner->aliveValues.insert(value);
    }
}

/**
 * @brief 将ownerBlock中owner时，value设为活跃
 * @param value 活跃的值
 * @param owner 活跃期间的指令
 * @param ownerBlock 活跃期间指令存在的块
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
        if (owner->type == PHI_MOV)  // phi_move指令，在blockALiveValues加入活跃value
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
