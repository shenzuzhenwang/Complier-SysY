#include "ir_optimize.h"

/**
 * @brief �鿴�����Ƿ��б�д��Ŀ���
 * @param globalVar ȫ�ֱ���
 * @return true ���ܱ�д�룻false �����ܱ�д��
 */
bool global_var_has_write_user(const shared_ptr<Value> &globalVar)
{
    for (auto &user : globalVar->users)
    {
        if (dynamic_cast<StoreInstruction *>(user.get()))
            return true;
        if (dynamic_cast<InvokeInstruction *>(user.get())) // ����ȫ�ֱ������飬��Ϊָ��
            return true;
        if (dynamic_cast<BinaryInstruction *>(user.get()))  // ��Ϊָ�룬���ܱ��ı�
            if (global_var_has_write_user(user))
                return true;
    }
    return false;
}
/**
 * @brief ȫ�ֱ�����Ϊ����
 * @param globalVar ��ȫ�ֱ���
 * @param module 
 */
void global_variable_to_constant(shared_ptr<Value> &globalVar, shared_ptr<Module> &module)
{
    for (auto it = module->globalVariables.begin(); it != module->globalVariables.end(); ++it) // ɾȥȫ�ֱ���
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
            if (!dynamic_cast<LoadInstruction *>(user.get()))  // intȫ�ֱ�����������load
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
                user->abandonUse();  // ȫ�ֱ���תΪ�˳���������load����
            }
        }
        global->abandonUse();  // ȫ�ֱ�������
    }
    else  // ȫ�������Ϊconst array
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
 * @brief ֻ��ȫ�ֱ���תΪ����
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
