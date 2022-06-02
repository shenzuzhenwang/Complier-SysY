#include "ir_optimize.h"

/**
 * @brief 去掉只有store指令的变量，包括全局变量与数组
 * @param module 
 */
void deadArrayDelete(shared_ptr<Module> &module)
{
    for (auto glb = module->globalVariables.begin(); glb != module->globalVariables.end();)
    {
        unordered_set<shared_ptr<Value>> users = (*glb)->users;
        for (auto &user : users)
        {
            if (!dynamic_cast<StoreInstruction *>(user.get()))  // 不是store指令，则换下一个全局变量
            {
                goto GLOBAL_USER_STORE_JUDGE;
            }
        }
        for (auto &user : users)  // 只有store指令，则将store指令去除，并将此全局变量去除
        {
            user->abandonUse();
        }
        (*glb)->abandonUse();
        glb = module->globalVariables.erase(glb);
        continue;
    GLOBAL_USER_STORE_JUDGE:
        ++glb;
    }
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            for (auto &ins : bb->instructions)
            {
                if (ins->type == InstructionType::ALLOC)
                {
                    bool canDelete = true;
                    for (auto &user : ins->users)
                    {
                        if (!dynamic_cast<StoreInstruction *>(user.get()))  // 出现非store指令
                        {
                            canDelete = false;
                            break;
                        }
                    }
                    if (canDelete) // 只有store指令，则将store指令去除，并将此变量去除
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
