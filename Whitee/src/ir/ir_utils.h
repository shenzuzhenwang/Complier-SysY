#ifndef COMPILER_IR_UTILS_H
#define COMPILER_IR_UTILS_H

#include "ir.h"
#include "ir_ssa.h"

// used at any time when optimizing ir.
extern const unordered_set<InstructionType> noResultTypes;

extern void unused_instruction_delete(shared_ptr<BasicBlock> &bb);

extern void unused_block_delete(shared_ptr<Function> &func);

extern void unused_function_delete(shared_ptr<Module> &module);

extern void block_predecessor_delete(shared_ptr<BasicBlock> &bb, shared_ptr<BasicBlock> &pre);

extern void function_is_side_effect(shared_ptr<Module> &module);

extern void user_use(const shared_ptr<Value> &user, initializer_list<shared_ptr<Value>> used);

extern void user_use(const shared_ptr<Value> &user, const vector<shared_ptr<Value>> &used);

// used in ir built finished.
extern void removePhiUserBlocksAndMultiCmp(shared_ptr<Module> &module);

// used in ir or optimize finished.
extern void fixRightValue(shared_ptr<Module> &module);

extern void getFunctionRequiredStackSize(shared_ptr<Function> &func);

extern void phi_elimination(shared_ptr<Function> &func);

extern void mergeAliveValuesToInstruction(shared_ptr<Function> &func);

#endif
