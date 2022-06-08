/*********************************************************************
 * @file   ir.h
 * @brief  ����IR�Ĳ�ָͬ����岻ͬ����
 * 
 * @author ����
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
    NUMBER,  // ��ʱ����������
    STRING,
    GLOBAL,  // ȫ�ֱ�������������򹫹�����
    PARAMETER,  // �����β�
    UNDEFINED,  // δ����SSA
    INSTRUCTION,  // ָ��õ���ֵ��һ��ֵ��
    MODULE,   // ÿ�������moudel
    FUNCTION,  // ����
    BASIC_BLOCK  // ������
};

enum VariableType
{
    INT,  // ����
    POINTER  // ����ָ��
};

enum InstructionType
{
    RET,  // return
    BR,   // branch
    JMP,  // jmp
    INVOKE,  // call
    UNARY,  // һԪ����
    BINARY, // ��Ԫ����
    CMP,    // cmp
    ALLOC,  // ��������
    LOAD,   // ȡ��
    STORE,  // �洢
    PHI,    // phi
    PHI_MOV  //  phi move ��phi�б������и�ֵ
};

enum InvokeType  // ��ͨ��������9������ʱ����
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
    R_VAL_RESULT,   // ��ֵ
    L_VAL_RESULT,
    OTHER_RESULT   // �޷���
};

class Value : public enable_shared_from_this<Value>
{
private:
    static unsigned int valueId;  // ָ�������

public:
    unsigned int id;      // ָ���ID
    ValueType valueType;  // ֵ����
    unordered_set<shared_ptr<Value>> users;  // ʹ�ö���

    bool valid = true;  // ��Ч

    static unsigned int getValueId();

    explicit Value(ValueType valueType) 
        : valueType(valueType), id(valueId++){};

    virtual string toString() = 0;

    virtual void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) = 0;  // �滻value

    virtual void abandonUse() = 0;  // ����ʹ��

    virtual unsigned long long hashCode() = 0;

    virtual bool equals(shared_ptr<Value> &value) = 0;
};

class Module : public Value
{
public:
    vector<shared_ptr<Value>> globalStrings;
    vector<shared_ptr<Value>> globalConstants;  // const array
    vector<shared_ptr<Value>> globalVariables;  // ȫ�ֱ���
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
    string name;  // ������
    FuncType funcType;
    vector<shared_ptr<Value>> params;
    vector<shared_ptr<BasicBlock>> blocks;
    shared_ptr<BasicBlock> entryBlock; // the entryBlock is the first block of a function.

    unordered_set<shared_ptr<Function>> callees;  // �˺����е�����������
    unordered_set<shared_ptr<Function>> callers;  // �˺�����������������

    unordered_map<shared_ptr<Value>, unsigned int> variableWeight; // value <--> weight.
    unordered_map<shared_ptr<Value>, string> variableRegs;         // value <--> registers.  ������ֵʹ�õļĴ���R4-R12
    unordered_set<shared_ptr<Value>> variableWithoutReg;   // ��������ڴ��еģ��Ĵ����Ų��µı���
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

    unordered_set<shared_ptr<BasicBlock>> predecessors;  // ǰ����
    unordered_set<shared_ptr<BasicBlock>> successors;   // ��̿�
    vector<shared_ptr<Instruction>> instructions;     // ����ָ��
    unordered_set<shared_ptr<PhiInstruction>> phis;   // ���е�phi

    unsigned int loopDepth = 1;                   // ���ڼĴ���Ȩ�ؼ���
    unordered_set<shared_ptr<Value>> aliveValues; // ��basic block�л�Ծ�ı���

    unordered_map<string, shared_ptr<Value>> localVarSsaMap;  // SSA MAP

    bool sealed = true;                                               // ��Ǵ�basic block�Ƿ��ܷ⣺û��ǰ���ᱻ��ӽ���
    unordered_map<string, shared_ptr<PhiInstruction>> incompletePhis; // �洢�������� phis

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
    shared_ptr<BasicBlock> block;   // ���ڵĻ�����

    ResultType resultType;
    string caughtVarName;                         // Lvalue �ֲ�������
    unordered_set<shared_ptr<Value>> aliveValues; // ��instructionʱ��Ծ�ı���.
    
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

    virtual string getIdent() = 0;  // ȡ�ñ�ʶ��

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override{};

    void abandonUse() override{};

    unsigned long long hashCode() override = 0;

    bool equals(shared_ptr<Value> &value) override = 0;
};

/**
 * ����δ�����SSA
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
 * ������ʱ����������
 */
