/*********************************************************************
 * @file   ir.h
 * @brief  根据IR的不同指令，定义不同的类
 * 
 * @author 神祖
 * @date   May 2022
 *********************************************************************/
#ifndef COMPILER_IR_H
#define COMPILER_IR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <unordered_set>
#include <unordered_map>

#include "../front/syntax/syntax_tree.h"

using namespace std;

class Value;

class BasicBlock;

class Function;

class Instruction;

class Module;

class BaseValue;

class ConstantValue;

class StringValue;

class GlobalValue;

class ParameterValue;

class NumberValue;

class UndefinedValue;

class ReturnInstruction;

class BranchInstruction;

class JumpInstruction;

class InvokeInstruction;

class UnaryInstruction;

class BinaryInstruction;

class AllocInstruction;

class LoadInstruction;

class StoreInstruction;

class PhiInstruction;

class PhiMoveInstruction;

enum ValueType
{
    CONSTANT,  // const array
    NUMBER,  // 临时变量即数字
    STRING,
    GLOBAL,  // 全局变量，包括数组或公共变量
    PARAMETER,  // 函数形参
    UNDEFINED,  // 未定义SSA
    INSTRUCTION,  // 指令得到的值（一般值）
    MODULE,   // 每个程序的moudel
    FUNCTION,  // 函数
    BASIC_BLOCK  // 基本块
};

enum VariableType
{
    INT,  // 整型
    POINTER  // 整型指针
};

enum InstructionType
{
    RET,  // return
    BR,   // branch
    JMP,  // jmp
    INVOKE,  // call
    UNARY,  // 一元操作
    BINARY, // 二元操作
    CMP,    // cmp
    ALLOC,  // 分配数组
    LOAD,   // 取用
    STORE,  // 存储
    PHI,    // phi
    PHI_MOV  //  phi move 对phi中变量进行赋值
};

enum InvokeType  // 普通函数，与9种运行时函数
{
    COMMON,
    GET_INT,
    GET_CHAR,
    GET_ARRAY,
    PUT_INT,
    PUT_CHAR,
    PUT_ARRAY,
    PUT_F,
    START_TIME,
    STOP_TIME
};

enum ResultType
{
    R_VAL_RESULT,   // 右值
    L_VAL_RESULT,
    OTHER_RESULT   // 无返回
};

class Value : public enable_shared_from_this<Value>
{
private:
    static unsigned int valueId;  // 指令的总数

public:
    unsigned int id;      // 指令的ID
    ValueType valueType;  // 值类型
    unordered_set<shared_ptr<Value>> users;  // 使用对象

    bool valid = true;  // 有效

    static unsigned int getValueId();

    explicit Value(ValueType valueType) 
        : valueType(valueType), id(valueId++){};

    virtual string toString() = 0;

    virtual void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) = 0;  // 替换value

    virtual void abandonUse() = 0;  // 放弃使用

    virtual unsigned long long hashCode() = 0;

    virtual bool equals(shared_ptr<Value> &value) = 0;
};

class Module : public Value
{
public:
    vector<shared_ptr<Value>> globalStrings;
    vector<shared_ptr<Value>> globalConstants;  // const array
    vector<shared_ptr<Value>> globalVariables;  // 全局变量
    vector<shared_ptr<Function>> functions;

