#include "ir_optimize.h"

#include <set>
#include <stack>

void outputRegisterAllocResult(const shared_ptr<Module> &module);

/**
 * @brief 最后的优化
 * @param module 
 * @param level 优化等级
 */
void endOptimize(shared_ptr<Module> &module, OptimizeLevel level)
{
    for (auto &func : module->functions)
    {
        phiElimination(func);
    }
    if (_debugIr)
    {
        ofstream irStream;
        irStream.open(debugMessageDirectory + "ir_final.txt", ios::out | ios::trunc);
        irStream << module->toString() << endl;
        irStream.close();
    }
    if (_debugIrOptimize)
    {
        ofstream irStream;
        irStream.open(debugMessageDirectory + "ir_conflict_graph.txt", ios::out | ios::trunc);
        irStream << "[Conflict Graph]" << endl;
        irStream.close();
    }
    for (auto &func : module->functions)
    {
        if (level >= O1)
        {
            calculateVariableWeight(func);
            registerAlloc(func);
        }
        getFunctionRequiredStackSize(func);
        mergeAliveValuesToInstruction(func);
    }
    if (_debugIrOptimize)
    {
        outputRegisterAllocResult(module);
    }
}

/**
 * @brief 输出寄存器分配结果
 * @param module 
 */
void outputRegisterAllocResult(const shared_ptr<Module> &module)
{
    ofstream irStream;
    irStream.open(debugMessageDirectory + "ir_register_alloc.txt", ios::out | ios::trunc);
    irStream << "[Global Register Alloc]" << endl;
    for (auto &func : module->functions)
    {
        irStream << "function <" << func->name << ">:" << endl;
        map<unsigned int, shared_ptr<Value>> idValueMap;
        for (auto &it : func->variableRegs)
        {
            idValueMap[it.first->id] = it.first;
        }
        int cnt = 0;
        for (auto &item : idValueMap)
        {
            if (cnt == 5)
            {
                cnt = 0;
                irStream << endl;
            }
            irStream << "<" << item.first << "> R" << func->variableRegs.at(item.second) << "\t\t";
            ++cnt;
        }
        irStream << endl;
        cnt = 0;
        set<unsigned int> withoutRegMap;
        for (auto &it : func->variableWithoutReg)
        {
            withoutRegMap.insert(it->id);
        }
        for (auto &item : withoutRegMap)
        {
            if (cnt == 5)
            {
                cnt = 0;
                irStream << endl;
            }
            irStream << "<" << item << "> None\t\t";
            ++cnt;
        }
        irStream << endl;
    }
    irStream.close();
}
