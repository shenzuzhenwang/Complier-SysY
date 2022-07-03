#include "ir_optimize.h"

/**
 * @brief 去除无用分支
 * @param module 
 */
void constant_branch_conversion(shared_ptr<Module> &module)
{
    for (auto &func : module->functions)
    {
        for (auto &bb : func->blocks)
        {
            for (auto &ins : bb->instructions)
            {
                if (ins->type == InstructionType::BR)  // 分支指令
                {
                    shared_ptr<BranchInstruction> br = s_p_c<BranchInstruction>(ins);
                    if (br->condition->valueType == ValueType::NUMBER)  // 分支条件为常数
                    {
                        shared_ptr<NumberValue> num = s_p_c<NumberValue>(br->condition);
                        if (num->number == 0)
                        {
                            removeBlockPredecessor(br->trueBlock, bb);  // 去掉其trueBlock
                            ins = make_shared<JumpInstruction>(br->falseBlock, bb);  // 变分支指令为跳转
                            br->abandonUse();
                        }
                        else
                        {
                            removeBlockPredecessor(br->falseBlock, bb);  // 去掉其falseBlock
                            ins = make_shared<JumpInstruction>(br->trueBlock, bb);
                            br->abandonUse();
                        }
                    }
                }
            }
        }
    }
}
