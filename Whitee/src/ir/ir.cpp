#include "ir.h"
#include <cmath>

unsigned int Value::valueId = 0;

unsigned int Value::getValueId ()
{
	return valueId++;
}

ConstantValue::ConstantValue (shared_ptr<ConstDefNode>& constDef)
	: BaseValue (ValueType::CONSTANT)
{
	name = constDef->ident->ident->usageName;
	dimensions = constDef->ident->ident->numOfEachDimension;
	size = 1;
	for (const int& i : dimensions)
	{
		size *= i;
	}
	vector<pair<int, int>> vec = constDef->constInitVal->toOneDimensionArray (0, size);
	for (const auto p : vec)
	{
		values.insert (p);
	}
}

ConstantValue::ConstantValue (shared_ptr<GlobalValue>& globalVar)
	: BaseValue (ValueType::CONSTANT)
{
	name = globalVar->name;
	dimensions = globalVar->dimensions;
	size = globalVar->size;
	values = globalVar->initValues;
	for (int i = 0; i < size; ++i)
	{
		if (values.count (i) == 0)
			values[i] = 0;
	}
}

ParameterValue::ParameterValue (shared_ptr<Function>& function, shared_ptr<FuncFParamNode>& funcFParam)
	: BaseValue (ValueType::PARAMETER)
{
	value_type = ValueType::PARAMETER;
	name = funcFParam->ident->ident->usageName;
	this->function = function;
	variableType = funcFParam->dimension == 0 ? VariableType::INT : VariableType::POINTER;
	dimensions = funcFParam->dimensions;
}

GlobalValue::GlobalValue (shared_ptr<VarDefNode>& varDef)
	: BaseValue (ValueType::GLOBAL)
{
	name = varDef->ident->ident->usageName;
	value_type = ValueType::GLOBAL;
	initType = varDef->type;
	variableType = varDef->dimension == 0 ? VariableType::INT : VariableType::POINTER;
	dimensions = varDef->dimensions;
	size = 1;
	for (const int& i : dimensions)
	{
		size *= i;
	}
	if (varDef->ident->ident->globalVarInitVal)
	{
		vector<pair<int, int>> vec = varDef->ident->ident->globalVarInitVal->toOneDimensionArray (0, size);
		for (const auto p : vec)
		{
			initValues.insert (p);
		}
	}
}

void Function::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Function> self = s_p_c<Function> (shared_from_this ());
	for (auto& f : callers)
	{
		f->callees.erase (self);
	}
	for (auto& f : callees)
	{
		f->callers.erase (self);
	}
	callers.clear ();
	callees.clear ();
	params.clear ();
	blocks.clear ();
	variables.clear ();
	entryBlock = nullptr;
	name = "abandon_function_" + name;
}

// 替换value
void BasicBlock::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	for (auto& it : ssa_map)
	{
		if (it.second == toBeReplaced)
		{
			toBeReplaced->users.erase (self);
			it.second = replaceValue;
			if (dynamic_cast<PhiInstruction*>(replaceValue.get ()))
				replaceValue->users.insert (self);
		}
	}
}

void BasicBlock::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<BasicBlock> self = s_p_c<BasicBlock> (shared_from_this ());
	for (auto& it : successors)
	{
		it->predecessors.erase (self);
	}
	for (auto& it : predecessors)
	{
		it->successors.erase (self);
	}
	predecessors.clear ();
	successors.clear ();
	instructions.clear ();
	phis.clear ();
}

bool NumberValue::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == self)
		return true;
	if (value->value_type != NUMBER)
		return false;
	return s_p_c<NumberValue> (value)->number == number;
}

bool ConstantValue::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == self)
		return true;
	if (value->value_type != CONSTANT)
		return false;
	return s_p_c<ConstantValue> (value)->name == name;
}

bool ParameterValue::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == self)
		return true;
	if (value->value_type != PARAMETER)
		return false;
	return s_p_c<ParameterValue> (value)->name == name;
}

bool GlobalValue::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == self)
		return true;
	if (value->value_type != GLOBAL)
		return false;
	return s_p_c<GlobalValue> (value)->name == name;
}

bool StringValue::equals (shared_ptr<Value>& value)
{
	if (value->value_type != STRING)
		return false;
	return s_p_c<StringValue> (value)->str == str;
}

// 替换value
void ReturnInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == toBeReplaced)
	{
		value->users.erase (self);
		value = replaceValue;
		replaceValue->users.insert (self);
	}
}

void ReturnInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	value->users.erase (self);
	if (value->users.empty () && !dynamic_cast<InvokeInstruction*>(value.get ()))
	{
		value->abandonUse ();
	}
}

// 替换value
void BranchInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	if (condition == toBeReplaced)
	{
		condition->users.erase (self);
		condition = replaceValue;
		replaceValue->users.insert (self);
	}
}

void BranchInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	condition->users.erase (self);
	if (condition->users.empty () && !dynamic_cast<InvokeInstruction*>(condition.get ()))
	{
		condition->abandonUse ();
	}
}

void JumpInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{}

void JumpInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
}

// 自带函数
unordered_map<string, InvokeType> InvokeInstruction::sysFuncMap {
																{"getint", InvokeType::GET_INT},
																{"getch", InvokeType::GET_CHAR},
																{"getarray", InvokeType::GET_ARRAY},
																{"putint", InvokeType::PUT_INT},
																{"putch", InvokeType::PUT_CHAR},
																{"putarray", InvokeType::PUT_ARRAY},
																{"putf", InvokeType::PUT_F},
																{"starttime", InvokeType::START_TIME},
																{"stoptime", InvokeType::STOP_TIME} };
// 替换value
void InvokeInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	for (auto& arg : params)
	{
		if (arg == toBeReplaced)
		{
			arg->users.erase (self);
			arg = replaceValue;
			replaceValue->users.insert (self);
		}
	}
}

void InvokeInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	for (auto& arg : params)
	{
		arg->users.erase (self);
		if (arg->users.empty () && !dynamic_cast<InvokeInstruction*>(arg.get ()))
		{
			arg->abandonUse ();
		}
	}
}

bool InvokeInstruction::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	return self == value;
}

// 替换value
void UnaryInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == toBeReplaced)
	{
		value->users.erase (self);
		value = replaceValue;
		replaceValue->users.insert (self);
	}
}

void UnaryInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	value->users.erase (self);
	if (value->users.empty () && !dynamic_cast<InvokeInstruction*>(value.get ()))
	{
		value->abandonUse ();
	}
}

bool UnaryInstruction::equals (shared_ptr<Value>& val)
{
	shared_ptr<Value> self = shared_from_this ();
	if (val == self)
		return true;
	if (!dynamic_cast<UnaryInstruction*>(val.get ()))
		return false;
	shared_ptr<UnaryInstruction> unary = s_p_c<UnaryInstruction> (val);
	if (unary->value == value && unary->op == op)
		return true;
	return unary->op == op && unary->value->equals (value);
}

void BinaryInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	if (lhs == toBeReplaced)
	{
		lhs->users.erase (self);
		lhs = replaceValue;
		replaceValue->users.insert (self);
	}
	if (rhs == toBeReplaced)
	{
		rhs->users.erase (self);
		rhs = replaceValue;
		replaceValue->users.insert (self);
	}
}

void BinaryInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	lhs->users.erase (self);
	rhs->users.erase (self);
	if (lhs->users.empty () && !dynamic_cast<InvokeInstruction*>(lhs.get ()))
	{
		lhs->abandonUse ();
	}
	if (rhs->users.empty () && !dynamic_cast<InvokeInstruction*>(rhs.get ()))
	{
		rhs->abandonUse ();
	}
}

bool BinaryInstruction::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == self)
		return true;
	if (!dynamic_cast<BinaryInstruction*>(value.get ()))
		return false;
	shared_ptr<BinaryInstruction> binary = s_p_c<BinaryInstruction> (value);
	if (binary->lhs == lhs && binary->rhs == rhs && binary->op == op)
		return true;
	if ((binary->op == "+" || binary->op == "*") && binary->op == op)
	{
		if (binary->lhs == rhs && binary->rhs == lhs)
			return true;
		return (binary->lhs->equals (rhs) && binary->rhs->equals (lhs)) || (binary->lhs->equals (lhs) && binary->rhs->equals (rhs));
	}
	return binary->op == op && binary->lhs->equals (lhs) && binary->rhs->equals (rhs);
}

void AllocInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{}

void AllocInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
}

bool AllocInstruction::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	return value == self;
}

void StoreInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	if (value == toBeReplaced)
	{
		value->users.erase (self);
		value = replaceValue;
		replaceValue->users.insert (self);
	}
	if (address == toBeReplaced)
	{
		address->users.erase (self);
		address = replaceValue;
		replaceValue->users.insert (self);
	}
	if (offset == toBeReplaced)
	{
		offset->users.erase (self);
		offset = replaceValue;
		replaceValue->users.insert (self);
	}
}

void StoreInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	value->users.erase (self);
	address->users.erase (self);
	offset->users.erase (self);
	if (value->users.empty () && !dynamic_cast<InvokeInstruction*>(value.get ()))
		value->abandonUse ();
	if (address->users.empty () && !dynamic_cast<InvokeInstruction*>(address.get ()))
		address->abandonUse ();
	if (offset->users.empty () && !dynamic_cast<InvokeInstruction*>(offset.get ()))
		offset->abandonUse ();
}

void LoadInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	if (address == toBeReplaced)
	{
		address->users.erase (self);
		address = replaceValue;
		replaceValue->users.insert (self);
	}
	if (offset == toBeReplaced)
	{
		offset->users.erase (self);
		offset = replaceValue;
		replaceValue->users.insert (self);
	}
}

void LoadInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	address->users.erase (self);
	offset->users.erase (self);
	if (address->users.empty () && !dynamic_cast<InvokeInstruction*>(address.get ()))
		address->abandonUse ();
	if (offset->users.empty () && !dynamic_cast<InvokeInstruction*>(offset.get ()))
		offset->abandonUse ();
}

bool LoadInstruction::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	return value == self;
}

/**
 * phi指令不仅要替换值，还要替换基本块。
 */
void PhiInstruction::replaceUse (shared_ptr<Value>& toBeReplaced, shared_ptr<Value>& replaceValue)
{
	shared_ptr<Value> self = shared_from_this ();
	for (auto& op : operands)
	{
		if (op.second == toBeReplaced)
		{
			op.second->users.erase (self);
			replaceValue->users.insert (self);
			op.second = replaceValue;
		}
	}
}

void PhiInstruction::replaceUse (shared_ptr<BasicBlock>& toBeReplaced, shared_ptr<BasicBlock>& replaceBlock)
{
	for (auto& op : operands)
	{
		if (op.first == toBeReplaced)
		{
			shared_ptr<Value> oldVal = op.second;
			operands.erase (op.first);
			operands[replaceBlock] = oldVal;
			oldVal->users.erase (toBeReplaced);
			return;
		}
	}
}

void PhiInstruction::abandonUse ()
{
	if (!valid)
		return;
	valid = false;
	shared_ptr<Value> self = shared_from_this ();
	for (auto& it : operands)
	{
		it.second->users.erase (self);
		if (it.second->users.empty () && !dynamic_cast<InvokeInstruction*>(it.second.get ()))
		{
			it.second->abandonUse ();
		}
	}
}

// 计算phi Operand中与value相等的个数
int PhiInstruction::getOperandValueCount (const shared_ptr<Value>& value)
{
	int cnt = 0;
	for (auto& it : operands)
	{
		cnt += it.second == value;
	}
	return cnt;
}

// 仅被基本块使用
bool PhiInstruction::onlyHasBlockUserOrUserEmpty ()
{
	for (auto& user : users)
	{
		if (user->value_type != ValueType::BASIC_BLOCK)
			return false;
	}
	return true;
}

bool PhiInstruction::equals (shared_ptr<Value>& value)
{
	shared_ptr<Value> self = shared_from_this ();
	return self == value;
}

PhiMoveInstruction::PhiMoveInstruction (shared_ptr<PhiInstruction>& phi)
	: Instruction (InstructionType::PHI_MOV, phi->block, L_VAL_RESULT), phi (phi)
{
	caughtVarName = phi->caughtVarName;
	for (auto& op : phi->operands)
	{
		unordered_set<shared_ptr<Value>> tempSet;
		blockALiveValues[op.first] = tempSet;
	}
}

unordered_map<int, shared_ptr<NumberValue>> numberValueMap;  // 常数公用表

shared_ptr<NumberValue> Number (int number)
{
	if (numberValueMap.count (number) == 0)
		numberValueMap[number] = make_shared<NumberValue> (number);
	return numberValueMap.at (number);
}

// 生成参数 LVal 名称
string generateArgumentLeftValueName (const string& functionName)
{
	static unordered_map<string, int> functionCallTimesMap;
	if (functionCallTimesMap.count (functionName) != 0)
	{
		functionCallTimesMap[functionName] = functionCallTimesMap.at (functionName) + 1;
		return "Arg_" + functionName + "_" + to_string (functionCallTimesMap.at (functionName));
	}
	functionCallTimesMap[functionName] = 0;
	return "Arg_" + functionName + "_" + to_string (functionCallTimesMap.at (functionName));
}

// 生成phi LVal 名称
string generatePhiLeftValueName (const string& phiName)
{
	static unordered_map<string, int> phiLeftValueMap;
	if (phiLeftValueMap.count (phiName) != 0)
	{
		phiLeftValueMap[phiName] = phiLeftValueMap.at (phiName) + 1;
		return "Phi_" + phiName + "_" + to_string (phiLeftValueMap.at (phiName));
	}
	phiLeftValueMap[phiName] = 0;
	return "Phi_" + phiName + "_" + to_string (phiLeftValueMap.at (phiName));
}

// 生成临时 LVal 名称
string generateTempLeftValueName ()
{
	static int tempCount = 0;
	return "Temp_" + to_string (tempCount++);
}

// 计算权重：base + pow (_LOOP_WEIGHT_BASE, depth)
unsigned int countWeight (unsigned int depth, unsigned int base)
{
	unsigned int max = depth < _MAX_DEPTH ? depth : _MAX_DEPTH;
	return (unsigned int) (pow (_LOOP_WEIGHT_BASE, max) + base < _MAX_LOOP_WEIGHT ? pow (_LOOP_WEIGHT_BASE, max) + base : base);
}
