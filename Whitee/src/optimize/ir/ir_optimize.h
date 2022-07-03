/*********************************************************************
 * @file   ir_optimize.h
 * @brief  https://compileroptimizations.com/index.html 优化类型
 * 
 * @author 神祖
 * @date   July 2022
 *********************************************************************/
#ifndef COMPILER_IR_OPTIMIZE_H
#define COMPILER_IR_OPTIMIZE_H

#include "../../ir/ir.h"
#include "../../ir/ir_utils.h"
#include "../../ir/ir_ssa.h"
#include "../../ir/ir_check.h"

#include <iostream>
#include <fstream>

extern string debugMessageDirectory;

extern const unsigned int OPTIMIZE_TIMES;

extern unsigned long DEAD_BLOCK_CODE_GROUP_DELETE_TIMEOUT;
extern unsigned long CONFLICT_GRAPH_TIMEOUT;

extern void optimizeIr(shared_ptr<Module> &module, OptimizeLevel level);

void constant_folding(shared_ptr<Module> &module);

void dead_code_delete(shared_ptr<Module> &module);

//void functionInline(shared_ptr<Module> &module);

void constant_branch_conversion(shared_ptr<Module> &module);

void block_combination(shared_ptr<Module> &module);

void read_only_variable_to_constant(shared_ptr<Module> &module);

void array_folding(shared_ptr<Module> &module);

void dead_array_delete(shared_ptr<Module> &module);

void array_external(shared_ptr<Module> &module);

//void deadBlockCodeGroupDelete(shared_ptr<Module> &module);

void loop_invariant_code_motion(shared_ptr<Module> &module);

void local_common_subexpression_elimination(shared_ptr<Module> &module);

// some end optimize functions.
void endOptimize(shared_ptr<Module> &module, OptimizeLevel level);

void calculateVariableWeight(shared_ptr<Function> &func);

void registerAlloc(shared_ptr<Function> &func);

#endif
