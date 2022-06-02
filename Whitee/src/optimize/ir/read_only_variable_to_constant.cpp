#include "ir_optimize.h"

/**
 * @brief 查看变量是否有被写入的可能
 * @param globalVar 全局变量
 * @return true 可能被写入；false 不可能被写入
 */
bool globalVarHasWriteUser(const shared_ptr<Value> &globalVar)
{
    for (auto &user : globalVar->users)
    {
        if (dynamic_cast<StoreInstruction *>(user.get()))
            return true;
        if (dynamic_cast<InvokeInstruction *>(user.get()))
            return true;
        if (dynamic_cast<BinaryInstruction *>(user.get()))
            if (globalVarHasWriteUser(user))
                return true;
    }
    return false;
}
/**
 * @brief 全局变量变为常量
 * @param globalVar 此全局变量
 * @param module 
 */
void globalVariableToConstant(shared_ptr<Value> &globalVar, shared_ptr<Module> &module)
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
        shared_ptr<Value> constantNumber = getNumberValue(global->initValues.at(0));
        unordered_set<shared_ptr<Value>> users = global->users;
        for (auto user : users)
        {
            if (!dynamic_cast<LoadInstruction *>(user.get()))
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
                user->abandonUse();
            }
        }
        global->abandonUse();
    }
    else  // 全局数组
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
void readOnlyVariableToConstant(shared_ptr<Module> &module)
{
    vector<shared_ptr<Value>> globalVariables = module->globalVariables;
    for (auto &globalVar : globalVariables)
    {
        if (!globalVarHasWriteUser(globalVar))
        {
            globalVariableToConstant(globalVar, module);
        }
    }
}
