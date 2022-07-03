#include "ir_optimize.h"

/**
 * @brief 只写变量清除，去掉只有store指令的变量，包括全局变量与数组
 * @param module 
 */
void dead_array_delete(shared_ptr<Module> &module)
{
    for (auto var = module->globalVariables.begin(); var != module->globalVariables.end();)
    {
        bool all_store = true;
        unordered_set<shared_ptr<Value>> users = (*var)->users;
        for (auto &user : users)
        {
            if (!dynamic_cast<StoreInstruction *>(user.get()))  // 不是store指令，则换下一个全局变量
            {
                all_store = false;
                break;
            }
        }
        if (all_store)
        {
            for (auto& user : users)  // 只有store指令，则将store指令去除，并将此全局变量去除
            {
                user->abandonUse ();
            }
            (*var)->abandonUse ();
            var = module->globalVariables.erase (var);
        }
        else
            ++var;
    }
    for (auto &func : module->functions)
    {
        for (auto &block : func->blocks)
        {
            for (auto &ins : block->instructions)
            {
                if (ins->type == InstructionType::ALLOC)
                {
                    bool can_delete = true;
                    for (auto &user : ins->users)
                    {
                        if (!dynamic_cast<StoreInstruction *>(user.get()))  // 出现非store指令
                        {
                            can_delete = false;
                            break;
                        }
                    }
                    if (can_delete) // 只有store指令，则将store指令去除，并将此变量去除
                    {
                        ins->abandonUse();
                        unordered_set<shared_ptr<Value>> users = ins->users;
                        for (auto &user : users)
                        {
                            user->abandonUse();
                        }
                    }
                }
            }
        }
    }
}
