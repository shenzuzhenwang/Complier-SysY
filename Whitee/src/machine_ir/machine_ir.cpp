/*********************************************************************
 * @file   machine_ir.cpp
 * @brief  使用的arm架构为armv7ve，不知道为啥armv7不支持除法
 * 
 * @author 神祖
 * @date   June 2022
 *********************************************************************/
#include "machine_ir.h"

#include <iostream>
#include <set>

using namespace std;

ofstream machineIrStream;  // 汇编输出文件
extern int const_pool_id;
extern int ins_count;
extern int pre_ins_count;
extern set<int> invalid_imm;

extern OptimizeLevel optimizeLevel;

extern unordered_map<mit::InsType, string> instype2string;  // 汇编指令
extern unordered_map<SType, string> stype2string;  // 移位汇编指令
extern unordered_map<Cond, string> cond2string;  // 条件的汇编指令

string convertImm(int imm, const string &reg);

string convertImm(int imm, const string &reg, bool mov);

void MachineModule::toARM()   // 汇编载入全局变量与const array
{
    machineIrStream << ".data" << endl;
    for (const auto &variable : globalVariables)
    {
        if (s_p_c<GlobalValue>(variable)->variableType == INT)
        {
            machineIrStream << s_p_c<GlobalValue>(variable)->name + ": .word " + to_string(s_p_c<GlobalValue>(variable)->initValues.at(0)) << endl;
        }
        else if (s_p_c<GlobalValue>(variable)->variableType == POINTER)
        {
            machineIrStream << s_p_c<GlobalValue>(variable)->name + ":" << endl;
            int start = 0, space;
            for (auto iter : s_p_c<GlobalValue>(variable)->initValues)
            {
                space = iter.first * 4 - start;
                if (space != 0)
                {
                    machineIrStream << "    .zero " + to_string(space) << endl;
                }
                machineIrStream << "    .word " + to_string(iter.second) << endl;
                start = iter.first * 4 + 4;
            }
            if (start < s_p_c<GlobalValue>(variable)->size * 4)
            {
                machineIrStream << "    .zero " + to_string(s_p_c<GlobalValue>(variable)->size * 4 - start) << endl;
            }
        }
    }
    for (const auto &const_array : globalConstants)
    {
        machineIrStream << s_p_c<ConstantValue>(const_array)->name + ":" << endl;
        int start = 0;
        int space;
        map<int, int>::iterator iter;
        for (iter = s_p_c<ConstantValue>(const_array)->values.begin(); iter != s_p_c<ConstantValue>(const_array)->values.end(); iter++)
        {
            space = iter->first * 4 - start;
            if (space != 0)
            {
                machineIrStream << "    .zero " + to_string(space) << endl;
            }
            machineIrStream << "    .word " + to_string(iter->second) << endl;
            start = iter->first * 4 + 4;
        }
        if (start < s_p_c<ConstantValue>(const_array)->size * 4)
        {
            machineIrStream << "    .zero " + to_string(s_p_c<ConstantValue>(const_array)->size * 4 - start) << endl;
        }
    }
    machineIrStream << ".text" << endl;
    machineIrStream << ".global main" << endl;
    for (const auto &func : machineFunctions)
    {
        func->toARM(this->globalVariables, this->globalConstants);
    }
}

void MachineFunc::toARM(vector<shared_ptr<Value>> &global_vars, vector<shared_ptr<Value>> &global_consts)
{
    machineIrStream << name + ":" << endl;
    for (const auto &machinebb : machineBlocks)
    {
        machinebb->toARM(global_vars, global_consts);
    }
}

void MachineBB::toARM(vector<shared_ptr<Value>> &global_vars, vector<shared_ptr<Value>> &global_consts)
{
    for (const auto &ins : MachineInstructions)
    {
        ins->toARM(function);
    }
}

/**
 * @brief 立即数可以移位偶数次得到
 * @param number 立即数
 * @return true 可以得到；false 不能
 */
bool canRotateShiftEvenTimes(unsigned int number)
{
    for (int i = 0; i < 16; ++i)
    {
        if (!(number & 0xffffff00))
        {
            return true;
        }
        number = ((0x3fffffffU & number) << 2U) | ((0xc0000000 & number) >> 30U);
    }
    return false;
}

/**
 * @brief 判断立即数是否有效   每个立即数都是由一个8位的常循环右移偶数位得到
 * @param imm 立即数
 * @param mov 是否可以取反
 * @return 
 */
bool judgeImmValid(unsigned int imm, bool mov)
{
    bool valid = canRotateShiftEvenTimes(imm);
    if (!valid && mov)
    {
        valid = canRotateShiftEvenTimes(~imm);
    }
    else if (!valid)
    {
        valid = canRotateShiftEvenTimes(~imm + 1);
    }
    return valid;
}

/**
 * @brief 取出立即数
 * @param imm 立即数
 * @param reg 寄存器
 * @param mov 可以mov指令
 * @return 寄存器
 */
