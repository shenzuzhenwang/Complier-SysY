/*********************************************************************
 * @file   machine_ir.h
 * @brief  不同的机器码指令，创建不同的类
 * 
 * @author 神祖
 * @date   June 2022
 *********************************************************************/
#ifndef COMPILER_MACHINE_IR_H
#define COMPILER_MACHINE_IR_H

#include "../ir/ir.h"

#include <fstream>

using namespace std;

extern ofstream machineIrStream;

class MachineModule;

class MachineFunc;

class MachineBB;

class Shift;

class MachineIns;

class Operand;

class BinaryIns;

class MemoryIns;

class StackIns;

class CmpIns;

class MovIns;

class BIns;

class BLIns;

class BXIns;

class GlobalIns;

class PhiTmp;

extern bool readRegister(shared_ptr<Value> &val, shared_ptr<Operand> &op, shared_ptr<MachineFunc> &machineFunc, vector<shared_ptr<MachineIns>> &res, bool mov, bool regRequired);

extern bool writeRegister(shared_ptr<Value> &val, shared_ptr<Operand> &op, shared_ptr<MachineFunc> &machineFunc, vector<shared_ptr<MachineIns>> &res);

extern void releaseTempRegister(const string &reg);

extern string allocTempRegister();

extern void store2Memory(shared_ptr<Operand> &des, int val_id, shared_ptr<MachineFunc> &machineFunc, vector<shared_ptr<MachineIns>> &res);

/**
 * mit means machine ins type
 */
namespace mit
{
    enum InsType
    {
        ADD,  // 加法
        SUB,  // 减法
        RSB,  // 反相减法
        MUL,  // 乘法
        DIV,  // 除法
        MLS,  // 乘减
        MLA,  // 乘加
        AND,  // 与
        ORR,  // 或
        ASR,  // 算数右移
        LSR,  // 逻辑右移
        LSL,  // 逻辑左移
        SMULL,// 长整型有符号乘法
        LOAD, // 加载
        PSEUDO_LOAD, //加载非法立即数
        STORE,// 存储
        POP,  // 弹出
        PUSH, // 推入
        MOV,  // 移动
        MOVW, // 
        MOVT, // 移至顶部
        CMP,  // 比较
        BRANCH, // B 跳转
        BLINK,  // BL
        BRETURN, 
        GLOBAL,
        COMMENT  // 注释
    };
}

/**
 * For ins which own a shift type
 */
enum SType
{
    NONE,
    ASR,  // 算数右移
    LSR,  // 逻辑右移
    LSL   // 逻辑左移
};

/**
 * For ins which own a Condition check type
 */
enum Cond
{
    NON, //
    EQ,  // ==
    NE,  // !=
    LS,  // <
    LE,  // <=
    GE,  // >=
    GT   // >
};

/**
 * For represent an Operand type
 */
enum State
{
    GLOB_INT,
    GLOB_POINTER,
    VIRTUAL, // 局部变量
    REG,   // register
    IMM,   // immediate
    LABEL
};

class MachineModule
{
public:
    vector<shared_ptr<MachineFunc>> machineFunctions;
    vector<shared_ptr<Value>> globalVariables;
    vector<shared_ptr<Value>> globalConstants;

    void toARM();
};

class MachineFunc
{
public:
    string name;
    FuncType funcType;
    vector<shared_ptr<Value>> params;
    vector<shared_ptr<MachineBB>> machineBlocks;
    unordered_map<string, int> var2offset;  // 局部变量偏移  id <--> 偏移量
    // stack size
    int stackSize;
    int stackPointer;  // 当前栈顶的值

    void toARM(vector<shared_ptr<Value>> &global_vars, vector<shared_ptr<Value>> &global_consts);
};

class MachineBB
{
public:
    int index;
    shared_ptr<MachineFunc> function;
    vector<shared_ptr<MachineIns>> MachineInstructions;

    explicit MachineBB(int index, shared_ptr<MachineFunc> &function) 
        : index(index), function(function){};

