#ifndef COMPILER_COMPILE_STD_H
#define COMPILER_COMPILE_STD_H

extern bool _debugSyntax;
extern bool _debugLexer;
extern bool _debugIr;
extern bool _debugAst;
extern bool _debugIrOptimize;
extern bool _debugMachineIr;
extern bool _isBuildingIr;
extern bool _optimizeMachineIr;

//enum OptimizeLevel
//{
//    O0,  // SSA IR生成优化 常量传播、复制传播，FIR优化 临时寄存器分配
//    O1,  // SSA IR优化 死代码删除、常量折叠，MIR优化 汇编窥孔优化
//    O2,  // SSA IR优化 函数内联，MIR优化 乘除优化
//    O3   // SSA IR优化 局部数组传播、常量数组全局化
//};
enum OptimizeLevel
{
    O0,  // SSA IR生成优化 常量传播、复制传播，FIR优化 临时寄存器分配
    O1,  // SSA IR优化 死代码删除、常量折叠、局部数组传播、常量数组全局化
    O2,  // MIR优化 汇编窥孔优化
};

#define s_p_c static_pointer_cast  // 静态指针类型转换

#define _W_LEN 4
#define _GLB_REG_CNT 9    // R4-R12为函数左值寄存器（图着色分配）
#define _TMP_REG_CNT 5
#define _GLB_REG_START 4  // R0-R3为临时寄存器
#define _TMP_REG_START 0   // R13为SP，R14为LR，R15为PC
#define _LOOP_WEIGHT_BASE 10
#define _MAX_DEPTH 6
#define _MAX_LOOP_WEIGHT 1000000000

#define _SCO_SUCCESS 0
#define _SCO_ARG_ERR -1
#define _SCO_HELP -2
#define _SCO_DBG_ERR -3
#define _SCO_CHK_ERR -4
#define _SCO_DBG_PATH_ERR -5
#define _SCO_OP_ERR -6
#define _SCO_ST_ERR -7

#define _INIT_SUCCESS 0
#define _INIT_CRT_ERR -26

#define _LEX_ERR -51
#define _IR_CHK_ERR -61
#define _IR_OP_CHK_ERR -62
#define _MACH_IR_ERR -71

#if defined(WIN32)
#define _SLASH_CHAR '\\'
#define _SLASH_STRING "\\"
#define _O_SLASH_CHAR '/'
#else
#define _SLASH_CHAR '/'
#define _SLASH_STRING "/"
#define _O_SLASH_CHAR '\\'
#endif

#endif
