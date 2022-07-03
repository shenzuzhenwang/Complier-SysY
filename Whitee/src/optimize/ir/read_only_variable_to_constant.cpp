#include "ir_optimize.h"

/**
 * @brief 查看变量是否有被写入的可能
 * @param globalVar 全局变量
 * @return true 可能被写入；false 不可能被写入
 */
bool global_var_has_write_user(const shared_ptr<Value> &globalVar)
{
    for (auto &user : globalVar->users)
    {
        if (dynamic_cast<StoreInstruction *>(user.get()))
            return true;
        if (dynamic_cast<InvokeInstruction *>(user.get())) // 调用全局变量数组，作为指针
            return true;
        if (dynamic_cast<BinaryInstruction *>(user.get()))  // 作为指针，可能被改变
            if (global_var_has_write_user(user))
                return true;
    }
    return false;
}
/**
 * @brief 全局变量变为常量
 * @param globalVar 此全局变量
 * @param module 
 */
void global_variable_to_constant(shared_ptr<Value> &globalVar, shared_ptr<Module> &module)
{
    for (auto it = module->globalVariables.begin(); it != module->globalVariables.end(); ++it) // 删去全局变量
    {
        if (*it == globalVar)
        {
            module->globalVariables.erase(it);
            break;
        }
    }
    shared_ptr<GlobalValue> global = s_p_c<GlobalValue>(globalVar);
    if (global->variableType == VariableType::INT)
    {
        shared_ptr<Value> constantNumber = Number(global->initValues.at(0));
        unordered_set<shared_ptr<Value>> users = global->users;
        for (auto user : users)
        {
            if (!dynamic_cast<LoadInstruction *>(user.get()))  // int全局变量，仅允许load
            {
                cerr << "Error occurs in process global variable to constant: global has other type users except load."<< endl;
            }
            else
            {
                unordered_set<shared_ptr<Value>> userUsers = user->users;
                for (auto &loadUser : userUsers)
                {
                    loadUser->replaceUse(user, constantNumber);
                }
                user->abandonUse();  // 全局变量转为了常数，无需load操作
            }
        }
        global->abandonUse();  // 全局变量弃用
    }
    else  // 全局数组变为const array
    {
        shared_ptr<Value> constantArray = make_shared<ConstantValue>(global);
        unordered_set<shared_ptr<Value>> users = global->users;
        for (auto &user : users)
        {
            user->replaceUse(globalVar, constantArray);
        }
        global->abandonUse();
        module->globalConstants.push_back(constantArray);
    }
}

/**
 * @brief 只读全局变量转为常数
 * @param module 
 */
void read_only_variable_to_constant(shared_ptr<Module> &module)
{
    vector<shared_ptr<Value>> globalVariables = module->globalVariables;
    for (auto &globalVar : globalVariables)
    {
        if (!global_var_has_write_user(globalVar))
        {
            global_variable_to_constant(globalVar, module);
        }
    }
}