    void toARM(vector<shared_ptr<Value>> &global_vars, vector<shared_ptr<Value>> &global_consts);
};

class Shift
{
public:
    SType type;
    int shift; // shift bits
};

class MachineIns
{
public:
    mit::InsType type;
    Cond cond;
    shared_ptr<Shift> shift;

    explicit MachineIns(mit::InsType type) 
        : type(type){};

    MachineIns(mit::InsType type, Cond cond, shared_ptr<Shift> &shift) 
        : type(type), cond(cond), shift(shift){};

    MachineIns(mit::InsType type, Cond cond, SType stype, int shift) 
        : type(type), cond(cond)
    {
        shared_ptr<Shift> shift_ptr = make_shared<Shift>();
        shift_ptr->shift = shift;
        shift_ptr->type = stype;
        this->shift = shift_ptr;
    };

    virtual string toString() = 0;

    virtual void toARM(shared_ptr<MachineFunc> &machineFunc) = 0;
};

class Operand
{
public:
    State state;
    string value; // op value

    Operand(State state, string value) 
        : state(state), value(value){};
};

class BinaryIns : public MachineIns
{
public:
    shared_ptr<Operand> op1;
    shared_ptr<Operand> op2;
    shared_ptr<Operand> rd;

    BinaryIns(mit::InsType type, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2, shared_ptr<Operand> &rd)
        : MachineIns(type), op1(op1), op2(op2), rd(rd){};

    BinaryIns(mit::InsType type, Cond condition, SType stype, int shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2, shared_ptr<Operand> &rd)
        : MachineIns(type, condition, stype, shift), op1(op1), op2(op2), rd(rd){};

