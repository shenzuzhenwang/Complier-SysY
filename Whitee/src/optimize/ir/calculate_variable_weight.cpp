#include "ir_optimize.h"

/**
 * @brief 计算每个变量的权重，将使用对象的所在块loop_depth次方相加  计算权重：base + pow (_LOOP_WEIGHT_BASE, depth)
 * @param func
 */
void calculateVariableWeight (shared_ptr<Function>& func)
{
	for (auto& arg : func->params)
	{
		unsigned int tempWeight = countWeight (0, 0);   // 初始权重1
		if (func->variableWeight.count (arg) != 0)
		{
			tempWeight = func->variableWeight.at (arg);
		}
		for (auto& user : arg->users)
		{
			if (user->valueType == INSTRUCTION && s_p_c<Instruction> (user)->type != PHI)  // 使用指令为非phi指令，则权重累加
			{
				tempWeight = countWeight (s_p_c<Instruction> (user)->block->loopDepth, tempWeight);
			}
			else if (user->valueType != INSTRUCTION)
			{
				cerr << "Error occurs in process calculate variable weight: user is not an instruction." << endl;
			}
		}
		func->variableWeight[arg] = tempWeight;
	}
	for (auto& bb : func->blocks)
	{
		for (auto& ins : bb->instructions)
		{
			if (ins->resultType == L_VAL_RESULT && ins->type != PHI_MOV)  // 此指令得到左值
			{
				unsigned int tempWeight = 0;
				if (func->variableWeight.count (ins) != 0)
				{
					tempWeight = func->variableWeight.at (ins);
				}
				tempWeight = countWeight (bb->loopDepth, tempWeight);  // 此指令加权
				for (auto& user : ins->users)
				{
					if (user->valueType == INSTRUCTION && s_p_c<Instruction> (user)->type != PHI)
					{
						tempWeight = countWeight (s_p_c<Instruction> (user)->block->loopDepth, tempWeight);
					}
					else if (user->valueType != INSTRUCTION)
					{
						cerr << "Error occurs in process calculate variable weight: user is not an instruction." << endl;
					}
				}
				func->variableWeight[ins] = tempWeight;
			}
			else if (ins->type == PHI_MOV)  // phi move加权
			{
				unsigned int tempWeight = 0;
				if (func->variableWeight.count (ins) != 0)
				{
					tempWeight = func->variableWeight.at (ins);
				}
				tempWeight = countWeight (bb->loopDepth, tempWeight);
				func->variableWeight[ins] = tempWeight;
			}
		}
		for (auto& phi : bb->phis)   // phi
		{
			unsigned int tempWeight = 0;
			if (func->variableWeight.count (phi) != 0)
			{
				tempWeight = func->variableWeight.at (phi);
			}
			tempWeight = countWeight (bb->loopDepth, tempWeight); // phi加权
			for (auto& user : phi->users)
			{
				if (user->valueType == INSTRUCTION && s_p_c<Instruction> (user)->type != PHI)
				{
					tempWeight = countWeight (s_p_c<Instruction> (user)->block->loopDepth, tempWeight);
				}
				else if (user->valueType != INSTRUCTION)
				{
					cerr << "Error occurs in process calculate variable weight: user is not an instruction." << endl;
				}
			}
			func->variableWeight[phi] = tempWeight;
			for (auto& operand : phi->operands)   // phi的操作数再次加权
			{
				if (operand.second->valueType != INSTRUCTION)
					continue;
				unsigned int opWeight = 0;
				if (func->variableWeight.count (operand.second) != 0)
				{
					opWeight = func->variableWeight.at (operand.second);
				}
				opWeight = countWeight (operand.first->loopDepth, opWeight);
				func->variableWeight[operand.second] = opWeight;
			}
			if (phi->phiMove == nullptr)
			{
				cerr << "Error occurs in process calculate variable weight: null pointer phi move." << endl;
			}
			else    //phi_move加权
			{
				shared_ptr<PhiMoveInstruction> phiMov = phi->phiMove;
				unsigned int movWeight = 0;
				if (func->variableWeight.count (phiMov) != 0)
				{
					movWeight = func->variableWeight.at (phiMov);
				}
				movWeight = countWeight (bb->loopDepth, movWeight);
				func->variableWeight[phiMov] = movWeight;
			}
		}
	}
}
