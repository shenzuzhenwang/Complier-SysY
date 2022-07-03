#ifndef COMPILER_IR_SSA_H
#define COMPILER_IR_SSA_H

#include "ir.h"

extern shared_ptr<Value> read_variable(shared_ptr<BasicBlock> &bb, string &varName);

extern void write_variable(shared_ptr<BasicBlock> &bb, const string &varName, const shared_ptr<Value> &value);

extern void seal_basic_block(shared_ptr<BasicBlock> &bb);

extern shared_ptr<Value> add_phi_operands(shared_ptr<BasicBlock> &bb, string &varName, shared_ptr<PhiInstruction> &phi);

extern shared_ptr<Value> remove_trivial_phi(shared_ptr<PhiInstruction> &phi);

#endif
