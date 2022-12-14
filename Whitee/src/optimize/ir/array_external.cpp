#include "ir_optimize.h"

/**
 * @brief 局部常量数组全局化
 * @param module 
 */
void array_external(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            for (auto &ins : bb->instructions)
            {
                if (ins->type == InstructionType::ALLOC)  // 对于一个局部数组
                {
                    bool can_lift = true;
                    map<int, int> const_values;
                    unordered_set<shared_ptr<Value>> ins_users = ins->users;
                    for (auto &user : ins_users)
                    {
                        if (dynamic_cast<StoreInstruction *>(user.get()))  // 数组的store指令
                        {
                            shared_ptr<StoreInstruction> store = s_p_c<StoreInstruction>(user);
                            if (store->value->value_type == NUMBER && store->offset->value_type == NUMBER)  // store的value与offset均为常数
                            {
                                int number = s_p_c<NumberValue>(store->offset)->number;
                                if (const_values.count(number) != 0)
                                {
                                    can_lift = false;
                                    break;
                                }
                                else  // 且每个元素只有一次store
                                {
                                    const_values[number] = s_p_c<NumberValue>(store->value)->number;
                                }
                            }
                            else
                            {
                                can_lift = false;
                                break;
                            }
                        }
                        else if (!dynamic_cast<LoadInstruction *>(user.get()))  // 如果其中有非load和store指令（即当做指针使用），则不能
                        {
                            can_lift = false;
                            break;
                        }
                    }
                    if (can_lift)
                    {
                        shared_ptr<AllocInstruction> alloc = s_p_c<AllocInstruction>(ins);
                        shared_ptr<Value> allocVal = alloc;
                        shared_ptr<ConstantValue> constant = make_shared<ConstantValue>();
                        shared_ptr<Value> constVal = constant;
                        constant->size = alloc->units;
                        constant->dimensions = vector({alloc->units});
                        constant->values = const_values;
                        constant->name = alloc->name;
                        module->globalConstants.push_back(constant);  // 局部数组转换为全局常量数组
                        unordered_set<shared_ptr<Value>> users = alloc->users;
                        for (auto &user : users)
                        {
                            if (dynamic_cast<StoreInstruction *>(user.get()))
                            {
                                user->abandonUse();
                            }
                            else
                            {
                                user->replaceUse(allocVal, constVal);
                            }
                        }
                        alloc->abandonUse();
                    }
                }
            }
        }
    }
}