string convertImm(int imm, const string &reg, bool mov)
{
    bool valid = judgeImmValid(imm, mov);
    if (valid && mov)
    {
        machineIrStream << "        MOV " + reg + ", #" + to_string((int)imm) << endl;
        ++ins_count;
        return reg;
    }
    if (valid)
        return "#" + to_string(imm);
    invalid_imm.insert(imm);
    if (imm < 0)
    {
        machineIrStream << "        LDR " + reg + ", invalid_imm_" + to_string(const_pool_id) + "__" + to_string(abs((int)imm)) << endl;
    }
    else
    {
        machineIrStream << "        LDR " + reg + ", invalid_imm_" + to_string(const_pool_id) + "_" + to_string((int)imm) << endl;
    }
    ++ins_count;
    return reg;
}

string convertImm(int imm, const string &reg)
{
    return convertImm(imm, reg, false);
}

void BinaryIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    string t_op1;
    //op1
    if (op1->state == GLOB_INT || op1->state == GLOB_POINTER)
    {
        cerr << "GG in virtual op1 bin." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == VIRTUAL)
    {
        cerr << "GG in virtual op1 bin." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == IMM)
    {
        cerr << "GG!!!In BinaryIns::toARM op1->state == IMM" << endl;
        exit(_MACH_IR_ERR);
    }
    else
    {
        t_op1 = "R" + op1->value;
    }
    //op2
    string t_op2;
    if (op2->state == GLOB_INT || op2->state == GLOB_POINTER)
    {
        cerr << "GG in bin op2 global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == VIRTUAL)
    {
        cerr << "GG in bin op2 virtual." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == IMM)
    {
        if (this->type == mit::MUL || this->type == mit::DIV)
        {
            cerr << "GG!!!In BinaryIns::toARM, MUL and DIV: op2->state == IMM" << endl;
            exit(_MACH_IR_ERR);
        }
        else
        {
            t_op2 = "#" + op2->value;
        }
    }
    else
    {
        t_op2 = "R" + op2->value;
    }
    //rd
    string des;
    if (rd->state == REG)
    {
        des = "R" + rd->value;
    }
    else
    {
        des = "R1";
    }
    //compute
    if (this->type == mit::ASR || this->type == mit::LSR || this->type == mit::LSL)
    {
        machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + des + ", " + t_op1 + ", #" + to_string(shift->shift) << endl;
    }
    else
    {
        machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + des + ", " + t_op1 + ", " + t_op2;
        ins_count++;
        if (this->shift->type != NONE)
        {
            machineIrStream << ", " + stype2string.at(this->shift->type) + " #" + to_string(this->shift->shift);
        }
        machineIrStream << endl;
    }

    ins_count++;
    //store rd
    if (rd->state == GLOB_INT)
    {
        cerr << "gg in bin ins rd global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (rd->state == VIRTUAL)
    {
        cerr << "gg in bin ins rd vir." << endl;
        exit(_MACH_IR_ERR);
    }
}

void TriIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    string t_op1;
    //op1
    if (op1->state == GLOB_INT || op1->state == GLOB_POINTER)
    {
        cerr << "GG in tri op1 global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == VIRTUAL)
    {
        cerr << "GG in tri op1 vir." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == IMM)
    {
        cerr << "GG!!!In TriIns::toARM op1->state == IMM" << endl;
        exit(_MACH_IR_ERR);
    }
    else
    {
        t_op1 = "R" + op1->value;
    }
    //op2
    string t_op2;
    if (op2->state == GLOB_INT || op2->state == GLOB_POINTER)
    {
        cerr << "GG in tri op2 vir." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == VIRTUAL)
    {
        cerr << "GG in tri op2 vir." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == IMM)
    {
        cerr << "GG!!!In TriIns::toARM op2->state == IMM" << endl;
        exit(_MACH_IR_ERR);
    }
    else
    {
        t_op2 = "R" + op2->value;
    }
    //op3
    string t_op3;
    if (op3->state == GLOB_INT || op3->state == GLOB_POINTER)
    {
        cerr << "gg in tri op3 global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op3->state == VIRTUAL)
    {
        cerr << "gg in tri op3 vir." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op3->state == IMM)
    {
        cerr << "GG!!!In TriIns::toARM op3->state == IMM" << endl;
        exit(_MACH_IR_ERR);
    }
    else
    {
        t_op3 = "R" + op3->value;
    }
    //rd
    string des;
    if (rd->state == REG)
    {
        des = "R" + rd->value;
    }
    else
    {
        des = "R4";
    }
    //compute
    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + des + ", " + t_op1 + ", " + t_op2 + ", " + t_op3 << endl;
    ins_count++;
    //store rd
    if (rd->state == GLOB_INT)
    {
        cerr << "GG in tri rd global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (rd->state == VIRTUAL)
    {
        cerr << "GG in tri rd vir." << endl;
        exit(_MACH_IR_ERR);
    }
}

void MemoryIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    //base
    string add = "R1";
    if (base->state == GLOB_INT || base->state == GLOB_POINTER)
    {
        cerr << "GG in global memory base." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (base->state == VIRTUAL)
    {
        cerr << "GG in virtual memory base." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (base->state == REG)
    {
        add = "R" + base->value;
    }
    //offset
    string off;
    if (offset->state == GLOB_INT)
    {
        cerr << "GG in global offset." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (offset->state == VIRTUAL)
    {
        cerr << "GG in virtual offset." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (offset->state == IMM)
    {
        off = "#" + this->offset->value;
    }
    else
    {
        off = "R" + offset->value;
    }
    //rd
    string des = "R2";
    if (rd->state == REG)
    {
        des = "R" + rd->value;
    }
    if (this->type == mit::STORE)
    {
        if (rd->state == GLOB_INT)
        {
            cerr << "GG memory global rd." << endl;
            exit(_MACH_IR_ERR);
        }
        else if (rd->state == VIRTUAL)
        {
            cerr << "GG memory virtual rd." << endl;
            exit(_MACH_IR_ERR);
        }
        else if (rd->state == IMM)
        {
            cerr << "GG!!!In MemoryIns::toARM, rd->state == IMM" << endl;
            exit(_MACH_IR_ERR);
        }
        machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + des + ", [" + add + ", " + off;
        if (this->shift->type != NONE)
        {
            machineIrStream << ", " + stype2string.at(this->shift->type) + " #" + to_string(this->shift->shift);
        }
        machineIrStream << "]" << endl;
        ins_count++;
    }
    else
    {
        machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + des + ", [" + add + ", " + off;
        if (this->shift->type != NONE)
        {
            machineIrStream << ", " + stype2string.at(this->shift->type) + " #" + to_string(this->shift->shift);
        }
        machineIrStream << "]" << endl;
        ins_count++;
        if (rd->state == VIRTUAL)
        {
            cerr << "gg in virtual memory rd." << endl;
            exit(_MACH_IR_ERR);
        }
        else if (rd->state == GLOB_INT)
        {
            cerr << "gg in global memory rd." << endl;
            exit(_MACH_IR_ERR);
        }
    }
}

void PseudoLoad::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    string des = "R" + rd->value;
    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + des + ", " + label->value << endl;
}

void StackIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    machineIrStream << "        " + instype2string.at(this->type) + " {";
    for (int i = 0; i < this->regs.size() - 1; i++)
    {
        machineIrStream << "R" + regs[i]->value + ", ";
    }
    machineIrStream << "R" + regs[regs.size() - 1]->value + "}" << endl;
    ins_count++;
}

void CmpIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    //op1
    string f_op;
    if (op1->state == GLOB_INT || op1->state == GLOB_POINTER)
    {
        cerr << "gg in op1 state global cmp." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == VIRTUAL)
    {
        cerr << "gg in op1 state vir cmp." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == IMM)
    {
        cerr << "GG!!!In CmpIns::toARM op1->state == IMM" << endl;
        exit(_MACH_IR_ERR);
    }
    else
    {
        f_op = "R" + op1->value;
    }
    //op2
    string s_op;
    if (op2->state == GLOB_INT || op2->state == GLOB_POINTER)
    {
        cerr << "GG in cmp op2 global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == VIRTUAL)
    {
        cerr << "GG in cmp op2 vir." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == IMM)
    {
        s_op = "#" + op2->value;
    }
    else
    {
        s_op = "R" + op2->value;
    }

    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + f_op + ", " + s_op;
    if (this->shift->type != NONE)
    {
        machineIrStream << ", " + stype2string.at(this->shift->type) + " #" + to_string(this->shift->shift);
    }
    machineIrStream << endl;
    ins_count++;
}

void MovIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    //op2
    string v_op;
    if (op2->state == GLOB_INT || op2->state == GLOB_POINTER)
    {
        cerr << "gg in op2 global move." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == VIRTUAL)
    {
        cerr << "gg in op2 virtual move." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op2->state == IMM)
    {
        v_op = "#" + op2->value;
    }
    else
    {
        v_op = "R" + op2->value;
    }
    //op1
    string t_op;
    if (op1->state == REG)
    {
        t_op = "R" + op1->value;
    }
    else
    {
        t_op = "R2";
    }
    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + t_op + ", " + v_op << endl;
    ins_count++;
    if (op1->state == GLOB_INT)
    {
        cerr << "gg in mov op1 global." << endl;
        exit(_MACH_IR_ERR);
    }
    else if (op1->state == VIRTUAL)
    {
        cerr << "gg in mov op1 vir." << endl;
        exit(_MACH_IR_ERR);
    }
}

void BIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + label << endl;
    ins_count++;
} 

void BLIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " " + label << endl;
    ins_count++;
}

void BXIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    machineIrStream << "        " + instype2string.at(this->type) + cond2string.at(this->cond) + " LR" << endl;
    ins_count++;
}

void GlobalIns::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    machineIrStream << this->name + ":" << endl;
    if (!this->value.empty())
    {
        machineIrStream << "    .long " + this->value << endl;
    }
}

void Comment::toARM(shared_ptr<MachineFunc> &machineFunc)
{
    machineIrStream << "    @" + content << endl;
}
