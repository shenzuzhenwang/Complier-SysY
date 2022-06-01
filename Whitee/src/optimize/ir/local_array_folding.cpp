#include "ir_optimize.h"

// �۵�alloc�ֲ�����
void foldLocalArray(shared_ptr<AllocInstruction> &alloc)
{
    bool visit = false;
    shared_ptr<BasicBlock> &bb = alloc->block;
    unordered_map<int, shared_ptr<Value>> arrValues;  // offset <--> value  ��ʾ���Ա��۵���offset��Ӧ��value
    unordered_map<int, shared_ptr<StoreInstruction>> arrStores;  // offset <--> StoreInstruction ��ʾ���Ա��۵���offset��Ӧ��StoreInstruction
    unordered_set<int> canErase;  // offset ��ʾ�ɱ��۵�offset
    for (auto ins = bb->instructions.begin(); ins != bb->instructions.end();)  // �������ڿ��ָ��
    {
        if (!visit && *ins != alloc)
        {
            ++ins;
            continue;
        }
        else if (!visit && *ins == alloc)
        {
            visit = true;  // �ӷ������鿪ʼ
            ++ins;
            continue;
        }

        if ((*ins)->type == InstructionType::INVOKE)
        {
            shared_ptr<InvokeInstruction> invoke = s_p_c<InvokeInstruction>(*ins);
            for (auto &arg : invoke->params)
            {
                if (arg == alloc)  // �����Ϊָ����Ϊ��������ʹ�ã����޷��۵�
                    return;
            }
        }
        else if ((*ins)->type == InstructionType::BINARY)
        {
            shared_ptr<BinaryInstruction> bin = s_p_c<BinaryInstruction>(*ins);
            if (bin->lhs == alloc || bin->rhs == alloc)  // �����Ϊָ����Ϊ������
                return;
        }
        else if ((*ins)->type == InstructionType::STORE)
        {
            shared_ptr<StoreInstruction> store = s_p_c<StoreInstruction>(*ins);
            if (store->address == alloc && store->offset->valueType == ValueType::NUMBER)  // ���storeʱ��offsetΪ����
            {
                shared_ptr<NumberValue> off = s_p_c<NumberValue>(store->offset);
                if (arrStores.count(off->number) != 0 && canErase.count(off->number) != 0)  // ������ǵ�һ��store
                {
                    arrStores.at(off->number)->abandonUse();  // ����һ��storeָ��ʧЧ
                }
                else
                {
                    canErase.insert(off->number);
                }
                arrValues[off->number] = store->value;
                arrStores[off->number] = store;
            }
            else if (store->address == alloc)  // ���offset��Ϊ����
            {
                for (auto &item : arrStores)
                {
                    if (canErase.count(item.first) != 0)
                        item.second->abandonUse();  // ɾ�����п��Ա��۵���storeָ���Ϊû���ã�
                }
                return;
            }
        }
        else if ((*ins)->type == InstructionType::LOAD)
        {
            shared_ptr<LoadInstruction> load = s_p_c<LoadInstruction>(*ins);
            if (load->address == alloc && load->offset->valueType == ValueType::NUMBER)  // ���loadʱ��offsetΪ����
            {
                shared_ptr<NumberValue> off = s_p_c<NumberValue>(load->offset);
                if (arrValues.count(off->number) != 0)  // ��offsetԪ�صĿɱ��۵�  ���滻��loadָ��Ķ���Ϊ��֪��value
                {
                    shared_ptr<Value> val = arrValues.at(off->number);
                    unordered_set<shared_ptr<Value>> users = load->users;
                    for (auto &u : users)  // ������ʹ��load��ֵ��ָ���滻Ϊʹ��value
                    {
                        shared_ptr<Value> toBeReplace = load;
                        u->replaceUse(toBeReplace, val);
                    }
                    if (val->valueType == INSTRUCTION && s_p_c<Instruction>(val)->resultType == R_VAL_RESULT)
                    {
                        s_p_c<Instruction>(val)->resultType = L_VAL_RESULT;
                        s_p_c<Instruction>(val)->caughtVarName = generateTempLeftValueName();
                    }
                    if (val->valueType != ValueType::NUMBER)
                        canErase.erase(off->number);  // �����ʱoffset���ֵ��Ϊ��������֮���ܱ��۵�
                    load->abandonUse();
                    ins = bb->instructions.erase(ins);
                    continue;
                }
            }
            else if (load->address == alloc)  // ��֪��load��������������ֵ�������۵�
            {
                canErase.clear();
            }
        }
        ++ins;
    }
}

// �ֲ������۵�
void localArrayFolding(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            vector<shared_ptr<Instruction>> instructions = bb->instructions;
            for (auto &ins : instructions)
            {
                if (ins->type == InstructionType::ALLOC)  // �����ֲ�����
                {
                    shared_ptr<AllocInstruction> alloc = s_p_c<AllocInstruction>(ins);
                    foldLocalArray(alloc);
                }
            }
        }
    }
}