    BinaryIns(mit::InsType type, Cond condition, shared_ptr<Shift> shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2, shared_ptr<Operand> &rd)
        : MachineIns(type, condition, shift), op1(op1), op2(op2), rd(rd){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class TriIns : public MachineIns
{
public:
    shared_ptr<Operand> op1;
    shared_ptr<Operand> op2;
    shared_ptr<Operand> op3;
    shared_ptr<Operand> rd;

    TriIns(mit::InsType type, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2, shared_ptr<Operand> &op3, shared_ptr<Operand> &rd)
        : MachineIns(type), op1(op1), op2(op2), op3(op3), rd(rd){};

    TriIns(mit::InsType type, Cond condition, SType stype, int shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2, 
           shared_ptr<Operand> &op3, shared_ptr<Operand> &rd)
        : MachineIns(type, condition, stype, shift), op1(op1), op2(op2), op3(op3), rd(rd){};

    TriIns(mit::InsType type, Cond condition, shared_ptr<Shift> shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2, 
           shared_ptr<Operand> &op3, shared_ptr<Operand> &rd)
        : MachineIns(type, condition, shift), op1(op1), op2(op2), op3(op3), rd(rd){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

/**
 * For load和store
 */
class MemoryIns : public MachineIns
{
public:
    shared_ptr<Operand> rd;
    shared_ptr<Operand> base; 
    shared_ptr<Operand> offset;

    MemoryIns(mit::InsType type, shared_ptr<Operand> &rd, shared_ptr<Operand> &base, shared_ptr<Operand> &offset)
        : MachineIns(type), rd(rd), base(base), offset(offset){};

    MemoryIns(mit::InsType type, Cond condition, SType stype, int shift, shared_ptr<Operand> &rd, shared_ptr<Operand> &base,shared_ptr<Operand> &offset)
        : MachineIns(type, condition, stype, shift), rd(rd), base(base), offset(offset){};

    MemoryIns(mit::InsType type, Cond condition, shared_ptr<Shift> shift, shared_ptr<Operand> &rd, shared_ptr<Operand> &base, shared_ptr<Operand> &offset)
        : MachineIns(type, condition, shift), rd(rd), base(base), offset(offset){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

/**
 * 加载非法立即数.
 */
class PseudoLoad : public MachineIns
{
public:
    shared_ptr<Operand> rd;
    shared_ptr<Operand> label;
    bool isGlob;

    PseudoLoad(Cond condition, SType stype, int shift, shared_ptr<Operand> &label, shared_ptr<Operand> &rd, bool isGlob)
        : MachineIns(mit::PSEUDO_LOAD, condition, stype, shift), label(label), rd(rd), isGlob(isGlob){};

    PseudoLoad(Cond condition, shared_ptr<Shift> shift, shared_ptr<Operand> &label, shared_ptr<Operand> &rd, bool isGlob)
        : MachineIns(mit::PSEUDO_LOAD, condition, shift), label(label), rd(rd), isGlob(isGlob){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

/**
 * For pop/push {r0, r1...}
 */
class StackIns : public MachineIns
{
public:
    vector<shared_ptr<Operand>> regs;

    explicit StackIns(bool isPush) : MachineIns(isPush ? mit::PUSH : mit::POP){};

    StackIns(Cond condition, SType stype, int shift, bool isPush) 
        : MachineIns(isPush ? mit::PUSH : mit::POP, condition, stype, shift){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class CmpIns : public MachineIns
{
public:
    shared_ptr<Operand> op1;
    shared_ptr<Operand> op2;

    CmpIns(shared_ptr<Operand> &op1, shared_ptr<Operand> &op2) : MachineIns(mit::CMP), op1(op1), op2(op2){};

    CmpIns(Cond condition, SType stype, int shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2) 
        : MachineIns(mit::CMP, condition, stype, shift), op1(op1), op2(op2){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class MovIns : public MachineIns
{
public:
    shared_ptr<Operand> op1; //rd
    shared_ptr<Operand> op2;

    MovIns(shared_ptr<Operand> &op1, shared_ptr<Operand> &op2) 
        : MachineIns(mit::MOV), op1(op1), op2(op2){};

    MovIns(Cond condition, SType stype, int shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2) 
        : MachineIns(mit::MOV, condition, stype, shift), op1(op1), op2(op2){};

    MovIns(mit::InsType type, Cond condition, SType stype, int shift, shared_ptr<Operand> &op1, shared_ptr<Operand> &op2) 
        : MachineIns(type, condition, stype, shift), op1(op1), op2(op2){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class BIns : public MachineIns
{
public:
    string label;

    explicit BIns(Cond condition, SType stype, int shift, string &label) 
        : MachineIns(mit::BRANCH, condition, stype, shift), label(label){};

    explicit BIns(string &label) : MachineIns(mit::BRANCH), label(label){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class BXIns : public MachineIns
{
public:
    BXIns() : MachineIns(mit::BRETURN){};

    BXIns(Cond condition, SType stype, int shift) 
        : MachineIns(mit::BRETURN, condition, stype, shift){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class BLIns : public MachineIns
{
public:
    string label;

    explicit BLIns(string &label) 
        : MachineIns(mit::BLINK), label(label){};

    BLIns(Cond condition, SType stype, int shift, string &label) 
        : MachineIns(mit::BLINK, condition, stype, shift), label(label){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

/**
 * For .data
 */
class GlobalIns : public MachineIns
{
public:
    string name;
    string value;

    GlobalIns(string name, string value) 
        : MachineIns(mit::GLOBAL, NON, NONE, 0), name(name), value(value){};

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class Comment : public MachineIns
{
public:
    string content;

    explicit Comment(string &content) : MachineIns(mit::COMMENT, NON, NONE, 0)
    {
        this->content = content;
        if (content.find('\n') != -1)
        {
            this->content.replace(content.find('\n'), 1, "");
        }
    };

    string toString() override;

    void toARM(shared_ptr<MachineFunc> &machineFunc) override;
};

class PhiTmp : public Value   //？？？？没用
{
public:
    PhiTmp() 
        : Value(ValueType::UNDEFINED){};

    string toString() override { return ""; };

    void replaceUse(shared_ptr<Value> &toBeReplaced, shared_ptr<Value> &replaceValue) override{};

    void abandonUse() override{};
};

#endif
