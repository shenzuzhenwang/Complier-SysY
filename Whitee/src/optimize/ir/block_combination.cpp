#include "ir_optimize.h"

/**
 * @brief 基本块合并
 * @param module 
 */
void blockCombination(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (int i = 0; i < func->blocks.size(); ++i)
        {
            shared_ptr<BasicBlock> &bb = func->blocks.at(i);
            if (bb->instructions.empty())
                continue;
            if (bb->successors.size() == 1)  // 只有一个后继块
            {
                shared_ptr<BasicBlock> successor = *bb->successors.begin();
                if (successor != bb && successor->predecessors.size() == 1)
                {
                    vector<shared_ptr<Instruction>> sIns = successor->instructions;
                    if (bb->instructions.at(bb->instructions.size() - 1)->type != InstructionType::JMP)
                    {
                        cerr << "Error occurs in process block combination: the last instruction is not jump." << endl;
                    }
                    bb->instructions.erase(--bb->instructions.end());  // 删去跳转
                    bb->instructions.insert(bb->instructions.end(), successor->instructions.begin(), successor->instructions.end());  // 将后继块所有指令加入
                    for (auto &ins : successor->instructions)
                    {
                        ins->block = bb;
                    }
                    if (!successor->phis.empty())
                    {
                        cerr << "Error occurs in process block combination: phis is not empty." << endl;
                    }
                    bb->successors = successor->successors;  // 更改前驱后继块
                    // TODO: MERGE LOCAL VAR SSA MAP?
                    unordered_set<shared_ptr<BasicBlock>> successors = bb->successors;
                    for (auto &it : successors)
                    {
                        it->predecessors.insert(bb);
                        unordered_set<shared_ptr<PhiInstruction>> phis = it->phis;
                        for (auto &phi : phis)  //  替换使用对象
                        {
                            phi->replaceUse(successor, bb);
                        }
                    }
                    successor->abandonUse();
                    --i;
                }
            }
        }
    }
}
