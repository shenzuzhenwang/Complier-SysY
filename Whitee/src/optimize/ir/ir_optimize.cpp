/*********************************************************************
 * @file   ir_optimize.cpp
 * @brief  IR优化
 * 
 * @author 神祖
 * @date   June 2022
 *********************************************************************/
#include "ir_optimize.h"
#include "../../basic/std/compile_std.h"

// 未检查IR是否在通过后发生变化。

const unsigned int OPTIMIZE_TIMES = 2;  // 优化重复次数

extern bool needIrCheck;      // 需要最后检查IR
extern bool needIrPassCheck;  // 需要每次检查IR

/**
 * @brief 优化IR
 * @param module 优化IR对象
 * @param level 优化等级
 */
void optimizeIr(shared_ptr<Module> &module, OptimizeLevel level)
{
    //for (int i = 0; i < OPTIMIZE_TIMES; ++i)    ？？？？感觉没用
    //    deadCodeElimination(module);

    for (int i = 0; i < OPTIMIZE_TIMES; ++i)  // 连续优化2次
    {
        globalIrCorrect = true;
        if (level >= O1)
        {
            readOnlyVariableToConstant(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Non-write variable to constant." << endl;
        }

        if (level >= O1)
        {
            constantFolding(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Constant Folding." << endl;
        }

        if (level >= O1)  // 可以不用
        {
            localArrayFolding(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Local Array Folding." << endl;
        }

        if (level >= O1)
        {
            deadArrayDelete(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Dead Array Delete." << endl;
        }

        if (level >= O1)
        {
            arrayExternalLift(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Array External Lift." << endl;
        }

        if (level >= O1)
        {
            loopInvariantCodeMotion(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Loop Invariant Code Motion." << endl;
        }

        if (level >= O1)
        {
            localCommonSubexpressionElimination(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Local Common Subexpression Elimination." << endl;
        }

        if (level >= O1)
        {
            constantBranchConversion(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Constant Branch Conversion." << endl;
        }

        if (level >= O1)
        {
            blockCombination(module);
            deadCodeElimination(module);
            if (needIrPassCheck && !irCheck(module))
                cerr << "Error: Block Combination." << endl;
        }

        if (_debugIrOptimize)
        {
            const string fileName = debugMessageDirectory + "optimize" + _SLASH_STRING + "ir_op_pass_" + to_string(i + 1) + ".txt";
            ofstream irOptimizeStream(fileName, ios::out | ios::trunc);
            irOptimizeStream << module->toString() << endl;
            irOptimizeStream.close();
        }

        if (needIrCheck && !irCheck(module))
        {
            cout << "Error: IR is not correct after optimize pass " << to_string(i + 1) << "." << endl;
            cerr << "Error: IR is not correct after optimize pass " << to_string(i + 1) << "." << endl;
            exit(_IR_OP_CHK_ERR);
        }
    }

    for (int i = 0; i < OPTIMIZE_TIMES; ++i)
        deadCodeElimination(module);

    irUserCheck = true;
    if (needIrCheck && !irCheck(module))
    {
        cout << "Error: IR is not correct after optimize." << endl;
        cerr << "Error: IR is not correct after optimize." << endl;
        exit(_IR_OP_CHK_ERR);
    }
    endOptimize(module, level);
}