class NumberValue : public BaseValue
{
public:
    int number;  // ���ִ�С

    explicit NumberValue(int number) 
        : BaseValue(ValueType::NUMBER), number(number){};

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return number; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * ������ά�� CONST ARRAYS.
 */
class ConstantValue : public BaseValue
{
public:
    string name;
    vector<int> dimensions;  // ����ά��
    map<int, int> values;    // ������ֵ
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
 * ���ڱ��溯���β�
 */
class ParameterValue : public BaseValue
{
public:
    string name;
    VariableType variableType;
    vector<int> dimensions;
    shared_ptr<Function> function;  // ���ں���

    ParameterValue(shared_ptr<Function> &function, shared_ptr<FuncFParamNode> &funcFParam);

    string toString() override;

    string getIdent() override;

    unsigned long long hashCode() override { return 0; }

    bool equals(shared_ptr<Value> &value) override;
};

/**
 * ��������ȫ�ֱ�������������򹫹�������
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
    shared_ptr<Value> value;  // ���ص�ֵ

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
    shared_ptr<Value> condition;  // ��ת����
    shared_ptr<BasicBlock> trueBlock;  // true��ת��
    shared_ptr<BasicBlock> falseBlock;  // false��ת��

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
    shared_ptr<BasicBlock> targetBlock;  // Ŀ���

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
    static unordered_map<string, InvokeType> sysFuncMap;  // ����ʱ������
    shared_ptr<Function> targetFunction;   // ���õĺ���
    vector<shared_ptr<Value>> params;    // ����
    InvokeType invokeType;
    string targetName;         // ����������ʱ����

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
    string op;  // һԪ������
    shared_ptr<Value> value; // ������

    UnaryInstruction(string &op, shared_ptr<Value> &value, shared_ptr<BasicBlock> &bb)
        : Instruction(InstructionType::UNARY, bb, R_VAL_RESULT), op(op), value(value){};

    string toString() override;

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override   // ��ͬ�ı��ʽ��Ӧ����ͬ��hashCode
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
    string op;   // ��Ԫ������
    shared_ptr<Value> lhs;  // ������
    shared_ptr<Value> rhs;  // ������

    BinaryInstruction(string &op, shared_ptr<Value> &lhs, shared_ptr<Value> &rhs, shared_ptr<BasicBlock> &bb)
        : Instruction(swapOp(op) != op ? InstructionType::CMP : InstructionType::BINARY, bb, R_VAL_RESULT), op(op), lhs(lhs), rhs(rhs){};

    string toString() override;

    inline static string swapOp(const string &op)  // ������ȡ��
    {
        return op == "==" ? "!=" : op == "!=" ? "=="
                               : op == ">"    ? "<="
                               : op == "<"    ? ">="
                               : op == "<="   ? ">"
                               : op == ">="   ? "<"
                                              : op;
    }

    inline static string swapOpConst(const string &op)  // ������ȡ��
    {
        return op == ">" ? "<" : op == "<" ? ">"
                             : op == "<="  ? ">="
                             : op == ">="  ? "<="
                                           : op;
    }

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override;

    void abandonUse() override;

    unsigned long long hashCode() override   // ��ͬ�ı��ʽ��Ӧ����ͬ��hashCode
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
 * Memory Alloc IR  ��������
 */
class AllocInstruction : public Instruction
{
public:
    string name;  // ������
    int bytes;    // ռ�ÿռ�
    int units;    // Ԫ�ظ���

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
    shared_ptr<Value> value;  // store��ֵ
    shared_ptr<Value> address;  // ����ַ
    shared_ptr<Value> offset;   // ƫ����

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
    shared_ptr<Value> address;  // ����ַ
    shared_ptr<Value> offset;   // ƫ����

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
    string localVarName;  // ������
    unordered_map<shared_ptr<BasicBlock>, shared_ptr<Value>> operands;  // phi�Ĳ����������ܵ�����

    shared_ptr<PhiMoveInstruction> phiMove; // phiָ�һ��phi_move��Ӧһ��phi������phi_move��ÿ��phi��operand�����

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
 * ��ÿ����ǰ�����ĩβ���� phi �Ĳ���
 */
class PhiMoveInstruction : public Instruction
{
public:
    shared_ptr<PhiInstruction> phi;  // phiָ�һ��phi_move��Ӧһ��phi������phi_move��ÿ��phi��operand�����

    unordered_map<shared_ptr<BasicBlock>, unordered_set<shared_ptr<Value>>> blockALiveValues;  // ���ڿ��л�Ծ�ı���

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
