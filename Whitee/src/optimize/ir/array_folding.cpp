#include "ir_optimize.h"

/**
 * @brief 局部数组传播
 * @param alloc 局部数组
 */
void fold_array(shared_ptr<AllocInstruction> &alloc)
{
    bool visit = false;
    shared_ptr<BasicBlock> &bb = alloc->block;
    unordered_map<int, shared_ptr<Value>> arrValues;  // offset <--> value  表示可以被折叠的offset对应的value
    unordered_map<int, shared_ptr<StoreInstruction>> arrStores;  // offset <--> StoreInstruction 表示可以被折叠的offset对应的StoreInstruction
    unordered_set<int> canErase;  // offset 表示可被折叠offset
    for (auto ins = bb->instructions.begin(); ins != bb->instructions.end();)  // 数组所在块的指令
    {
        if (!visit && *ins != alloc)
        {
            ++ins;
            continue;
        }
        else if (!visit && *ins == alloc)
        {
            visit = true;  // 从分配数组开始
            ++ins;
            continue;
        }

        if ((*ins)->type == InstructionType::INVOKE)
        {
            shared_ptr<InvokeInstruction> invoke = s_p_c<InvokeInstruction>(*ins);
            for (auto &arg : invoke->params)
            {
                if (arg == alloc)  // 如果视为指针作为函数参数使用，则无法折叠
                    return;
            }
        }
        else if ((*ins)->type == InstructionType::BINARY)
        {
            shared_ptr<BinaryInstruction> bin = s_p_c<BinaryInstruction>(*ins);
            if (bin->lhs == alloc || bin->rhs == alloc)  // 如果视为指针作为操作数，无法折叠
                return;
        }
        else if ((*ins)->type == InstructionType::STORE)
        {
            shared_ptr<StoreInstruction> store = s_p_c<StoreInstruction>(*ins);
            if (store->address == alloc && store->offset->value_type == ValueType::NUMBER)  // 如果store时，offset为常数
            {
                shared_ptr<NumberValue> off = s_p_c<NumberValue>(store->offset);
                if (arrStores.count(off->number) != 0 && canErase.count(off->number) != 0)  // 如果不是第一次store
                {
                    arrStores.at(off->number)->abandonUse();  // 则上一次store指令失效
                }
                else
                {
                    canErase.insert(off->number);
                }
                arrValues[off->number] = store->value;
                arrStores[off->number] = store;
            }
            else if (store->address == alloc)  // 如果offset不为常数
            {
                for (auto &item : arrStores)
                {
                    if (canErase.count(item.first) != 0)
                        item.second->abandonUse();  // 删除现有可以被折叠的store指令（因为没人用）
                }
                return;
            }
        }
        else if ((*ins)->type == InstructionType::LOAD)
        {
            shared_ptr<LoadInstruction> load = s_p_c<LoadInstruction>(*ins);
            if (load->address == alloc && load->offset->value_type == ValueType::NUMBER)  // 如果load时，offset为常数
            {
                shared_ptr<NumberValue> off = s_p_c<NumberValue>(load->offset);
                if (arrValues.count(off->number) != 0)  // 此offset元素的可被折叠  则替换词load指令的对象为已知的value
                {
                    shared_ptr<Value> val = arrValues.at(off->number);
                    unordered_set<shared_ptr<Value>> users = load->users;
                    for (auto &u : users)  // 将所有使用load的值的指令替换为使用value
                    {
                        shared_ptr<Value> toBeReplace = load;
                        u->replaceUse(toBeReplace, val);
                    }
                    if (val->value_type == INSTRUCTION && s_p_c<Instruction>(val)->resultType == R_VAL_RESULT)
                    {
                        s_p_c<Instruction>(val)->resultType = L_VAL_RESULT;
                        s_p_c<Instruction>(val)->caughtVarName = generateTempLeftValueName();
                    }
                    if (val->value_type != ValueType::NUMBER)
                        canErase.erase(off->number);  // 如果此时offset存的值不为常数，则之后不能被折叠
                    load->abandonUse();
                    ins = bb->instructions.erase(ins);
                    continue;
                }
            }
            else if (load->address == alloc)  // 不知道load数组的哪里，则所有值都不能折叠
            {
                canErase.clear();
            }
        }
        ++ins;
    }
}

/**
 * @brief 局部数组传播
 * @param module 
 */
void array_folding(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            vector<shared_ptr<Instruction>> instructions = bb->instructions;
            for (auto &ins : instructions)
            {
                if (ins->type == InstructionType::ALLOC)  // 分析局部数组
                {
                    shared_ptr<AllocInstruction> alloc = s_p_c<AllocInstruction>(ins);
                    fold_array(alloc);
                }
            }
        }
    }
}