    Module() 
        : Value(ValueType::MODULE){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override{};

    void abandonUse() override{};

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

class Function : public Value
{
public:
    string name;  // 函数名
    FuncType funcType;
    vector<shared_ptr<Value>> params;
    vector<shared_ptr<BasicBlock>> blocks;
    shared_ptr<BasicBlock> entryBlock; // the entryBlock is the first block of a function.

    unordered_set<shared_ptr<Function>> callees;  // 此函数中调用其他函数
    unordered_set<shared_ptr<Function>> callers;  // 此函数被其他函数调用

    unordered_map<shared_ptr<Value>, unsigned int> variableWeight; // value <--> weight.
    unordered_map<shared_ptr<Value>, string> variableRegs;         // value <--> registers.  函数左值使用的寄存器R4-R12
    unordered_set<shared_ptr<Value>> variableWithoutReg;   // 必须存在内存中的，寄存器放不下的变量
    unsigned int requiredStackSize = 0; // required size in bytes.

    bool hasSideEffect = true;

    unordered_map<string, VariableType> variables; // @Deprecated

    Function() 
        : Value(ValueType::FUNCTION), funcType(FuncType::FUNC_VOID){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override{};

    void abandonUse() override;

    bool fitInline(unsigned int maxInsCnt, unsigned int maxPointerSituationCnt);

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

class BasicBlock : public Value
{
public:
    shared_ptr<Function> function;

    unordered_set<shared_ptr<BasicBlock>> predecessors;  // 前驱块
    unordered_set<shared_ptr<BasicBlock>> successors;   // 后继块
    vector<shared_ptr<Instruction>> instructions;     // 块中指令
    unordered_set<shared_ptr<PhiInstruction>> phis;   // 块中的phi

    unsigned int loopDepth = 1;                   // 用于寄存器权重计算
    unordered_set<shared_ptr<Value>> aliveValues; // 此basic block中活跃的变量

    unordered_map<string, shared_ptr<Value>> localVarSsaMap;  // SSA MAP

    bool sealed = true;                                               // 标记此basic block是否密封：没有前驱会被添加进来
    unordered_map<string, shared_ptr<PhiInstruction>> incompletePhis; // 存储不完整的 phis

    BasicBlock() 
        : Value(ValueType::BASIC_BLOCK){};

    BasicBlock(shared_ptr<Function> &function, bool sealed, unsigned int loopDepth)
        : Value(ValueType::BASIC_BLOCK), function(function), sealed(sealed), loopDepth(loopDepth){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

class Instruction : public Value
{
public:
    InstructionType type;
    shared_ptr<BasicBlock> block;   // 属于的基本块

    ResultType resultType;
    string caughtVarName;                         // Lvalue 局部变量名
    unordered_set<shared_ptr<Value>> aliveValues; // 此instruction时活跃的变量.
    
    Instruction(InstructionType type, shared_ptr<BasicBlock> &block, ResultType resultType)
        : Value(ValueType::INSTRUCTION), type(type), resultType(resultType), block(block){};

    string toString() override = 0;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override = 0;

    void abandonUse() override = 0;

    unsigned long long hashCode() override = 0;

    bool equals(shared_ptr<Value> &value) override = 0;
};

class BaseValue : public Value
{
public:
    explicit BaseValue(ValueType type) 
        : Value(type){};

    virtual string getIdent() = 0;  // 取得标识名

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override{};

    void abandonUse() override{};

    unsigned long long hashCode() override = 0;

    bool equals(shared_ptr<Value> &value) override = 0;
};

/**
 * 代表未定义的SSA
 */
class UndefinedValue : public BaseValue
{
public:
    string originName;

    explicit UndefinedValue(string &name) 
        : BaseValue(ValueType::UNDEFINED), originName(name){};

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

/**
 * 代表临时变量即数字
 */
class NumberValue : public BaseValue
{
public:
    int number;  // 数字大小

    explicit NumberValue(int number) 
        : BaseValue(ValueType::NUMBER), number(number){};

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return number; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * 仅用于维持 CONST ARRAYS.
 */
class ConstantValue : public BaseValue
{
public:
    string name;
    vector<int> dimensions;  // 数组维数
    map<int, int> values;    // 数组中值
    int size = 0;

    ConstantValue() 
        : BaseValue(ValueType::CONSTANT){};

    explicit ConstantValue(shared_ptr<ConstDefNode> &constDef);

    explicit ConstantValue(shared_ptr<GlobalValue> &globalVar);

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * 用于保存函数形参
 */
class ParameterValue : public BaseValue
{
public:
    string name;
    VariableType variableType;
    vector<int> dimensions;
    shared_ptr<Function> function;  // 所在函数

    ParameterValue(shared_ptr<Function> &function, shared_ptr<FuncFParamNode> &funcFParam);

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * 用来保存全局变量，包括数组或公共变量。
 */
class GlobalValue : public BaseValue
{
public:
    string name;
    InitType initType;
    VariableType variableType;
    vector<int> dimensions;
    map<int, int> initValues; // index <--> value, default 0.
    int size;

    explicit GlobalValue(shared_ptr<VarDefNode> &varDef);

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

class StringValue : public BaseValue
{
public:
    string str;

    explicit StringValue(string &str) 
        : BaseValue(ValueType::STRING), str(str){};

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * Return IR.
 */
class ReturnInstruction : public Instruction
{
public:
    FuncType funcType;
    shared_ptr<Value> value;  // 返回的值

    ReturnInstruction(FuncType funcType, shared_ptr<Value> &value, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::RET, bb, OTHER_RESULT), funcType(funcType), value(value){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &val) override { return false; }
};

/**
 * Branch IR.
 */
class BranchInstruction : public Instruction
{
public:
    shared_ptr<Value> condition;  // 跳转条件
    shared_ptr<BasicBlock> trueBlock;  // true跳转块
    shared_ptr<BasicBlock> falseBlock;  // false跳转块

    BranchInstruction(shared_ptr<Value> &condition, shared_ptr<BasicBlock> &trueBlock, shared_ptr<BasicBlock> &falseBlock, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::BR, bb, OTHER_RESULT), condition(condition), trueBlock(trueBlock), falseBlock(falseBlock){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

/**
 * Jump IR.
 */
class JumpInstruction : public Instruction
{
public:
    shared_ptr<BasicBlock> targetBlock;  // 目标块

    JumpInstruction(shared_ptr<BasicBlock> &targetBlock, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::JMP, bb, OTHER_RESULT), targetBlock(targetBlock){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

/**
 * Invoke IR.
 */
class InvokeInstruction : public Instruction
{
public:
    static unordered_map<string, InvokeType> sysFuncMap;  // 运行时函数表
    shared_ptr<Function> targetFunction;   // 调用的函数
    vector<shared_ptr<Value>> params;    // 参数
    InvokeType invokeType;
    string targetName;         // 仅用于运行时函数

    InvokeInstruction(shared_ptr<Function> &targetFunction, vector<shared_ptr<Value>> &params, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::INVOKE, bb, targetFunction->funcType == FuncType::FUNC_INT ? R_VAL_RESULT : OTHER_RESULT),
          params(params), invokeType(InvokeType::COMMON), targetFunction(targetFunction){};

    InvokeInstruction(string &sysFuncName, vector<shared_ptr<Value>> &params, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::INVOKE, bb, sysFuncName == "getint" || sysFuncName == "getch" || sysFuncName == "getarray" ? R_VAL_RESULT : OTHER_RESULT),
          params(params), invokeType(sysFuncMap.at(sysFuncName)), targetName(sysFuncName == "starttime" ? "_sysy_starttime" : sysFuncName == "stoptime" ? "_sysy_stoptime" : sysFuncName){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * Unary IR.
 */
class UnaryInstruction : public Instruction
{
public:
    string op;  // 一元操作符
    shared_ptr<Value> value; // 操作数

    UnaryInstruction(string &op, shared_ptr<Value> &value, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::UNARY, bb, R_VAL_RESULT), op(op), value(value){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override   // 相同的表达式，应有相同的hashCode
    {
        return (unsigned long long)value.get() * (op.empty() ? 0 : op.at(0));
    }

    bool equals(shared_ptr<Value> &val) override;
};

/**
 * Binary IR.
 */
class BinaryInstruction : public Instruction
{
public:
    string op;   // 二元操作符
    shared_ptr<Value> lhs;  // 操作数
    shared_ptr<Value> rhs;  // 操作数

    BinaryInstruction(string &op, shared_ptr<Value> &lhs, shared_ptr<Value> &rhs, shared_ptr<BasicBlock> &bb)
        : Instruction(swapOp(op) != op ? InstructionType::CMP : InstructionType::BINARY, bb, R_VAL_RESULT), op(op), lhs(lhs), rhs(rhs){};

    string toString() override;

    inline static string swapOp(const string &op)  // 操作符取反
    {
        return op == "==" ? "!=" : op == "!=" ? "=="
                               : op == ">"    ? "<="
                               : op == "<"    ? ">="
                               : op == "<="   ? ">"
                               : op == ">="   ? "<"
                                              : op;
    }

    inline static string swapOpConst(const string &op)  // 操作符取反
    {
        return op == ">" ? "<" : op == "<" ? ">"
                             : op == "<="  ? ">="
                             : op == ">="  ? "<="
                                           : op;
    }

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override   // 相同的表达式，应有相同的hashCode
    {
        unsigned long long x = 0;
        if (!op.empty())
            x = x * 5 + op.at(0);
        if (op.size() > 1)
            x = x * 5 + op.at(1);
        return x * ((unsigned long long)lhs.get() + (unsigned long long)rhs.get());
    }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * Memory Alloc IR  对于数组
 */
class AllocInstruction : public Instruction
{
public:
    string name;  // 变量名
    int bytes;    // 占用空间
    int units;    // 元素个数

    AllocInstruction(string &name, int bytes, int units, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::ALLOC, bb, OTHER_RESULT), name(name), bytes(bytes), units(units){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * Memory store IR.
 */
class StoreInstruction : public Instruction
{
public:
    shared_ptr<Value> value;  // store的值
    shared_ptr<Value> address;  // 基地址
    shared_ptr<Value> offset;   // 偏移量

    StoreInstruction(shared_ptr<Value> &value, shared_ptr<Value> &address, shared_ptr<Value> &offset, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::STORE, bb, OTHER_RESULT), value(value), address(address), offset(offset){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &val) override { return false; }
};

/**
 * Memory load IR.
 */
class LoadInstruction : public Instruction
{
public:
    shared_ptr<Value> address;  // 基地址
    shared_ptr<Value> offset;   // 偏移量

    LoadInstruction(shared_ptr<Value> &address, shared_ptr<Value> &offset, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::LOAD, bb, R_VAL_RESULT), address(address), offset(offset){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * Phi instruction used in SSA IR.
 */
class PhiInstruction : public Instruction
{
public:
    string localVarName;  // 变量名
    unordered_map<shared_ptr<BasicBlock>, shared_ptr<Value>> operands;  // phi的操作数（可能的数）

    shared_ptr<PhiMoveInstruction> phiMove; // phi指令，一个phi_move对应一个phi，但此phi_move在每个phi的operand块最后

    PhiInstruction(string &localVarName, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::PHI, bb, L_VAL_RESULT), localVarName(localVarName)
    {
        caughtVarName = localVarName;
    };

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void replaceUse(shared_ptr<BasicBlock> &toBeReplaced, shared_ptr<BasicBlock> &replaceBlock);

    void abandonUse() override;

    int getOperandValueCount(const shared_ptr<Value> &value);

    bool onlyHasBlockUserOrUserEmpty();

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * 在每个块前驱快的末尾复制 phi 的操作
 */
class PhiMoveInstruction : public Instruction
{
public:
    shared_ptr<PhiInstruction> phi;  // phi指令，一个phi_move对应一个phi，但此phi_move在每个phi的operand块最后

    unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<Value>>> blockALiveValues;  // 所在块中活跃的变量

    explicit PhiMoveInstruction(shared_ptr<PhiInstruction> &phi);

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override{};

    void abandonUse() override{};

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override { return false; }
};

shared_ptr<NumberValue> getNumberValue(int);

string generateArgumentLeftValueName(const string &);

string generatePhiLeftValueName(const string &);

string generateTempLeftValueName();

unsigned int countWeight(unsigned int, unsigned int);

#endif
