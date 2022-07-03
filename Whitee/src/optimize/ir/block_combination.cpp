#include "ir_optimize.h"

/**
 * @brief 基本块合并
 * @param module 
 */
void block_combination(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (int i = 0; i < func->blocks.size(); ++i)
        {
            shared_ptr<BasicBlock> &block = func->blocks.at(i);
            if (block->instructions.empty())
                continue;
            if (block->successors.size() == 1)  // 只有一个后继块
            {
                shared_ptr<BasicBlock> successor = *block->successors.begin();
                if (successor != block && successor->predecessors.size() == 1)
                {
                    vector<shared_ptr<Instruction>> sIns = successor->instructions;
                    if (block->instructions.at(block->instructions.size() - 1)->type != InstructionType::JMP)
                    {
                        cerr << "Error occurs in process block combination: the last instruction is not jump." << endl;
                    }
                    block->instructions.erase(--block->instructions.end());  // 删去跳转
                    block->instructions.insert(block->instructions.end(), successor->instructions.begin(), successor->instructions.end());  // 将后继块所有指令加入
                    for (auto &ins : successor->instructions)
                    {
                        ins->block = block;
                    }
                    if (!successor->phis.empty())
                    {
                        cerr << "Error occurs in process block combination: phis is not empty." << endl;
                    }
                    block->successors = successor->successors;  // 更改前驱后继块
                    // TODO: MERGE LOCAL VAR SSA MAP?
                    unordered_set<shared_ptr<BasicBlock>> successors = block->successors;
                    for (auto &it : successors)
                    {
                        it->predecessors.insert(block);
                        unordered_set<shared_ptr<PhiInstruction>> phis = it->phis;
                        for (auto &phi : phis)  //  替换使用对象
                        {
                            phi->replaceUse(successor, block);
                        }
                    }
                    successor->abandonUse();
                    --i;
                }
            }
        }
    }
}
